#pragma once

#include "shared/core_defines.hpp"
#include <chrono>

struct lua_State;

namespace PG::Time
{

using Point = std::chrono::high_resolution_clock::time_point;

void Reset();

// milliseconds since initialization
f32 Time();

// how long the last frame took, in seconds
f32 DeltaTime();

void StartFrame();

void EndFrame();

Point GetTimePoint();

// Returns the number of milliseconds elapsed since Point
f64 GetTimeSince( const Point& point );

// Returns the number of milliseconds between two points
f64 GetElapsedTime( const Point& startPoint, const Point& endPoint );

} // namespace PG::Time
