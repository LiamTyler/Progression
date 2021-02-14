#pragma once

#include "utils/json_parsing.hpp"

class Serializer;


void EnvironmentMap_Parse( const rapidjson::Value& value );

// returns how many assets are out of date
int EnvironmentMap_CheckDependencies();

// returns how many assets were unable to be converted
int EnvironmentMap_Convert();

// returns true if no errors occurred, false otherwise
bool EnvironmentMap_BuildFastFile( Serializer* serializer );