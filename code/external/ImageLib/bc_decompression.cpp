#include "bc_compression.hpp"
#include "bc7_reference_tables.hpp"
//#include "compressonator/cmp_core/source/cmp_core.h"
#include "shared/assert.hpp"
#include "shared/bitops.hpp"
#include "shared/logger.hpp"

struct BC1Block
{
    uint16_t endpoint0;
    uint16_t endpoint1;
    uint32_t colorIndices;
};
static_assert( sizeof( BC1Block ) == 8 );

static void Color565To888( uint16_t color565, uint32_t& r, uint32_t& g, uint32_t& b )
{
    // If you just took the bits, and tried multiplying by 8 (4 for green) to rescale, it would almost work, but you wouldn't reach 255
    // Ex: red [0-31] * 8 -> [0, 248]. As a result, you don't want even 32 spaces of 8, but rather 7 times you want the space to be 9.
    // For red, we add the 3 most significant bits to the least significant, so that every multiple of 32 actually becomes 33.
    // r = 3 -> 24 vs 24. r = 4 -> 33 vs 32
    // r = 7 -> 57 vs 56. r = 8 -> 66 vs 64
    // r = 30 -> 247 vs 240. r = 31 -> 255 vs 248
    // You could also do r = Round( 255.0f * r / 31.0f ). That gives you the full range, and slightly different spacing, but
    // the average reconstruction error is actually better with that method (depending on how the data was packed). It is just more expensive than a couple bitshifts and adds

    r = (color565 & 0xF800) >> 8; // active bits: 1111,1000
    r += (r >> 5);
    g = (color565 & 0x07E0) >> 3; // active bits: 1111,1100
    g += (g >> 6);
    b = (color565 & 0x001F) << 3; // active bits: 1111,1000
    b += (b >> 5);
}


static void Decompress_BC1_Block( const uint8_t* compressedBlock, uint8_t decompressedBlock[64] )
{
    const BC1Block& block = *reinterpret_cast<const BC1Block*>( compressedBlock );
    uint32_t r0, g0, b0;
    uint32_t r1, g1, b1;
    Color565To888( block.endpoint0, r0, g0, b0 );
    Color565To888( block.endpoint1, r1, g1, b1 );

    uint32_t c0 = 0xFF << 24 | b0 << 16 | g0 << 8 | r0;
    uint32_t c1 = 0xFF << 24 | b1 << 16 | g1 << 8 | r1;
    uint32_t* decompressed = reinterpret_cast<uint32_t*>( decompressedBlock );
    if ( block.endpoint0 > block.endpoint1 )
    {
        // the +1 is for rounding when doing integer division / 3. Ex: 0 / 3 = 0, 1 / 3 = 0, 2 / 3 = 0, but we want the 2 / 3 to round up to 1
        uint32_t c2 = 0xFF << 24 | ((2*b0 + b1 + 1) / 3) << 16 | ((2*g0 + g1 + 1) / 3) << 8 | ((2*r0 + r1 + 1) / 3);
        uint32_t c3 = 0xFF << 24 | ((b0 + 2*b1 + 1) / 3) << 16 | ((g0 + 2*g1 + 1) / 3) << 8 | ((r0 + 2*r1 + 1) / 3);
        for ( int i = 0; i < 16; ++i )
        {
            uint32_t index = (block.colorIndices >> (2 * i)) & 0x3;
            switch ( index )
            {
            case 0:
                decompressed[i] = c0;
                break;
            case 1:
                decompressed[i] = c1;
                break;
            case 2:
                decompressed[i] = c2;
                break;
            case 3:
                decompressed[i] = c3;
                break;
            }
        }
    }
    else
    {
        uint32_t c2 = 0xFF << 24 | ((b0 + b1) / 2) << 16 | ((g0 + g1) / 2) << 8 | ((r0 + r1) / 2);
        uint32_t c3 = 0;
        for ( int i = 0; i < 16; ++i )
        {
            uint8_t index = (block.colorIndices >> (2 * i)) & 0x3;
            switch ( index )
            {
            case 0:
                decompressed[i] = c0;
                break;
            case 1:
                decompressed[i] = c1;
                break;
            case 2:
                decompressed[i] = c2;
                break;
            case 3:
                decompressed[i] = c3;
                break;
            }
        }
    }
}


static void Decompress_BC2_Block( const uint8_t* compressedBlock, uint8_t decompressedBlock[64] )
{
    Decompress_BC1_Block( compressedBlock + 8, decompressedBlock );
    const size_t& alphaBlock = *reinterpret_cast<const size_t*>( compressedBlock );
    for ( int i = 0; i < 16; ++i )
    {
        uint32_t alphaBits = (alphaBlock >> (4 * i)) & 0xF;
        decompressedBlock[4*i + 3] = (alphaBits << 4) + alphaBits;
    }
}


static void Decompress_BC4_Block_UNorm( const uint8_t* compressedBlock, uint8_t* decompressedBlock, int stride = 1 )
{
    uint8_t palette[8];
    palette[0] = *compressedBlock;
    palette[1] = *(compressedBlock + 1);
    uint8_t p0 = palette[0];
    uint8_t p1 = palette[1];
    if ( p0 > p1 )
    {
        // the +3 is for rounding, when doing integer division / 7. Similar to BC1
        palette[2] = static_cast<uint8_t>( (6 * p0 + 1 * p1 + 3) / 7 ); // bit code 010
        palette[3] = static_cast<uint8_t>( (5 * p0 + 2 * p1 + 3) / 7 ); // bit code 011
        palette[4] = static_cast<uint8_t>( (4 * p0 + 3 * p1 + 3) / 7 ); // bit code 100
        palette[5] = static_cast<uint8_t>( (3 * p0 + 4 * p1 + 3) / 7 ); // bit code 101
        palette[6] = static_cast<uint8_t>( (2 * p0 + 5 * p1 + 3) / 7 ); // bit code 110
        palette[7] = static_cast<uint8_t>( (1 * p0 + 6 * p1 + 3) / 7 ); // bit code 111
    }
    else
    {
        palette[2] = static_cast<uint8_t>( (4 * p0 + 1 * p1 + 2) / 5 ); // bit code 010
        palette[3] = static_cast<uint8_t>( (3 * p0 + 2 * p1 + 2) / 5 ); // bit code 011
        palette[4] = static_cast<uint8_t>( (2 * p0 + 3 * p1 + 2) / 5 ); // bit code 100
        palette[5] = static_cast<uint8_t>( (1 * p0 + 4 * p1 + 2) / 5 ); // bit code 101
        palette[6] = 0;                                                 // bit code 110
        palette[7] = 255;                                               // bit code 111
    }
    
    compressedBlock += 2;
    for ( int chunk = 0; chunk < 2; ++chunk )
    {
        uint32_t alpha8 = *reinterpret_cast<const uint32_t*>( compressedBlock + 3*chunk );
        for ( int i = 0; i < 8; ++i )
        {
            uint32_t index = (alpha8 >> (3 * i)) & 0x7;
            decompressedBlock[stride * (8*chunk + i)] = palette[index];
        }
    }
}


