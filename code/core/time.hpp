#pragma once

#include <chrono>

struct lua_State;

namespace PG
{

namespace Time
{

    using Point = std::chrono::high_resolution_clock::time_point;

    void Reset();

    // milliseconds since initialization
    float Time();

    // how long the last frame took, in seconds
    float DeltaTime();

    void StartFrame();

    void EndFrame();

    Point GetTimePoint();

    // Returns the number of milliseconds elapsed since Point
    double GetTimeSince( const Point& point );

    // Returns the number of milliseconds between two points
    double GetElapsedTime( const Point& startPoint, const Point& endPoint );

} // namespace Time
} // namespace PG
