#pragma once

#include "asset/types/base_asset.hpp"

class Serializer;

namespace PG
{

struct ScriptCreateInfo
{
    std::string name;
    std::string filename;
};

struct Script : public Asset
{
public:
    Script() = default;

    std::string scriptText;
};

bool Script_Load( Script* script, const ScriptCreateInfo& createInfo );

bool Fastfile_Script_Load( Script* script, Serializer* serializer );

bool Fastfile_Script_Save( const Script * const script, Serializer* serializer );


} // namespace PG