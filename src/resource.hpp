#pragma once

#include <memory>
#include <string>

namespace Progression
{

struct ResourceCreateInfo
{
    std::string name;
};

class Resource
{
public:
    Resource() : name( "" )
    {
    }
    Resource( const std::string& n ) : name( n )
    {
    }
    virtual ~Resource() = default;

    virtual bool Load( ResourceCreateInfo* createInfo = nullptr ) = 0;
    virtual void Move( std::shared_ptr< Resource > dst )          = 0;
    virtual bool Serialize( std::ofstream& outFile ) const        = 0;
    virtual bool Deserialize( char*& buffer )                     = 0;

    std::string name;
};

} // namespace Progression
