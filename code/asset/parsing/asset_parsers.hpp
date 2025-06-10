#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/font.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
#include "asset/types/pipeline.hpp"
#include "asset/types/script.hpp"
#include "asset/types/shader.hpp"
#include "asset/types/textureset.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"

namespace PG
{

class BaseAssetParser
{
public:
    using BaseInfoPtr      = std::shared_ptr<BaseAssetCreateInfo>;
    using ConstBaseInfoPtr = const std::shared_ptr<const BaseAssetCreateInfo>&;

    const AssetType assetType;

    BaseAssetParser( AssetType inAssetType ) : assetType( inAssetType ) {}
    virtual ~BaseAssetParser() = default;

    virtual BaseInfoPtr Parse( const rapidjson::Value& value, ConstBaseInfoPtr parentCreateInfo ) = 0;
};

template <typename DerivedInfo>
class BaseAssetParserTemplate : public BaseAssetParser
{
public:
    using DerivedInfoPtr = std::shared_ptr<DerivedInfo>;

    BaseAssetParserTemplate( AssetType inAssetType ) : BaseAssetParser( inAssetType ) {}
    virtual ~BaseAssetParserTemplate() = default;

    virtual BaseInfoPtr Parse( const rapidjson::Value& value, ConstBaseInfoPtr parentCreateInfo ) override
    {
        auto info = std::make_shared<DerivedInfo>();
        if ( parentCreateInfo )
            *info = *std::static_pointer_cast<const DerivedInfo>( parentCreateInfo );

        const std::string assetName = value["name"].GetString();
        info->name                  = assetName;

        return ParseInternal( value, info ) ? info : nullptr;
    }

protected:
    virtual bool ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info ) = 0;
};

struct NullParserCreateInfo : public BaseAssetCreateInfo
{
};

class NullParser : public BaseAssetParserTemplate<NullParserCreateInfo>
{
public:
    NullParser() : BaseAssetParserTemplate( ASSET_TYPE_COUNT ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info ) override
    {
        PG_UNUSED( value );
        PG_UNUSED( info );
        return false;
    }
};

#define PG_DECLARE_ASSET_PARSER( AssetName, AssetType, CreateInfo )                        \
    class AssetName##Parser : public BaseAssetParserTemplate<CreateInfo>                   \
    {                                                                                      \
    public:                                                                                \
        AssetName##Parser() : BaseAssetParserTemplate( AssetType ) {}                      \
                                                                                           \
    protected:                                                                             \
        bool ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info ) override; \
    }

PG_DECLARE_ASSET_PARSER( GfxImage, ASSET_TYPE_GFX_IMAGE, GfxImageCreateInfo );
PG_DECLARE_ASSET_PARSER( Material, ASSET_TYPE_MATERIAL, MaterialCreateInfo );
PG_DECLARE_ASSET_PARSER( Model, ASSET_TYPE_MODEL, ModelCreateInfo );
PG_DECLARE_ASSET_PARSER( Script, ASSET_TYPE_SCRIPT, ScriptCreateInfo );
PG_DECLARE_ASSET_PARSER( Pipeline, ASSET_TYPE_PIPELINE, PipelineCreateInfo );
PG_DECLARE_ASSET_PARSER( Font, ASSET_TYPE_FONT, FontCreateInfo );
PG_DECLARE_ASSET_PARSER( Textureset, ASSET_TYPE_TEXTURESET, TexturesetCreateInfo );

extern const std::unique_ptr<BaseAssetParser> g_assetParsers[ASSET_TYPE_COUNT];

} // namespace PG
