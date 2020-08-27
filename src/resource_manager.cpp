#include "resource_manager.hpp"
#include "assert.hpp"
#include "utils/logger.hpp"
#include <unordered_map>


namespace Progression
{
namespace ResourceManager
{

uint32_t GetResourceTypeIDHelper::IDCounter = 0;

enum ResourceTypes
{
    GFX_IMAGE = 0,
    TOTAL_RESOURCE_TYPES
};

static std::unordered_map< std::string, Resource* > s_resourceMaps[TOTAL_RESOURCE_TYPES];


void Init()
{
}


bool LoadFastFile( const std::string& fname )
{
    return false;
}


void Shutdown()
{
    for ( int i = 0; i < TOTAL_RESOURCE_TYPES; ++i )
    {
        for ( const auto& it : s_resourceMaps[i] )
        {
            delete it.second;
        }
        s_resourceMaps[i].clear();
    }
}


Resource* Get( uint32_t resourceTypeID, const std::string& name )
{
    PG_ASSERT( resourceTypeID < TOTAL_RESOURCE_TYPES, "Did you forget to update TOTAL_NUM_RESOURCES?" );
    auto it = s_resourceMaps[resourceTypeID].find( name );
    if ( it != s_resourceMaps[resourceTypeID].end() )
    {
        return it->second;
    }

    return nullptr;
}


} // namespace ResourceManager
} // namesapce Progression