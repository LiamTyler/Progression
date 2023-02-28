#pragma once

#include "asset/types/base_asset.hpp"

namespace PG
{

struct ScriptCreateInfo : public BaseAssetCreateInfo
{
    std::string filename;
};

std::string GetAbsPath_ScriptFilename( const std::string& filename );

struct Script : public BaseAsset
{
public:
    Script() = default;

    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

    std::string scriptText;
};

} // namespace PG