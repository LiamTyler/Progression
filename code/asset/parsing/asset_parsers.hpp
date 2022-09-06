#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
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
    using BaseInfoPtr = std::shared_ptr<BaseAssetCreateInfo>;
    using ConstBaseInfoPtr = const std::shared_ptr<const BaseAssetCreateInfo>&;

    const AssetType assetType;

    BaseAssetParser( AssetType inAssetType ) : assetType( inAssetType ) {}
    virtual ~BaseAssetParser() = default;

    virtual std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value ) = 0;
};


template<typename DerivedInfo>
class BaseAssetParserTemplate : public BaseAssetParser
{
public:
    using DerivedInfoPtr = std::shared_ptr<DerivedInfo>;

    BaseAssetParserTemplate( AssetType inAssetType ) : BaseAssetParser( inAssetType ) {}
    virtual ~BaseAssetParserTemplate() = default;

    virtual std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value ) override
    {
        auto info = std::make_shared<DerivedInfo>();
        const std::string assetName = value["name"].GetString();
        info->name = assetName;

        if ( !ParseInternal( value, info ) )
        {
            return nullptr;
        }
        return info;
    }

protected:
    virtual bool ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info ) = 0;
};


#define PG_DECLARE_ASSET_PARSER( AssetName, AssetType, CreateInfo ) \
    class AssetName##Parser : public BaseAssetParserTemplate<CreateInfo> \
    { \
    public: \
        AssetName##Parser() : BaseAssetParserTemplate( AssetType ) {} \
    protected: \
        bool ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info ) override; \
    }


PG_DECLARE_ASSET_PARSER( GfxImage, ASSET_TYPE_GFX_IMAGE, GfxImageCreateInfo );
PG_DECLARE_ASSET_PARSER( Material, ASSET_TYPE_MATERIAL, MaterialCreateInfo );
PG_DECLARE_ASSET_PARSER( Model, ASSET_TYPE_MODEL, ModelCreateInfo );
PG_DECLARE_ASSET_PARSER( Script, ASSET_TYPE_SCRIPT, ScriptCreateInfo );
PG_DECLARE_ASSET_PARSER( Shader, ASSET_TYPE_SHADER, ShaderCreateInfo );
PG_DECLARE_ASSET_PARSER( Textureset, ASSET_TYPE_TEXTURESET, TexturesetCreateInfo );

extern const std::shared_ptr<BaseAssetParser> g_assetParsers[ASSET_TYPE_COUNT];

} // namespace PG