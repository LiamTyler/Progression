#include "core/input.hpp"
#include "core/window.hpp"
#include "utils/logger.hpp"

static glm::vec2 s_lastCursorPos                         = glm::ivec2( 0 );
static glm::vec2 s_currentCursorPos                      = glm::ivec2( 0 );
static glm::vec2 s_scrollOffset                          = glm::ivec2( 0 );

enum KeyStatus : uint8_t
{
    KEY_RELEASED = 0, // first frame key released
    KEY_FREE     = 1, // all frames key is released besides first
    KEY_PRESSED  = 2, // first frame key pressed
    KEY_HELD     = 3, // all frames key is pressed besides first
};

static uint8_t s_keyStatus[GLFW_KEY_LAST + 1];
static uint8_t s_mouseButtonStatus[GLFW_MOUSE_BUTTON_LAST + 1];

static void KeyCallback( GLFWwindow* window, int key, int scancode, int action, int mods )
{
    if ( key == GLFW_KEY_UNKNOWN )
    {
        LOG_WARN( "Unknown key pressed" );
        return;
    }

    if ( action == GLFW_PRESS )
    {
        s_keyStatus[key] = KEY_PRESSED;
    }
    else if ( action == GLFW_RELEASE )
    {
        s_keyStatus[key] = KEY_RELEASED;
    }
}

static void CursorPositionCallback( GLFWwindow* window, double xpos, double ypos )
{
    s_currentCursorPos = glm::vec2( xpos, ypos );
}

static void MouseButtonCallback( GLFWwindow* window, int button, int action, int mods )
{
    if ( action == GLFW_PRESS )
    {
        s_mouseButtonStatus[button] = KEY_PRESSED;
    }
    else if ( action == GLFW_RELEASE )
    {
        s_mouseButtonStatus[button] = KEY_RELEASED;
    }
}

static void ScrollCallback( GLFWwindow* window, double xoffset, double yoffset )
{
    s_scrollOffset += glm::vec2( xoffset, yoffset );
}


namespace PG
{
namespace Input
{

    void Init()
    {
        GLFWwindow* window = GetMainWindow()->GetGLFWHandle();
        double x, y;
        glfwGetCursorPos( window, &x, &y );
        s_currentCursorPos = glm::vec2( x, y );
        s_lastCursorPos    = s_currentCursorPos;

        glfwSetCursorPosCallback( window, CursorPositionCallback );
        glfwSetMouseButtonCallback( window, MouseButtonCallback );
        glfwSetKeyCallback( window, KeyCallback );
        glfwSetScrollCallback( window, ScrollCallback );

        for ( int i = 0; i < GLFW_KEY_LAST + 1; ++i )
        {
            s_keyStatus[i] = KEY_FREE;
        }

        for ( int i = 0; i < GLFW_MOUSE_BUTTON_LAST + 1; ++i )
        {
            s_mouseButtonStatus[i] = KEY_FREE;
        }

        PollEvents();
    }

    void Shutdown()
    {
        glfwPollEvents();
    }

    void PollEvents()
    {
        for ( int i = 0; i < GLFW_KEY_LAST + 1; ++i )
        {
            s_keyStatus[i] += s_keyStatus[i] == KEY_RELEASED || s_keyStatus[i] == KEY_PRESSED;
        }

        for ( int i = 0; i < GLFW_MOUSE_BUTTON_LAST + 1; ++i )
        {
            s_mouseButtonStatus[i] += s_mouseButtonStatus[i] == KEY_RELEASED || s_mouseButtonStatus[i] == KEY_PRESSED;
        }
        s_lastCursorPos = s_currentCursorPos;
        s_scrollOffset  = glm::vec2( 0 );
        glfwPollEvents();
    }

    bool GetKeyDown( Key k )
    {
        return s_keyStatus[static_cast< int >( k )] == KEY_PRESSED;
    }

    bool GetKeyUp( Key k )
    {
        return s_keyStatus[static_cast< int >( k )] == KEY_RELEASED;
    }

    bool GetKeyHeld( Key k )
    {
        return s_keyStatus[static_cast< int >( k )] == KEY_PRESSED || s_keyStatus[static_cast< int >( k )] == KEY_HELD;
    }

    bool GetMouseButtonDown( MouseButton b )
    {
        return s_mouseButtonStatus[static_cast< int >( b )] == KEY_PRESSED;
    }

    bool GetMouseButtonUp( MouseButton b )
    {
        return s_mouseButtonStatus[static_cast< int >( b )] == KEY_RELEASED;
    }

    bool GetMouseButtonHeld( MouseButton b )
    {
        return s_mouseButtonStatus[static_cast< int >( b )] == KEY_PRESSED || s_mouseButtonStatus[static_cast< int >( b )] == KEY_HELD;
    }

    glm::vec2 GetMousePosition()
    {
        return s_currentCursorPos;
    }

    glm::vec2 GetMouseChange()
    {
        return s_currentCursorPos - s_lastCursorPos;
    }

    glm::vec2 GetScrollChange()
    {
        return s_scrollOffset;
    }

} // namespace Input
} // namespace PG