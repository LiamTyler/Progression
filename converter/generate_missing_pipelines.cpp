#include "generate_missing_pipelines.hpp"
#include "asset/asset_manager.hpp"
#include "asset/asset_versions.hpp"
#include "asset/shader_preprocessor.hpp"
#include "converter.hpp"
#include "converters/model_converter.hpp"
#include "core/scene.hpp"
#include "ecs/components/model_renderer.hpp"
#include "renderer/r_renderpasses.hpp"
#include "utils/bitops.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/hash.hpp"
#include <fstream>
#include <unordered_map>
#include <unordered_set>


enum ModelShaderDefines : uint8_t
{
    MODEL_HAS_NORMALS,
    MODEL_HAS_UVS,
    MODEL_HAS_TANGENTS,

    NUM_MODEL_SHADER_DEFINES
};

const char* ModelShaderDefinesStrings[] =
{
    "MODEL_HAS_NORMALS",
    "MODEL_HAS_UVS",
    "MODEL_HAS_TANGENTS",
};

enum MaterialShaderDefines : uint8_t
{
    MTL_HAS_ALBEDO_MAP,

    NUM_MTL_SHADER_DEFINES
};

const char* MaterialShaderDefinesStrings[] =
{
    "MTL_HAS_ALBEDO_MAP",
};

enum GeneralShaderDefines : uint8_t
{
    DEPTH_ONLY,

    NUM_GENERAL_SHADER_DEFINES
};

const char* GeneralShaderDefinesStrings[] =
{
    "DEPTH_ONLY",
};


typedef uint32_t FeatureMask;
static_assert( NUM_MODEL_SHADER_DEFINES + NUM_MTL_SHADER_DEFINES + NUM_GENERAL_SHADER_DEFINES <= 32, "Exceeding 32 shader permutation features, might need to increase FeatureMask size from uint32_t" );


static std::string s_materialTypeVertexShaders[] =
{
    "model.vert", // MaterialType::LIT
};

static std::string s_materialTypeFragmentShaders[] =
{
    "lit.frag", // MaterialType::LIT
};


std::string GetNextToken( const char* str, int len, int& index )
{
    while ( index < len && isspace( str[index] ) ) ++index;

    int start = index;
    if ( start == len )
    {
        return "";
    }
    while ( index < len && !isspace( str[index] ) ) ++index;

    return std::string( str + start, index - start );
}


std::string GetNextToken( const std::string& str, int& index )
{
    return GetNextToken( str.c_str(), static_cast< int >( str.length() ), index );
}


void SplitString( const std::string& str, char delim, std::vector< std::string >& results )
{
    results.clear();
    if ( str.empty() )
    {
        return;
    }

    size_t lastPos = 0;
    size_t pos = str.find( delim, lastPos );
    while ( pos != std::string::npos )
    {
        results.emplace_back( str.substr( lastPos, pos - lastPos ) );
        lastPos = pos + 1;
        pos = str.find( delim, lastPos );
    }
    results.emplace_back( str.substr( lastPos ) );
}


class ShaderPreprocDatabase
{
public:
    ShaderPreprocDatabase() = default;

    ShaderPreprocDatabase( const std::string& filename ) : m_shaderFilename( filename )
    {
        Load();
    }


    bool Load()
    {
        const std::string databasePath = PG_ASSET_DIR "cache/shader_preproc/" + m_shaderFilename;
        std::ifstream in( databasePath );
        if ( !in )
        {
            return true;
        }

        time_t shaderTimestamp = GetFileTimestamp( PG_ASSET_DIR "shaders/" + m_shaderFilename );
        if ( !shaderTimestamp )
        {
            LOG_ERR( "Could not get timestamp for source glsl file '%s'", m_shaderFilename.c_str() );
            return false;
        }

        std::string shaderVersionStr;
        std::getline( in, shaderVersionStr );
        uint32_t shaderVersion = std::stoul( shaderVersionStr );
        if ( shaderVersion != PG::PG_SHADER_VERSION )
        {
            return true;
        }

        std::string hashStr, includesStr, pipelineMasksStr;
        std::vector< std::string > splitStringResults;
        splitStringResults.reserve( 100 );
        while ( std::getline( in, hashStr ) )
        {
            std::getline( in, includesStr );
            std::getline( in, pipelineMasksStr );
            std::string compiledShaderFilename = PG_ASSET_DIR "cache/shaders/" + m_shaderFilename + "_" + hashStr + ".spv";
            if ( IsFileOutOfDate( compiledShaderFilename, shaderTimestamp ) )
            {
                continue;
            }
            
            SplitString( includesStr, ',', splitStringResults );
            splitStringResults.pop_back(); // Save function writes out an extra comma
            if ( IsFileOutOfDate( compiledShaderFilename, splitStringResults ) )
            {
                continue;
            }
            size_t hash = std::stoull( hashStr );
            m_preprocHashSet.insert( hash );
            m_hashToIncludedFiles[hash] = std::move( splitStringResults );

            splitStringResults = {};
            SplitString( pipelineMasksStr, ',', splitStringResults );
            splitStringResults.pop_back(); // Save function writes out an extra comma
            std::vector< FeatureMask > masks;
            masks.reserve( splitStringResults.size() );
            for ( const std::string& maskStr : splitStringResults )
            {
                FeatureMask featMask = std::stoul( maskStr );
                m_featureMaskToFileHash[featMask] = featMask;
                masks.push_back( featMask );
            }
        }

        return true;
    }


