#include "asset_parsers.hpp"

namespace PG
{

static_assert( ASSET_TYPE_COUNT == 8 );

const std::shared_ptr<BaseAssetParser> g_assetParsers[ASSET_TYPE_COUNT] = {
    std::make_shared<GfxImageParser>(),   // ASSET_TYPE_GFX_IMAGE
    std::make_shared<MaterialParser>(),   // ASSET_TYPE_MATERIAL
    std::make_shared<ScriptParser>(),     // ASSET_TYPE_SCRIPT
    std::make_shared<ModelParser>(),      // ASSET_TYPE_MODEL
    std::make_shared<NullParser>(),       // ASSET_TYPE_SHADER
    std::make_shared<UILayoutParser>(),   // ASSET_TYPE_UI_LAYOUT
    std::make_shared<PipelineParser>(),   // ASSET_TYPE_PARSER
    std::make_shared<TexturesetParser>(), // ASSET_TYPE_TEXTURESET
};

#define BEGIN_STR_TO_ENUM_MAP( EnumName )                           \
    static EnumName EnumName##_StringToEnum( std::string_view str ) \
    {                                                               \
        static std::pair<std::string, EnumName> arr[] = {

#define BEGIN_STR_TO_ENUM_MAP_SCOPED( EnumName, Namespace )                    \
    static Namespace::EnumName EnumName##_StringToEnum( std::string_view str ) \
    {                                                                          \
        static std::pair<std::string, Namespace::EnumName> arr[] = {

#define STR_TO_ENUM_VALUE( EnumName, val ) { #val, EnumName::val },

#define END_STR_TO_ENUM_MAP( EnumName, defaultVal )                                                            \
    }                                                                                                          \
    ;                                                                                                          \
                                                                                                               \
    for ( i32 i = 0; i < ARRAY_COUNT( arr ); ++i )                                                             \
    {                                                                                                          \
        if ( arr[i].first == str )                                                                             \
        {                                                                                                      \
            return arr[i].second;                                                                              \
        }                                                                                                      \
    }                                                                                                          \
                                                                                                               \
    LOG_WARN( "No " #EnumName " found with name '%s'. Using default '" #defaultVal "' instead.", str.data() ); \
    return EnumName::defaultVal;                                                                               \
    }

// clang-format off

BEGIN_STR_TO_ENUM_MAP( GfxImageSemantic )
    STR_TO_ENUM_VALUE( GfxImageSemantic, COLOR )
    STR_TO_ENUM_VALUE( GfxImageSemantic, GRAY )
    STR_TO_ENUM_VALUE( GfxImageSemantic, ALBEDO_METALNESS )
    STR_TO_ENUM_VALUE( GfxImageSemantic, NORMAL_ROUGHNESS )
    STR_TO_ENUM_VALUE( GfxImageSemantic, ENVIRONMENT_MAP )
    STR_TO_ENUM_VALUE( GfxImageSemantic, ENVIRONMENT_MAP_IRRADIANCE )
    STR_TO_ENUM_VALUE( GfxImageSemantic, UI )
END_STR_TO_ENUM_MAP( GfxImageSemantic, NUM_IMAGE_SEMANTICS )

BEGIN_STR_TO_ENUM_MAP( GfxImageFilterMode )
    STR_TO_ENUM_VALUE( GfxImageFilterMode, NEAREST )
    STR_TO_ENUM_VALUE( GfxImageFilterMode, BILINEAR )
    STR_TO_ENUM_VALUE( GfxImageFilterMode, TRILINEAR )
END_STR_TO_ENUM_MAP( GfxImageFilterMode, COUNT )

bool GfxImageParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper< GfxImageCreateInfo& > mapping(
    {
        { "filename", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[0] = ParseString( v ); } },
        { "equirectangularFilename", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[0]  = ParseString( v ); } },
        { "left",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_LEFT]   = ParseString( v ); } },
        { "right",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_RIGHT]  = ParseString( v ); } },
        { "front",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_FRONT]  = ParseString( v ); } },
        { "back",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BACK]   = ParseString( v ); } },
        { "top",    []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_TOP]    = ParseString( v ); } },
        { "bottom", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BOTTOM] = ParseString( v ); } },
        { "flipVertically", []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.flipVertically = ParseBool( v ); } },
        { "semantic",  []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.semantic = GfxImageSemantic_StringToEnum( ParseString( v ) ); } },
        { "dstFormat", []( const rapidjson::Value& v, GfxImageCreateInfo& i )
            {
                i.dstPixelFormat = PixelFormatFromString( ParseString( v ) );
                if ( i.dstPixelFormat == PixelFormat::INVALID )
                {
                    LOG_ERR( "Invalid dstFormat '%s'", v.GetString() );
                }
            }
        },
        { "clampHorizontal", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.clampHorizontal = ParseBool( v ); } },
        { "clampVertical",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.clampVertical = ParseBool( v ); } },
        { "filterMode",      []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filterMode = GfxImageFilterMode_StringToEnum( ParseString( v ) ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

BEGIN_STR_TO_ENUM_MAP( MaterialType )
    STR_TO_ENUM_VALUE( MaterialType, SURFACE )
    STR_TO_ENUM_VALUE( MaterialType, DECAL )
END_STR_TO_ENUM_MAP( MaterialType, COUNT )

bool MaterialParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    using namespace rapidjson;
    static JSONFunctionMapper< MaterialCreateInfo& > mapping(
    {
        { "type", []( const Value& v, MaterialCreateInfo& i ) {
            i.type = MaterialType_StringToEnum( ParseString( v ) );
        } },
        { "textureset", []( const Value& v, MaterialCreateInfo& i ) { i.texturesetName = ParseString( v ); } },
        { "albedoTint", []( const Value& v, MaterialCreateInfo& i ) { i.albedoTint = ParseVec3( v ); } },
        { "metalnessTint", []( const Value& v, MaterialCreateInfo& i ) { i.metalnessTint = ParseNumber<f32>( v ); } },
        { "emissiveTint", []( const Value& v, MaterialCreateInfo& i ) { i.emissiveTint = ParseVec3( v ); } },
        { "roughnessTint", []( const Value& v, MaterialCreateInfo& i ) { i.roughnessTint = ParseNumber<f32>( v ); } },
        { "applyAlbedo", []( const Value& v, MaterialCreateInfo& i ) { i.applyAlbedo = ParseBool( v ); } },
        { "applyMetalness", []( const Value& v, MaterialCreateInfo& i ) { i.applyMetalness = ParseBool( v ); } },
        { "applyNormals", []( const Value& v, MaterialCreateInfo& i ) { i.applyNormals = ParseBool( v ); } },
        { "applyRoughness", []( const Value& v, MaterialCreateInfo& i ) { i.applyRoughness = ParseBool( v ); } },
        { "applyEmissive", []( const Value& v, MaterialCreateInfo& i ) { i.applyEmissive = ParseBool( v ); } },
    });
    mapping.ForEachMember( value, *info );

   return true;
}

bool ModelParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<ModelCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = ParseString( v ); } },
        { "flipTexCoordsVertically", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.flipTexCoordsVertically = ParseBool( v ); } },
        { "recalculateNormals", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.recalculateNormals = ParseBool( v ); } },
    });
    mapping.ForEachMember( value, *info );

    if ( info->filename.empty() )
    {
        LOG_ERR( "Must specify a filename for Model asset '%s'", info->name.c_str() );
        return false;
    }

    return true;
}

