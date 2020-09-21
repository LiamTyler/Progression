#pragma once

#include "utils/json_parsing.hpp"

class Serializer;


void Model_Parse( const rapidjson::Value& value );

// returns how many assets are out of date
int Model_CheckDependencies();

// returns how many assets were unable to be converted
int Model_Convert();

// returns true if no errors occurred, false otherwise
bool Model_BuildFastFile( Serializer* serializer );