static void Decompress_BC4_Block_SNorm( const int8_t* compressedBlock, int8_t* decompressedBlock, int stride = 1 )
{
    int8_t palette[8];
    palette[0] = *compressedBlock;
    palette[1] = *(compressedBlock + 1);
    int8_t p0 = palette[0];
    int8_t p1 = palette[1];
    if ( p0 > p1 )
    {
        // the +3 is for rounding, when doing integer division / 7. Similar to BC1
        palette[2] = static_cast< int8_t >( (6 * p0 + 1 * p1 + 3) / 7 ); // bit code 010
        palette[3] = static_cast< int8_t >( (5 * p0 + 2 * p1 + 3) / 7 ); // bit code 011
        palette[4] = static_cast< int8_t >( (4 * p0 + 3 * p1 + 3) / 7 ); // bit code 100
        palette[5] = static_cast< int8_t >( (3 * p0 + 4 * p1 + 3) / 7 ); // bit code 101
        palette[6] = static_cast< int8_t >( (2 * p0 + 5 * p1 + 3) / 7 ); // bit code 110
        palette[7] = static_cast< int8_t >( (1 * p0 + 6 * p1 + 3) / 7 ); // bit code 111
    }
    else
    {
        palette[2] = static_cast< int8_t >( (4 * p0 + 1 * p1 + 2) / 5 ); // bit code 010
        palette[3] = static_cast< int8_t >( (3 * p0 + 2 * p1 + 2) / 5 ); // bit code 011
        palette[4] = static_cast< int8_t >( (2 * p0 + 3 * p1 + 2) / 5 ); // bit code 100
        palette[5] = static_cast< int8_t >( (1 * p0 + 4 * p1 + 2) / 5 ); // bit code 101
        palette[6] = -128;                                               // bit code 110
        palette[7] = 127;                                                // bit code 111
    }
    
    compressedBlock += 2;
    for ( int chunk = 0; chunk < 2; ++chunk )
    {
        uint32_t alpha8 = *reinterpret_cast<const uint32_t*>( compressedBlock + 3*chunk );
        for ( int i = 0; i < 8; ++i )
        {
            uint32_t index = (alpha8 >> (3 * i)) & 0x7;
            decompressedBlock[stride * (8*chunk + i)] = palette[index];
        }
    }
}


static void Decompress_BC3_Block( const uint8_t* compressedBlock, uint8_t decompressedBlock[64] )
{
    // TODO: do bc2 + bc3 ignore the endpoint ordering in bc1 for punchthrough alpha?
    uint8_t alphaDecompressed[16];
    Decompress_BC4_Block_UNorm( compressedBlock, alphaDecompressed );
    Decompress_BC1_Block( compressedBlock + 8, decompressedBlock );
    for ( int i = 0; i < 16; ++i )
    {
        decompressedBlock[4*i + 3] = alphaDecompressed[i];
    }
}


static void Decompress_BC5_Block_UNorm( const uint8_t* compressedBlock, uint8_t decompressedBlock[32] )
{
    Decompress_BC4_Block_UNorm( compressedBlock, decompressedBlock, 2 );
    Decompress_BC4_Block_UNorm( compressedBlock + 8, decompressedBlock + 1, 2 );
}


static void Decompress_BC5_Block_SNorm( const int8_t* compressedBlock, int8_t decompressedBlock[32] )
{
    Decompress_BC4_Block_SNorm( compressedBlock, decompressedBlock, 2 );
    Decompress_BC4_Block_SNorm( compressedBlock + 8, decompressedBlock + 1, 2 );
}


template <typename T = uint32_t>
T ExtractBits( const uint8_t* base, uint32_t& startIndex, int bitsToExtract )
{
    uint32_t result = 0;
    for ( int i = 0; i < bitsToExtract; ++i )
    {
        int element = (startIndex + i) / 8;
        int bitIndex = (startIndex + i) - 8 * element;
        uint32_t bit = ((base[element] >> bitIndex) & 0x1);
        result |= (bit << i);
    }

    startIndex += bitsToExtract;
    return static_cast<T>( result );
}


// like ExtractBits, except no updating of the startIndex
template <typename T = uint32_t>
T ExtractBitSegment( const uint8_t* base, uint32_t startIndex, int bitsToExtract )
{
    uint32_t result = 0;
    for ( int i = 0; i < bitsToExtract; ++i )
    {
        int element = (startIndex + i) / 8;
        int bitIndex = (startIndex + i) - 8 * element;
        uint32_t bit = ((base[element] >> bitIndex) & 0x1);
        result |= (bit << i);
    }

    return static_cast<T>( result );
}


struct BC6ModeInfo
{
    uint8_t modeIndex;
    uint8_t modeBits;
    uint8_t regionCount;
    uint8_t transformedEndpoints;
    uint8_t partitionBits;
    uint8_t endpointBits;
    uint8_t deltaBits[3];
};

