#pragma once

#include "model_exporter_common.hpp"

extern std::string g_textureSearchDir;

struct MaterialContext
{
    const aiScene* scene;
    const aiMaterial* assimpMat;
    std::string localMatName;
    std::string modelName;
    std::string file;
};

bool OutputMaterial( const MaterialContext& context, std::string& outputJSON, std::string& outMaterialName );
