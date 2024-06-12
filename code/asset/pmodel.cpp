#include "pmodel.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "shared/string.hpp"
#include <chrono>
#include <set>

using namespace std::chrono;
using Clock = high_resolution_clock;

namespace PG
{

bool PModel::Vertex::AddBone( u32 boneIdx, f32 weight )
{
    if ( numBones < PMODEL_MAX_BONE_WEIGHTS_PER_VERT )
    {
        boneWeights[numBones] = weight;
        boneIndices[numBones] = boneIdx;
        ++numBones;
        return true;
    }

    f32 minWeight = boneWeights[0];
    u32 minIdx    = 0;
    for ( u32 slot = 1; slot < numBones; ++slot )
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

bool PModel::LoadBinary( std::string_view filename )
{
    Serializer serializer;
    if ( !serializer.OpenForRead( filename.data() ) )
    {
        LOG_ERR( "Failed to open pmodelb file '%s'", filename.data() );
        return false;
    }

    PModelVersionNum version;
    serializer.Read( version );
    if ( version < PModelVersionNum::LAST_SUPPORTED_VERSION )
    {
        LOG_ERR( "PModelb file %s contains a version (%u) that is no longer supported. Please re-export the source file", filename.data(),
            version );
        return false;
    }

    u16 numUniqueMaterials;
    serializer.Read( numUniqueMaterials );
    for ( u16 matIdx = 0; matIdx < numUniqueMaterials; ++matIdx )
    {
        u16 strLen;
        serializer.Read( strLen );
        serializer.Skip( strLen );
    }

    meshes.resize( serializer.Read<u32>() );
    for ( Mesh& mesh : meshes )
    {
        serializer.Read<u16>( mesh.name );
        serializer.Read<u16>( mesh.materialName );
        serializer.Read( mesh.numUVChannels );
        serializer.Read( mesh.numColorChannels );
        serializer.Read( mesh.hasTangents );
        serializer.Read( mesh.hasBoneWeights );

        u64 numIndices, num16BitIndices;
        serializer.Read( numIndices );
        serializer.Read( num16BitIndices );
        mesh.indices.resize( numIndices );
        for ( size_t i = 0; i < num16BitIndices; ++i )
            mesh.indices[i] = serializer.Read<u16>();
        for ( size_t i = num16BitIndices; i < numIndices; ++i )
            serializer.Read( mesh.indices[i] );

        mesh.vertices.resize( serializer.Read<u32>() );
        for ( size_t vIdx = 0; vIdx < mesh.vertices.size(); ++vIdx )
        {
            Vertex& v = mesh.vertices[vIdx];
            serializer.Read( v.pos );
            serializer.Read( v.normal );
            if ( mesh.hasTangents )
            {
                serializer.Read( v.tangent );
                serializer.Read( v.bitangent );
            }
            for ( u32 uvIdx = 0; uvIdx < mesh.numUVChannels; ++uvIdx )
                serializer.Read( v.uvs[uvIdx] );
            for ( u32 cIdx = 0; cIdx < mesh.numColorChannels; ++cIdx )
                serializer.Read( v.colors[cIdx] );
            if ( mesh.hasBoneWeights )
            {
                serializer.Read( v.numBones );
                for ( u8 i = 0; i < v.numBones; ++i )
                {
                    serializer.Read( v.boneIndices[i] );
                    serializer.Read( v.boneWeights[i] );
                }
            }
        }
    }

    return true;
}

bool PModel::LoadText( std::string_view filename )
{
    std::ifstream in( filename.data() );
    if ( !in )
    {
        LOG_ERR( "Failed to open pmodelt file '%s'", filename.data() );
        return false;
    }

    char tmpBuffer[128];
    std::string tmp;
    in >> tmp; // pmodelFormat
    u16 version;
    in >> version;
    if ( version < Underlying( PModelVersionNum::LAST_SUPPORTED_VERSION ) )
    {
        LOG_ERR( "PModelt file %s contains a version (%u) that is no longer supported. Please re-export the source file", filename.data(),
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

        u32 meshNumUVs        = 0;
        u32 meshNumColors     = 0;
        u32 meshNumTangents   = 0;
        u32 meshNumBitangents = 0;

        mesh.vertices.reserve( 65536 );
        while ( !line.empty() && line[0] == 'V' )
        {
            Vertex& v         = mesh.vertices.emplace_back();
            u32 numUVs        = 0;
            u32 numColors     = 0;
            u32 numTangents   = 0;
            u32 numBitangents = 0;

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
                LOG_WARN( "Mesh %s vertex %u has more than 1 tangent specified", mesh.name.c_str(), (u32)mesh.vertices.size() - 1 );
            if ( numBitangents > 1 )
                LOG_WARN( "Mesh %s vertex %u has more than 1 bittangent specified", mesh.name.c_str(), (u32)mesh.vertices.size() - 1 );
            if ( numTangents && !numBitangents )
                LOG_WARN( "Mesh %s vertex %u has a tangent specified, but no bitangent", mesh.name.c_str(), (u32)mesh.vertices.size() - 1 );
            if ( !numTangents && numBitangents )
                LOG_WARN( "Mesh %s vertex %u has a bitangent specified, but no tangent", mesh.name.c_str(), (u32)mesh.vertices.size() - 1 );
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
        if ( meshNumBitangents && meshNumBitangents != mesh.vertices.size() )
            LOG_WARN( "Only some, but not all of the vertices in mesh %s specified bitangents!", mesh.name.c_str() );

        mesh.hasTangents = meshNumTangents > 0 && meshNumBitangents > 0;

        PG_ASSERT( line == "Tris:" );
        std::getline( in, line );
        mesh.indices.reserve( mesh.vertices.size() * 2 );
        while ( !line.empty() )
        {
            u32 i0, i1, i2;
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

bool PModel::Load( std::string_view filename )
{
    std::string ext = GetFileExtension( filename.data() );
    if ( ext == ".pmodelt" )
        return LoadText( filename );
    else if ( ext == ".pmodelb" )
        return LoadBinary( filename );

    LOG_ERR( "PModel::Load: Invalid file extension '%s'", ext.c_str() );

    return false;
}

static constexpr f32 PZ( f32 x ) { return x == 0.0f ? +0.0f : x; }

bool PModel::SaveText( std::string_view filename, u32 f32Precision, bool logProgress ) const
{
    std::ofstream outFile( filename.data() );
    if ( !outFile )
    {
        LOG_ERR( "Failed to open file '%s' to save pmodel", filename.data() );
        return false;
    }

    auto Start = Clock::now();

    enum
    {
        POS_AND_NORMAL,
        TANGENTS,
        UV,
        COLOR,
        BONE
    };
    std::string fmtStrings[] = {
        "V %u\np %.6g %.6g %.6g\nn %.6g %.6g %.6g\n",
        "t %.6g %.6g %.6g\nb %.6g %.6g %.6g\n",
        "uv %.6g %.6g\n",
        "c %.6g %.6g %.6g %.6g\n",
        "bw %u %.6g\n",
    };

    f32Precision = std::max( 1u, std::min( 9u, f32Precision ) );
    for ( i32 strIdx = 0; strIdx < ARRAY_COUNT( fmtStrings ); ++strIdx )
        SingleCharReplacement( &fmtStrings[strIdx][0], '6', '0' + f32Precision );

    size_t totalVerts = 0;
    size_t totalTris  = 0;
    std::set<std::string_view> materialSet;
    for ( const PModel::Mesh& mesh : meshes )
    {
        totalVerts += mesh.vertices.size();
        totalTris += mesh.indices.size() / 3;
        materialSet.insert( mesh.materialName );
    }

    outFile << "pmodelFormat: " << Underlying( PModelVersionNum::CURRENT_VERSION ) << "\n\n";

    for ( std::string_view matName : materialSet )
    {
        outFile << matName << "\n";
    }
    outFile << "\n";

    size_t vertsWritten          = 0;
    const size_t tenPercentVerts = totalVerts / 10;
    i32 lastPercent              = 0;
    char buffer[1024];
    for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
    {
        const PModel::Mesh& m = meshes[meshIdx];
        sprintf( buffer, "Mesh %u: %s\nMat: %s\nNumUVs %u\nNumColors: %u\n\n", (u32)meshIdx, m.name.c_str(), m.materialName.c_str(),
            m.numUVChannels, m.numColorChannels );
        outFile << buffer;

        PG_ASSERT( m.vertices.size() <= UINT32_MAX );
        for ( u32 vIdx = 0; vIdx < (u32)m.vertices.size(); ++vIdx )
        {
            const PModel::Vertex& v = m.vertices[vIdx];
            i32 pos = sprintf( buffer, fmtStrings[POS_AND_NORMAL].c_str(), vIdx, PZ( v.pos.x ), PZ( v.pos.y ), PZ( v.pos.z ),
                PZ( v.normal.x ), PZ( v.normal.y ), PZ( v.normal.z ) );
            if ( m.hasTangents )
            {
                pos += sprintf( buffer + pos, fmtStrings[TANGENTS].c_str(), PZ( v.tangent.x ), PZ( v.tangent.y ), PZ( v.tangent.z ),
                    PZ( v.bitangent.x ), PZ( v.bitangent.y ), PZ( v.bitangent.z ) );
            }
            for ( u32 i = 0; i < m.numUVChannels; ++i )
                pos += sprintf( buffer + pos, fmtStrings[UV].c_str(), PZ( v.uvs[i].x ), PZ( v.uvs[i].y ) );

            for ( u32 i = 0; i < m.numColorChannels; ++i )
                pos += sprintf( buffer + pos, fmtStrings[COLOR].c_str(), PZ( v.colors[i].x ), PZ( v.colors[i].y ), PZ( v.colors[i].z ),
                    PZ( v.colors[i].w ) );

            if ( m.hasBoneWeights )
            {
                for ( u8 i = 0; i < v.numBones; ++i )
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
        i32 currentPercent = static_cast<i32>( vertsWritten / (f64)totalVerts * 100 );
        if ( logProgress && currentPercent - lastPercent >= 10 )
        {
            LOG( "%d%%...", currentPercent );
            lastPercent = currentPercent;
        }
    }

    auto now = Clock::now();
    f64 time = duration_cast<microseconds>( now - Start ).count() / 1000.0;
    LOG( "Exported mesh in %f sec", time / 1000.0 );

    return outFile.good();
}

bool PModel::SaveBinary( std::string_view filename ) const
{
    Serializer serializer;
    if ( !serializer.OpenForWrite( filename.data() ) )
    {
        LOG_ERR( "Failed to open file '%s' to save pmodel", filename.data() );
        return false;
    }

    serializer.Write( PModelVersionNum::CURRENT_VERSION );

    std::set<std::string_view> materialSet;
    for ( const PModel::Mesh& mesh : meshes )
    {
        materialSet.insert( mesh.materialName );
    }

    PG_ASSERT( materialSet.size() <= UINT16_MAX );
    serializer.Write( (u16)materialSet.size() );
    for ( std::string_view matName : materialSet )
    {
        serializer.Write( (u16)matName.length() );
        serializer.Write( matName.data(), matName.length() );
    }

    serializer.Write( (u32)meshes.size() );
    for ( const Mesh& mesh : meshes )
    {
        serializer.Write<u16>( mesh.name );
        serializer.Write<u16>( mesh.materialName );
        serializer.Write( mesh.numUVChannels );
        serializer.Write( mesh.numColorChannels );
        serializer.Write( mesh.hasTangents );
        serializer.Write( mesh.hasBoneWeights );

        serializer.Write( mesh.indices.size() );
        // to save some space, try to save as many 16 bit indices as possible initially
        size_t first32Index = 0;
        while ( first32Index < mesh.indices.size() && mesh.indices[first32Index] <= UINT16_MAX )
            ++first32Index;

        serializer.Write( first32Index );
        for ( size_t i = 0; i < first32Index; ++i )
            serializer.Write( (u16)mesh.indices[i] );
        for ( size_t i = first32Index; i < mesh.indices.size(); ++i )
            serializer.Write( mesh.indices[i] );

        PG_ASSERT( mesh.vertices.size() <= UINT32_MAX );
        u32 numVerts = (u32)mesh.vertices.size();
        serializer.Write( numVerts );
        for ( u32 vIdx = 0; vIdx < numVerts; ++vIdx )
        {
            const Vertex& v = mesh.vertices[vIdx];
            serializer.Write( v.pos );
            serializer.Write( v.normal );
            if ( mesh.hasTangents )
            {
                serializer.Write( v.tangent );
                serializer.Write( v.bitangent );
            }
            for ( u32 uvIdx = 0; uvIdx < mesh.numUVChannels; ++uvIdx )
                serializer.Write( v.uvs[uvIdx] );
            for ( u32 cIdx = 0; cIdx < mesh.numColorChannels; ++cIdx )
                serializer.Write( v.colors[cIdx] );
            if ( mesh.hasBoneWeights )
            {
                serializer.Write( v.numBones );
                for ( u8 i = 0; i < v.numBones; ++i )
                {
                    serializer.Write( v.boneIndices[i] );
                    serializer.Write( v.boneWeights[i] );
                }
            }
        }
    }
    serializer.Close();

    return true;
}

bool PModel::Save( std::string_view filename, u32 f32Precision, bool logProgress ) const
{
    std::string ext = GetFileExtension( filename.data() );
    if ( ext == ".pmodelt" )
        return SaveText( filename, f32Precision, logProgress );
    else if ( ext == ".pmodelb" )
        return SaveBinary( filename );

    LOG_ERR( "PModel::Save: Invalid file extension '%s'", ext.c_str() );

    return false;
}

std::vector<std::string> GetUsedMaterialsPModel( const std::string& filename )
{
    std::vector<std::string> materials;
    std::string ext = GetFileExtension( filename );
    if ( ext == ".pmodelt" )
    {
        std::ifstream in( filename );
        if ( !in )
        {
            LOG_ERR( "Failed to open pmodelt file '%s'", filename.c_str() );
            return {};
        }

        std::string tmp;
        in >> tmp; // pmodelFormat
        u16 version;
        in >> version;
        if ( version < Underlying( PModelVersionNum::LAST_SUPPORTED_VERSION ) )
        {
            LOG_ERR( "PModelt file %s contains a version (%u) that is no longer supported. Please re-export the source file",
                filename.c_str(), version );
            return {};
        }

        //  skip to material list
        std::string line;
        do
        {
            std::getline( in, line );
        } while ( line.empty() );

        do
        {
            materials.push_back( line );
            std::getline( in, line );
        } while ( !line.empty() );

        return materials;
    }
    else if ( ext == ".pmodelb" )
    {
        Serializer serializer;
        if ( !serializer.OpenForRead( filename.c_str() ) )
        {
            LOG_ERR( "Failed to open pmodelb file '%s'", filename.c_str() );
            return {};
        }

        PModelVersionNum version;
        serializer.Read( version );
        if ( version < PModelVersionNum::LAST_SUPPORTED_VERSION )
        {
            LOG_ERR( "PModelb file %s contains a version (%u) that is no longer supported. Please re-export the source file",
                filename.data(), version );
            return {};
        }

        u16 numUniqueMaterials;
        serializer.Read( numUniqueMaterials );
        materials.resize( numUniqueMaterials );
        for ( u16 matIdx = 0; matIdx < numUniqueMaterials; ++matIdx )
        {
            serializer.Read<u16>( materials[matIdx] );
        }

        return materials;
    }

    LOG_ERR( "PModel::Load: Invalid file extension '%s'", ext.c_str() );
    return {};
}

} // namespace PG
