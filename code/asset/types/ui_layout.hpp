#pragma once

#include "asset/types/script.hpp"
#include "ui/ui_element.hpp"

namespace PG
{

struct UILayoutCreateInfo : public BaseAssetCreateInfo
{
    std::string xmlFilename;
};

struct UIElementCreateInfo
{
    UI::UIElement element;
    std::string updateFuncName;
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