#pragma once

#include "utils/json_parsing.hpp"

class Serializer;


void Material_Parse( const rapidjson::Value& value );

// returns how many assets are out of date
int Material_CheckDependencies();

// returns how many assets were unable to be converted
int Material_Convert();

// returns true if no errors occurred, false otherwise
bool Material_BuildFastFile( Serializer* serializer );