BC6ModeInfo s_bc6ModeTable[] =
{
    { 1,  0b00,      2, 1,  5,  10,  {5, 5, 5}     },
    { 2,  0b01,      2, 1,  5,  7,   {6, 6, 6}     },
    { 3,  0b00010,   2, 1,  5,  11,  {5, 4, 4}     },
    { 4,  0b00110,   2, 1,  5,  11,  {4, 5, 4}     },
    { 5,  0b01010,   2, 1,  5,  11,  {4, 4, 5}     },
    { 6,  0b01110,   2, 1,  5,  9,   {5, 5, 5}     },
    { 7,  0b10010,   2, 1,  5,  8,   {6, 5, 5}     },
    { 8,  0b10110,   2, 1,  5,  8,   {5, 6, 5}     },
    { 9,  0b11010,   2, 1,  5,  8,   {5, 5, 6}     },
    { 10, 0b11110,   2, 0,  5,  6,   {6, 6, 6}     },
    { 11, 0b00011,   1, 0,  0,  10,  {10, 10, 10}  },
    { 12, 0b00111,   1, 1,  0,  11,  {9, 9, 9}     },
    { 13, 0b01011,   1, 1,  0,  12,  {8, 8, 8}     },
    { 14, 0b01111,   1, 1,  0,  16,  {4, 4, 4}     }
};


static bool BC6_GetModeInfo( uint32_t modeBits, BC6ModeInfo& info )
{
    for ( int i = 0; i < ARRAY_COUNT( s_bc6ModeTable ); ++i )
    {
        if ( modeBits == s_bc6ModeTable[i].modeBits )
        {
            info = s_bc6ModeTable[i];
            return true;
        }
    }

    return false;
}


