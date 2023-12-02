#include "asset/types/ui_layout.hpp"
#include "asset/asset_manager.hpp"
#include "pugixml-1.13/src/pugixml.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <fstream>

namespace PG
{

std::string GetAbsPath_UILayoutFilename( const std::string& filename ) { return PG_ASSET_DIR + filename; }

using namespace UI;

static glm::vec2 ParseVec2( const char* str )
{
    glm::vec2 v( 0 );
    char* endPtr1;
    v.x = std::strtof( str, &endPtr1 );
    v.y = std::strtof( endPtr1, NULL );
    return v;
}

static glm::vec4 ParseVec4( const char* str )
{
    glm::vec4 v( 0 );
    char *endPtr1, *endPtr2;
    v.x = std::strtof( str, &endPtr1 );
    v.y = std::strtof( endPtr1, &endPtr2 );
    v.z = std::strtof( endPtr2, &endPtr1 );
    v.w = std::strtof( endPtr1, NULL );
    return v;
}

static UIElementBlendMode ParseBlendMode( const char* str )
{
    static const char* blendStrs[] =
    {
        "opaque",
        "blend",
        "additive",
    };
    static_assert( ARRAY_COUNT( blendStrs ) == Underlying( UIElementBlendMode::COUNT ) );

    for ( int i = 0; i < ARRAY_COUNT( blendStrs ); ++i )
    {
        if ( !strcmp( str, blendStrs[i] ) )
        {
            return static_cast<UIElementBlendMode>( i );
        }
    }

    LOG_ERR( "Unrecognized UIElementBlendMode '%s' in xml layout file, using opaque instead. Is case sensitive", str );
    return UIElementBlendMode::OPAQUE;
}

static UIElementType ParseElementType( const char* str )
{
    static const char* strs[] =
    {
        "regular",
        "mkdd_diag_tint",
    };
    static_assert( ARRAY_COUNT( strs ) == Underlying( UIElementType::COUNT ) );

    for ( int i = 0; i < ARRAY_COUNT( strs ); ++i )
    {
        if ( !strcmp( str, strs[i] ) )
        {
            return static_cast<UIElementType>( i );
        }
    }

    LOG_ERR( "Unrecognized UIElementType '%s' in xml layout file, using opaque instead. Is case sensitive", str );
    return UIElementType::DEFAULT;
}

#if USING( CONVERTER )
static UIElementHandle ParseUIElement(
    const pugi::xml_node& element, std::vector<UIElementCreateInfo>& createInfos, UIElementHandle parentIdx )
{
    UIElementHandle idx = static_cast<UIElementHandle>( createInfos.size() );
    createInfos.emplace_back();
    createInfos[idx].element.parent      = parentIdx;
    createInfos[idx].element.prevSibling = UI_NULL_HANDLE;
    createInfos[idx].element.nextSibling = UI_NULL_HANDLE;
    createInfos[idx].element.firstChild  = UI_NULL_HANDLE;
    createInfos[idx].element.lastChild   = UI_NULL_HANDLE;

    using namespace pugi;
    for ( auto attribIt = element.attributes_begin(); attribIt != element.attributes_end(); ++attribIt )
    {
        const xml_attribute& attrib = *attribIt;
        const char* val             = attrib.value();

        if ( !strcmp( attrib.name(), "pos" ) )
        {
            createInfos[idx].element.pos = ParseVec2( val );
        }
        else if ( !strcmp( attrib.name(), "size" ) )
        {
            createInfos[idx].element.dimensions = ParseVec2( val );
        }
        else if ( !strcmp( attrib.name(), "tint" ) )
        {
            createInfos[idx].element.tint = glm::clamp( ParseVec4( val ), glm::vec4( 0 ), glm::vec4( 1 ) );
        }
        else if ( !strcmp( attrib.name(), "blendMode" ) )
        {
            createInfos[idx].element.blendMode = ParseBlendMode( val );
        }
        else if ( !strcmp( attrib.name(), "visible" ) )
        {
            if ( attrib.as_bool() )
                createInfos[idx].element.userFlags |= UIElementUserFlags::VISIBLE;
            else
                createInfos[idx].element.userFlags &= ~UIElementUserFlags::VISIBLE;
        }
        else if ( !strcmp( attrib.name(), "tonemap" ) )
        {
            if ( attrib.as_bool() )
                createInfos[idx].element.userFlags |= UIElementUserFlags::APPLY_TONEMAPPING;
            else
                createInfos[idx].element.userFlags &= ~UIElementUserFlags::APPLY_TONEMAPPING;
        }
        else if ( !strcmp( attrib.name(), "image" ) )
        {
            createInfos[idx].imageName = val;
        }
        else if ( !strcmp( attrib.name(), "update" ) )
        {
            createInfos[idx].updateFuncName = val;
            createInfos[idx].element.scriptFlags |= UIElementScriptFlags::UPDATE;
        }
        else if ( !strcmp( attrib.name(), "mouseButtonDown" ) )
        {
            createInfos[idx].mouseButtonDownFuncName = val;
            createInfos[idx].element.scriptFlags |= UIElementScriptFlags::MOUSE_BUTTON_DOWN;
        }
        else if ( !strcmp( attrib.name(), "mouseButtonUp" ) )
        {
            createInfos[idx].mouseButtonUpFuncName = val;
            createInfos[idx].element.scriptFlags |= UIElementScriptFlags::MOUSE_BUTTON_UP;
        }
        else if ( !strcmp( attrib.name(), "type" ) )
        {
            createInfos[idx].element.type = ParseElementType( val );
        }
        else
        {
            LOG_WARN( "UIElement: unknown attribute '%s' with value '%s'", attrib.name(), val );
        }
    }

    UIElementHandle prevChild = UI_NULL_HANDLE;
    for ( xml_node child : element.children() )
    {
        UIElementHandle childIdx = ParseUIElement( child, createInfos, idx );
        if ( prevChild == UI_NULL_HANDLE )
        {
            createInfos[idx].element.firstChild = childIdx;
        }
        else
        {
            createInfos[prevChild].element.nextSibling = childIdx;
            createInfos[childIdx].element.prevSibling  = prevChild;
        }

        if ( !child.next_sibling() )
        {
            createInfos[idx].element.lastChild = childIdx;
        }

        prevChild = childIdx;
    }

    return idx;
}
#endif // #if USING( CONVERTER )

bool UILayout::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const UILayoutCreateInfo* createInfo = (const UILayoutCreateInfo*)baseInfo;
    name                                 = createInfo->name;

#if USING( CONVERTER )
    const std::string absPath = GetAbsPath_UILayoutFilename( createInfo->xmlFilename );
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file( absPath.c_str() );
    if ( !result )
    {
        LOG_ERR( "Could not load UILayout xml file '%s'.\n\tError: %s", createInfo->xmlFilename.c_str(), result.description() );
        return false;
    }

