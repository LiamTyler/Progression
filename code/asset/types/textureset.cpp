#include "asset/types/textureset.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <fstream>

namespace PG
{

bool Textureset::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const TexturesetCreateInfo* createInfo = (const TexturesetCreateInfo*)baseInfo;
    name = createInfo->name;

    return true;
}


bool Textureset::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    serializer->Read( name );

    return true;
}


bool Textureset::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
    serializer->Write( name );

    return true;
}

} // namespace PG
