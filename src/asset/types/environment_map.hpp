#pragma once

#include "asset/types/gfx_image.hpp"

class Serializer;

namespace PG
{

struct EnvironmentMapCreateInfo : public BaseAssetCreateInfo
{
    std::string flattenedCubemapFilename;
    std::string equirectangularFilename;
    std::string faceFilenames[6];
    bool flipVertically = false;
};

struct EnvironmentMap : public BaseAsset
{
public:
    EnvironmentMap() = default;

    GfxImage* image;
};

bool EnvironmentMap_Load( EnvironmentMap* map, const EnvironmentMapCreateInfo& createInfo );

bool Fastfile_EnvironmentMap_Load( EnvironmentMap* map, Serializer* serializer );

bool Fastfile_EnvironmentMap_Save( const EnvironmentMap * const map, Serializer* serializer );


} // namespace PG