// https://learn.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format#bc6h-compressed-endpoint-format
// Microsoft naming to my naming:
// rw/gw/bw (endpt[0].A[0-2]) = endpoints[0][0-2]
// rx/gx/bx (endpt[0].B[0-2]) = endpoints[1][0-2]
// ry/gy/by (endpt[1].A[0-2]) = endpoints[2][0-2]
// rz/gz/bz (endpt[1].B[0-2]) = endpoints[3][0-2]
static void BC6_ExtractEndpoints( uint32_t modeBits, const uint8_t* compressedBlock, int packedEndpoints[4][3] )
{
    switch ( modeBits )
    {
    // 2 line segment modes
    case 0:
    {
        // 10:5:5:5
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  10 );
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 10 );
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 10 );
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 5 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 5 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 5 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 5 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 ) | (ExtractBitSegment( compressedBlock, 2, 1 ) << 4);
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 ) | (ExtractBitSegment( compressedBlock, 3, 1 ) << 4);
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 5 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 ) | (ExtractBitSegment( compressedBlock, 40, 1 ) << 4);
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 50, 1 ) | (ExtractBitSegment( compressedBlock, 60, 1 ) << 1) |
            (ExtractBitSegment( compressedBlock, 70, 1 ) << 2) | (ExtractBitSegment( compressedBlock, 76, 1 ) << 3) | (ExtractBitSegment( compressedBlock, 4, 1 ) << 4);
        break;
    }
    case 1:
    {
        // 7:6:6:6
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  7 );
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 7 );
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 7 );
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 6 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 6 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 6 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 6 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 ) | (ExtractBitSegment( compressedBlock, 24, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 2, 1 ) << 5);
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 ) | (ExtractBitSegment( compressedBlock, 14, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 22, 1 ) << 5);
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 6 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 ) | (ExtractBitSegment( compressedBlock, 3, 2 ) << 4);
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 12, 2 ) | (ExtractBitSegment( compressedBlock, 23, 1 ) << 2) |
            (ExtractBitSegment( compressedBlock, 32, 1 ) << 3) | (ExtractBitSegment( compressedBlock, 34, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 33, 1 ) << 5);
        break;
    }
    case 2:
    {
        // 11:5:4:4
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  10 ) | (ExtractBitSegment( compressedBlock, 40, 1 ) << 10);
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 10 ) | (ExtractBitSegment( compressedBlock, 49, 1 ) << 10);
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 10 ) | (ExtractBitSegment( compressedBlock, 59, 1 ) << 10);
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 5 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 4 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 4 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 5 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 );
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 );
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 5 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 );
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 50, 1 ) | (ExtractBitSegment( compressedBlock, 60, 1 ) << 1) |
            (ExtractBitSegment( compressedBlock, 70, 1 ) << 2) | (ExtractBitSegment( compressedBlock, 76, 1 ) << 3);
        break;
    }
    case 6:
    {
        // 11:4:5:4
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  10 ) | (ExtractBitSegment( compressedBlock, 39, 1 ) << 10);
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 10 ) | (ExtractBitSegment( compressedBlock, 50, 1 ) << 10);
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 10 ) | (ExtractBitSegment( compressedBlock, 59, 1 ) << 10);
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 4 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 5 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 4 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 4 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 ) | (ExtractBitSegment( compressedBlock, 75, 1 ) << 4);
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 );
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 4 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 ) | (ExtractBitSegment( compressedBlock, 40, 1 ) << 4);
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 69, 1 ) | (ExtractBitSegment( compressedBlock, 60, 1 ) << 1) |
            (ExtractBitSegment( compressedBlock, 70, 1 ) << 2) | (ExtractBitSegment( compressedBlock, 76, 1 ) << 3);
        break;
    }

    case 10:
    {
        // 11:4:4:5
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  10 ) | (ExtractBitSegment( compressedBlock, 39, 1 ) << 10);
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 10 ) | (ExtractBitSegment( compressedBlock, 49, 1 ) << 10);
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 10 ) | (ExtractBitSegment( compressedBlock, 60, 1 ) << 10);
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 4 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 4 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 5 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 4 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 );
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 ) | (ExtractBitSegment( compressedBlock, 40, 1 ) << 4);
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 4 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 );
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 50, 1 ) | (ExtractBitSegment( compressedBlock, 69, 1 ) << 1) |
            (ExtractBitSegment( compressedBlock, 70, 1 ) << 2) | (ExtractBitSegment( compressedBlock, 76, 1 ) << 3) |
            (ExtractBitSegment( compressedBlock, 75, 1 ) << 4);
        break;
    }
    case 14:
    {
        // 9:5:5:5
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  9 );
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 9 );
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 9 );
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 5 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 5 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 5 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 5 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 ) | (ExtractBitSegment( compressedBlock, 24, 1 ) << 4);
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 ) | (ExtractBitSegment( compressedBlock, 14, 1 ) << 4);
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 5 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 ) | (ExtractBitSegment( compressedBlock, 40, 1 ) << 4);
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 50, 1 ) | (ExtractBitSegment( compressedBlock, 60, 1 ) << 1) |
            (ExtractBitSegment( compressedBlock, 70, 1 ) << 2) | (ExtractBitSegment( compressedBlock, 76, 1 ) << 3) | (ExtractBitSegment( compressedBlock, 34, 1 ) << 4);
        break;
    }
    case 18:
    {
        // 8:6:5:5
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  8 );
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 8 );
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 8 );
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 6 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 5 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 5 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 6 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 ) | (ExtractBitSegment( compressedBlock, 24, 1 ) << 4);
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 ) | (ExtractBitSegment( compressedBlock, 14, 1 ) << 4);
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 6 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 ) | (ExtractBitSegment( compressedBlock, 13, 1 ) << 4);
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 50, 1 ) | (ExtractBitSegment( compressedBlock, 60, 1 ) << 1) |
            (ExtractBitSegment( compressedBlock, 23, 1 ) << 2) | (ExtractBitSegment( compressedBlock, 33, 2 ) << 3);
        break;
    }
    case 22:
    {
        // 8:5:6:5
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  8 );
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 8 );
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 8 );
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 5 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 6 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 5 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 5 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 ) | (ExtractBitSegment( compressedBlock, 24, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 23, 1 ) << 5);
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 ) | (ExtractBitSegment( compressedBlock, 14, 1 ) << 4);
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 5 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 ) | (ExtractBitSegment( compressedBlock, 40, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 33, 1 ) << 5);
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 13, 1 ) | (ExtractBitSegment( compressedBlock, 60, 1 ) << 1) |
            (ExtractBitSegment( compressedBlock, 70, 1 ) << 2) | (ExtractBitSegment( compressedBlock, 76, 1 ) << 3) | (ExtractBitSegment( compressedBlock, 34, 1 ) << 4);
        break;
    }
    case 26:
    {
        // 8:5:5:6
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  8 );
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 8 );
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 8 );
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 5 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 5 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 6 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 5 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 ) | (ExtractBitSegment( compressedBlock, 24, 1 ) << 4);
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 ) | (ExtractBitSegment( compressedBlock, 14, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 23, 1 ) << 5);
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 5 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 ) | (ExtractBitSegment( compressedBlock, 40, 1 ) << 4);
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 50, 1 ) | (ExtractBitSegment( compressedBlock, 13, 1 ) << 1) | (ExtractBitSegment( compressedBlock, 70, 1 ) << 2) |
            (ExtractBitSegment( compressedBlock, 76, 1 ) << 3) | (ExtractBitSegment( compressedBlock, 34, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 33, 1 ) << 5);
        break;
    }
    case 30:
    {
        // 6:6:6:6
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  6 );
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 6 );
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 6 );
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 6 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 6 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 6 );
        packedEndpoints[2][0] = ExtractBitSegment( compressedBlock, 65, 6 );
        packedEndpoints[2][1] = ExtractBitSegment( compressedBlock, 41, 4 ) | (ExtractBitSegment( compressedBlock, 24, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 21, 1 ) << 5);
        packedEndpoints[2][2] = ExtractBitSegment( compressedBlock, 61, 4 ) | (ExtractBitSegment( compressedBlock, 14, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 22, 1 ) << 5);
        packedEndpoints[3][0] = ExtractBitSegment( compressedBlock, 71, 6 );
        packedEndpoints[3][1] = ExtractBitSegment( compressedBlock, 51, 4 ) | (ExtractBitSegment( compressedBlock, 11, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 31, 1 ) << 5);
        packedEndpoints[3][2] = ExtractBitSegment( compressedBlock, 12, 2 ) | (ExtractBitSegment( compressedBlock, 23, 1 ) << 2) |
            (ExtractBitSegment( compressedBlock, 32, 2 ) << 3) | (ExtractBitSegment( compressedBlock, 34, 1 ) << 4) | (ExtractBitSegment( compressedBlock, 33, 1 ) << 5);
        break;
    }
    // 1 line segment modes
    case 3:
    {
        // 10:10
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  10 );
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 10 );
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 10 );
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 10 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 10 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 10 );
        break;
    }
    case 7:
    {
        // 11:9
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  10 )  | (ExtractBitSegment( compressedBlock, 44, 1 ) << 10);
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 10 ) | (ExtractBitSegment( compressedBlock, 54, 1 ) << 10);
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 10 ) | (ExtractBitSegment( compressedBlock, 64, 1 ) << 10);
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 9 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 9 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 9 );
        break;
    }
    case 11:
    {
        // 12:8
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  10 )  | (ExtractBitSegment( compressedBlock, 43, 1 ) << 10);
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 10 ) | (ExtractBitSegment( compressedBlock, 53, 1 ) << 10);
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 10 ) | (ExtractBitSegment( compressedBlock, 63, 1 ) << 10);
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 8 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 8 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 8 );
        break;
    }
    case 15:
    {
        // 16:4
        packedEndpoints[0][0] = ExtractBitSegment( compressedBlock, 5,  10 ) | (ExtractBitSegment( compressedBlock, 39, 6 ) << 10);
        packedEndpoints[0][1] = ExtractBitSegment( compressedBlock, 15, 10 ) | (ExtractBitSegment( compressedBlock, 49, 6 ) << 10);
        packedEndpoints[0][2] = ExtractBitSegment( compressedBlock, 25, 10 ) | (ExtractBitSegment( compressedBlock, 59, 6 ) << 10);
        packedEndpoints[1][0] = ExtractBitSegment( compressedBlock, 35, 4 );
        packedEndpoints[1][1] = ExtractBitSegment( compressedBlock, 45, 4 );
        packedEndpoints[1][2] = ExtractBitSegment( compressedBlock, 55, 4 );
        break;
    }
    default:
        break;
    };
}


