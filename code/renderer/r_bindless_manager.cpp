#include "renderer/r_bindless_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
#include "c_shared/bindless.h"
#include "c_shared/material.h"
#include "c_shared/model.h"
#include "data_structures/free_slot_bit_array.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/r_globals.hpp"

using namespace PG;
using namespace PG::Gfx;
using std::vector;

struct PerFrameManagerData
{
    vector<TextureIndex> pendingTextureRemovals;
    vector<BufferIndex> pendingBufferRemovals;
    vector<BufferIndex> pendingMeshBufferRemovals;
    vector<MaterialIndex> pendingMaterialRemovals;
};

struct PendingTextureAdd
{
    VkImageView imgView;
    TextureIndex slot;
    bool isSampled;
    bool isStorage;
};

struct PendingBufferAdd
{
    VkDeviceAddress bufferAddress;
    BufferIndex slot;
};

struct PendingMaterialAdd
{
    const Material* material;
    MaterialIndex slot;
};

static PerFrameManagerData s_frameData[NUM_FRAME_OVERLAP];
static PerFrameManagerData* s_currFrameData;
static FreeSlotBitArray<PG_MAX_BINDLESS_TEXTURES> s_freeTextureSlots;
static FreeSlotBitArray<PG_MAX_BINDLESS_BUFFERS> s_freeBufferSlots;
static FreeSlotBitArray<PG_MAX_BINDLESS_MATERIALS> s_freeMaterialSlots;

static vector<PendingTextureAdd> s_pendingTextureAdds;
static vector<PendingBufferAdd> s_pendingBufferAdds;
static vector<PendingMaterialAdd> s_pendingMaterialAdds;

#if USING( PG_DESCRIPTOR_BUFFER )
static VkPhysicalDeviceDescriptorBufferPropertiesEXT s_dbProps;
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
static vector<VkWriteDescriptorSet> s_scratchWrites;
static vector<VkDescriptorImageInfo> s_scratchImgInfos;
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

static Buffer s_bindlessBufferPointers;
static Buffer s_bindlessMaterials;

namespace PG::Gfx::BindlessManager
{

void Init()
{
    PGP_ZONE_SCOPEDN( "BindlessManager::Init" );
    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_frameData[i].pendingTextureRemovals.reserve( 128 );
        s_frameData[i].pendingBufferRemovals.reserve( 128 );
        s_frameData[i].pendingMeshBufferRemovals.reserve( 128 );
        s_frameData[i].pendingMaterialRemovals.reserve( 128 );
    }
    s_currFrameData = &s_frameData[0];

    s_freeTextureSlots.Clear();
    s_freeTextureSlots.GetFreeSlot();
    static_assert( PG_INVALID_TEXTURE_INDEX == 0, "update the free slot list above if this changes" );

    s_freeBufferSlots.Clear();
    s_freeBufferSlots.GetFreeSlot();
    static_assert( PG_INVALID_BUFFER_INDEX == 0, "update the free slot list above if this changes" );

    s_freeMaterialSlots.Clear();
    s_freeMaterialSlots.GetFreeSlot();
    static_assert( PG_INVALID_MATERIAL_INDEX == 0, "update the free slot list above if this changes" );

    s_pendingTextureAdds.reserve( 128 );
    s_pendingBufferAdds.reserve( 128 );
    s_pendingMaterialAdds.reserve( 128 );

#if USING( PG_DESCRIPTOR_BUFFER )
    s_dbProps = rg.physicalDevice.GetProperties().dbProps;
    s_descriptorBuffer.Create( s_descriptorSetLayout );
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    s_scratchWrites.reserve( 2 * 128 );
    s_scratchImgInfos.reserve( 2 * 128 );
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

    BufferCreateInfo bufInfo = {};
    bufInfo.size             = PG_MAX_BINDLESS_BUFFERS * sizeof( VkDeviceAddress );
    bufInfo.bufferUsage      = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
    bufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    s_bindlessBufferPointers = rg.device.NewBuffer( bufInfo, "global_bindlessBufferPointers" );

    bufInfo.size        = PG_MAX_BINDLESS_MATERIALS * sizeof( GpuData::PackedMaterial );
    bufInfo.bufferUsage = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
    bufInfo.flags       = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    s_bindlessMaterials = rg.device.NewBuffer( bufInfo, "global_bindlessMaterials" );

