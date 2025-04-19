#include "asset/types/ui_layout.hpp"
#include "asset/asset_manager.hpp"
#include "pugixml-1.13/src/pugixml.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <fstream>

namespace PG
{

bool UILayout::Load( const BaseAssetCreateInfo* baseInfo ) { return false; }

bool UILayout::FastfileLoad( Serializer* serializer ) { return false; }

bool UILayout::FastfileSave( Serializer* serializer ) const { return false; }

} // namespace PG