static void BC6_ExtractIndices( const uint8_t* compressedBlock, const uint32_t shapeIndex, const int regionCount, uint8_t indices[16] )
{
    if ( regionCount == 1 )
    {
        uint32_t startbit = 65;
        indices[0] = ExtractBits<uint8_t>( compressedBlock, startbit, 3 );
        for ( int i = 1; i < 16; ++i )
        {
            indices[i] = ExtractBits<uint8_t>( compressedBlock, startbit, 4 );
        }
    }
    else
    {
        uint32_t startbit = 82;
        indices[0] = ExtractBits<uint8_t>( compressedBlock, startbit, 2 );
        for ( int i = 1; i < 16; ++i )
        {
            uint8_t nBits = anchorTable_2SubsetPartitioning_subset2[shapeIndex] == i ? 2 : 3;
            indices[i] = ExtractBits<uint8_t>( compressedBlock, startbit, nBits );
        }
    }
}


#define NCHANNELS 3
#define MASK( n )               ( (1 << (n)) - 1 )
#define HAS_BIT( x, bit )       ( signed( x ) & (1 << (bit)) )
#define SIGN_EXTEND( w, tbits ) ( (HAS_BIT( w, tbits - 1 ) ? (~0 << (tbits)) : 0) | signed( w ) )

static void BC6_SignExtendTwoRegion( const BC6ModeInfo &info, const bool isSignedF16, int endpoints[4][3] )
{
    for ( int i = 0; i < NCHANNELS; ++i )
    {
        if ( isSignedF16 )
            endpoints[0][i] = SIGN_EXTEND( endpoints[0][i], info.endpointBits );

        if ( info.transformedEndpoints || isSignedF16 )
        {
            endpoints[1][i] = SIGN_EXTEND( endpoints[1][i], info.deltaBits[i] );
            endpoints[2][i] = SIGN_EXTEND( endpoints[2][i], info.deltaBits[i] );
            endpoints[3][i] = SIGN_EXTEND( endpoints[3][i], info.deltaBits[i] );
        }
    }
}


static void BC6_SignExtendOneRegion( const BC6ModeInfo &info, const bool isSignedF16, int endpoints[4][3] )
{
    for ( int i = 0; i < NCHANNELS; ++i )
    {
        if ( isSignedF16 )
            endpoints[0][i] = SIGN_EXTEND( endpoints[0][i], info.endpointBits );
        if ( info.transformedEndpoints || isSignedF16 )
            endpoints[1][i] = SIGN_EXTEND( endpoints[1][i], info.deltaBits[i] );
    }
}


static void BC6_TransformEndpointsTwoRegion( const BC6ModeInfo &info, const bool isSignedF16, int endpoints[4][3] )
{
    int t;
    if ( info.transformedEndpoints )
    {
        for ( int i = 0; i < NCHANNELS; ++i )
        {
            int anchor = endpoints[0][i];

            t = (anchor + endpoints[1][i]) & MASK( info.endpointBits );
            endpoints[1][i] = isSignedF16 ? SIGN_EXTEND( t, info.endpointBits ) : t;
            t = (anchor + endpoints[2][i]) & MASK( info.endpointBits );
            endpoints[2][i] = isSignedF16 ? SIGN_EXTEND( t, info.endpointBits ) : t;
            t = (anchor + endpoints[3][i]) & MASK( info.endpointBits );
            endpoints[3][i] = isSignedF16 ? SIGN_EXTEND( t, info.endpointBits ) : t;
        }
    }
}


static void BC6_TransformEndpointsOneRegion( const BC6ModeInfo &info, const bool isSignedF16, int endpoints[4][3] )
{
    if ( info.transformedEndpoints )
    {
        for ( int i = 0; i < NCHANNELS; ++i )
        {
            int anchor = endpoints[0][i];
            int t = (anchor + endpoints[1][i]) & MASK( info.endpointBits );
            endpoints[1][i] = isSignedF16 ? SIGN_EXTEND( t, info.endpointBits ) : t;
        }
    }
}


static int BC6_Unquantize( int comp, int uBitsPerComp, bool isSignedF16 )
{
    int unq, s = 0;
    if ( !isSignedF16 )
    {
        if ( uBitsPerComp >= 15 )
            unq = comp;
        else if( comp == 0 )
            unq = 0;
        else if ( comp == ((1 << uBitsPerComp) - 1) )
            unq = 0xFFFF;
        else
            unq = ((comp << 16) + 0x8000) >> uBitsPerComp;
    }
    else
    {
        if ( uBitsPerComp >= 16 )
            unq = comp;
        else
        {
            if ( comp < 0 )
            {
                s = 1;
                comp = -comp;
            }

            if ( comp == 0 )
                unq = 0;
            else if ( comp >= ((1 << (uBitsPerComp - 1)) - 1) )
                unq = 0x7FFF;
            else
                unq = ((comp << 15) + 0x4000) >> (uBitsPerComp-1);

            if ( s )
                unq = -unq;
        }
    }

    return unq;
}


static uint16_t BC6_FinishUnquantize( int comp, bool isSignedF16 )
{
    if ( !isSignedF16 )
    {
        comp = (comp * 31) >> 6; // scale the magnitude by 31/64
        return (uint16_t)comp;
    }
    else
    {
        comp = (comp < 0) ? -(((-comp) * 31) >> 5) : (comp * 31) >> 5; // scale the magnitude by 31/32
        int s = 0;
        if ( comp < 0 )
        {
            s = 0x8000;
            comp = -comp;
        }
        return (uint16_t)(s | comp);
    }
}


static void BC6_GeneratePaletteUnquantized( const BC6ModeInfo &info, uint8_t regionIdx, bool isSignedF16, const int endpoints[4][3], uint16_t palette[16][3] )
{
    const uint8_t uNumIndices = info.regionCount == 2 ? 8 : 16;
    int* aWeights;
    if ( uNumIndices == 8 )
        aWeights = (int*)indexInterpolationWeights_3Bit;
    else // uNumIndices == 16
        aWeights = (int*)indexInterpolationWeights_4Bit;

    for ( int chan = 0; chan < 3; ++chan )
    {
        int a = BC6_Unquantize( endpoints[2 * regionIdx + 0][chan], info.endpointBits, isSignedF16 ); 
        int b = BC6_Unquantize( endpoints[2 * regionIdx + 1][chan], info.endpointBits, isSignedF16 );

        uint8_t baseIdx = 8 * regionIdx;
        for ( uint8_t idx = 0; idx < uNumIndices; ++idx )
        {
            //palette[baseIdx + idx][chan] = BC6_FinishUnquantize( (a * (64 - aWeights[idx]) + b * aWeights[idx] ) >> 6, isSignedF16 );
            palette[baseIdx + idx][chan] = BC6_FinishUnquantize( (a * (64 - aWeights[idx]) + b * aWeights[idx] + 32) >> 6, isSignedF16 );
        }
    }
}


