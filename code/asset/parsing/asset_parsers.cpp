#include "asset_parsers.hpp"

namespace PG
{

static_assert( ASSET_TYPE_COUNT == 7 );

const std::shared_ptr<BaseAssetParser> g_assetParsers[ASSET_TYPE_COUNT] =
{
    std::make_shared<GfxImageParser>(),   // ASSET_TYPE_GFX_IMAGE
    std::make_shared<MaterialParser>(),   // ASSET_TYPE_MATERIAL
    std::make_shared<ScriptParser>(),     // ASSET_TYPE_SCRIPT
    std::make_shared<ModelParser>(),      // ASSET_TYPE_MODEL
    std::make_shared<ShaderParser>(),     // ASSET_TYPE_SHADER
    std::make_shared<UILayoutParser>(),   // ASSET_TYPE_UI_LAYOUT
    std::make_shared<TexturesetParser>(), // ASSET_TYPE_TEXTURESET
};


#define BEGIN_STR_TO_ENUM_MAP( EnumName ) \
static EnumName EnumName ## _StringToEnum( std::string_view str ) \
{ \
    static std::pair<std::string, EnumName> arr[] = \
    {

#define BEGIN_STR_TO_ENUM_MAP_SCOPED( EnumName, Namespace ) \
static Namespace::EnumName EnumName ## _StringToEnum( std::string_view str ) \
{ \
    static std::pair<std::string, Namespace::EnumName> arr[] = \
    {

#define STR_TO_ENUM_VALUE( EnumName, val ) { #val, EnumName::val },

#define END_STR_TO_ENUM_MAP( EnumName, defaultVal ) \
    }; \
       \
    for ( int i = 0; i < ARRAY_COUNT( arr ); ++i ) \
    { \
        if ( arr[i].first == str ) \
        { \
            return arr[i].second; \
        } \
    } \
      \
    LOG_WARN( "No " #EnumName " found with name '%s'. Using default '" #defaultVal "' instead.", str.data() ); \
    return EnumName::defaultVal; \
}

std::string String( const rapidjson::Value& value )
{
    PG_ASSERT( value.IsString() );
    return value.GetString();
}



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
        { "filename", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[0] = String( v ); } },
        { "equirectangularFilename", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[0]  = String( v ); } },
        { "left",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_LEFT]   = String( v ); } },
        { "right",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_RIGHT]  = String( v ); } },
        { "front",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_FRONT]  = String( v ); } },
        { "back",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BACK]   = String( v ); } },
        { "top",    []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_TOP]    = String( v ); } },
        { "bottom", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BOTTOM] = String( v ); } },
        { "flipVertically", []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.flipVertically = v.GetBool(); } },
        { "semantic",  []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.semantic = GfxImageSemantic_StringToEnum( String( v ) ); } },
        { "dstFormat", []( const rapidjson::Value& v, GfxImageCreateInfo& i )
            {
                i.dstPixelFormat = PixelFormatFromString( String( v ) );
                if ( i.dstPixelFormat == PixelFormat::INVALID )
                {
                    LOG_ERR( "Invalid dstFormat '%s'", v.GetString() );
                }
            }
        },
        { "clampHorizontal", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.clampHorizontal = v.GetBool(); } },
        { "clampVertical",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.clampVertical = v.GetBool(); } },
        { "filterMode",      []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filterMode = GfxImageFilterMode_StringToEnum( String( v ) ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}


bool MaterialParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    using namespace rapidjson;
    static JSONFunctionMapper< MaterialCreateInfo& > mapping(
    {
        { "textureset", []( const Value& v, MaterialCreateInfo& i ) { i.texturesetName = String( v ); } },
        { "albedoTint",     []( const Value& v, MaterialCreateInfo& i ) { i.albedoTint     = ParseVec3( v ); } },
        { "metalnessTint",  []( const Value& v, MaterialCreateInfo& i ) { i.metalnessTint  = ParseNumber<float>( v ); } },
        { "emissiveTint",  []( const Value& v, MaterialCreateInfo& i ) { i.emissiveTint  = ParseVec3( v ); } },
        { "roughnessTint",  []( const Value& v, MaterialCreateInfo& i ) { i.roughnessTint  = ParseNumber<float>( v ); } },
    });
    mapping.ForEachMember( value, *info );

   return true;
}


bool ModelParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<ModelCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = String( v ); } },
        { "flipTexCoordsVertically", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.flipTexCoordsVertically = v.GetBool(); } },
        { "recalculateNormals", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.recalculateNormals = v.GetBool(); } },
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
        { "filename", []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.filename = String( v ); } },
    });
    mapping.ForEachMember( value, *info );
    return true;
}


BEGIN_STR_TO_ENUM_MAP( ShaderStage )
    STR_TO_ENUM_VALUE( ShaderStage, VERTEX )
    STR_TO_ENUM_VALUE( ShaderStage, TESSELLATION_CONTROL )
    STR_TO_ENUM_VALUE( ShaderStage, TESSELLATION_EVALUATION )
    STR_TO_ENUM_VALUE( ShaderStage, GEOMETRY )
    STR_TO_ENUM_VALUE( ShaderStage, FRAGMENT )
    STR_TO_ENUM_VALUE( ShaderStage, COMPUTE )
END_STR_TO_ENUM_MAP( ShaderStage, NUM_SHADER_STAGES )

bool ShaderParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<ShaderCreateInfo&> mapping(
    {
        { "name",        []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.name = String( v ); } },
        { "filename",    []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.filename = String( v ); } },
        { "shaderStage", []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.shaderStage = ShaderStage_StringToEnum( String( v ) ); } },
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
        //{ "name", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.name = String( v ); } },
        { "clampHorizontal", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampHorizontal = v.GetBool(); } },
        { "clampVertical", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampVertical = v.GetBool(); } },
        { "flipVertically", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.flipVertically = v.GetBool(); } },
        { "albedoMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.albedoMap = String( v ); } },
        { "metalnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessMap = String( v ); } },
        { "metalnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessSourceChannel = Channel_StringToEnum( String( v ) ); } },
        { "metalnessScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessScale = ParseNumber<float>( v ); } },
        { "normalMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.normalMap = String( v ); } },
        { "slopeScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.slopeScale = ParseNumber<float>( v ); } },
        { "normalMapIsYUp", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.normalMapIsYUp = v.GetBool(); } },
        { "roughnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessMap = String( v ); } },
        { "roughnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessSourceChannel = Channel_StringToEnum( String( v ) ); } },
        { "invertRoughness", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.invertRoughness = v.GetBool(); } },
        { "roughnessScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessScale = v.GetBool(); } },
        { "emissiveMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.emissiveMap = String( v ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}


bool UILayoutParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<UILayoutCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, UILayoutCreateInfo& s ) { s.xmlFilename = String( v ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

} // namespace PG