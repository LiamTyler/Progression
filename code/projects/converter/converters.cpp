#include "converters.hpp"
#include "converters/gfx_image_converter.hpp"
#include "converters/material_converter.hpp"
#include "converters/model_converter.hpp"
#include "converters/shader_converter.hpp"
#include "converters/script_converter.hpp"

namespace PG
{

std::shared_ptr<BaseAssetConverter> g_converters[NUM_ASSET_TYPES];

void InitConverters()
{
    g_converters[ASSET_TYPE_GFX_IMAGE] = std::make_shared<GfxImageConverter>();
    g_converters[ASSET_TYPE_MATERIAL] = std::make_shared<MaterialConverter>();
    g_converters[ASSET_TYPE_SCRIPT] = std::make_shared<ScriptConverter>();
    g_converters[ASSET_TYPE_MODEL] = std::make_shared<ModelConverter>();
    g_converters[ASSET_TYPE_SHADER] = std::make_shared<ShaderConverter>();
    g_converters[ASSET_TYPE_TEXTURESET] = nullptr;
}


void ShutdownConverters()
{
    for ( int i = 0; i < NUM_ASSET_TYPES; ++i )
    {
        g_converters[i].reset();
    }
}

} // namespace PG