static void Decompress_BC6_Block( const uint8_t* compressedBlock, uint16_t decompressedBlock[48], bool isSignedF16 )
{
    uint32_t numModeBits = (*compressedBlock & 0x3) < 2 ? 2 : 5;
    uint32_t currentBitOffset = 0;
    uint32_t modeBits = ExtractBits( compressedBlock, currentBitOffset, numModeBits );

    BC6ModeInfo modeInfo;
    if ( !BC6_GetModeInfo( modeBits, modeInfo ) )
    {
        memset( decompressedBlock, 0, 48 * sizeof( uint16_t ) );
        return;
    }
    
    int endpoints[4][3];
    BC6_ExtractEndpoints( modeInfo.modeBits, compressedBlock, endpoints );

    uint32_t shapeIndex = 0; // partition, same thing
    if ( modeInfo.modeIndex <= 10 )
    {
        shapeIndex = ExtractBitSegment( compressedBlock, 77, 5 );
    }

    uint8_t indices[16];
    BC6_ExtractIndices( compressedBlock, shapeIndex, modeInfo.regionCount, indices );

    if ( modeInfo.regionCount == 2 )
    {
        BC6_SignExtendTwoRegion( modeInfo, isSignedF16, endpoints );
        BC6_TransformEndpointsTwoRegion( modeInfo, isSignedF16, endpoints );
    }
    else
    {
        BC6_SignExtendOneRegion( modeInfo, isSignedF16, endpoints );
        BC6_TransformEndpointsOneRegion( modeInfo, isSignedF16, endpoints );
    }

    uint16_t palette[16][3];
    for ( uint8_t regionIdx = 0; regionIdx < modeInfo.regionCount; ++regionIdx )
    {
        BC6_GeneratePaletteUnquantized( modeInfo, regionIdx, isSignedF16, endpoints, palette );
    }

    for ( int i = 0; i < 16; ++i )
    {
        int region = modeInfo.regionCount == 1 ? 0 : partitionTable_2Subsets[shapeIndex][i];
        int paletteIndex = 8 * region + indices[i];
        for ( int chan = 0; chan < 3; ++chan )
        {
            decompressedBlock[3 * i + chan] = palette[paletteIndex][chan];
        }
    }
}


// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
struct BC7ModeBitCounts
{
    uint8_t numSubsets;
    uint8_t partitionBits;
    uint8_t rotationBits;
    uint8_t indexSelectionBits;
    uint8_t colorBits;
    uint8_t alphaBits;
    uint8_t separatePBits;
    uint8_t sharedPBits;
    uint8_t indexBitsPerElement;
    uint8_t secondaryIndexBitsPerElement;
};

BC7ModeBitCounts s_bc7BitTable[8] =
{
//  numSubsets partitionBits rotationBits  indexSelectionBits colorBits alphaBits separatePBits sharedPBits indexBitsPerElement secondaryIndexBitsPerElement
    { 3,            4,            0,               0,             4,        0,         1,           0,              3,                  0 }, // mode 0
    { 2,            6,            0,               0,             6,        0,         0,           1,              3,                  0 }, // mode 1
    { 3,            6,            0,               0,             5,        0,         0,           0,              2,                  0 }, // mode 2
    { 2,            6,            0,               0,             7,        0,         1,           0,              2,                  0 }, // mode 3
    { 1,            0,            2,               1,             5,        6,         0,           0,              2,                  3 }, // mode 4
    { 1,            0,            2,               0,             7,        8,         0,           0,              2,                  2 }, // mode 5
    { 1,            0,            0,               0,             7,        7,         1,           0,              4,                  0 }, // mode 6
    { 2,            6,            0,               0,             5,        5,         1,           0,              2,                  0 }, // mode 7
};


static void BC7_ExtractEndpoints( uint32_t mode, uint8_t endPoints[3][2][4], const uint8_t* compressedBlock, uint32_t& currentBitOffset )
{
    const BC7ModeBitCounts& info = s_bc7BitTable[mode];
    uint32_t numColorBits = info.colorBits;
    uint32_t numAlphaBits = info.alphaBits;

    // parse color data
    for ( int channel = 0; channel < 3; ++channel )
    {
        for ( int subset = 0; subset < info.numSubsets; ++subset )
        {
            endPoints[subset][0][channel] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, info.colorBits );
            endPoints[subset][1][channel] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, info.colorBits );
        }
    }

    // alpha data (only modes 4,5,6,7 have any)
    if ( mode >= 4 )
    {
        for ( int subset = 0; subset < info.numSubsets; ++subset )
        {
            endPoints[subset][0][3] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, info.alphaBits );
            endPoints[subset][1][3] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, info.alphaBits );
        }
    }
}


