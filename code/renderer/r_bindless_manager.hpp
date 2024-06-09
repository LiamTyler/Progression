#pragma once

#include "renderer/graphics_api/descriptor.hpp"
#include "vulkan.hpp"

namespace PG
{
struct Mesh;
struct Material;
} // namespace PG

namespace PG::Gfx
{
class Buffer;
class Texture;

using TextureIndex  = u16;
using BufferIndex   = u16;
using MaterialIndex = u16;

} // namespace PG::Gfx

namespace PG::Gfx::BindlessManager
{

void Init();
void Shutdown();
void Update();

TextureIndex AddTexture( const Texture* texture );
void RemoveTexture( TextureIndex index );

// can return 0 (invalid) if the buffer isn't bindless compatible (ex: descriptor buffers, AS buffers, etc)
BufferIndex AddBuffer( const Buffer* buffer );
void RemoveBuffer( BufferIndex index );

BufferIndex AddMeshBuffers( Mesh* mesh );
void RemoveMeshBuffers( Mesh* mesh );

MaterialIndex AddMaterial( const Material* material );
void RemoveMaterial( MaterialIndex index );

} // namespace PG::Gfx::BindlessManager