    VkDescriptorBufferInfo bBufferInfo{ s_bindlessBufferPointers.GetHandle(), 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet          = GetGlobalDescriptorSet();
    write.dstBinding      = PG_BINDLESS_BUFFER_DSET_BINDING;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo     = &bBufferInfo;
    vkUpdateDescriptorSets( rg.device, 1, &write, 0, nullptr );

    VkDescriptorBufferInfo mBufferInfo{ s_bindlessMaterials.GetHandle(), 0, VK_WHOLE_SIZE };
    write.dstBinding  = PG_BINDLESS_MATERIALS_DSET_BINDING;
    write.pBufferInfo = &mBufferInfo;
    vkUpdateDescriptorSets( rg.device, 1, &write, 0, nullptr );

    InitSamplers();
}

void Shutdown()
{
    FreeSamplers();
    s_bindlessMaterials.Free();
    s_bindlessBufferPointers.Free();
    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_frameData[i].pendingTextureRemovals    = vector<TextureIndex>();
        s_frameData[i].pendingBufferRemovals     = vector<BufferIndex>();
        s_frameData[i].pendingMeshBufferRemovals = vector<BufferIndex>();
        s_frameData[i].pendingMaterialRemovals   = vector<MaterialIndex>();
    }
    s_pendingTextureAdds  = vector<PendingTextureAdd>();
    s_pendingBufferAdds   = vector<PendingBufferAdd>();
    s_pendingMaterialAdds = vector<PendingMaterialAdd>();
}

GpuData::PackedMaterial CpuToGpuMaterial( const Material* mat )
{
    GpuData::PackedMaterial gpuMat;
    gpuMat.albedoTint    = mat->albedoTint;
    gpuMat.metalnessTint = mat->metalnessTint;
    gpuMat.roughnessTint = mat->roughnessTint;
    gpuMat.albedoMetalnessMapIndex =
        mat->albedoMetalnessImage ? mat->albedoMetalnessImage->gpuTexture.GetBindlessIndex() : PG_INVALID_TEXTURE_INDEX;
    gpuMat.normalRoughnessMapIndex =
        mat->normalRoughnessImage ? mat->normalRoughnessImage->gpuTexture.GetBindlessIndex() : PG_INVALID_TEXTURE_INDEX;
    gpuMat.emissiveTint     = mat->emissiveTint;
    gpuMat.emissiveMapIndex = mat->emissiveImage ? mat->emissiveImage->gpuTexture.GetBindlessIndex() : PG_INVALID_TEXTURE_INDEX;

    return gpuMat;
}

