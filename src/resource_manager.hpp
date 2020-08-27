#pragma once

#include "resource.hpp"

namespace Progression
{
namespace ResourceManager
{

struct GetResourceTypeIDHelper
{
    static uint32_t IDCounter;
};

template <typename Derived>
struct GetResourceTypeID : public GetResourceTypeIDHelper
{
    static uint32_t ID()
    {
        static uint32_t id = IDCounter++;
        return id;
    }
};


void Init();

bool LoadFastFile( const std::string& fname );

void Shutdown();

Resource* Get( uint32_t resourceTypeID, const std::string& name );

template < typename T >
T* Get( const std::string& name )
{
    static_assert( std::is_base_of< Resource, T >::value && !std::is_same< Resource, T >::value, "Resource manager only manages classes derived from 'Resource'" );
    Resource* res = Get( GetResourceTypeID< T >::ID(), name );
    return static_cast< T* >( res );
}


} // namespace ResourceManager
} // namesapce ResourceManager