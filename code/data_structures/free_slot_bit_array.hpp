#pragma once

#include "shared/assert.hpp"
#include "shared/bitops.hpp"
#include <bit>

#define SLOT_TO_CHUNK( x ) ( ( x ) >> 6 )
#define BIT_IN_CHUNK( x ) ( ( x ) & 63u )

template <size_t SIZE>
class FreeSlotBitArray
{
    static_assert( SIZE <= ~0u, "change return types to 64 bit" );
    constexpr static size_t NUM_CHUNKS = ( SIZE + 63 ) / 64;

public:
    FreeSlotBitArray() { Clear(); }

    void Clear()
    {
        memset( data, 0xFF, sizeof( data ) );
        firstFreeSlot = 0;
    }

    uint32_t GetFreeSlot()
    {
        uint32_t slot = firstFreeSlot;
        Unset( slot );
        AdvanceFirstFreeSlot();
        return slot;
    }

    uint32_t GetConsecutiveFreeSlots( uint32_t numSlots )
    {
        PG_ASSERT( numSlots <= 64, "Only grabbing within a single data chunk" );
        uint32_t slot     = firstFreeSlot;
        uint32_t chunkIdx = SLOT_TO_CHUNK( slot );
        do
        {
            size_t chunk        = data[chunkIdx];
            uint32_t bitInChunk = ctzll( chunk );
            while ( (uint32_t)std::popcount( chunk ) >= numSlots )
            {
                uint32_t currentFreeSlots = 1;
                for ( uint32_t i = bitInChunk + 1; i < 64 && currentFreeSlots < numSlots; ++i )
                {
                    if ( !( chunk & ( 1ull << i ) ) )
                        break;
                    ++currentFreeSlots;
                }

                if ( currentFreeSlots >= numSlots )
                {
                    data[chunkIdx] &= ~GetMaskForRange( bitInChunk, numSlots );
                    AdvanceFirstFreeSlot();
                    return chunkIdx * 64 + bitInChunk;
                }

                data[chunkIdx] &= ~GetMaskForRange( bitInChunk, currentFreeSlots );
                bitInChunk = ctzll_nonzero( chunk );
            }

            ++chunkIdx;
        } while ( chunkIdx < NUM_CHUNKS );

        PG_ASSERT( false, "No consecutive free slots available" );
        return ~0u;
    }

    void FreeSlot( uint32_t slot )
    {
        PG_ASSERT( !IsSet( slot ) );
        if ( slot < firstFreeSlot )
            firstFreeSlot = slot;
        Unset( slot );
    }

    void FreeConsecutiveSlots( uint32_t firstSlot, uint32_t numSlots )
    {
        PG_ASSERT( !IsSet( firstSlot ) );
        if ( firstSlot < firstFreeSlot )
            firstFreeSlot = firstSlot;

        uint32_t bitInChunk = BIT_IN_CHUNK( firstSlot );
        uint32_t chunkIdx   = SLOT_TO_CHUNK( firstSlot );
        data[chunkIdx] |= GetMaskForRange( bitInChunk, numSlots );
    }

private:
    size_t GetMaskForRange( uint32_t firstBit, uint32_t numBits )
    {
        size_t lowMask   = firstBit == 0 ? ~0ull : ~( ( 1ull << firstBit ) - 1ull );
        uint32_t lastBit = firstBit + numBits;
        size_t highMask  = lastBit == 64 ? ~0ull : ( ( 1ull << ( firstBit + numBits ) ) - 1ull );
        return lowMask & highMask;
    }

    void Set( uint32_t slot )
    {
        uint32_t chunkIdx = SLOT_TO_CHUNK( slot );
        data[chunkIdx] |= ( 1ull << BIT_IN_CHUNK( slot ) );
    }

    void Unset( uint32_t slot )
    {
        uint32_t chunkIdx = SLOT_TO_CHUNK( slot );
        data[chunkIdx] &= ~( 1ull << BIT_IN_CHUNK( slot ) );
    }

    bool IsSet( uint32_t slot ) const
    {
        uint32_t chunkIdx = SLOT_TO_CHUNK( slot );
        return data[chunkIdx] & ( 1ull << BIT_IN_CHUNK( slot ) );
    }

    void AdvanceFirstFreeSlot()
    {
        uint32_t chunkIdx = SLOT_TO_CHUNK( firstFreeSlot );
        while ( chunkIdx < NUM_CHUNKS )
        {
            if ( !data[chunkIdx] )
            {
                ++chunkIdx;
                continue;
            }

            firstFreeSlot = chunkIdx * 64 + ctzll_nonzero( data[chunkIdx] );
            return;
        }

        PG_ASSERT( false, "No free slots remaining!" );
    }

    uint32_t firstFreeSlot;
    size_t data[NUM_CHUNKS];
};

#undef SLOT_TO_CHUNK
#undef BIT_IN_CHUNK
