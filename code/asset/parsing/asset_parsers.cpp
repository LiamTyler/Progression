#include "asset_parsers.hpp"

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



BEGIN_STR_TO_ENUM_MAP( GfxImageSemantic )
    STR_TO_ENUM_VALUE( GfxImageSemantic, COLOR )
    STR_TO_ENUM_VALUE( GfxImageSemantic, GRAY )
    STR_TO_ENUM_VALUE( GfxImageSemantic, ALBEDO_METALNESS )
    STR_TO_ENUM_VALUE( GfxImageSemantic, ENVIRONMENT_MAP )
END_STR_TO_ENUM_MAP( GfxImageSemantic, NUM_IMAGE_SEMANTICS )

bool GfxImageParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    #define ABS_PATH( v ) PG_ASSET_DIR + std::string( v.GetString() )
    static JSONFunctionMapper< GfxImageCreateInfo& > mapping(
    {
        { "filename", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[0] = ABS_PATH( v ); } },
        { "left",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_LEFT]   = ABS_PATH( v ); } },
        { "right",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_RIGHT]  = ABS_PATH( v ); } },
        { "front",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_FRONT]  = ABS_PATH( v ); } },
        { "back",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BACK]   = ABS_PATH( v ); } },
        { "top",    []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_TOP]    = ABS_PATH( v ); } },
        { "bottom", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BOTTOM] = ABS_PATH( v ); } },
        { "flipVertically", []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.flipVertically = v.GetBool(); } },
        { "semantic",  []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { GfxImageSemantic_StringToEnum( v.GetString() ); } },
        { "dstFormat", []( const rapidjson::Value& v, GfxImageCreateInfo& i )
            {
                i.dstPixelFormat = PixelFormatFromString( v.GetString() );
                if ( i.dstPixelFormat == PixelFormat::INVALID )
                {
                    LOG_ERR( "Invalid dstFormat '%s'", v.GetString() );
                }
            }
        },
        { "clampHorizontal", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.clampHorizontal = v.GetBool(); } },
        { "clampVertical",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.clampVertical = v.GetBool(); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}




bool MaterialParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    using namespace rapidjson;
    static JSONFunctionMapper< MaterialCreateInfo& > mapping(
    {
        { "textureset", []( const Value& v, MaterialCreateInfo& i ) { i.texturesetName = v.GetString(); } },
        { "albedo",     []( const Value& v, MaterialCreateInfo& i ) { i.albedoTint     = ParseVec3( v ); } },
        { "metalness",  []( const Value& v, MaterialCreateInfo& i ) { i.metalnessTint  = ParseNumber<float>( v ); } },
        //{ "roughness",  []( const Value& v, MaterialCreateInfo& i ) { i.roughnessTint  = ParseNumber<float>( v ); } },
    });
    mapping.ForEachMember( value, *info );

   return true;
}



bool ModelParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<ModelCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
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
        { "filename", []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
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
        { "name",        []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.name = v.GetString(); } },
        { "filename",    []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.filename = PG_ASSET_DIR "shaders/" + std::string( v.GetString() ); } },
        { "shaderStage", []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.shaderStage = ShaderStage_StringToEnum( v.GetString() ); } },
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
        { "name", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.name = v.GetString(); } },
        { "clampHorizontal", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampHorizontal = v.GetBool(); } },
        { "clampVertical", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampVertical = v.GetBool(); } },
        { "albedoMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.albedoMap = v.GetString(); } },
        { "metalnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessMap = v.GetString(); } },
        { "metalnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessSourceChannel = Channel_StringToEnum( v.GetString() ); } },
        //{ "normalMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.normalMap = v.GetString(); } },
        //{ "slopeScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.slopeScale = ParseNumber<float>( v ); } },
        //{ "roughnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessMap = v.GetString(); } },
        //{ "roughnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessSourceChannel = Channel_StringToEnum( v.GetString() ); } },
        //{ "invertRoughness", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.invertRoughness = v.GetBool(); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

} // namespace PG