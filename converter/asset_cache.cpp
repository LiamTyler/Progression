#include "asset_cache.hpp"
#include "utils/filesystem.hpp"
#include "core/platform_defines.hpp"

#define ROOT_DIR PG_ASSET_DIR "cache/"

namespace PG
{

AssetCache::AssetCache()
{
    CreateDirectory( ROOT_DIR );
    CreateDirectory( ROOT_DIR "fastfiles/" );
    CreateDirectory( ROOT_DIR "images/" );
    CreateDirectory( ROOT_DIR "materials/" );
    CreateDirectory( ROOT_DIR "models/" );
    CreateDirectory( ROOT_DIR "scripts/" );
    CreateDirectory( ROOT_DIR "shaders/" );
    CreateDirectory( ROOT_DIR "shader_preproc/" );
}

} // namespace PG