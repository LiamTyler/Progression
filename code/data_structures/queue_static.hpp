#pragma once

#include "shared/assert.hpp"

template <typename T, int N>
class StaticQueue
{
public:
    StaticQueue() : m_backIndex( 0 ), m_frontIndex( 0 ) {}

    void push( const T& item )
    {
        m_data[m_backIndex] = item;
        m_backIndex         = ( m_backIndex + 1 ) % N;
    }

    T front() const { return m_data[m_frontIndex]; }

    void pop() { m_frontIndex = ( m_frontIndex + 1 ) % N; }

    bool empty() const { return m_backIndex == m_frontIndex; }

private:
    T m_data[N];
    u32 m_backIndex;
    u32 m_frontIndex;
};
