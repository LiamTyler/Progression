#pragma once

#include <string>

namespace PG
{

struct BaseAssetCreateInfo
{
    std::string name;
};

struct BaseAsset
{
    virtual ~BaseAsset() = default;

    virtual void Free() {}
    
    std::string name;
};

} // namespace PG
