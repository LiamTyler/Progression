#pragma once

#include "ecs/ecs.hpp"
#include "shared/json_parsing.hpp"
#include <string>

namespace PG
{

void ParseComponent( const rapidjson::Value& value, entt::entity e, entt::registry& registry, const std::string& typeName );

} // namespace PG
