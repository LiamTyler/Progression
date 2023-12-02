#include "converters/base_asset_converter.hpp"

namespace PG
{

extern std::shared_ptr<BaseAssetConverter> g_converters[ASSET_TYPE_COUNT];

void InitConverters();
void ShutdownConverters();

} // namespace PG
