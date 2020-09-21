#pragma once

#include <string>

namespace PG
{

struct Asset
{
    virtual ~Asset() = default;

    virtual void Free() {}
    
    std::string name;
};

} // namespace PG
