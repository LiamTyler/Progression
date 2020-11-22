#pragma once

#include "ecs/ecs.hpp"

namespace PG
{

struct NameComponent
{
    std::string name;
};

struct EntityMetaData
{
    entt::entity parent = entt::null;
    bool isStatic       = false;
};

} // namespace PG
