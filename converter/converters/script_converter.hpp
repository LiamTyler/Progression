#pragma once

#include "utils/json_parsing.hpp"

class Serializer;


void Script_Parse( const rapidjson::Value& value );

// returns how many assets are out of date
int Script_CheckDependencies();

// returns how many assets were unable to be converted
int Script_Convert();

// returns true if no errors occurred, false otherwise
bool Script_BuildFastFile( Serializer* serializer );