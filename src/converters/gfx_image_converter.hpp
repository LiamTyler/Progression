#pragma once

#include "utils/json_parsing.hpp"

void GfxImage_Parse( rapidjson::Value& value );

// returns how many assets are out of date
int GfxImage_CheckDependencies();

// returns how many assets were unable to be converted
int GfxImage_Convert();