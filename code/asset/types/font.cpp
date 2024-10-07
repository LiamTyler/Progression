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
    DeserializeAssetName( serializer, &fontAtlasTexture );
    fontAtlasTexture.FastfileLoad( serializer );
    serializer->Read( glyphs );
    serializer->Read( metrics );

    return true;
}

bool Font::FastfileSave( Serializer* serializer ) const
{
    SerializeName( serializer );
    fontAtlasTexture.FastfileSave( serializer );
    serializer->Write( glyphs );
    serializer->Write( metrics );

    return true;
}

const Font::Glyph& Font::GetGlyph( char c ) const
{
    u32 charCode = (u32)c;
    if ( charCode < FONT_FIRST_CHARACTER_CODE || charCode > FONT_LAST_CHARACTER_CODE )
        return glyphs[(u32)'?' - FONT_FIRST_CHARACTER_CODE];
    else
        return glyphs[charCode - FONT_FIRST_CHARACTER_CODE];
}

} // namespace PG