static void BC7_FullyDecodeEndpoints( uint32_t mode, uint8_t endPoints[3][2][4], const uint8_t* compressedBlock, uint32_t& currentBitOffset )
{
    const BC7ModeBitCounts& info = s_bc7BitTable[mode];
    uint32_t colorPrecisionBits = info.colorBits;
    uint32_t alphaPrecisionBits = info.alphaBits;
    if ( info.separatePBits || info.sharedPBits ) // modes 0,1,3,6,7
    {
        // if there are pbits, precision increases by one
        colorPrecisionBits += 1;
        if ( mode >= 4 )
        {
            alphaPrecisionBits += 1;
        }
        for ( int subset = 0; subset < info.numSubsets; ++subset )
        {
            uint8_t pBit0 = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, 1 );
            uint8_t pBit1 = pBit0; // sharedPBit case (mode 1)
            if ( info.separatePBits )
            {
                pBit1 = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, 1 );
            }
            for ( int channel = 0; channel < 4; ++channel )
            {
                endPoints[subset][0][channel] <<= 1;
                endPoints[subset][1][channel] <<= 1;
                endPoints[subset][0][channel] |= pBit0;
                endPoints[subset][1][channel] |= pBit1;
            }
        }
    }
    
    for ( int subset = 0; subset < info.numSubsets; ++subset )
    {
        for ( int channel = 0; channel < 3; ++channel )
        {
            endPoints[subset][0][channel] <<= (8 - colorPrecisionBits);
            endPoints[subset][1][channel] <<= (8 - colorPrecisionBits);

            // replicate MSB onto lower bits, like BC1 does with Color565To888
            endPoints[subset][0][channel] |= (endPoints[subset][0][channel] >> colorPrecisionBits);
            endPoints[subset][1][channel] |= (endPoints[subset][1][channel] >> colorPrecisionBits);
        }
        endPoints[subset][0][3] <<= (8 - alphaPrecisionBits);
        endPoints[subset][1][3] <<= (8 - alphaPrecisionBits);
        endPoints[subset][0][3] |= (endPoints[subset][0][3] >> alphaPrecisionBits);
        endPoints[subset][1][3] |= (endPoints[subset][1][3] >> alphaPrecisionBits);
    }

    if ( mode < 4 )
    {
        endPoints[0][0][3] = 255;
        endPoints[0][1][3] = 255;
        endPoints[1][0][3] = 255;
        endPoints[1][1][3] = 255;
        endPoints[2][0][3] = 255;
        endPoints[2][1][3] = 255;
    }
}


// BC7 always does 6 bit interpolation
static uint32_t BC7_Interpolate( uint32_t e0, uint32_t e1, uint8_t bitsPerIndex, int index )
{
    uint32_t lerpWeight;
    if ( bitsPerIndex == 2 )      lerpWeight = indexInterpolationWeights_2Bit[index];
    else if ( bitsPerIndex == 3 ) lerpWeight = indexInterpolationWeights_3Bit[index];
    else                          lerpWeight = indexInterpolationWeights_4Bit[index];

    // +32 is for rounding in integer division by 64 (>> 6)
    return ((64-lerpWeight) * e0 + lerpWeight * e1 + 32) >> 6;
}


static void Decompress_BC7_Block( const uint8_t* compressedBlock, uint8_t decompressedBlock[64] )
{
    uint32_t mode = ctz( *compressedBlock );
    if ( mode > 7 )
    {
        memset( decompressedBlock, 0, 64 );
        return;
    }

    const BC7ModeBitCounts& info = s_bc7BitTable[mode];
    uint32_t numSubsets = info.numSubsets;
    uint32_t currentBitOffset = mode + 1;

    uint32_t partition = ExtractBits( compressedBlock, currentBitOffset, info.partitionBits );
    uint32_t rotation  = ExtractBits( compressedBlock, currentBitOffset, info.rotationBits );
    uint32_t indexSelection = ExtractBits( compressedBlock, currentBitOffset, info.indexSelectionBits );

    uint8_t endpoints[3][2][4] = { 0 }; // at most 3 subsets and 4 color channels. Always 2 endpoints
    BC7_ExtractEndpoints( mode, endpoints, compressedBlock, currentBitOffset );
    BC7_FullyDecodeEndpoints( mode, endpoints, compressedBlock, currentBitOffset );

    // figure out which subset each pixel belongs to
    uint8_t pixelSubsetIndices[16];
    for ( int i = 0; i < 16; ++i )
    {
        if ( numSubsets == 1 )
        {
            pixelSubsetIndices[i] = 0;
        }
        else if ( numSubsets == 2 )
        {
            pixelSubsetIndices[i] = partitionTable_2Subsets[partition][i];
        }
        else
        {
            pixelSubsetIndices[i] = partitionTable_3Subsets[partition][i];
        }
    }

    // figure out which index (0-15) is the anchor index, per subset
    uint8_t anchorIndices[3];
    for ( uint32_t subset = 0; subset < numSubsets; ++subset )
    {
        if ( subset == 0 )
        {
            anchorIndices[subset] = 0;
        }
        else if ( numSubsets == 2 )
        {
            anchorIndices[subset] = anchorTable_2SubsetPartitioning_subset2[partition];
        }
        else if ( subset == 1 )
        {
            anchorIndices[subset] = anchorTable_3SubsetPartitioning_subset2[partition];
        }
        else
        {
            anchorIndices[subset] = anchorTable_3SubsetPartitioning_subset3[partition];
        }
    }

    uint8_t colorIndices[16];
    uint8_t alphaIndices[16];
    for ( int i = 0; i < 16; ++i )
    {
        // for the anchor indices the highest bit is implied to be 0, so read one less bit
        uint8_t subset = pixelSubsetIndices[i];
        int bitsToExtract = i == anchorIndices[subset] ? info.indexBitsPerElement - 1 : info.indexBitsPerElement;
        
        // modes 4 and 5 have two sets of indices, but in mode 4 they are 2 and 3 bits. If the index selection bit == 0, then
        // the 2 bit indices are for the color, otherwise if index selection == 1, they are for the alpha
        if ( mode == 4 )
        {
            if ( indexSelection )
            {
                alphaIndices[i] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, bitsToExtract );
            }
            else
            {
                colorIndices[i] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, bitsToExtract );
            }
        }
        else
        {
            colorIndices[i] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, bitsToExtract );
            alphaIndices[i] = colorIndices[i];
        }
    }

    // modes 4 and 5 have two sets of indices
    if ( info.secondaryIndexBitsPerElement > 0 )
    {
        for ( int i = 0; i < 16; ++i )
        {
            uint8_t subset = pixelSubsetIndices[i];
            int bitsToExtract = i == anchorIndices[subset] ? info.secondaryIndexBitsPerElement - 1 : info.secondaryIndexBitsPerElement;
            if ( indexSelection )
            {
                colorIndices[i] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, bitsToExtract );
            }
            else
            {
                alphaIndices[i] = (uint8_t)ExtractBits( compressedBlock, currentBitOffset, bitsToExtract );
            }
        }
    }

    // actually interpolate the endpoints to find pixel values
    uint8_t bitsPerColorIndex = mode != 4 || !indexSelection ? info.indexBitsPerElement : info.secondaryIndexBitsPerElement;
    uint8_t bitsPerAlphaIndex = mode != 4 || indexSelection ? info.indexBitsPerElement : info.secondaryIndexBitsPerElement;
    for ( int i = 0; i < 16; ++i )
    {
        uint8_t subset = pixelSubsetIndices[i];
        uint8_t endpoint0[4] = { endpoints[subset][0][0], endpoints[subset][0][1], endpoints[subset][0][2], endpoints[subset][0][3] };
        uint8_t endpoint1[4] = { endpoints[subset][1][0], endpoints[subset][1][1], endpoints[subset][1][2], endpoints[subset][1][3] };

        decompressedBlock[4*i + 0] = (uint8_t)BC7_Interpolate( endpoint0[0], endpoint1[0], bitsPerColorIndex, colorIndices[i] );
        decompressedBlock[4*i + 1] = (uint8_t)BC7_Interpolate( endpoint0[1], endpoint1[1], bitsPerColorIndex, colorIndices[i] );
        decompressedBlock[4*i + 2] = (uint8_t)BC7_Interpolate( endpoint0[2], endpoint1[2], bitsPerColorIndex, colorIndices[i] );
        decompressedBlock[4*i + 3] = (uint8_t)BC7_Interpolate( endpoint0[3], endpoint1[3], bitsPerAlphaIndex, alphaIndices[i] );

        if ( rotation )
        {
            std::swap( decompressedBlock[4*i + 3], decompressedBlock[4*i + rotation - 1] );
        }
    }

}


