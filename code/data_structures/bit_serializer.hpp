#pragma once

#include "shared/core_defines.hpp"
#include <vector>

struct BitWriter
{
    using ChunkType                     = u8;
    static constexpr int BITS_PER_CHUNK = 8 * sizeof( ChunkType );

    std::vector<ChunkType> data;
    int currentChunk = 0;
    int currentBit   = 0;

    BitWriter() = default;
    BitWriter( u32 numBytes )
    {
        currentBit = currentChunk = 0;
        data.reserve( numBytes );
    }

    void Write( const void* srcData, u32 numBits )
    {
        const ChunkType* srcChunks = (const ChunkType*)srcData;
        u32 bytesToWrite           = numBits / BITS_PER_CHUNK;
        ChunkType lowerMask        = ( 1u << ( BITS_PER_CHUNK - currentBit ) ) - 1;

        for ( u32 i = 0; i < bytesToWrite; ++i )
        {
            if ( currentBit == 0 )
            {
                ChunkType& end = data.emplace_back();
                end            = srcChunks[i];
            }
            else
            {
                ChunkType byte = srcChunks[i];
                ChunkType& end = data.back();
                end |= ( byte & lowerMask ) << currentBit;

                ChunkType& next = data.emplace_back( 0 );
                next |= ( byte & ~lowerMask ) >> ( BITS_PER_CHUNK - currentBit );
            }
        }

        u32 numRemainderBits = numBits % BITS_PER_CHUNK;
        if ( numRemainderBits )
        {
            ChunkType remainderMask = ( 1u << numRemainderBits ) - 1;
            ChunkType remainingData = srcChunks[bytesToWrite] & remainderMask;
            if ( currentBit == 0 )
            {
                ChunkType& end = data.emplace_back( 0 );
                end            = remainingData;
            }
            else
            {
                ChunkType& end = data.back();
                end |= ( remainingData & lowerMask ) << currentBit;

                if ( currentBit + numRemainderBits > BITS_PER_CHUNK )
                {
                    ChunkType& next = data.emplace_back( 0 );
                    next |= ( remainingData & ~lowerMask ) >> ( BITS_PER_CHUNK - currentBit );
                }
            }

            currentBit = ( currentBit + numRemainderBits ) % BITS_PER_CHUNK;
        }
    }

    template <typename T>
    void Write( const T& srcData )
    {
        Write( &srcData, sizeof( T ) * 8 );
    }

    void BeginReading() { currentChunk = currentBit = 0; }

    void Read( void* dstData, u32 numBits )
    {
        ChunkType* dstChunks = (ChunkType*)dstData;
        u32 chunksToRead     = numBits / BITS_PER_CHUNK;
        for ( u32 i = 0; i < chunksToRead; ++i )
        {
            ChunkType chunk = data[currentChunk];
            dstChunks[i]    = chunk >> currentBit;
            ++currentChunk;

            if ( currentBit != 0 )
            {
                chunk               = data[currentChunk];
                ChunkType upperBits = chunk << ( BITS_PER_CHUNK - currentBit );
                dstChunks[i] |= upperBits;
            }
        }

        u32 numRemainderBits = numBits % BITS_PER_CHUNK;
        if ( !numRemainderBits )
            return;

        ChunkType remainderMask = ( 1u << numRemainderBits ) - 1;
        ChunkType dstChunk      = ( data[currentChunk] >> currentBit ) & remainderMask;
        if ( currentBit + numRemainderBits > BITS_PER_CHUNK )
        {
            u32 numFinalBits = numRemainderBits - ( BITS_PER_CHUNK - currentBit );
            remainderMask    = ( 1u << numFinalBits ) - 1;
            dstChunk |= ( data[currentChunk + 1] & remainderMask ) << ( BITS_PER_CHUNK - currentBit );
        }
        dstChunks[chunksToRead] = dstChunk;

        if ( currentBit + numRemainderBits >= BITS_PER_CHUNK )
            ++currentChunk;
        currentBit = ( currentBit + numRemainderBits ) % BITS_PER_CHUNK;
    }

    template <typename T>
    T Read( u32 numBits = sizeof( T ) * 8 )
    {
        T dstData;
        Read( (ChunkType*)&dstData, numBits );
        return dstData;
    }

    u32 Size() const { return static_cast<u32>( data.size() * sizeof( ChunkType ) ); }
};
