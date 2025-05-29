#include "core/window.hpp"
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
#include "core/cpu_profiling.hpp"
#include "core/engine_globals.hpp"
#include "core/input.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#include "shared/logger.hpp"
#include <unordered_set>

static std::unordered_set<size_t> s_debugMessages;
static std::chrono::high_resolution_clock::time_point s_lastFPSUpdateTime;
static u32 s_framesDrawnSinceLastFPSUpdate;
static PG::Window* s_mainWindow;

static void ErrorCallback( i32 err, const char* description ) { LOG_ERR( "GLFW ERROR %d: '%s'", err, description ); }

namespace PG
{

void RegisterLuaFunctions_Window( lua_State* state )
{
    sol::state_view lua( state );

    lua["GetMainWindow"]              = &GetMainWindow;
    sol::usertype<Window> window_type = lua.new_usertype<Window>( "Window" );
    window_type.set_function( "Width", &Window::Width );
    window_type.set_function( "Height", &Window::Height );
    window_type.set_function( "SetRelativeMouse", &Window::SetRelativeMouse );
    window_type.set_function( "IsRelativeMouse", &Window::IsRelativeMouse );
    window_type.set_function( "SetTitle", &Window::SetTitle );
}

bool InitWindowSystem( const WindowCreateInfo& info )
{
    PGP_ZONE_SCOPEDN( "InitWindowSystem" );
    if ( !SDL_Init( SDL_INIT_VIDEO ) )
    {
        LOG_ERR( "Could not initialize SDL3. Error '%s'", SDL_GetError() );
        return false;
    }

    s_mainWindow = new Window;
    s_mainWindow->Init( info );

    return true;
}

void ShutdownWindowSystem()
{
    if ( s_mainWindow )
    {
        delete s_mainWindow;
        s_mainWindow = nullptr;
    }
    SDL_Quit();
}

Window* GetMainWindow() { return s_mainWindow; }

Window::~Window()
{
    if ( m_window )
    {
        SDL_DestroyWindow( m_window );
        m_window = nullptr;
    }
}

void Window::Init( const WindowCreateInfo& createInfo )
{
    m_title   = createInfo.title;
    m_width   = createInfo.width;
    m_height  = createInfo.height;
    m_visible = createInfo.visible;

    SDL_WindowFlags windowFlags = (SDL_WindowFlags)( SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE );
    m_window                    = SDL_CreateWindow( m_title.c_str(), m_width, m_height, windowFlags );
    if ( !m_window )
    {
        LOG_ERR( "Window or context creation failed with error '%s'", SDL_GetError() );
        SDL_Quit();
        exit( EXIT_FAILURE );
    }
    if ( !SDL_GetWindowSizeInPixels( m_window, &m_framebufferWidth, &m_framebufferHeight ) )
    {
        LOG_ERR( "SDL_GetWindowSizeInPixels failed with '%s'", SDL_GetError() );
    }

    s_framesDrawnSinceLastFPSUpdate = 0;
    s_lastFPSUpdateTime             = Time::GetTimePoint();
}

void Window::StartFrame()
{
    Time::StartFrame();
    i32 fW, fH;
    SDL_GetWindowSize( m_window, &m_width, &m_height );
    SDL_GetWindowSizeInPixels( m_window, &fW, &fH );
    if ( fW != m_framebufferWidth || fH != m_framebufferHeight )
    {
        eg.resizeRequested  = true;
        m_framebufferWidth  = fW;
        m_framebufferHeight = fH;
    }
}

void Window::EndFrame()
{
    ++s_framesDrawnSinceLastFPSUpdate;
    f64 msSinceLastUpdate = Time::GetTimeSince( s_lastFPSUpdateTime );
    if ( msSinceLastUpdate > 1000.0 )
    {
        f64 msPerFrame           = msSinceLastUpdate / s_framesDrawnSinceLastFPSUpdate;
        int fps                  = (int)round( 1000.0 / msPerFrame );
        std::string titleWithFps = m_title + " -- FPS: " + std::to_string( fps );
        SDL_SetWindowTitle( m_window, titleWithFps.c_str() );
        s_framesDrawnSinceLastFPSUpdate = 0;
        s_lastFPSUpdateTime             = Time::GetTimePoint();
    }
    PGP_FRAME_MARK;
}

void Window::SetRelativeMouse( bool b )
{
    SDL_SetWindowRelativeMouseMode( m_window, b );
    m_relativeMouse = b;
    Input::MouseCursorChange();
}

void Window::SetTitle( const std::string& title )
{
    m_title = title;
    SDL_SetWindowTitle( m_window, m_title.c_str() );
}

} // namespace PG
