#include "converters/base_asset_converter.hpp"

namespace PG
{

extern BaseAssetConverter* g_converters[ASSET_TYPE_COUNT];

void InitConverters();
void ShutdownConverters();

} // namespace PG
