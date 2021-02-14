#pragma once

#include "asset/types/gfx_image.hpp"

class Serializer;

namespace PG
{

struct EnvironmentMapCreateInfo
{
    std::string name;
    std::string cubeFaceFilenames[6];
    bool flipVertically      = false;
    bool convertFromLDRtoHDR = false;
};

struct EnvironmentMap : public Asset
{
public:
    EnvironmentMap() = default;

    GfxImage* image;
};

bool EnvironmentMap_Load( EnvironmentMap* map, const EnvironmentMapCreateInfo& createInfo );

bool Fastfile_EnvironmentMap_Load( EnvironmentMap* map, Serializer* serializer );

bool Fastfile_EnvironmentMap_Save( const EnvironmentMap * const map, Serializer* serializer );


} // namespace PG