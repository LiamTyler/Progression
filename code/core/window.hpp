#pragma once

#ifdef PG_USE_SDL
#include "SDL3/SDL.h"
#else // #ifdef PG_USE_SDL
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#endif // #else // #ifdef PG_USE_SDL
#include "shared/core_defines.hpp"
#include <string>

struct lua_State;

namespace PG
{

struct WindowCreateInfo
{
    std::string title = "Untitled";
    i32 width         = 1280;
    i32 height        = 720;
    bool visible      = true;
    bool debugContext = true;
    bool vsync        = false;
};

class Window
{
public:
    Window() = default;
    ~Window();

    void Init( const struct WindowCreateInfo& createInfo );
    void StartFrame();
    void EndFrame();

    auto GetHandle() const { return m_window; }
    i32 Width() const { return m_width; }
    i32 Height() const { return m_height; }
    i32 FramebufferWidth() const { return m_framebufferWidth; }
    i32 FramebufferHeight() const { return m_framebufferHeight; }
    void SetRelativeMouse( bool b );
    bool IsRelativeMouse() const { return m_relativeMouse; }
    void SetTitle( const std::string& title );

protected:
#ifdef PG_USE_SDL
    SDL_Window* m_window = nullptr;
#else  // #ifdef PG_USE_SDL
    GLFWwindow* m_window = nullptr;
#endif // #else // #ifdef PG_USE_SDL
    std::string m_title     = "";
    i32 m_width             = 0;
    i32 m_height            = 0;
    i32 m_framebufferWidth  = 0;
    i32 m_framebufferHeight = 0;
    bool m_visible          = false;
    bool m_relativeMouse    = false;
};

void RegisterLuaFunctions_Window( lua_State* state );
bool InitWindowSystem( const WindowCreateInfo& info );
void ShutdownWindowSystem();
Window* GetMainWindow();

} // namespace PG