void Update()
{
    PGP_ZONE_SCOPEDN( "BindlessManager::Update" );
    for ( TextureIndex idx : s_currFrameData->pendingTextureRemovals )
        s_freeTextureSlots.FreeSlot( idx );
    for ( BufferIndex idx : s_currFrameData->pendingBufferRemovals )
        s_freeBufferSlots.FreeSlot( idx );
    for ( BufferIndex firstSlot : s_currFrameData->pendingMeshBufferRemovals )
        s_freeBufferSlots.FreeConsecutiveSlots( firstSlot, MESH_NUM_BUFFERS );
    for ( MaterialIndex idx : s_currFrameData->pendingMaterialRemovals )
        s_freeMaterialSlots.FreeSlot( idx );

    s_currFrameData->pendingTextureRemovals.clear();
    s_currFrameData->pendingBufferRemovals.clear();
    s_currFrameData->pendingMeshBufferRemovals.clear();
    s_currFrameData->pendingMaterialRemovals.clear();
    s_currFrameData = &s_frameData[rg.currentFrameIdx];

    u32 numTexAdds = static_cast<u32>( s_pendingTextureAdds.size() );
    if ( numTexAdds )
    {
#if USING( PG_DESCRIPTOR_BUFFER )
        DescriptorBuffer& descriptorBuffer = GetGlobalDescriptorBuffer();
        char* dbPtr                        = descriptorBuffer.buffer.GetMappedPtr();
        for ( u32 i = 0; i < numAdds; ++i )
        {
            const PendingTexAdd& pendingAdd = s_pendingTexAdds[i];

            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imgInfo.imageView   = pendingAdd.imgView;

            VkDescriptorGetInfoEXT descriptorInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
            descriptorInfo.type               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            descriptorInfo.data.pStorageImage = &imgInfo;
            size_t descSize                   = s_dbProps.storageImageDescriptorSize;
            vkGetDescriptorEXT( rg.device, &descriptorInfo, descSize, dbPtr + descriptorBuffer.offset + pendingAdd.slot * descSize );
        }
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
        s_scratchWrites.reserve( 2 * numTexAdds );
        s_scratchImgInfos.reserve( 2 * numTexAdds );
        for ( u32 i = 0; i < numTexAdds; ++i )
        {
            const PendingTextureAdd& pendingAdd = s_pendingTextureAdds[i];

            if ( pendingAdd.isSampled )
            {
                VkDescriptorImageInfo& imgInfo = s_scratchImgInfos.emplace_back();
                imgInfo.sampler                = VK_NULL_HANDLE;
                imgInfo.imageView              = pendingAdd.imgView;
                imgInfo.imageLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkWriteDescriptorSet& write = s_scratchWrites.emplace_back( VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
                write.dstSet                = GetGlobalDescriptorSet();
                write.dstBinding            = PG_BINDLESS_SAMPLED_TEXTURE_DSET_BINDING;
                write.dstArrayElement       = pendingAdd.slot;
                write.descriptorCount       = 1;
                write.descriptorType        = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                write.pImageInfo            = &imgInfo;
            }
            if ( pendingAdd.isStorage )
            {
                VkDescriptorImageInfo& imgInfo = s_scratchImgInfos.emplace_back();
                imgInfo.sampler                = VK_NULL_HANDLE;
                imgInfo.imageView              = pendingAdd.imgView;
                imgInfo.imageLayout            = VK_IMAGE_LAYOUT_GENERAL;

                VkWriteDescriptorSet& write = s_scratchWrites.emplace_back( VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
                write.dstSet                = GetGlobalDescriptorSet();
                write.dstBinding            = PG_BINDLESS_STORAGE_IMAGE_DSET_BINDING;
                write.dstArrayElement       = pendingAdd.slot;
                write.descriptorCount       = 1;
                write.descriptorType        = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                write.pImageInfo            = &imgInfo;
            }
        }

        vkUpdateDescriptorSets( rg.device, (u32)s_scratchWrites.size(), s_scratchWrites.data(), 0, nullptr );

        s_scratchWrites.clear();
        s_scratchImgInfos.clear();
#endif // #if USING( PG_DESCRIPTOR_BUFFER )
        s_pendingTextureAdds.clear();
    }

    VkDeviceAddress* bufData = reinterpret_cast<VkDeviceAddress*>( s_bindlessBufferPointers.GetMappedPtr() );
    for ( const PendingBufferAdd& pendingAdd : s_pendingBufferAdds )
    {
        bufData[pendingAdd.slot] = pendingAdd.bufferAddress;
    }
    s_pendingBufferAdds.clear();

    GpuData::PackedMaterial* gpuMatData = reinterpret_cast<GpuData::PackedMaterial*>( s_bindlessMaterials.GetMappedPtr() );
    for ( const PendingMaterialAdd& pendingAdd : s_pendingMaterialAdds )
    {
        gpuMatData[pendingAdd.slot] = CpuToGpuMaterial( pendingAdd.material );
    }
    s_pendingMaterialAdds.clear();
}

TextureIndex AddTexture( const Texture* texture )
{
    bool isSampled = texture->GetUsage() & VK_IMAGE_USAGE_SAMPLED_BIT;
    bool isStorage = texture->GetUsage() & VK_IMAGE_USAGE_STORAGE_BIT;
    if ( !isSampled && !isStorage )
        return PG_INVALID_TEXTURE_INDEX;

    TextureIndex openSlot  = (TextureIndex)s_freeTextureSlots.GetFreeSlot();
    PendingTextureAdd& add = s_pendingTextureAdds.emplace_back();
    add.imgView            = texture->GetView();
    add.slot               = openSlot;
    add.isSampled          = isSampled;
    add.isStorage          = isStorage;

    return openSlot;
}

void RemoveTexture( TextureIndex index )
{
    if ( index == PG_INVALID_TEXTURE_INDEX )
        return;
    s_currFrameData->pendingTextureRemovals.push_back( index );
}

static BufferIndex GetBufferSlot()
{
    BufferIndex openSlot = (BufferIndex)s_freeBufferSlots.GetFreeSlot();
    return openSlot;
}

BufferIndex AddBuffer( const Buffer* buffer )
{
    VkDescriptorType descType = buffer->GetDescriptorType();
    if ( descType == VK_DESCRIPTOR_TYPE_MAX_ENUM )
        return PG_INVALID_BUFFER_INDEX;

    BufferIndex openSlot = GetBufferSlot();
    s_pendingBufferAdds.emplace_back( buffer->GetDeviceAddress(), openSlot );

    return openSlot;
}

void RemoveBuffer( BufferIndex index )
{
    if ( index == PG_INVALID_BUFFER_INDEX )
        return;
    s_currFrameData->pendingBufferRemovals.push_back( index );
}

BufferIndex AddMeshBuffers( Mesh* mesh )
{
    BufferIndex firstSlot = s_freeBufferSlots.GetConsecutiveFreeSlots( MESH_NUM_BUFFERS );
    s_pendingBufferAdds.emplace_back( mesh->buffers[Mesh::MESHLET_BUFFER].GetDeviceAddress(), firstSlot + MESH_BUFFER_MESHLETS );
    s_pendingBufferAdds.emplace_back( mesh->buffers[Mesh::CULL_DATA_BUFFER].GetDeviceAddress(), firstSlot + MESH_BUFFER_MESHLET_CULL_DATA );
    s_pendingBufferAdds.emplace_back( mesh->buffers[Mesh::TRI_BUFFER].GetDeviceAddress(), firstSlot + MESH_BUFFER_TRIS );

    VkDeviceAddress vData = mesh->buffers[Mesh::VERTEX_BUFFER].GetDeviceAddress();
    s_pendingBufferAdds.emplace_back( vData, firstSlot + MESH_BUFFER_POSITIONS );
    vData += ROUND_UP_TO_MULT( mesh->numVertices * ( PACKED_VERTS ? sizeof( u16vec3 ) : sizeof( vec3 ) ), 4 );

    s_pendingBufferAdds.emplace_back( vData, firstSlot + MESH_BUFFER_NORMALS );
#if PACKED_VERTS
    vData += ROUND_UP_TO_MULT( BITS_PER_NORMAL * mesh->numVertices, 32 ) / 8;
#else  // #if PACKED_VERTS
    vData += mesh->numVertices * sizeof( vec3 );
#endif // #else // #if PACKED_VERTS

    if ( mesh->hasTexCoords )
    {
        s_pendingBufferAdds.emplace_back( vData, firstSlot + MESH_BUFFER_UVS );
        vData += mesh->numVertices * sizeof( vec2 );
    }
    else
    {
        s_pendingBufferAdds.emplace_back( 0, firstSlot + MESH_BUFFER_UVS );
    }

    if ( mesh->hasTangents )
    {
        s_pendingBufferAdds.emplace_back( vData, firstSlot + MESH_BUFFER_TANGENTS );
#if PACKED_VERTS
        vData += ROUND_UP_TO_MULT( BITS_PER_TANGENT * mesh->numVertices, 32 ) / 8;
#else  // #if PACKED_VERTS
        vData += mesh->numVertices * sizeof( vec4 );
#endif // #else // #if PACKED_VERTS
    }
    else
    {
        s_pendingBufferAdds.emplace_back( 0, firstSlot + MESH_BUFFER_TANGENTS );
    }

    return firstSlot;
}

void RemoveMeshBuffers( Mesh* mesh ) { s_currFrameData->pendingMeshBufferRemovals.push_back( mesh->bindlessBuffersSlot ); }

MaterialIndex AddMaterial( const Material* material )
{
    MaterialIndex openSlot = (MaterialIndex)s_freeMaterialSlots.GetFreeSlot();
    s_pendingMaterialAdds.emplace_back( material, openSlot );

    return openSlot;
}

void RemoveMaterial( MaterialIndex index )
{
    if ( index == PG_INVALID_MATERIAL_INDEX )
        return;
    s_currFrameData->pendingMaterialRemovals.push_back( index );
}

} // namespace PG::Gfx::BindlessManager
