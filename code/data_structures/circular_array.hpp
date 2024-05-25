#pragma once

#include <cstddef>
#include <type_traits>

template <typename T, size_t SIZE>
class CircularArray
{
    using StorageType = std::conditional_t<SIZE <= 0xFF, uint8_t,
        std::conditional_t<SIZE <= 0xFFFF, uint16_t, std::conditional_t<SIZE <= ~0u, uint32_t, uint64_t>>>;

public:
    CircularArray() : m_currentSize( 0 ), m_frontIndex( 0 ), m_backIndex( 0 ) {}

    StorageType Size() const { return m_currentSize; }
    StorageType FrontIndex() const { return m_frontIndex; }
    StorageType BackIndex() const { return m_backIndex; }
    T& operator[]( size_t index ) { return data[( m_frontIndex + index ) % SIZE]; }
    const T& operator[]( size_t index ) const { return data[( m_frontIndex + index ) % SIZE]; }

    void Clear() { m_currentSize = m_frontIndex = m_backIndex = 0; }

    void Pushback( const T& val )
    {
        data[m_backIndex] = val;
        m_backIndex       = ( m_backIndex + 1 ) % SIZE;
        if ( m_backIndex == m_frontIndex )
            m_frontIndex = ( m_frontIndex + 1 ) % SIZE;
        else
            ++m_currentSize;
    }

    T data[SIZE];

private:
    StorageType m_currentSize;
    StorageType m_frontIndex;
    StorageType m_backIndex;
};
