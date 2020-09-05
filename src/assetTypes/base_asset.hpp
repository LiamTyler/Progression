#pragma once

#include <memory>
#include <string>

namespace Progression
{


struct Asset
{
    virtual ~Asset() = default;

    virtual void Free() {}
    
    std::string name;
};


} // namespace Progression
