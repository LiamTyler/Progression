#pragma once

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

    // If the asset supports runtime loading, then it can override this function and implement it.
    //
    virtual bool Load( const BaseAssetCreateInfo* baseInfo ) { return false; }
    
    virtual bool FastfileLoad( Serializer* serializer ) = 0;
    virtual bool FastfileSave( Serializer* serializer ) const = 0;
    virtual void Free() {}
    
    std::string name;
};

} // namespace PG
