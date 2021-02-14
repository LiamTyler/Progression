#include "asset/types/environment_map.hpp"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include <fstream>

namespace PG
{


bool EnvironmentMap_Load( EnvironmentMap* map, const EnvironmentMapCreateInfo& createInfo )
{
    return false;
}


bool Fastfile_EnvironmentMap_Load( EnvironmentMap* map, Serializer* serializer )
{
    return false;
}


bool Fastfile_EnvironmentMap_Save( const EnvironmentMap * const map, Serializer* serializer )
{
    return false;
}


} // namespace PG
