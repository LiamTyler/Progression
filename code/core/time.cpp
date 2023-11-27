#include "core/time.hpp"

using namespace std::chrono;
using Clock = high_resolution_clock;
using TimePoint = time_point< Clock >;

static TimePoint s_startTime = TimePoint();
static float s_currentFrameStartTime = 0;
static float s_lastFrameStartTime    = 0;
static float s_deltaTime             = 0;

namespace PG
{
namespace Time
{

    void Reset()
    {
        s_startTime             = Clock::now();
        s_currentFrameStartTime = 0;
        s_deltaTime             = 0;
        s_currentFrameStartTime = 0;
    }


    float Time()
    {
        return static_cast< float >( GetTimeSince( s_startTime ) );
    }


    float DeltaTime()
    {
        return s_deltaTime;
    }


    void StartFrame()
    {
        s_currentFrameStartTime = Time();
        s_deltaTime             = 0.001f * ( s_currentFrameStartTime - s_lastFrameStartTime );
    }


    void EndFrame()
    {
        s_lastFrameStartTime = s_currentFrameStartTime;
    }


    Point GetTimePoint()
    {
        return Clock::now();
    }


    double GetTimeSince( const Point& point )
    {
        auto now = Clock::now();
        return duration_cast< microseconds >( now - point ).count() / 1000.0;
    }


    double GetElapsedTime( const Point& startPoint, const Point& endPoint )
    {
        return duration_cast< microseconds >( endPoint - startPoint ).count() / 1000.0;
    }

} // namespace Time
} // namespace PG
