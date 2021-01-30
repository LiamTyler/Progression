#pragma once

#include "asset/types/base_asset.hpp"
#include "glm/vec3.hpp"
#include "core/platform_defines.hpp"

class Serializer;

namespace PG
{

enum MaterialType : uint8_t
{
    MATERIAL_TYPE_LIT,

    NUM_MATERIAL_TYPES
};

struct MaterialCreateInfo
{
    std::string name;
    MaterialType type;
    glm::vec3 Kd;
    std::string map_Kd_name;
};

struct GfxImage;

struct Material : public Asset
{
    MaterialType type;
    glm::vec3 Kd;
    GfxImage* map_Kd = nullptr;

    // Info needed for generated pipelines in the converter. Don't actually need the 
#if USING( COMPILING_CONVERTER )
    std::string map_Kd_name;
#endif // #if USING( COMPILING_CONVERTER )
};

bool Material_Load( Material* material, const MaterialCreateInfo& createInfo );

bool Fastfile_Material_Load( Material* material, Serializer* serializer );

bool Fastfile_Material_Save( const MaterialCreateInfo * const matcreateInfo, Serializer* serializer );

} // namespace PG
