#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"

#define CONVERTER_ERROR( ... ) { LOG_ERR( __VA_ARGS__ ); g_converterStatus.parsingError = true; return; }
#define PARSE_ERROR( ... ) { LOG_ERR( __VA_ARGS__ ); return nullptr; }

namespace PG
{

struct ConverterConfigOptions
{
    bool force = false;
};

struct ConverterStatus
{
    bool parsingError           = false;
    int checkDependencyErrors   = 0;
};

extern ConverterConfigOptions g_converterConfigOptions;
extern ConverterStatus g_converterStatus;

// IsAssetOutOfDate will take care of all the individual input files for each asset, but
// AddFastfileDependency needs to be called on each converted .ffi file
void AddFastfileDependency( const std::string& file );
void ClearAllFastfileDependencies();
time_t GetLatestFastfileDependency();

void FilenameSlashesToUnderscores( std::string& str );

class BaseAssetConverter
{
public:
    BaseAssetConverter( const std::string& assetName, AssetType assetType ) : m_assetNameInJsonFile( assetName ), m_assetType( assetType ) {}
    ~BaseAssetConverter();

    virtual std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value, std::shared_ptr<const BaseAssetCreateInfo> parent ) = 0;
    int ConvertAll();
    int CheckAllDependencies();
    virtual bool BuildFastFile( Serializer* serializer ) const;
    std::string AssetNameInJSONFile() const { return m_assetNameInJsonFile; }

protected:
    virtual std::string GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const = 0;
    virtual bool IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo ) = 0;
    virtual bool ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const = 0;

    template< typename DerivedAssetType, typename DerivedAssetCreateInfoType >
    bool ConvertSingleInternal( const BaseAssetCreateInfo* baseInfo ) const
    {
        const DerivedAssetCreateInfoType* info = (const DerivedAssetCreateInfoType*)baseInfo;
        LOG( "Converting %s '%s'...", m_assetNameInJsonFile.c_str(), info->name.c_str() );
        DerivedAssetType asset;
        if ( !asset.Load( baseInfo ) )
        {
            return false;
        }
        std::string fastfileName = GetFastFileName( info );
        try
        {
            Serializer serializer;
            if ( !serializer.OpenForWrite( fastfileName ) )
            {
                return false;
            }
            if ( !asset.FastfileSave( &serializer ) )
            {
                LOG_ERR( "Error while writing %s '%s' to fastfile", m_assetNameInJsonFile.c_str(), info->name.c_str() );
                serializer.Close();
                DeleteFile( fastfileName );
                return false;
            }
            serializer.Close();
        }
        catch ( std::exception& e )
        {
            DeleteFile( fastfileName );
            throw e;
        }

        return true;
    }

    const std::string m_assetNameInJsonFile;
    const AssetType m_assetType;
    std::vector< BaseAssetCreateInfo* > m_parsedAssets;
    std::vector< BaseAssetCreateInfo* > m_outOfDateAssets;
};

} // namespace PG