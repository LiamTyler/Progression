#include "pmodel.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/string.hpp"
#include <chrono>
#include <set>

using namespace std::chrono;
using Clock     = high_resolution_clock;
using TimePoint = time_point<Clock>;

namespace PG
{

bool PModel::Vertex::AddBone( uint32_t boneIdx, float weight )
{
    if ( numBones < PMODEL_MAX_BONE_WEIGHTS_PER_VERT )
    {
        boneWeights[numBones] = weight;
        boneIndices[numBones] = boneIdx;
        ++numBones;
        return true;
    }

    float minWeight = boneWeights[0];
    uint32_t minIdx = 0;
    for ( uint32_t slot = 1; slot < numBones; ++slot )
    {
        if ( boneWeights[slot] < minWeight )
        {
            minWeight = boneWeights[slot];
            minIdx    = slot;
        }
    }

    return false;
}

static std::string GetNameAfterColon( const std::string& line )
{
    size_t startIdx = line.find( ':' );
    while ( std::isspace( line[++startIdx] ) )
        ;
    return line.substr( startIdx );
}

bool PModel::Load( const std::string& filename )
{
    std::ifstream in( filename );
    if ( !in )
    {
        LOG_ERR( "Failed to open pmodel file '%s'", filename.c_str() );
        return false;
    }

    char tmpBuffer[128];
    std::string tmp;
    in >> tmp; // pmodelFormat
    uint32_t version;
    in >> version;
    if ( version < (uint32_t)PModelVersionNum::LAST_SUPPORTED_VERSION )
    {
        LOG_ERR( "PModel file %s contains a version (%u) that is no longer supported. Please re-export the source file", filename.c_str(),
            version );
        return false;
    }

    auto SkipEmptyLines = []( std::ifstream& inFile, std::string& line )
    {
        do
        {
            std::getline( inFile, line );
        } while ( line.empty() );
    };

    //  skip to material list
    std::string line;
    SkipEmptyLines( in, line );

    // parse material list (not actually needed outside of converter)
    do
    {
        std::getline( in, line );
    } while ( !line.empty() );

    while ( std::getline( in, line ) && line.substr( 0, 4 ) == "Mesh" )
    {
        Mesh& mesh = meshes.emplace_back();
        mesh.name  = GetNameAfterColon( line );
        std::getline( in, line );
        mesh.materialName = GetNameAfterColon( line );
        std::getline( in, line );
        sscanf( line.c_str(), "%s %u", tmpBuffer, &mesh.numUVChannels );
        std::getline( in, line );
        sscanf( line.c_str(), "%s %u", tmpBuffer, &mesh.numColorChannels );

        // skip to first vertex
        SkipEmptyLines( in, line );

        uint32_t meshNumUVs        = 0;
        uint32_t meshNumColors     = 0;
        uint32_t meshNumTangents   = 0;
        uint32_t meshNumBitangents = 0;

        mesh.vertices.reserve( 65536 );
        while ( !line.empty() && line[0] == 'V' )
        {
            Vertex& v              = mesh.vertices.emplace_back();
            uint32_t numUVs        = 0;
            uint32_t numColors     = 0;
            uint32_t numTangents   = 0;
            uint32_t numBitangents = 0;

            std::getline( in, line );
            while ( !line.empty() )
            {
                if ( line[0] == 'p' )
                    sscanf( line.c_str(), "%s %f %f %f", tmpBuffer, &v.pos.x, &v.pos.y, &v.pos.z );
                else if ( line[0] == 'n' )
                    sscanf( line.c_str(), "%s %f %f %f", tmpBuffer, &v.normal.x, &v.normal.y, &v.normal.z );
                else if ( line[0] == 't' )
                {
                    sscanf( line.c_str(), "%s %f %f %f", tmpBuffer, &v.tangent.x, &v.tangent.y, &v.tangent.z );
                    ++numTangents;
                }
                else if ( line[0] == 'u' && line[1] == 'v' )
                {
                    sscanf( line.c_str(), "%s %f %f", tmpBuffer, &v.uvs[numUVs].x, &v.uvs[numUVs].y );
                    ++numUVs;
                }
                else if ( line[0] == 'c' )
                {
                    sscanf( line.c_str(), "%s %f %f %f %f", tmpBuffer, &v.colors[numColors].x, &v.colors[numColors].y,
                        &v.colors[numColors].z, &v.colors[numColors].w );
                    ++numColors;
                }
                else if ( line[0] == 'b' && line[1] == 'w' )
                {
                    sscanf( line.c_str(), "%s %u %f", tmpBuffer, &v.boneIndices[v.numBones], &v.boneWeights[v.numBones] );
                    ++v.numBones;
                }
                else if ( line[0] == 'b' )
                {
                    sscanf( line.c_str(), "%s %f %f %f", tmpBuffer, &v.bitangent.x, &v.bitangent.y, &v.bitangent.z );
                    ++numBitangents;
                }

                std::getline( in, line );
            }

#if USING( DEBUG_BUILD )
            if ( numTangents > 1 )
                LOG_WARN( "Mesh %s vertex %u has more than 1 tangent specified", mesh.name.c_str(), (uint32_t)mesh.vertices.size() - 1 );
            if ( numBitangents > 1 )
                LOG_WARN( "Mesh %s vertex %u has more than 1 bittangent specified", mesh.name.c_str(), (uint32_t)mesh.vertices.size() - 1 );
            if ( numTangents && !numBitangents )
                LOG_WARN(
                    "Mesh %s vertex %u has a tangent specified, but no bitangent", mesh.name.c_str(), (uint32_t)mesh.vertices.size() - 1 );
            if ( !numTangents && numBitangents )
                LOG_WARN(
                    "Mesh %s vertex %u has a bitangent specified, but no tangent", mesh.name.c_str(), (uint32_t)mesh.vertices.size() - 1 );
#endif // #if USING( DEBUG_BUILD )

            meshNumUVs += numUVs;
            meshNumColors += numColors;
            meshNumTangents += numTangents;
            meshNumBitangents += numBitangents;
            mesh.hasBoneWeights = mesh.hasBoneWeights || ( v.numBones > 0 );

            std::getline( in, line );
        }

        if ( meshNumUVs != mesh.vertices.size() * mesh.numUVChannels )
            LOG_WARN( "Not every vertex in mesh %s specified the expected uvs!", mesh.name.c_str() );
        if ( meshNumColors != mesh.vertices.size() * mesh.numColorChannels )
            LOG_WARN( "Not every vertex in mesh %s specified the expected vertex colors!", mesh.name.c_str() );
        if ( meshNumTangents && meshNumTangents != mesh.vertices.size() )
            LOG_WARN( "Only some, but not all of the vertices in mesh %s specified tangents!", mesh.name.c_str() );
        if ( meshNumBitangents != mesh.vertices.size() )
            LOG_WARN( "Only some, but not all of the vertices in mesh %s specified bitangents!", mesh.name.c_str() );

        mesh.hasTangents = meshNumTangents > 0 && meshNumBitangents > 0;

        PG_ASSERT( line == "Tris:" );
        std::getline( in, line );
        mesh.indices.reserve( mesh.vertices.size() * 2 );
        while ( !line.empty() )
        {
            uint32_t i0, i1, i2;
            sscanf( line.c_str(), "%u %u %u", &i0, &i1, &i2 );
            mesh.indices.push_back( i0 );
            mesh.indices.push_back( i1 );
            mesh.indices.push_back( i2 );
            std::getline( in, line );
        }
    }
    PG_ASSERT( meshes.size() );

    return true;
}

static constexpr float PZ( float x ) { return x == 0.0f ? +0.0f : x; }

bool PModel::Save( std::ofstream& outFile, uint32_t floatPrecision, bool logProgress ) const
{
    auto Start = Clock::now();

    enum
    {
        POS_AND_NORMAL,
        TANGENTS,
        UV,
        COLOR,
        BONE
    };
    std::string fmtStrings[] =
    {
        "V %u\np %.6g %.6g %.6g\nn %.6g %.6g %.6g\n",
        "t %.6g %.6g %.6g\nb %.6g %.6g %.6g\n",
        "uv %.6g %.6g\n",
        "c %.6g %.6g %.6g %.6g\n",
        "bw %u %.6g\n",
    };

    floatPrecision = std::max( 1u, std::min( 9u, floatPrecision ) );
    for ( int strIdx = 0; strIdx < ARRAY_COUNT( fmtStrings ); ++strIdx )
        SingleCharReplacement( &fmtStrings[strIdx][0], '6', '0' + floatPrecision );

    size_t totalVerts = 0;
    size_t totalTris  = 0;
    std::set<std::string> materialSet;
    for ( const PModel::Mesh& mesh : meshes )
    {
        totalVerts += mesh.vertices.size();
        totalTris += mesh.indices.size() / 3;
        materialSet.insert( mesh.materialName );
    }

    outFile << "pmodelFormat: " << (uint32_t)PModelVersionNum::CURRENT_VERSION << "\n\n";

    for ( const auto& matName : materialSet )
    {
        outFile << matName << "\n";
    }
    outFile << "\n";

    size_t vertsWritten          = 0;
    const size_t tenPercentVerts = totalVerts / 10;
    int lastPercent              = 0;
    char buffer[1024];
    for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
    {
        const PModel::Mesh& m = meshes[meshIdx];
        sprintf( buffer, "Mesh %u: %s\nMat: %s\nNumUVs %u\nNumColors: %u\n\n", (uint32_t)meshIdx, m.name.c_str(), m.materialName.c_str(),
            m.numUVChannels, m.numColorChannels );
        outFile << buffer;

        for ( uint32_t vIdx = 0; vIdx < (uint32_t)m.vertices.size(); ++vIdx )
        {
            const PModel::Vertex& v = m.vertices[vIdx];
            int pos = sprintf( buffer, fmtStrings[POS_AND_NORMAL].c_str(), vIdx, PZ( v.pos.x ), PZ( v.pos.y ), PZ( v.pos.z ),
                PZ( v.normal.x ), PZ( v.normal.y ), PZ( v.normal.z ) );
            if ( m.hasTangents )
            {
                pos += sprintf( buffer + pos, fmtStrings[TANGENTS].c_str(), PZ( v.tangent.x ), PZ( v.tangent.y ), PZ( v.tangent.z ),
                    PZ( v.bitangent.x ), PZ( v.bitangent.y ), PZ( v.bitangent.z ) );
            }
            for ( uint32_t i = 0; i < m.numUVChannels; ++i )
                pos += sprintf( buffer + pos, fmtStrings[UV].c_str(), PZ( v.uvs[i].x ), PZ( v.uvs[i].y ) );

            for ( uint32_t i = 0; i < m.numColorChannels; ++i )
                pos += sprintf( buffer + pos, fmtStrings[COLOR].c_str(), PZ( v.colors[i].x ), PZ( v.colors[i].y ), PZ( v.colors[i].z ),
                    PZ( v.colors[i].w ) );

            if ( m.hasBoneWeights )
            {
                for ( uint8_t i = 0; i < v.numBones; ++i )
                    pos += sprintf( buffer + pos, fmtStrings[BONE].c_str(), v.boneIndices[i], PZ( v.boneWeights[i] ) );
            }

            pos += sprintf( buffer + pos, "\n" );
            outFile.write( buffer, pos );
        }

        outFile << "Tris:\n";
        for ( size_t i = 0; i < m.indices.size(); i += 3 )
        {
            sprintf( buffer, "%u %u %u\n", m.indices[i + 0], m.indices[i + 1], m.indices[i + 2] );
            outFile << buffer;
        }

        outFile << '\n';

        vertsWritten += m.vertices.size();
        int currentPercent = static_cast<int>( vertsWritten / (double)totalVerts * 100 );
        if ( logProgress && currentPercent - lastPercent >= 10 )
        {
            LOG( "%d%%...", currentPercent );
            lastPercent = currentPercent;
        }
    }

    auto now    = Clock::now();
    double time = duration_cast<microseconds>( now - Start ).count() / static_cast<float>( 1000 );
    LOG( "Exported mesh in %f sec", time / 1000.0 );

    return outFile.good();
}

bool PModel::Save( const std::string& filename, uint32_t floatPrecision, bool logProgress ) const
{
    std::ofstream out( filename );
    if ( !out )
    {
        LOG_ERR( "Failed to open file '%s' to save pmodel", filename.c_str() );
        return false;
    }

    return Save( out, floatPrecision, logProgress );
}

std::vector<std::string> GetUsedMaterialsPModel( const std::string& filename )
{
    std::ifstream in( filename );
    if ( !in )
    {
        LOG_ERR( "Failed to open pmodel file '%s'", filename.c_str() );
        return {};
    }

    std::string tmp;
    in >> tmp; // pmodelFormat
    uint32_t version;
    in >> version;
    if ( version < (uint32_t)PModelVersionNum::LAST_SUPPORTED_VERSION )
    {
        LOG_ERR( "PModel file %s contains a version (%u) that is no longer supported. Please re-export the source file", filename.c_str(),
            version );
        return {};
    }

    //  skip to material list
    std::string line;
    do
    {
        std::getline( in, line );
    } while ( line.empty() );

    std::vector<std::string> materials;
    do
    {
        materials.push_back( line );
        std::getline( in, line );
    } while ( !line.empty() );

    return materials;
}

} // namespace PG
