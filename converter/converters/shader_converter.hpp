#pragma once

#include "utils/json_parsing.hpp"

class Serializer;


void Shader_Parse( const rapidjson::Value& value );

// returns how many assets are out of date
int Shader_CheckDependencies();

// returns how many assets were unable to be converted
int Shader_Convert();

// returns true if no errors occurred, false otherwise
bool Shader_BuildFastFile( Serializer* serializer );