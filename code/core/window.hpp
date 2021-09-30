#pragma once

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <string>

struct lua_State;

namespace PG
{

struct WindowCreateInfo
{
    std::string title = "Untitled";
    int width         = 1280;
    int height        = 720;
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

    GLFWwindow* GetGLFWHandle() const { return m_window; }
    int Width() const { return m_width; }
    int Height() const { return m_height; }
    void SetRelativeMouse( bool b );
    bool IsRelativeMouse() const { return m_relativeMouse; }
    void SetTitle( const std::string& title );

protected:
    GLFWwindow* m_window = nullptr;
    std::string m_title  = "";
    int m_width          = 0;
    int m_height         = 0;
    bool m_visible       = false;
    bool m_relativeMouse = false;
};

void RegisterLuaFunctions_Window( lua_State* state );
void InitWindowSystem( const WindowCreateInfo& info );
void ShutdownWindowSystem();
Window* GetMainWindow();

} // namespace PG