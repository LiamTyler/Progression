#pragma once

#include "core/cpu_profiling.hpp"
#include "shared/platform_defines.hpp"
#include <string>

#define ASSET_NAMES USE_IF( !USING( GAME ) || !USING( SHIP_BUILD ) )

#if USING( ASSET_NAMES )
#define PGP_ZONE_SCOPED_ASSET( name ) PGP_ZONE_SCOPED_FMT( name " %s", m_name )
#else // #if USING( ASSET_NAMES )
#define PGP_ZONE_SCOPED_ASSET( name ) PGP_ZONE_SCOPEDN( name )
#endif // #else // #if USING( ASSET_NAMES )

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
    BaseAsset( std::string_view inName );
    virtual ~BaseAsset();

    BaseAsset( const BaseAsset& )            = delete;
    BaseAsset& operator=( const BaseAsset& ) = delete;

    // Not pure virtual because assets are not required to support runtime loading (this function) though most do
    // They can choose to have it handled through the corresponding asset converter class if need be (Material, for example)
    virtual bool Load( const BaseAssetCreateInfo* baseInfo ) { return false; }

    virtual bool FastfileLoad( Serializer* serializer )       = 0;
    virtual bool FastfileSave( Serializer* serializer ) const = 0;
    virtual void Free() {}

    void SetName( std::string_view inName );
    const char* GetName() const;

#if USING( CONVERTER )
    // occassionally handy to have access to this during the Load() function, mostly uneeded though
    std::string cacheName;
#endif // #if USING( CONVERTER )

protected:
#if USING( ASSET_NAMES )
    char* m_name = nullptr; // the AssetManager always takes care of deserializing this
#endif                      // #if USING( ASSET_NAMES )

    void SerializeName( Serializer* serializer ) const;
};

} // namespace PG