bool ScriptParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<ScriptCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.filename = ParseString( v ); } },
    });
    mapping.ForEachMember( value, *info );
    return true;
}

bool UILayoutParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<UILayoutCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, UILayoutCreateInfo& s ) { s.xmlFilename = ParseString( v ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

BEGIN_STR_TO_ENUM_MAP_SCOPED( CompareFunction, Gfx )
    STR_TO_ENUM_VALUE( Gfx::CompareFunction, NEVER )
    STR_TO_ENUM_VALUE( Gfx::CompareFunction, LESS )
    STR_TO_ENUM_VALUE( Gfx::CompareFunction, EQUAL )
    STR_TO_ENUM_VALUE( Gfx::CompareFunction, LEQUAL )
    STR_TO_ENUM_VALUE( Gfx::CompareFunction, GREATER )
    STR_TO_ENUM_VALUE( Gfx::CompareFunction, NEQUAL )
    STR_TO_ENUM_VALUE( Gfx::CompareFunction, GEQUAL )
    STR_TO_ENUM_VALUE( Gfx::CompareFunction, ALWAYS )
END_STR_TO_ENUM_MAP( Gfx::CompareFunction, NUM_COMPARE_FUNCTION )

bool PipelineParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<Gfx::PipelineColorAttachmentInfo&> cMapping(
    {
        { "format", []( const rapidjson::Value& v, Gfx::PipelineColorAttachmentInfo& s ) { s.format = PixelFormatFromString( ParseString( v ) ); } },
    });

    static JSONFunctionMapper<Gfx::PipelineDepthInfo&> dMapping(
    {
        { "format",             []( const rapidjson::Value& v, Gfx::PipelineDepthInfo& s ) { s.format = PixelFormatFromString( ParseString( v ) ); } },
        { "depthTestEnabled",   []( const rapidjson::Value& v, Gfx::PipelineDepthInfo& s ) { s.depthTestEnabled = ParseBool( v ); } },
        { "depthWriteEnabled",  []( const rapidjson::Value& v, Gfx::PipelineDepthInfo& s ) { s.depthWriteEnabled = ParseBool( v ); } },
        { "compareFunc",        []( const rapidjson::Value& v, Gfx::PipelineDepthInfo& s ) { s.compareFunc = CompareFunction_StringToEnum( ParseString( v ) ); } },
    });

    static JSONFunctionMapper<PipelineCreateInfo&> mapping(
    {
        { "name",           []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.name = ParseString( v ); } },
        { "computeShader",  []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::COMPUTE ); } },
        { "vertexShader",   []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::VERTEX ); } },
        { "geometryShader", []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::GEOMETRY ); } },
        { "fragmentShader", []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::FRAGMENT ); } },
        { "taskShader",     []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::TASK ); } },
        { "meshShader",     []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::MESH ); } },
        { "define",         []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.defines.emplace_back( ParseString( v ) ); } },
        { "generateDebugPermutation", []( const rapidjson::Value& v, PipelineCreateInfo& s ) { s.generateDebugPermutation = ParseBool( v ); } },
        { "colorAttachment", [&]( const rapidjson::Value& v, PipelineCreateInfo& s ) {
            Gfx::PipelineColorAttachmentInfo& attach = s.graphicsInfo.colorAttachments.emplace_back();
            cMapping.ForEachMember( v, attach );
        }},
        { "depthAttachment", [&]( const rapidjson::Value& v, PipelineCreateInfo& s ) { dMapping.ForEachMember( v, s.graphicsInfo.depthInfo ); }},

    });
    mapping.ForEachMember( value, *info );

    return true;
}

