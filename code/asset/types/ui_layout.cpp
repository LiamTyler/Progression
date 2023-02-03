#include "asset/types/ui_layout.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "pugixml-1.13/src/pugixml.hpp"
#include <fstream>

namespace PG
{

#if USING( CONVERTER )
static void ParseUIElement( const pugi::xml_node& element, UIElementCreateInfo& createInfo )
{
    for ( auto attribIt = element.attributes_begin(); attribIt != element.attributes_end(); ++attribIt )
    {
        const pugi::xml_attribute& attrib = *attribIt;
        if ( !strcmp( attrib.name(), "tint" ) )
        {
            printf( "" );
        }
        else if ( !strcmp( attrib.name(), "blendMode" ) )
        {
            printf( "" );
        }
        else
        {
            LOG_WARN( "UIElement: unknown attribute '%s'", attrib.name() );
        }
    }
}
#endif // #if USING( CONVERTER )
 
bool UILayout::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const UILayoutCreateInfo* createInfo = (const UILayoutCreateInfo*)baseInfo;
    name = createInfo->name;

#if USING( CONVERTER )
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file( createInfo->xmlFilename.c_str() );
    if ( !result )
    {
        LOG_ERR( "Could not load UILayout xml file '%s'", createInfo->xmlFilename.c_str() );
        return false;
    }

    for ( auto it = doc.begin(); it != doc.end(); ++it )
    {
        if ( !strcmp( it->name(), "element" ) )
        {
            UIElementCreateInfo& createInfo = createInfos.emplace_back();
            ParseUIElement( *it, createInfo );
        }
    }

    return true;
#else // #if // #if USING( CONVERTER )
    return false;
#endif // #else // #if // #if USING( CONVERTER )
}


bool UILayout::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    serializer->Read( name );

    return true;
}


bool UILayout::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
    serializer->Write( name );

    return true;
}

} // namespace PG