    void Save()
    {
        const std::string databasePath = PG_ASSET_DIR "cache/shader_preproc/" + m_shaderFilename;
        std::ofstream out( databasePath );
        out << PG::PG_SHADER_VERSION << "\n";
        for ( const size_t hash : m_preprocHashSet )
        {
            out << hash << "\n";
            const std::vector< std::string >& includes = m_hashToIncludedFiles[hash];
            for ( const auto& include : includes )
            {
                out << include << ",";
            }
            out << "\n";
            const auto& pipelines = m_hashToFeatureMasks[hash];
            for ( const FeatureMask& mask: pipelines )
            {
                out << mask << ",";
            }
            out << "\n";
        }
    }

    
    bool ContainsFeatureMask( FeatureMask featureMask, size_t& fileHash )
    {
        auto it = m_featureMaskToFileHash.find( featureMask );
        if ( it != m_featureMaskToFileHash.end() )
        {
            fileHash = it->second;
            return true;
        }

        return false;
    }


    bool ContainsShaderHash( size_t fileHash )
    {
        return m_preprocHashSet.find( fileHash ) != m_preprocHashSet.end();
    }


    void AddFileHash( size_t fileHash, std::vector< std::string >&& includedFiles )
    {
        m_preprocHashSet.insert( fileHash );
        m_hashToIncludedFiles[fileHash] = std::move( includedFiles );
    }


    void AddFeatureMask( FeatureMask mask, size_t fileHash )
    {
        m_preprocHashSet.insert( fileHash );
        m_featureMaskToFileHash[mask] = fileHash;
        m_hashToFeatureMasks[fileHash].push_back( mask );
    }

private:
    std::unordered_set< size_t > m_preprocHashSet;
    std::unordered_map< size_t, std::vector< FeatureMask > > m_hashToFeatureMasks;
    std::unordered_map< FeatureMask, size_t > m_featureMaskToFileHash;
    std::unordered_map< size_t, std::vector< std::string > > m_hashToIncludedFiles;
    std::string m_shaderFilename;
};


static std::unordered_map< std::string, ShaderPreprocDatabase > s_shaderDatabases;

