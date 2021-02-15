#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"

namespace PG
{

struct ConverterConfigOptions
{
    bool force                   = false;
    bool generateShaderDebugInfo = false;
    bool saveShaderPreproc       = false;
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

class Converter
{
public:
    Converter( const std::string& assetName, AssetType assetType ) : m_assetNameInJsonFile( assetName ), m_assetType( assetType ) {}
    ~Converter();

    virtual void Parse( const rapidjson::Value& value ) = 0;
    int ConvertAll();
    int CheckAllDependencies();
    virtual bool BuildFastFile( Serializer* serializer ) const;
    std::string AssetNameInJSONFile() const { return m_assetNameInJsonFile; }

protected:
    virtual std::string GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const = 0;
    virtual bool IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo ) = 0;
    virtual bool ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const = 0;

    const std::string m_assetNameInJsonFile;
    const AssetType m_assetType;
    std::vector< BaseAssetCreateInfo* > m_parsedAssets;
    std::vector< BaseAssetCreateInfo* > m_outOfDateAssets;
};

} // namespace PG