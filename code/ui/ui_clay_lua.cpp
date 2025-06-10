#include "ui_clay_lua.hpp"
#include "asset/asset_manager.hpp"
#include "core/lua.hpp"
#include "ui/ui_text.hpp"
#define CLAY_IMPLEMENTATION
#include "clay/clay.h"

static std::unordered_map<std::string, u16> s_fontNameToFontIdMap;
static std::vector<PG::Font*> s_fonts;
static Clay_Arena s_clayMemory;

namespace PG::UI
{

Font* GetFontById( u16 fontID ) { return s_fonts[fontID]; }

static u16 GetFontByName( const std::string& name )
{
    auto it = s_fontNameToFontIdMap.find( name );
    if ( it != s_fontNameToFontIdMap.end() )
        return it->second;

    u16 id                      = static_cast<u16>( s_fonts.size() );
    s_fontNameToFontIdMap[name] = id;
    Font* font                  = AssetManager::Get<Font>( name );
    s_fonts.push_back( font );
    return id;
}

static inline Clay_Dimensions MeasureText( Clay_StringSlice text, Clay_TextElementConfig* config, void* userData )
{
    Font* font = GetFontById( config->fontId );
    vec2 dims  = Text::MeasureText( text.chars, text.length, font, config->fontSize );
    return { dims.x, dims.y };
}

Clay_Sizing CreateSizing( int widthType, float widthVal, int heightType, float heightVal )
{
    Clay_Sizing sizing;
    sizing.width.type = (Clay__SizingType)widthType;
    if ( sizing.width.type == CLAY__SIZING_TYPE_PERCENT )
        sizing.width.size.percent = widthVal;
    else
        sizing.width.size.minMax = { widthVal };

    sizing.height.type = (Clay__SizingType)heightType;
    if ( sizing.height.type == CLAY__SIZING_TYPE_PERCENT )
        sizing.height.size.percent = heightVal;
    else
        sizing.height.size.minMax = { heightVal };

    return sizing;
}

Clay_LayoutConfig CreateLayout() { return {}; }

Clay_ImageElementConfig GetImage( const std::string& imgName )
{
    Clay_ImageElementConfig ret;
    GfxImage* img = AssetManager::Get<GfxImage>( imgName );
    PG_ASSERT( img, "No image with name '%s' found", imgName.c_str() );
    ret.imageData = img;

    return ret;
}

Clay_ElementId CreateID( const char* idStr )
{
    Clay_String s;
    s.chars  = idStr;
    s.length = (int)strlen( idStr );
    return CLAY_SID( s );
}

void AddText( const char* str, const Clay_TextElementConfig& config )
{
    Clay_String cStr;
    cStr.isStaticallyAllocated = false;
    cStr.length                = (int)strlen( str );
    cStr.chars                 = str;
    CLAY_TEXT( cStr, CLAY_TEXT_CONFIG( config ) );
}

void HandleClayErrors( Clay_ErrorData errorData ) { LOG_ERR( "Clay error: %s", errorData.errorText.chars ); }

bool Init_ClayLua()
{
    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    s_clayMemory                = Clay_CreateArenaWithCapacityAndMemory( clayRequiredMemory, malloc( clayRequiredMemory ) );

    Clay_Initialize( s_clayMemory, Clay_Dimensions{ 1920, 1080 }, Clay_ErrorHandler{ HandleClayErrors } );
    Clay_SetMeasureTextFunction( MeasureText, nullptr );

    GetFontByName( "arial" );

    Clay_SetMeasureTextFunction( MeasureText, nullptr );

    sol::state_view state( Lua::State() );
    sol::table uiNamespace = state.create_named_table( "UI" );

    uiNamespace["SIZING_TYPE_FIT"]         = CLAY__SIZING_TYPE_FIT;
    uiNamespace["SIZING_TYPE_GROW"]        = CLAY__SIZING_TYPE_GROW;
    uiNamespace["SIZING_TYPE_PERCENT"]     = CLAY__SIZING_TYPE_PERCENT;
    uiNamespace["SIZING_TYPE_FIXED"]       = CLAY__SIZING_TYPE_FIXED;
    sol::usertype<Clay_Sizing> sizing_type = uiNamespace.new_usertype<Clay_Sizing>( "Sizing" );
    uiNamespace["CreateSizing"]            = CreateSizing;

    sol::usertype<Clay_Padding> padding_type = uiNamespace.new_usertype<Clay_Padding>( "Padding" );
    padding_type["left"]                     = &Clay_Padding::left;
    padding_type["right"]                    = &Clay_Padding::right;
    padding_type["top"]                      = &Clay_Padding::top;
    padding_type["bottom"]                   = &Clay_Padding::bottom;
    uiNamespace["CreatePadding"]             = []( u16 l, u16 r, u16 t, u16 b ) { return Clay_Padding{ l, r, t, b }; };
    uiNamespace["CreatePaddingEqual"]        = []( u16 all ) { return Clay_Padding{ all, all, all, all }; };

    uiNamespace["ALIGN_X_LEFT"]                       = CLAY_ALIGN_X_LEFT;
    uiNamespace["ALIGN_X_RIGHT"]                      = CLAY_ALIGN_X_RIGHT;
    uiNamespace["ALIGN_X_CENTER"]                     = CLAY_ALIGN_X_CENTER;
    uiNamespace["ALIGN_Y_TOP"]                        = CLAY_ALIGN_Y_TOP;
    uiNamespace["ALIGN_Y_BOTTOM"]                     = CLAY_ALIGN_Y_BOTTOM;
    uiNamespace["ALIGN_Y_CENTER"]                     = CLAY_ALIGN_Y_CENTER;
    sol::usertype<Clay_ChildAlignment> alignment_type = uiNamespace.new_usertype<Clay_ChildAlignment>( "ChildAlignment" );
    uiNamespace["CreateAlignment"] = []( u8 x, u8 y ) { return Clay_ChildAlignment{ (Clay_LayoutAlignmentX)x, (Clay_LayoutAlignmentY)y }; };

    uiNamespace["LEFT_TO_RIGHT"]                             = CLAY_LEFT_TO_RIGHT;
    uiNamespace["TOP_TO_BOTTOM"]                             = CLAY_TOP_TO_BOTTOM;
    sol::usertype<Clay_LayoutDirection> layoutDirection_type = uiNamespace.new_usertype<Clay_LayoutDirection>( "LayoutDirection" );

    sol::usertype<Clay_LayoutConfig> layout_type = uiNamespace.new_usertype<Clay_LayoutConfig>( "LayoutConfig" );
    layout_type["sizing"]                        = &Clay_LayoutConfig::sizing;
    layout_type["padding"]                       = &Clay_LayoutConfig::padding;
    layout_type["childGap"]                      = &Clay_LayoutConfig::childGap;
    layout_type["childAlignment"]                = &Clay_LayoutConfig::childAlignment;
    layout_type["layoutDirection"]               = &Clay_LayoutConfig::layoutDirection;
    uiNamespace["CreateLayout"]                  = CreateLayout;

    sol::usertype<Clay_BorderWidth> borderWidth_type = uiNamespace.new_usertype<Clay_BorderWidth>( "BorderWidth" );
    borderWidth_type["left"]                         = &Clay_BorderWidth::left;
    borderWidth_type["right"]                        = &Clay_BorderWidth::right;
    borderWidth_type["top"]                          = &Clay_BorderWidth::top;
    borderWidth_type["bottom"]                       = &Clay_BorderWidth::bottom;
    borderWidth_type["betweenChildren"]              = &Clay_BorderWidth::betweenChildren;

    sol::usertype<Clay_BorderElementConfig> border_type = uiNamespace.new_usertype<Clay_BorderElementConfig>( "BorderConfig" );
    border_type["color"]                                = &Clay_BorderElementConfig::color;
    border_type["width"]                                = &Clay_BorderElementConfig::width;
    uiNamespace["BorderWidth"]                          = []( u16 l, u16 r, u16 t, u16 b ) { return Clay_BorderWidth{ l, r, t, b }; };

    uiNamespace["Color"] = []( float r, float g, float b, float a ) { return Clay_Color{ r, g, b, a }; };
    sol::usertype<Clay_ElementDeclaration> eDeclatation_type = uiNamespace.new_usertype<Clay_ElementDeclaration>( "ElementDeclaration" );
    eDeclatation_type["id"]                                  = &Clay_ElementDeclaration::id;
    eDeclatation_type["layout"]                              = &Clay_ElementDeclaration::layout;
    eDeclatation_type["backgroundColor"]                     = &Clay_ElementDeclaration::backgroundColor;
    eDeclatation_type["image"]                               = &Clay_ElementDeclaration::image;
    eDeclatation_type["border"]                              = &Clay_ElementDeclaration::border;

    sol::usertype<Clay_TextElementConfig> text_config_type = uiNamespace.new_usertype<Clay_TextElementConfig>( "TextConfig" );
    text_config_type["textColor"]                          = &Clay_TextElementConfig::textColor;
    text_config_type["fontId"]                             = &Clay_TextElementConfig::fontId;
    text_config_type["fontSize"]                           = &Clay_TextElementConfig::fontSize;
    text_config_type["wrapMode"]                           = &Clay_TextElementConfig::wrapMode;
    text_config_type["textAlignment"]                      = &Clay_TextElementConfig::textAlignment;

    uiNamespace["TEXT_WRAP_WORDS"]    = CLAY_TEXT_WRAP_WORDS;
    uiNamespace["TEXT_WRAP_NEWLINES"] = CLAY_TEXT_WRAP_NEWLINES;
    uiNamespace["TEXT_WRAP_NONE"]     = CLAY_TEXT_WRAP_NONE;

    uiNamespace["CLAY_TEXT_ALIGN_LEFT"]   = CLAY_TEXT_ALIGN_LEFT;
    uiNamespace["CLAY_TEXT_ALIGN_CENTER"] = CLAY_TEXT_ALIGN_CENTER;
    uiNamespace["CLAY_TEXT_ALIGN_RIGHT"]  = CLAY_TEXT_ALIGN_RIGHT;

    uiNamespace["OpenElement"]      = Clay__OpenElement;
    uiNamespace["ConfigureElement"] = Clay__ConfigureOpenElement;
    uiNamespace["CloseElement"]     = Clay__CloseElement;
    uiNamespace["GetImage"]         = GetImage;
    uiNamespace["CreateID"]         = CreateID;
    uiNamespace["GetFont"]          = GetFontByName;
    uiNamespace["AddText"]          = AddText;

    return true;
}

void Shutdown_ClayLua()
{
    s_fontNameToFontIdMap.clear();
    s_fonts.clear();
}

} // namespace PG::UI
