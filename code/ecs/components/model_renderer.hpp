#pragma once

#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
#include <vector>

namespace PG
{

struct ModelRenderer
{
    Model* model;
    std::vector<Material*> materials;
};

} // namespace PG
