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

    virtual bool Load( const BaseAssetCreateInfo* baseInfo )  { return false; }
    virtual bool FastfileLoad( Serializer* serializer ) { return false; }
    virtual bool FastfileSave( Serializer* serializer ) const { return false; }
    virtual void Free() {}
    
    std::string name;
};

} // namespace PG
