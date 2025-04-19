#pragma once

#include "asset/types/script.hpp"
#include "ui/ui_element.hpp"

namespace PG
{

struct UILayoutCreateInfo : public BaseAssetCreateInfo
{
    std::string xmlFilename;
};

std::string GetAbsPath_UILayoutFilename( const std::string& filename );

struct UIElementCreateInfo
{
};

struct UILayout : public BaseAsset
{
public:
    UILayout() = default;

    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

    Script* script;
};

} // namespace PG
