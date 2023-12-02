#include "model_exporter_materials.hpp"
#include "assimp/GltfMaterial.h"
#include "shared/filesystem.hpp"
#include "shared/string.hpp"
#include <optional>

std::string g_textureSearchDir;

struct ImageInfo
{
    std::string name;
    std::string filename;
    std::string semantic;
    std::string type;
};

static bool GetAssimpTexturePath( const aiMaterial* assimpMat, aiTextureType texType, std::string& absPathToTex )
{
    aiString path;
    namespace fs   = std::filesystem;
    uint32_t uvSet = UINT_MAX;
    if ( assimpMat->GetTexture( texType, 0, &path, NULL, &uvSet, NULL, NULL, NULL ) == AI_SUCCESS )
    {
        std::string name = StripWhitespace( path.data );
        absPathToTex     = name;
        // LOG( "UVSet: %u", uvSet );

        std::string fullPath;
        std::string basename = GetRelativeFilename( name );
        for ( auto itEntry = fs::recursive_directory_iterator( g_textureSearchDir ); itEntry != fs::recursive_directory_iterator();
              ++itEntry )
        {
            std::string itFile = itEntry->path().filename().string();
            if ( basename == itEntry->path().filename().string() )
            {
                fullPath = GetAbsolutePath( itEntry->path().string() );
                break;
            }
        }

        if ( fullPath == "" )
        {
            return false;
        }
        absPathToTex = fullPath;
    }
    else
    {
        LOG_ERR( "Could not get texture of type: '%s'", aiTextureTypeToString( texType ) );
        return false;
    }

    return true;
}

static std::optional<std::string> GetAssimpTexture(
    const aiMaterial* assimpMat, aiTextureType texType, const std::string& texTypeStr, const std::string& matName )
{
    std::string imagePath;
    if ( assimpMat->GetTextureCount( texType ) > 0 )
    {
        if ( assimpMat->GetTextureCount( texType ) > 1 )
        {
            LOG_WARN( "Material '%s' has more than 1 %s texture", matName.c_str(), texTypeStr.c_str() );
        }
        else if ( !GetAssimpTexturePath( assimpMat, texType, imagePath ) )
        {
            LOG_WARN( "Material '%s': can't find %s texture, using filename '%s'", matName.c_str(), texTypeStr.c_str(), imagePath.c_str() );
        }

        if ( !imagePath.empty() )
        {
            std::string relPath = GetRelativePathToDir( imagePath, g_options.rootDir );
            if ( !relPath.empty() )
                imagePath = relPath;
        }
    }

    if ( !imagePath.empty() )
        return imagePath;

    return {};
}

static std::optional<vec3> GetVec3( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    aiColor3D v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return vec3( v.r, v.g, v.b );
    return {};
}

static std::optional<float> GetFloat( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    ai_real v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return (float)v;
    return {};
}

static std::optional<int> GetInt( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    ai_int v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return (int)v;
    return {};
}

static std::optional<std::string> GetString( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    aiString v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return std::string( v.C_Str() );
    return {};
}

static std::optional<bool> GetBool( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    bool v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return v;
    return {};
}

const vec3 DEFAULT_ALBEDO_TINT     = vec3( 1 );
const float DEFAULT_METALLIC_TINT  = 1.0f;
const float DEFAULT_ROUGHNESS_TINT = 1.0f;
const vec3 DEFAULT_EMISSIVE_TINT   = vec3( 0 );

