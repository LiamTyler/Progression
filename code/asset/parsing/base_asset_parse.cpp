#include "base_asset_parse.hpp"
#include "gfx_image_parse.hpp"
#include "material_parse.hpp"
#include "model_parse.hpp"
#include "script_parse.hpp"
#include "shader_parse.hpp"
#include "textureset_parse.hpp"

namespace PG
{

static_assert( NUM_ASSET_TYPES == 6 );

const std::shared_ptr<BaseAssetParser> g_assetParsers[NUM_ASSET_TYPES] =
{
    std::make_shared<GfxImageParser>(),   // ASSET_TYPE_GFX_IMAGE
    std::make_shared<MaterialParser>(),   // ASSET_TYPE_MATERIAL
    std::make_shared<ScriptParser>(),     // ASSET_TYPE_SCRIPT
    std::make_shared<ModelParser>(),      // ASSET_TYPE_MODEL
    std::make_shared<ShaderParser>(),     // ASSET_TYPE_SHADER
    std::make_shared<TexturesetParser>(), // ASSET_TYPE_TEXTURESET
};

} // namespace PG