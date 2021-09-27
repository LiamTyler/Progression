#pragma once

namespace PG
{

class NonCopyable
{
public:
    NonCopyable()          = default;

    NonCopyable( const NonCopyable& ) = delete;
    NonCopyable& operator=( const NonCopyable& ) = delete;

    NonCopyable( NonCopyable&& ) = default;
    NonCopyable& operator=( NonCopyable&& ) = default;
};

} // namespace PG