bool OutputMaterial( const MaterialContext& context, std::string& outputJSON )
{
    // gltf assumes packed metallic roughness textures with metallic in G and roughness in B
    bool sourceIsGLTF = GetFileExtension( context.file ) == ".gltf";

    // Non-texture settings
    std::vector<std::string> matSettings;
    std::string matName = GetUniqueAssetName( ASSET_TYPE_MATERIAL, context.localMatName );
    AddJSON( matSettings, "name", matName );

    auto albedoTint = GetVec3( context.assimpMat, AI_MATKEY_BASE_COLOR );
    if ( !albedoTint )
        albedoTint = GetVec3( context.assimpMat, AI_MATKEY_COLOR_DIFFUSE );
    auto metalnessTint = GetFloat( context.assimpMat, AI_MATKEY_METALLIC_FACTOR );
    auto roughnessTint = GetFloat( context.assimpMat, AI_MATKEY_ROUGHNESS_FACTOR );
    auto emissiveTint  = GetVec3( context.assimpMat, AI_MATKEY_COLOR_EMISSIVE );
    // auto specularTint = GetVec3( context.assimpMat, AI_MATKEY_COLOR_SPECULAR );

    if ( albedoTint && albedoTint != DEFAULT_ALBEDO_TINT )
        AddJSON( matSettings, "albedoTint", *albedoTint );
    if ( metalnessTint && metalnessTint != DEFAULT_METALLIC_TINT )
        AddJSON( matSettings, "metalnessTint", *metalnessTint );
    if ( roughnessTint && roughnessTint != DEFAULT_ROUGHNESS_TINT )
        AddJSON( matSettings, "roughnessTint", *roughnessTint );
    if ( emissiveTint && emissiveTint != DEFAULT_EMISSIVE_TINT )
        AddJSON( matSettings, "emissiveTint", *emissiveTint );

    // Textures
    std::vector<std::string> texSettings;
    texSettings.push_back( "" ); // reserve slot for name (calculated later)

    // so with any tintable texture below, if there was a non-default tint specified, but NO texture of that same
    // type specified, then assign the $white texture anyways, so that the final result in the shader is the specified tint
    if ( auto albedoMap = GetAssimpTexture( context.assimpMat, aiTextureType_BASE_COLOR, "baseColor", matName ) )
        AddJSON( texSettings, "albedoMap", *albedoMap );
    else if ( auto albedoMap = GetAssimpTexture( context.assimpMat, aiTextureType_DIFFUSE, "diffuse", matName ) )
        AddJSON( texSettings, "albedoMap", *albedoMap );
    else if ( albedoTint && albedoTint != vec3( 1 ) )
        AddJSON( texSettings, "albedoMap", "$white" );

    if ( auto normalMap = GetAssimpTexture( context.assimpMat, aiTextureType_NORMALS, "normal", matName ) )
    {
        AddJSON( texSettings, "normalMap", *normalMap );
        auto slopeScale = GetFloat( context.assimpMat, AI_MATKEY_GLTF_TEXTURE_SCALE( aiTextureType_NORMALS, 0 ) );
        if ( slopeScale && slopeScale != 1.0f )
            AddJSON( texSettings, "slopeScale", *slopeScale );
    }

    auto metalnessMap = GetAssimpTexture( context.assimpMat, aiTextureType_METALNESS, "metalness", matName );
    if ( metalnessMap )
    {
        AddJSON( texSettings, "metalnessMap", *metalnessMap );
        if ( sourceIsGLTF )
            AddJSON( texSettings, "metalnessSourceChannel", "B" );
    }
    else if ( metalnessTint && *metalnessTint != 1.0 )
        AddJSON( texSettings, "metalnessMap", "$white" );

    if ( auto roughnessMap = GetAssimpTexture( context.assimpMat, aiTextureType_DIFFUSE_ROUGHNESS, "roughness", matName ) )
    {
        AddJSON( texSettings, "roughnessMap", *roughnessMap );
        if ( sourceIsGLTF )
            AddJSON( texSettings, "roughnessSourceChannel", "G" );
    }
    else if ( auto roughnessMap = GetAssimpTexture( context.assimpMat, aiTextureType_SHININESS, "roughness", matName ) )
    {
        AddJSON( texSettings, "roughnessMap", *roughnessMap );
        if ( sourceIsGLTF )
            AddJSON( texSettings, "roughnessSourceChannel", "G" );
    }
    else if ( roughnessTint && *roughnessTint != 1.0 )
        AddJSON( texSettings, "roughnessMap", "$white" );

    if ( auto emissiveMap = GetAssimpTexture( context.assimpMat, aiTextureType_EMISSIVE, "emissive", matName ) )
        AddJSON( texSettings, "emissiveMap", *emissiveMap );
    else if ( emissiveTint && emissiveTint != vec3( 0 ) )
        AddJSON( texSettings, "emissiveMap", "$white" );

    // Only name + output a textureset if it has non-default values
    if ( texSettings.size() > 1 )
    {
        std::string texturesetName = GetUniqueAssetName( ASSET_TYPE_TEXTURESET, matName );
        AddJSON( texSettings, "name", texturesetName );
        AddJSON( matSettings, "textureset", texturesetName );
        std::swap( texSettings[0], texSettings[texSettings.size() - 1] );
        texSettings.pop_back();

        outputJSON += "\t{ \"Textureset\": {\n";
        for ( size_t i = 0; i < texSettings.size() - 1; ++i )
            outputJSON += "\t\t" + texSettings[i] + ",\n";
        outputJSON += "\t\t" + texSettings[texSettings.size() - 1] + "\n";
        outputJSON += "\t} },\n";
    }

    outputJSON += "\t{ \"Material\": {\n";
    for ( size_t i = 0; i < matSettings.size() - 1; ++i )
        outputJSON += "\t\t" + matSettings[i] + ",\n";
    outputJSON += "\t\t" + matSettings[matSettings.size() - 1] + "\n";
    outputJSON += "\t} },\n";

#if 0
    std::vector<std::string> allMatSettings;
    if ( auto v = GetString( context.assimpMat, AI_MATKEY_NAME ) ) AddJSON( allMatSettings, "Name", *v );
    if ( auto v = GetBool( context.assimpMat, AI_MATKEY_TWOSIDED ) ) AddJSON( allMatSettings, "Two Sided", *v );
    if ( auto v = GetInt( context.assimpMat, AI_MATKEY_SHADING_MODEL ) ) AddJSON( allMatSettings, "Shading Model", *v );
    if ( auto v = GetBool( context.assimpMat, AI_MATKEY_ENABLE_WIREFRAME ) ) AddJSON( allMatSettings, "Enable Wireframe", *v );
    if ( auto v = GetInt( context.assimpMat, AI_MATKEY_BLEND_FUNC ) ) AddJSON( allMatSettings, "BlendFunc", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_OPACITY ) ) AddJSON( allMatSettings, "Opacity", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_TRANSPARENCYFACTOR ) ) AddJSON( allMatSettings, "Transparency Factor", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_BUMPSCALING ) ) AddJSON( allMatSettings, "BumpScaling", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_SHININESS ) ) AddJSON( allMatSettings, "Shininess", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_REFLECTIVITY ) ) AddJSON( allMatSettings, "Reflectivity", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_SHININESS_STRENGTH ) ) AddJSON( allMatSettings, "Shininess Strength", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_REFRACTI ) ) AddJSON( allMatSettings, "Refracti", *v );
    if ( auto v = GetVec3( context.assimpMat, AI_MATKEY_COLOR_DIFFUSE ) ) AddJSON( allMatSettings, "Diffuse", *v );
    if ( auto v = GetVec3( context.assimpMat, AI_MATKEY_COLOR_AMBIENT ) ) AddJSON( allMatSettings, "Ambient", *v );
    if ( auto v = GetVec3( context.assimpMat, AI_MATKEY_COLOR_SPECULAR ) ) AddJSON( allMatSettings, "Specular", *v );
    if ( auto v = GetVec3( context.assimpMat, AI_MATKEY_COLOR_EMISSIVE ) ) AddJSON( allMatSettings, "Emissive", *v );
    if ( auto v = GetVec3( context.assimpMat, AI_MATKEY_COLOR_TRANSPARENT ) ) AddJSON( allMatSettings, "Transparent", *v );
    if ( auto v = GetVec3( context.assimpMat, AI_MATKEY_COLOR_REFLECTIVE ) ) AddJSON( allMatSettings, "Reflective", *v );
    if ( auto v = GetBool( context.assimpMat, AI_MATKEY_USE_COLOR_MAP ) ) AddJSON( allMatSettings, "Use Color Map", *v );
    if ( auto v = GetVec3( context.assimpMat, AI_MATKEY_BASE_COLOR ) ) AddJSON( allMatSettings, "Base Color", *v );
    if ( auto v = GetBool( context.assimpMat, AI_MATKEY_USE_METALLIC_MAP ) ) AddJSON( allMatSettings, "Use Metallic Map", *v );

    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_METALLIC_FACTOR ) ) AddJSON( allMatSettings, "Metallic Factor", *v );
    if ( auto v = GetBool( context.assimpMat, AI_MATKEY_USE_ROUGHNESS_MAP ) ) AddJSON( allMatSettings, "Use Roughness Map", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_ROUGHNESS_FACTOR ) ) AddJSON( allMatSettings, "Roughness Factor", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_ANISOTROPY_FACTOR ) ) AddJSON( allMatSettings, "Anisotropy Factor", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_SPECULAR_FACTOR ) ) AddJSON( allMatSettings, "Specular Factor", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_GLOSSINESS_FACTOR ) ) AddJSON( allMatSettings, "Glossiness Factor", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_SHEEN_COLOR_FACTOR ) ) AddJSON( allMatSettings, "Sheen Color Factor", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_SHEEN_ROUGHNESS_FACTOR ) ) AddJSON( allMatSettings, "Sheen Roughness Factor", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_CLEARCOAT_FACTOR ) ) AddJSON( allMatSettings, "Clearcoat Factor", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR ) ) AddJSON( allMatSettings, "Clearcoat Roughness Factor ", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_TRANSMISSION_FACTOR ) ) AddJSON( allMatSettings, "Transmission Factor", *v );
    if ( auto v = GetBool( context.assimpMat, AI_MATKEY_USE_EMISSIVE_MAP ) ) AddJSON( allMatSettings, "Use Emissive Map", *v );
    if ( auto v = GetFloat( context.assimpMat, AI_MATKEY_EMISSIVE_INTENSITY ) ) AddJSON( allMatSettings, "Emissive Intensity", *v );
    if ( auto v = GetBool( context.assimpMat, AI_MATKEY_USE_AO_MAP ) ) AddJSON( allMatSettings, "Use AO Map", *v );

    for ( size_t i = 0; i < allMatSettings.size(); ++i )
    {
        LOG( "%s", allMatSettings[i].c_str() );
    }

    aiString oString;
    aiReturn a = aiGetMaterialString( context.assimpMat, "$tex.file", UINT_MAX, 0, &oString );
    std::string actualString( oString.C_Str() );

    for ( int i = 0; i < (int)AI_TEXTURE_TYPE_MAX; ++i )
    {
        auto t = (aiTextureType)(aiTextureType_DIFFUSE + i);
        int x = context.assimpMat->GetTextureCount( t );
        if ( x )
        {
            LOG( "%s: %d", aiTextureTypeToString( t ), x );
            std::string imagePath;
            GetAssimpTexturePath( context.assimpMat, t, imagePath );
            LOG( "\t%s", imagePath.c_str() );
        }
    }
#endif

    return true;
}