BEGIN_STR_TO_ENUM_MAP( Channel )
    STR_TO_ENUM_VALUE( Channel, R )
    STR_TO_ENUM_VALUE( Channel, G )
    STR_TO_ENUM_VALUE( Channel, B )
    STR_TO_ENUM_VALUE( Channel, A )
END_STR_TO_ENUM_MAP( Channel, R )

bool TexturesetParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<TexturesetCreateInfo&> mapping(
    {
        //{ "name", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.name = ParseString( v ); } },
        { "clampHorizontal", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampHorizontal = ParseBool( v ); } },
        { "clampVertical", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampVertical = ParseBool( v ); } },
        { "flipVertically", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.flipVertically = ParseBool( v ); } },
        { "albedoMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.albedoMap = ParseString( v ); } },
        { "metalnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessMap = ParseString( v ); } },
        { "metalnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessSourceChannel = Channel_StringToEnum( ParseString( v ) ); } },
        { "metalnessScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessScale = ParseNumber<f32>( v ); } },
        { "normalMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.normalMap = ParseString( v ); } },
        { "slopeScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.slopeScale = ParseNumber<f32>( v ); } },
        { "normalMapIsYUp", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.normalMapIsYUp = ParseBool( v ); } },
        { "roughnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessMap = ParseString( v ); } },
        { "roughnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessSourceChannel = Channel_StringToEnum( ParseString( v ) ); } },
        { "invertRoughness", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.invertRoughness = ParseBool( v ); } },
        { "roughnessScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessScale = ParseBool( v ); } },
        { "emissiveMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.emissiveMap = ParseString( v ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

// clang-format on

} // namespace PG