namespace PG
{

static std::string GetShaderName( const std::string& entryFile, size_t preprocHash )
{
    return entryFile + "_" + std::to_string( preprocHash );
}


static bool FindOrCreateShader( FeatureMask modelFeatureMask, FeatureMask materialFeatureMask, FeatureMask generalFeatureMask, const std::string& shaderRelativePath, ShaderStage shaderStage, std::string& outShaderName )
{
    FeatureMask totalMask = 0;
    totalMask |= modelFeatureMask;
    totalMask |= materialFeatureMask << NUM_MODEL_SHADER_DEFINES;
    totalMask |= generalFeatureMask << (NUM_MODEL_SHADER_DEFINES + NUM_MTL_SHADER_DEFINES);

    ShaderPreprocDatabase& shaderDB = s_shaderDatabases[shaderRelativePath];
    size_t shaderHash;
    if ( shaderDB.ContainsFeatureMask( totalMask, shaderHash ) )
    {
        outShaderName = GetShaderName( shaderRelativePath, shaderHash );
        return true;
    }

    ShaderDefineList defines;
    defines.reserve( 40 );

    ForEachBit( modelFeatureMask, [&defines]( uint32_t feature )
    {
        defines.emplace_back( ModelShaderDefinesStrings[feature], "1" );
    });
    ForEachBit( materialFeatureMask, [&defines]( uint32_t feature )
    {
        defines.emplace_back( MaterialShaderDefinesStrings[feature], "1" );
    });
    ForEachBit( generalFeatureMask, [&defines]( uint32_t feature )
    {
        defines.emplace_back( GeneralShaderDefinesStrings[feature], "1" );
    });

    ShaderPreprocessOutput preproc = PreprocessShader( PG_ASSET_DIR "shaders/" + shaderRelativePath, defines, shaderStage );
    if ( !preproc.success )
    {
        return false;
    }

    shaderHash = HashString( preproc.outputShader );
    if ( shaderDB.ContainsShaderHash( shaderHash ) )
    {
        outShaderName = GetShaderName( shaderRelativePath, shaderHash );
        shaderDB.AddFeatureMask( totalMask, shaderHash );
        return true;
    }

    std::string path = shaderRelativePath + "_" + std::to_string( shaderHash );
    ShaderCreateInfo createInfo;
    createInfo.name = path;
    createInfo.shaderStage = shaderStage;
    createInfo.generateDebugInfo = g_converterConfigOptions.generateShaderDebugInfo;
    createInfo.preprocOutput = std::move( preproc.outputShader );

    Shader shader;
    if ( !Shader_Load( &shader, createInfo ) )
    {
        return false;
    }

    Serializer serializer;
    if ( !serializer.OpenForWrite( PG_ASSET_DIR "cache/shaders/" + path ) )
    {
        return false;
    }

    Fastfile_Shader_Save( &shader, &serializer );
    serializer.Close();

    shaderDB.AddFileHash( shaderHash, std::move( preproc.includedFiles ) );
    shaderDB.AddFeatureMask( totalMask, shaderHash );
    outShaderName = GetShaderName( shaderRelativePath, shaderHash );

    return true;
}


struct GraphicsPipelineSerializeInfo
{
    std::string vertexShader;
    std::string fragmentShader;
    uint32_t modelVertexFeatures;
    Gfx::RenderPasses renderPass;
    // rasterInfo
    // depthInfo
    // colorAttachments
};

static bool GenerateSinglePipeline( FeatureMask modelFeatureMask, FeatureMask materialFeatureMask, FeatureMask generalFeatureMask, MaterialType mtlType )
{
    std::vector< std::pair< ShaderStage, std::string > > inputShaders =
    {
        { ShaderStage::VERTEX,   s_materialTypeVertexShaders[mtlType] },
        { ShaderStage::FRAGMENT, s_materialTypeFragmentShaders[mtlType] },
    };

    GraphicsPipelineSerializeInfo info;

    FeatureMask generalFeatureMask = 0;
    for ( const auto& [stage, file] : inputShaders )
    {
        std::string shaderName;
        if ( !file.empty() )
        {
            if ( !FindOrCreateShader( modelFeatureMask, materialFeatureMask, generalFeatureMask, file, stage, shaderName ) )
            {
                return false;
            }

            switch ( stage )
            {
            case ShaderStage::VERTEX:
                info.vertexShader = shaderName;
                break;
            case ShaderStage::FRAGMENT:
                info.fragmentShader = shaderName;
                break;
            }
        }
    }
}


static bool GeneratePipelineSet( FeatureMask modelFeatureMask, FeatureMask materialFeatureMask, MaterialType mtlType )
{
    GraphicsPipelineSerializeInfo info;

    FeatureMask generalFeatureMask = 0;
    switch( mtlType )
    {

    }
}


bool GenerateMissingPipelines( Scene* scene )
{
    scene->registry.view< ModelRenderer >().each( [&]( ModelRenderer& renderer )
    {
        ModelHeader header;
        if ( !GetModelHeader( renderer.modelName, &header ) )
        {
            LOG_ERR( "Could not read header for model '%s'", renderer.modelName.c_str() );
            return;
        }

        FeatureMask modelFeatureMask = 0;
        if ( header.numNormals > 0 )    modelFeatureMask |= 1 << MODEL_HAS_NORMALS;
        if ( header.numTexCoords > 0 )  modelFeatureMask |= 1 << MODEL_HAS_UVS;
        if ( header.numTangents > 0 )   modelFeatureMask |= 1 << MODEL_HAS_TANGENTS;

        std::vector< std::string > usedMaterials;
        if ( renderer.materialOverride.empty() )
        {
            usedMaterials = header.usedMaterials;
        }
        else
        {
            usedMaterials = { renderer.materialOverride };
        }

        for ( size_t i = 0; i < usedMaterials.size(); ++i )
        {
            const std::string materialName = usedMaterials[i];
            Material* material = AssetManager::Get< Material >( materialName );

            FeatureMask materialFeatureMask = 0;
            if ( !material->map_Kd_name.empty() ) materialFeatureMask |= 1 << MTL_HAS_ALBEDO_MAP;

             ( modelFeatureMask, materialFeatureMask, material->type );
        }
    });

    return true;
}


bool AddShaderDatabase( const std::string& shader )
{
    s_shaderDatabases[shader] = ShaderPreprocDatabase( shader );
    if ( !s_shaderDatabases[shader].Load() )
    {
        LOG_ERR( "Could not init shader database '%s'", shader.c_str() );
        return false;
    }

    return true;
}


bool InitPipelineGenerator()
{
    bool success = true;
    for ( int i = 0; i < NUM_MATERIAL_TYPES; ++i )
    {
        success = success && AddShaderDatabase( s_materialTypeVertexShaders[i] );
        success = success && AddShaderDatabase( s_materialTypeFragmentShaders[i] );
    }

    return success;
}


void ShutdownPipelineGenerator()
{
    for ( auto& db : s_shaderDatabases )
    {
        db.second.Save();
    }
}

} // namespace PG