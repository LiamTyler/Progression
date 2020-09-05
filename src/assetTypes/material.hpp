#pragma once

#include "assetTypes/base_asset.hpp"
#include "glm/vec3.hpp"

class Serializer;

namespace Progression
{

struct MaterialCreateInfo
{
    std::string name;
    glm::vec3 Kd;
    std::string map_Kd_name;
};

struct GfxImage;

struct Material : public Asset
{
    glm::vec3 Kd;
    GfxImage* map_Kd = nullptr;
};

bool Material_Load( Material* material, const MaterialCreateInfo& createInfo );

bool Fastfile_Material_Load( Material* material, Serializer* serializer );

bool Fastfile_Material_Save( const MaterialCreateInfo * const matcreateInfo, Serializer* serializer );

} // namespace Progression
