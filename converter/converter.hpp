#pragma once

#include "asset/asset_versions.hpp"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"

struct ConverterConfigOptions
{
    std::string assetListFile;
    bool force                  = false;
    bool generateShaderDebugInfo   = false;
    bool saveShaderPreproc      = false;
};

struct ConverterStatus
{
    bool parsingError           = false;
    int checkDependencyErrors   = 0;
    bool convertAssetErrors     = false;
    bool buildFastfileErrors    = false;
};

extern ConverterConfigOptions g_converterConfigOptions;
extern ConverterStatus g_converterStatus;