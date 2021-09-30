#pragma once

#include "ecs/ecs.hpp"

namespace PG
{

struct EntityMetaData
{
    std::string name;
    entt::entity parent = entt::null;
    bool isStatic       = false;
};

} // namespace PG
