#pragma once

#include "asset/types/model.hpp"
#include "asset/types/material.hpp"
#include <vector>

namespace PG
{

// In the converter, we need to know what all the model renderers are to build the necessary shaders,
// but the assets haven't been converted and loaded to get pointers to them by the time the components are created during scene parsing.
// The assets will however, be converted by the time we actually access the model renderer in the converter (generate_missing_pipelines)
struct ModelRenderer
{
#if USING( COMPILING_CONVERTER )
    std::string modelName;
    std::string materialOverride; // currently only actually can specify 1 material override for all meshes. Can be empty
#else // #if USING( COMPILING_CONVERTER )
    Model* model;
    std::vector< Material* > materials;
#endif // #else // #if USING( COMPILING_CONVERTER )

};

} // namespace PG
