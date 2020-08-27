#pragma once

#include <memory>
#include <string>

namespace Progression
{


struct Resource
{
    virtual ~Resource() = default;
    
    // For reloading resources at runtime at already existed. Move the data of the
    // new resource into the old address, keeping pointers to the old one valid
    virtual void Move( Resource* dst ) = 0;

    std::string name;
};


} // namespace Progression
