#pragma once

#include "renderer/graphics_api/descriptor.hpp"
#include "vulkan.hpp"

namespace PG::Gfx
{
class Buffer;
} // namespace PG::Gfx

namespace PG::Gfx::BindlessManager
{

enum class Usage : uint8_t
{
    NONE       = 0,
    READ       = 1 << 0,
    WRITE      = 1 << 1,
    READ_WRITE = READ | WRITE,
};
PG_DEFINE_ENUM_OPS( Usage );

void Init();
void Shutdown();
void Update();
uint16_t AddTexture( VkImageView imgView, Usage usage = Usage::READ_WRITE );
void RemoveTexture( uint16_t index );

// can return 0 (invalid) if the buffer isn't bindless compatible (ex: descriptor buffers, AS buffers, etc)
uint16_t AddBuffer( const Buffer* buffer );
void RemoveBuffer( uint16_t index );

} // namespace PG::Gfx::BindlessManager