template<int FORMAT>
void Decompress_BC_Internal( const RawImage2D& compressedImage, RawImage2D& outputImage )
{
    const int bytesPerOutputPixel = outputImage.BitsPerPixel() / 8;
    const int bytesPerBlock = compressedImage.BytesPerBlock();
    const int numBlocksX = compressedImage.BlocksX();
    const int numBlocksY = compressedImage.BlocksY();

    #pragma omp parallel for
    for ( int blockY = 0; blockY < numBlocksY; ++blockY )
    {
        for ( int blockX = 0; blockX < numBlocksX; ++blockX )
        {
            uint8_t decompressedBlock[96]; // 16 for bc4, 32 for bc5, 96 for bc6, 64 for everything else
            const uint8_t* compressedBlock = compressedImage.Raw<uint8_t>() + bytesPerBlock * (blockY * numBlocksX + blockX);

            if constexpr      ( FORMAT == Underlying( ImageFormat::BC1_UNORM ) ) Decompress_BC1_Block( compressedBlock, decompressedBlock );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC2_UNORM ) ) Decompress_BC2_Block( compressedBlock, decompressedBlock );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC3_UNORM ) ) Decompress_BC3_Block( compressedBlock, decompressedBlock );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC4_UNORM ) ) Decompress_BC4_Block_UNorm( compressedBlock, decompressedBlock );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC4_SNORM ) ) Decompress_BC4_Block_SNorm( (const int8_t*)compressedBlock, (int8_t*)decompressedBlock );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC5_UNORM ) ) Decompress_BC5_Block_UNorm( compressedBlock, decompressedBlock );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC5_SNORM ) ) Decompress_BC5_Block_SNorm( (const int8_t*)compressedBlock, (int8_t*)decompressedBlock );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC6H_U16F ) ) Decompress_BC6_Block( compressedBlock, (uint16_t*)decompressedBlock, false );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC6H_S16F ) ) Decompress_BC6_Block( compressedBlock, (uint16_t*)decompressedBlock, true );
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC7_UNORM ) ) Decompress_BC7_Block( compressedBlock, decompressedBlock );

            uint32_t rowsToCopy = std::min( outputImage.height - 4*blockY, 4u );
            uint32_t colsToCopy = std::min( outputImage.width -  4*blockX, 4u );
            for ( uint32_t blockRow = 0; blockRow < rowsToCopy; ++blockRow )
            {
                int dstRow = 4 * blockY + blockRow;
                int dstCol = 4 * blockX;
                int dstIndex = bytesPerOutputPixel * (dstRow * compressedImage.width + dstCol);
                int srcIndex = bytesPerOutputPixel * 4 * blockRow;
                memcpy( outputImage.Raw() + dstIndex, decompressedBlock + srcIndex, bytesPerOutputPixel * colsToCopy );
            }
        }
    }
}


RawImage2D DecompressBC( const RawImage2D& compressedImage )
{
    ImageFormat outputFormat = GetFormatAfterDecompression( compressedImage.format );
    if ( outputFormat == ImageFormat::INVALID )
    {
        return {};
    }

    RawImage2D decompressedImage( compressedImage.width, compressedImage.height, outputFormat );
    switch ( compressedImage.format )
    {
        case ImageFormat::BC1_UNORM:
            Decompress_BC_Internal<Underlying( ImageFormat::BC1_UNORM )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC2_UNORM:
            Decompress_BC_Internal<Underlying( ImageFormat::BC2_UNORM )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC3_UNORM:
            Decompress_BC_Internal<Underlying( ImageFormat::BC3_UNORM )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC4_UNORM:
            Decompress_BC_Internal<Underlying( ImageFormat::BC4_UNORM )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC4_SNORM:
            Decompress_BC_Internal<Underlying( ImageFormat::BC4_SNORM )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC5_UNORM:
            Decompress_BC_Internal<Underlying( ImageFormat::BC5_UNORM )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC5_SNORM:
            Decompress_BC_Internal<Underlying( ImageFormat::BC5_SNORM )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC6H_U16F:
            Decompress_BC_Internal<Underlying( ImageFormat::BC6H_U16F )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC6H_S16F:
            Decompress_BC_Internal<Underlying( ImageFormat::BC6H_S16F )>( compressedImage, decompressedImage );
            break;
        case ImageFormat::BC7_UNORM:
            Decompress_BC_Internal<Underlying( ImageFormat::BC7_UNORM )>( compressedImage, decompressedImage );
            break;
    }

    return decompressedImage;
}
