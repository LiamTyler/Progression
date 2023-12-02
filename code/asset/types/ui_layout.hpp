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
    UI::UIElement element;
    std::string updateFuncName;
    std::string mouseButtonDownFuncName;
    std::string mouseButtonUpFuncName;
    std::string imageName;
};

struct UILayout : public BaseAsset
{
public:
    UILayout() = default;

    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

    std::vector<UIElementCreateInfo> createInfos;
    Script* script;
};

} // namespace PG
