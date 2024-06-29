#include "core/window.hpp"
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

void InitWindowSystem( const WindowCreateInfo& info )
{
    PGP_ZONE_SCOPEDN( "InitWindowSystem" );
    if ( !glfwInit() )
    {
        LOG_ERR( "Could not initialize GLFW" );
        exit( EXIT_FAILURE );
    }

    s_mainWindow = new Window;
    s_mainWindow->Init( info );
}

void ShutdownWindowSystem()
{
    delete s_mainWindow;
    glfwTerminate();
}

Window* GetMainWindow() { return s_mainWindow; }

Window::~Window()
{
    if ( m_window )
    {
        glfwDestroyWindow( m_window );
        m_window = nullptr;
    }
}

static void WindowCloseCallback( GLFWwindow* window ) { eg.shutdown = true; }

void Window::Init( const WindowCreateInfo& createInfo )
{
    m_title   = createInfo.title;
    m_width   = createInfo.width;
    m_height  = createInfo.height;
    m_visible = createInfo.visible;

    glfwWindowHint( GLFW_VISIBLE, m_visible );
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );
    m_window = glfwCreateWindow( m_width, m_height, m_title.c_str(), nullptr, nullptr );
    if ( !m_window )
    {
        LOG_ERR( "Window or context creation failed" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    glfwSetWindowCloseCallback( m_window, WindowCloseCallback );
    glfwSetErrorCallback( ErrorCallback );

    s_framesDrawnSinceLastFPSUpdate = 0;
    s_lastFPSUpdateTime             = Time::GetTimePoint();
    glfwGetFramebufferSize( m_window, &m_framebufferWidth, &m_framebufferHeight );
}

void Window::StartFrame()
{
    Time::StartFrame();
    glfwGetWindowSize( m_window, &m_width, &m_height );
    i32 fW, fH;
    glfwGetFramebufferSize( m_window, &fW, &fH );
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
        glfwSetWindowTitle( m_window, titleWithFps.c_str() );
        s_framesDrawnSinceLastFPSUpdate = 0;
        s_lastFPSUpdateTime             = Time::GetTimePoint();
    }
    PGP_FRAME_MARK;
}

void Window::SetRelativeMouse( bool b )
{
    glfwSetInputMode( m_window, GLFW_CURSOR, b ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL );
    m_relativeMouse = b;
    Input::MouseCursorChange();
}

void Window::SetTitle( const std::string& title )
{
    m_title = title;
    glfwSetWindowTitle( m_window, m_title.c_str() );
}

} // namespace PG