    // add a dummy root node, to guarantee a single root node to the entire layout
    {
        UIElement& rootElement  = createInfos.emplace_back().element;
        rootElement.parent      = UI_NULL_HANDLE;
        rootElement.prevSibling = UI_NULL_HANDLE;
        rootElement.nextSibling = UI_NULL_HANDLE;
        rootElement.firstChild  = UI_NULL_HANDLE;
        rootElement.lastChild   = UI_NULL_HANDLE;
    }

    UIElementHandle prevElement = UI_NULL_HANDLE;
    for ( auto it = doc.begin(); it != doc.end(); ++it )
    {
        if ( !strcmp( it->name(), "element" ) )
        {
            UIElementHandle element = ParseUIElement( *it, createInfos, 0 );
            if ( prevElement != UI_NULL_HANDLE )
            {
                createInfos[element].element.prevSibling     = prevElement;
                createInfos[prevElement].element.nextSibling = element;
            }

            UIElement& rootElement = createInfos[0].element;
            if ( rootElement.firstChild == UI_NULL_HANDLE )
            {
                rootElement.firstChild = element;
                rootElement.lastChild  = element;
            }
            else
            {
                rootElement.lastChild = element;
            }

            prevElement = element;
        }
    }

    script                  = nullptr;
    std::string scriptFName = GetFilenameMinusExtension( absPath ) + ".lua";
    if ( PathExists( scriptFName ) )
    {
        script = AssetManager::Get<Script>( createInfo->name );
        PG_ASSERT( script );
    }

    return true;
#else // #if USING( CONVERTER )
    return false;
#endif // #else // #if USING( CONVERTER )
}

bool UILayout::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    serializer->Read( name );
    uint32_t numElements;
    serializer->Read( numElements );
    createInfos.resize( numElements );
    for ( auto& info : createInfos )
    {
        serializer->Read( info.element.parent );
        serializer->Read( info.element.prevSibling );
        serializer->Read( info.element.nextSibling );
        serializer->Read( info.element.firstChild );
        serializer->Read( info.element.lastChild );
        serializer->Read( info.element.type );
        serializer->Read( info.element.userFlags );
        serializer->Read( info.element.scriptFlags );
        serializer->Read( info.element.blendMode );
        serializer->Read( info.element.pos );
        serializer->Read( info.element.dimensions );
        serializer->Read( info.element.tint );
        serializer->Read( info.imageName );
        serializer->Read( info.updateFuncName );
        serializer->Read( info.mouseButtonDownFuncName );
        serializer->Read( info.mouseButtonUpFuncName );
    }
    std::string scriptName;
    serializer->Read( scriptName );
    script = AssetManager::Get<Script>( scriptName );
    PG_ASSERT( script, "Cannot find Script '%s' for UILayout '%s'", scriptName.c_str(), name.c_str() );

    return true;
}

bool UILayout::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
    serializer->Write( name );
    uint32_t numElements = static_cast<uint32_t>( createInfos.size() );
    serializer->Write( numElements );
    for ( const auto& info : createInfos )
    {
        serializer->Write( info.element.parent );
        serializer->Write( info.element.prevSibling );
        serializer->Write( info.element.nextSibling );
        serializer->Write( info.element.firstChild );
        serializer->Write( info.element.lastChild );
        serializer->Write( info.element.type );
        serializer->Write( info.element.userFlags );
        serializer->Write( info.element.scriptFlags );
        serializer->Write( info.element.blendMode );
        serializer->Write( info.element.pos );
        serializer->Write( info.element.dimensions );
        serializer->Write( info.element.tint );
        serializer->Write( info.imageName );
        serializer->Write( info.updateFuncName );
        serializer->Write( info.mouseButtonDownFuncName );
        serializer->Write( info.mouseButtonUpFuncName );
    }

    std::string scriptName = script ? script->name : "";
    serializer->Write( scriptName );

    return true;
}

} // namespace PG
