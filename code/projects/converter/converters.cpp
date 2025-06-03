#include "converters.hpp"
#include "converters/font_converter.hpp"
#include "converters/gfx_image_converter.hpp"
#include "converters/material_converter.hpp"
#include "converters/model_converter.hpp"
#include "converters/pipeline_converter.hpp"
#include "converters/script_converter.hpp"
#include "converters/shader_converter.hpp"
#include "converters/ui_layout_converter.hpp"

namespace PG
{

BaseAssetConverter* g_converters[ASSET_TYPE_COUNT];

void InitConverters()
{
    PGP_ZONE_SCOPEDN( "InitConverters" );
    g_converters[ASSET_TYPE_GFX_IMAGE] = new GfxImageConverter;
    g_converters[ASSET_TYPE_MATERIAL]  = new MaterialConverter;
    g_converters[ASSET_TYPE_SCRIPT]    = new ScriptConverter;
    g_converters[ASSET_TYPE_MODEL]     = new ModelConverter;
    g_converters[ASSET_TYPE_SHADER]    = new ShaderConverter;
    g_converters[ASSET_TYPE_UI_LAYOUT] = new UILayoutConverter;
    g_converters[ASSET_TYPE_PIPELINE]  = new PipelineConverter;
    g_converters[ASSET_TYPE_FONT]      = new FontConverter;
    // pass through, as texturesets aren't actually converted
    g_converters[ASSET_TYPE_TEXTURESET] = new BaseAssetConverter( ASSET_TYPE_TEXTURESET );
    static_assert( ASSET_TYPE_COUNT == 9 );

    InitShaderIncludeCache();
}

void ShutdownConverters()
{
    CloseShaderIncludeCache();
    for ( u32 i = 0; i < ASSET_TYPE_COUNT; ++i )
    {
        delete g_converters[i];
    }
}

} // namespace PG
