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

    return true;
}

bool Font::FastfileSave( Serializer* serializer ) const
{
    SerializeName( serializer );
    fontAtlasTexture.FastfileSave( serializer );
    serializer->Write( glyphs );

    return true;
}

const Font::Glyph& Font::GetGlyph( char c ) const { return glyphs[(u32)c - FONT_FIRST_CHARACTER_CODE]; }

} // namespace PG
