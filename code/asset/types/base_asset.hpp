#pragma once

#include "shared/platform_defines.hpp"
#include <string>

class Serializer;

namespace PG
{

struct BaseAssetCreateInfo
{
    std::string name;
};

class BaseAsset
{
public:
    BaseAsset() = default;
    BaseAsset( const std::string& inName ) : name( inName ) {}
    virtual ~BaseAsset() = default;

    // Not pure virtual because assets are not required to support runtime loading (this function) though most do
    // They can choose to have it handled through the corresponding asset converter class if need be (Material, for example)
    virtual bool Load( const BaseAssetCreateInfo* baseInfo ) { return false; }
    
    virtual bool FastfileLoad( Serializer* serializer ) = 0;
    virtual bool FastfileSave( Serializer* serializer ) const = 0;
    virtual void Free() {}
    
    std::string name;

#if USING( CONVERTER )
    std::string cacheName; // occassionally handy to have access to this during the Load() function, mostly uneeded though
#endif // #if USING( CONVERTER )
};

} // namespace PG
