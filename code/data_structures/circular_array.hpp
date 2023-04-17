#pragma once

#include <cstddef>
#include <type_traits>

template <typename T, size_t SIZE>
class CircularArray
{
    using StorageType =
        std::conditional_t<SIZE <=  8, uint8_t,
        std::conditional_t<SIZE <= 16, uint16_t,
        std::conditional_t<SIZE <= 32, uint32_t, uint64_t>>>;

public:
    CircularArray() : currentSize( 0 ), frontIndex( 0 ), backIndex( 0 ) {}

    StorageType Size() const { return currentSize; }
    StorageType FrontIndex() const { return frontIndex; }
    StorageType BackIndex() const { return backIndex; }
    T& operator[]( size_t index ) { return data[(frontIndex + index) % SIZE]; }
    const T& operator[]( size_t index ) const { return data[(frontIndex + index) % SIZE]; }

    void Clear()
    {
        currentSize = frontIndex = backIndex = 0;
    }

    void Pushback( const T& val )
    {
        if ( backIndex == SIZE )
        {
            data[0] = val;
            if ( frontIndex == 0 )
                ++frontIndex;
            backIndex = 1;
        }
        else
        {
            data[backIndex] = val;
            ++currentSize;
            ++backIndex;
        }
    }

    T data[SIZE];

private:
    StorageType currentSize;
    StorageType frontIndex;
    StorageType backIndex;
};