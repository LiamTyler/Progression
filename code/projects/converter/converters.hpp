#include "converters/base_asset_converter.hpp"

namespace PG
{

extern std::shared_ptr<BaseAssetConverter> g_converters[NUM_ASSET_TYPES];

void InitConverters();
void ShutdownConverters();

} // namespace PG