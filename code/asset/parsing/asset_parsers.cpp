#include "asset_parsers.hpp"

using cjval = const rapidjson::Value&;

namespace PG
{

static_assert( ASSET_TYPE_COUNT == 9 );

const std::unique_ptr<BaseAssetParser> g_assetParsers[ASSET_TYPE_COUNT] = {
    std::make_unique<GfxImageParser>(),   // ASSET_TYPE_GFX_IMAGE
    std::make_unique<MaterialParser>(),   // ASSET_TYPE_MATERIAL
    std::make_unique<ScriptParser>(),     // ASSET_TYPE_SCRIPT
    std::make_unique<ModelParser>(),      // ASSET_TYPE_MODEL
    std::make_unique<NullParser>(),       // ASSET_TYPE_SHADER
    std::make_unique<UILayoutParser>(),   // ASSET_TYPE_UI_LAYOUT
    std::make_unique<PipelineParser>(),   // ASSET_TYPE_PIPELINE
    std::make_unique<FontParser>(),       // ASSET_TYPE_FONT
    std::make_unique<TexturesetParser>(), // ASSET_TYPE_TEXTURESET
};

#define BEGIN_STR_TO_ENUM_MAP( EnumName )           \
    static EnumName EnumName##_ParseEnum( cjval v ) \
    {                                               \
        static std::pair<std::string, EnumName> arr[] = {

#define BEGIN_STR_TO_ENUM_MAP_SCOPED( EnumName, Namespace )    \
    static Namespace::EnumName EnumName##_ParseEnum( cjval v ) \
    {                                                          \
        static std::pair<std::string, Namespace::EnumName> arr[] = {

#define STR_TO_ENUM_VALUE( EnumName, val ) { #val, EnumName::val },

#define END_STR_TO_ENUM_MAP( EnumName, defaultVal )                                                            \
    }                                                                                                          \
    ;                                                                                                          \
                                                                                                               \
    const std::string str = ParseString( v );                                                                  \
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

bool GfxImageParser::ParseInternal( cjval value, DerivedInfoPtr info )
{
    static JSONFunctionMapper< GfxImageCreateInfo& > mapping(
    {
        { "filename",       []( cjval v, GfxImageCreateInfo& s ) { s.filenames[0] = ParseString( v ); } },
        { "equirectangularFilename",
                            []( cjval v, GfxImageCreateInfo& s ) { s.filenames[0]  = ParseString( v ); } },
        { "left",           []( cjval v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_LEFT]   = ParseString( v ); } },
        { "right",          []( cjval v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_RIGHT]  = ParseString( v ); } },
        { "front",          []( cjval v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_FRONT]  = ParseString( v ); } },
        { "back",           []( cjval v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BACK]   = ParseString( v ); } },
        { "top",            []( cjval v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_TOP]    = ParseString( v ); } },
        { "bottom",         []( cjval v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BOTTOM] = ParseString( v ); } },
        { "flipVertically", []( cjval v, GfxImageCreateInfo& i ) { i.flipVertically = ParseBool( v ); } },
        { "semantic",       []( cjval v, GfxImageCreateInfo& i ) { i.semantic = GfxImageSemantic_ParseEnum( v ); } },
        { "dstFormat",      []( cjval v, GfxImageCreateInfo& i )
            {
                i.dstPixelFormat = PixelFormatFromString( ParseString( v ) );
                if ( i.dstPixelFormat == PixelFormat::INVALID )
                {
                    LOG_ERR( "Invalid dstFormat '%s'", v.GetString() );
                }
            }
        },
        { "clampHorizontal", []( cjval v, GfxImageCreateInfo& s ) { s.clampHorizontal = ParseBool( v ); } },
        { "clampVertical",   []( cjval v, GfxImageCreateInfo& s ) { s.clampVertical = ParseBool( v ); } },
        { "filterMode",      []( cjval v, GfxImageCreateInfo& s ) { s.filterMode = GfxImageFilterMode_ParseEnum( v ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

BEGIN_STR_TO_ENUM_MAP( MaterialType )
    STR_TO_ENUM_VALUE( MaterialType, SURFACE )
    STR_TO_ENUM_VALUE( MaterialType, DECAL )
END_STR_TO_ENUM_MAP( MaterialType, SURFACE )

bool MaterialParser::ParseInternal( cjval value, DerivedInfoPtr info )
{
    using namespace rapidjson;
    static JSONFunctionMapper< MaterialCreateInfo& > mapping(
    {
        { "type",           []( const Value& v, MaterialCreateInfo& i ) { i.type = MaterialType_ParseEnum( v ); } },
        { "textureset",     []( const Value& v, MaterialCreateInfo& i ) { i.texturesetName = ParseString( v ); } },
        { "albedoTint",     []( const Value& v, MaterialCreateInfo& i ) { i.albedoTint = ParseVec3( v ); } },
        { "metalnessTint",  []( const Value& v, MaterialCreateInfo& i ) { i.metalnessTint = ParseNumber<f32>( v ); } },
        { "emissiveTint",   []( const Value& v, MaterialCreateInfo& i ) { i.emissiveTint = ParseVec3( v ); } },
        { "roughnessTint",  []( const Value& v, MaterialCreateInfo& i ) { i.roughnessTint = ParseNumber<f32>( v ); } },
        { "applyAlbedo",    []( const Value& v, MaterialCreateInfo& i ) { i.applyAlbedo = ParseBool( v ); } },
        { "applyMetalness", []( const Value& v, MaterialCreateInfo& i ) { i.applyMetalness = ParseBool( v ); } },
        { "applyNormals",   []( const Value& v, MaterialCreateInfo& i ) { i.applyNormals = ParseBool( v ); } },
        { "applyRoughness", []( const Value& v, MaterialCreateInfo& i ) { i.applyRoughness = ParseBool( v ); } },
        { "applyEmissive",  []( const Value& v, MaterialCreateInfo& i ) { i.applyEmissive = ParseBool( v ); } },
    });
    mapping.ForEachMember( value, *info );

   return true;
}

bool ModelParser::ParseInternal( cjval value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<ModelCreateInfo&> mapping(
    {
        { "filename",                []( cjval v, ModelCreateInfo& i ) { i.filename = ParseString( v ); } },
        { "flipTexCoordsVertically", []( cjval v, ModelCreateInfo& i ) { i.flipTexCoordsVertically = ParseBool( v ); } },
        { "recalculateNormals",      []( cjval v, ModelCreateInfo& i ) { i.recalculateNormals = ParseBool( v ); } },
    });
    mapping.ForEachMember( value, *info );

    if ( info->filename.empty() )
    {
        LOG_ERR( "Must specify a filename for Model asset '%s'", info->name.c_str() );
        return false;
    }

    return true;
}

bool ScriptParser::ParseInternal( cjval value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<ScriptCreateInfo&> mapping(
    {
        { "filename", []( cjval v, ScriptCreateInfo& s ) { s.filename = ParseString( v ); } },
    });
    mapping.ForEachMember( value, *info );
    return true;
}

bool UILayoutParser::ParseInternal( cjval value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<UILayoutCreateInfo&> mapping(
    {
        { "filename", []( cjval v, UILayoutCreateInfo& s ) { s.xmlFilename = ParseString( v ); } },
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

BEGIN_STR_TO_ENUM_MAP_SCOPED( PrimitiveType, Gfx )
    STR_TO_ENUM_VALUE( Gfx::PrimitiveType, POINTS )
    STR_TO_ENUM_VALUE( Gfx::PrimitiveType, LINES )
    STR_TO_ENUM_VALUE( Gfx::PrimitiveType, LINE_STRIP )
    STR_TO_ENUM_VALUE( Gfx::PrimitiveType, TRIANGLES )
    STR_TO_ENUM_VALUE( Gfx::PrimitiveType, TRIANGLE_STRIP )
    STR_TO_ENUM_VALUE( Gfx::PrimitiveType, TRIANGLE_FAN )
END_STR_TO_ENUM_MAP( Gfx::PrimitiveType, NUM_PRIMITIVE_TYPE )

BEGIN_STR_TO_ENUM_MAP_SCOPED( WindingOrder, Gfx )
    STR_TO_ENUM_VALUE( Gfx::WindingOrder, COUNTER_CLOCKWISE )
    STR_TO_ENUM_VALUE( Gfx::WindingOrder, CLOCKWISE )
END_STR_TO_ENUM_MAP( Gfx::WindingOrder, COUNTER_CLOCKWISE )

BEGIN_STR_TO_ENUM_MAP_SCOPED( CullFace, Gfx )
    STR_TO_ENUM_VALUE( Gfx::CullFace, NONE )
    STR_TO_ENUM_VALUE( Gfx::CullFace, FRONT )
    STR_TO_ENUM_VALUE( Gfx::CullFace, BACK )
    STR_TO_ENUM_VALUE( Gfx::CullFace, FRONT_AND_BACK )
END_STR_TO_ENUM_MAP( Gfx::CullFace, NONE )

BEGIN_STR_TO_ENUM_MAP_SCOPED( PolygonMode, Gfx )
    STR_TO_ENUM_VALUE( Gfx::PolygonMode, FILL )
    STR_TO_ENUM_VALUE( Gfx::PolygonMode, LINE )
    STR_TO_ENUM_VALUE( Gfx::PolygonMode, POINT )
END_STR_TO_ENUM_MAP( Gfx::PolygonMode, FILL )

BEGIN_STR_TO_ENUM_MAP_SCOPED( BlendMode, Gfx )
    STR_TO_ENUM_VALUE( Gfx::BlendMode, NONE )
    STR_TO_ENUM_VALUE( Gfx::BlendMode, ADDITIVE )
    STR_TO_ENUM_VALUE( Gfx::BlendMode, ALPHA_BLEND )
END_STR_TO_ENUM_MAP( Gfx::BlendMode, NUM_BLEND_MODES )

bool PipelineParser::ParseInternal( cjval value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<Gfx::PipelineColorAttachmentInfo&> cMapping(
    {
        { "format", []( cjval v, Gfx::PipelineColorAttachmentInfo& s ) {
            s.format = PixelFormatFromString( ParseString( v ) );
            if ( s.format == PixelFormat::INVALID )
                LOG_WARN( "Invalid color attachment format '%s'", v.GetString() );
        } },
        { "blendMode", []( cjval v, Gfx::PipelineColorAttachmentInfo& s ) { s.blendMode = BlendMode_ParseEnum( v ); } },
    });

    static JSONFunctionMapper<Gfx::PipelineDepthInfo&> dMapping(
    {
        { "format",             []( cjval v, Gfx::PipelineDepthInfo& s ) {
            s.format = PixelFormatFromString( ParseString( v ) );
            if ( s.format == PixelFormat::INVALID )
                LOG_WARN( "Invalid depth attachment format '%s'", v.GetString() );
        } },
        { "depthTestEnabled",   []( cjval v, Gfx::PipelineDepthInfo& s ) { s.depthTestEnabled = ParseBool( v ); } },
        { "depthWriteEnabled",  []( cjval v, Gfx::PipelineDepthInfo& s ) { s.depthWriteEnabled = ParseBool( v ); } },
        { "compareFunc",        []( cjval v, Gfx::PipelineDepthInfo& s ) { s.compareFunc = CompareFunction_ParseEnum( v ); } },
    });

    static JSONFunctionMapper<Gfx::RasterizerInfo&> rMapping(
    {
        { "winding",         []( cjval v, Gfx::RasterizerInfo& r ) { r.winding = WindingOrder_ParseEnum( v ); } },
        { "cullFace",        []( cjval v, Gfx::RasterizerInfo& r ) { r.cullFace = CullFace_ParseEnum( v ); } },
        { "polygonMode",     []( cjval v, Gfx::RasterizerInfo& r ) { r.polygonMode = PolygonMode_ParseEnum( v ); } },
        { "depthBiasEnable", []( cjval v, Gfx::RasterizerInfo& r ) { r.depthBiasEnable = ParseBool( v ); } },
    });

    static JSONFunctionMapper<PipelineCreateInfo&> mapping(
    {
        { "name",           []( cjval v, PipelineCreateInfo& s ) { s.name = ParseString( v ); } },
        { "computeShader",  []( cjval v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::COMPUTE ); } },
        { "vertexShader",   []( cjval v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::VERTEX ); } },
        { "geometryShader", []( cjval v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::GEOMETRY ); } },
        { "fragmentShader", []( cjval v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::FRAGMENT ); } },
        { "taskShader",     []( cjval v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::TASK ); } },
        { "meshShader",     []( cjval v, PipelineCreateInfo& s ) { s.shaders.emplace_back( ParseString( v ), ShaderStage::MESH ); } },
        { "define",         []( cjval v, PipelineCreateInfo& s ) { s.defines.emplace_back( ParseString( v ) ); } },
        { "primitiveType",   []( cjval v, PipelineCreateInfo& s ) { s.graphicsInfo.primitiveType = PrimitiveType_ParseEnum( v ); } },
        { "generateDebugPermutation", []( cjval v, PipelineCreateInfo& s ) { s.generateDebugPermutation = ParseBool( v ); } },
        { "colorAttachment", [&]( cjval v, PipelineCreateInfo& s ) {
            Gfx::PipelineColorAttachmentInfo& attach = s.graphicsInfo.colorAttachments.emplace_back();
            cMapping.ForEachMember( v, attach );
        }},
        { "depthAttachment", [&]( cjval v, PipelineCreateInfo& s ) { dMapping.ForEachMember( v, s.graphicsInfo.depthInfo ); } },
        { "rasterizerInfo",  [&]( cjval v, PipelineCreateInfo& s ) { rMapping.ForEachMember( v, s.graphicsInfo.rasterizerInfo ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

bool FontParser::ParseInternal( cjval value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<FontCreateInfo&> mapping(
    {
        { "filename",          []( cjval v, FontCreateInfo& s ) { s.filename = ParseString( v ); } },
        { "glyphSize",         []( cjval v, FontCreateInfo& s ) { s.glyphSize = ParseNumber<i32>( v ); } },
        { "maxSignedDistance", []( cjval v, FontCreateInfo& s ) { s.maxSignedDistance = ParseNumber<f32>( v ); } },
    });
    mapping.ForEachMember( value, *info );

    if ( info->glyphSize < 8 || info->glyphSize > 128 )
    {
        info->glyphSize = Max( 8, Min( 128, info->glyphSize ) );
        LOG_WARN( "glyphSize for font '%s' needs to be within [8, 128]. Clamping to %d", info->name.c_str(), info->glyphSize );
    }
    if ( info->maxSignedDistance < 0.0f || info->maxSignedDistance > 0.5f )
    {
        info->maxSignedDistance = Max( 0.0f, Min( 0.5f, info->maxSignedDistance ) );
        LOG_WARN( "maxSignedDistance for font '%s' needs to be within [0.0, 0.5]. Clamping to %.2f", info->name.c_str(), info->maxSignedDistance );
    }

    return true;
}

BEGIN_STR_TO_ENUM_MAP( Channel )
    STR_TO_ENUM_VALUE( Channel, R )
    STR_TO_ENUM_VALUE( Channel, G )
    STR_TO_ENUM_VALUE( Channel, B )
    STR_TO_ENUM_VALUE( Channel, A )
END_STR_TO_ENUM_MAP( Channel, R )

bool TexturesetParser::ParseInternal( cjval value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<TexturesetCreateInfo&> mapping(
    {
        { "name",                   []( cjval v, TexturesetCreateInfo& s ) { s.name = ParseString( v ); } },
        { "clampHorizontal",        []( cjval v, TexturesetCreateInfo& s ) { s.clampHorizontal = ParseBool( v ); } },
        { "clampVertical",          []( cjval v, TexturesetCreateInfo& s ) { s.clampVertical = ParseBool( v ); } },
        { "flipVertically",         []( cjval v, TexturesetCreateInfo& s ) { s.flipVertically = ParseBool( v ); } },
        { "albedoMap",              []( cjval v, TexturesetCreateInfo& s ) { s.albedoMap = ParseString( v ); } },
        { "metalnessMap",           []( cjval v, TexturesetCreateInfo& s ) { s.metalnessMap = ParseString( v ); } },
        { "metalnessSourceChannel", []( cjval v, TexturesetCreateInfo& s ) { s.metalnessSourceChannel = Channel_ParseEnum( v ); } },
        { "metalnessScale",         []( cjval v, TexturesetCreateInfo& s ) { s.metalnessScale = ParseNumber<f32>( v ); } },
        { "normalMap",              []( cjval v, TexturesetCreateInfo& s ) { s.normalMap = ParseString( v ); } },
        { "slopeScale",             []( cjval v, TexturesetCreateInfo& s ) { s.slopeScale = ParseNumber<f32>( v ); } },
        { "normalMapIsYUp",         []( cjval v, TexturesetCreateInfo& s ) { s.normalMapIsYUp = ParseBool( v ); } },
        { "roughnessMap",           []( cjval v, TexturesetCreateInfo& s ) { s.roughnessMap = ParseString( v ); } },
        { "roughnessSourceChannel", []( cjval v, TexturesetCreateInfo& s ) { s.roughnessSourceChannel = Channel_ParseEnum( v ); } },
        { "invertRoughness",        []( cjval v, TexturesetCreateInfo& s ) { s.invertRoughness = ParseBool( v ); } },
        { "roughnessScale",         []( cjval v, TexturesetCreateInfo& s ) { s.roughnessScale = ParseBool( v ); } },
        { "emissiveMap",            []( cjval v, TexturesetCreateInfo& s ) { s.emissiveMap = ParseString( v ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

// clang-format on

} // namespace PG
