#pragma once

#include "utils/json_parsing.hpp"

class Serializer;


void GfxImage_Parse( const rapidjson::Value& value );

// returns how many assets are out of date
int GfxImage_CheckDependencies();

// returns how many assets were unable to be converted
int GfxImage_Convert();

// returns true if no errors occurred, false otherwise
bool GfxImage_BuildFastFile( Serializer* serializer );