#include "asset/types/font.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"

namespace PG
{

std::string GetAbsPath_FontFilename( const std::string& filename ) { return PG_ASSET_DIR + filename; }

void Font::Free() { fontAtlasTexture.Free(); }

bool Font::FastfileLoad( Serializer* serializer )
{
    fontAtlasTexture.FastfileLoad( serializer );
    serializer->Read( metrics );
    u32 numGlyphs = serializer->Read<u32>();
    glyphs.resize( numGlyphs );
    for ( u32 i = 0; i < numGlyphs; ++i )
    {
        Glyph& g = glyphs[i];
        serializer->Read( g.planeMin );
        serializer->Read( g.planeMax );
        serializer->Read( g.uvMin );
        serializer->Read( g.uvMax );
        serializer->Read( g.advance );
        serializer->Read( g.characterCode );
        serializer->Read( g.kernings );
    }

    return true;
}

bool Font::FastfileSave( Serializer* serializer ) const
{
    fontAtlasTexture.FastfileSave( serializer );
    serializer->Write( metrics );
    u32 numGlyphs = (u32)glyphs.size();
    serializer->Write( numGlyphs );
    for ( u32 i = 0; i < numGlyphs; ++i )
    {
        const Glyph& g = glyphs[i];
        serializer->Write( g.planeMin );
        serializer->Write( g.planeMax );
        serializer->Write( g.uvMin );
        serializer->Write( g.uvMax );
        serializer->Write( g.advance );
        serializer->Write( g.characterCode );
        serializer->Write( g.kernings );
    }

    return true;
}

bool IsCharValid( char c ) { return FONT_FIRST_CHARACTER_CODE <= c && c <= FONT_LAST_CHARACTER_CODE; }

const Font::Glyph& Font::GetGlyph( char c ) const
{
    return IsCharValid( c ) ? glyphs[c - FONT_FIRST_CHARACTER_CODE] : glyphs['?' - FONT_FIRST_CHARACTER_CODE];
}

float Font::GetKerning( char from, char to ) const
{
    if ( !IsCharValid( from ) || !IsCharValid( to ) )
        return 0;
    return glyphs[from - FONT_FIRST_CHARACTER_CODE].kernings[to - FONT_FIRST_CHARACTER_CODE];
}

} // namespace PG
