#include "core/input.hpp"
#include "core/lua.hpp"
#include "core/window.hpp"
#include "shared/logger.hpp"

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
static bool s_anyKeyDown;
static bool s_anyKeyUp;
static bool s_anyMouseButtonDown;
static bool s_anyMouseButtonUp;

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
        s_anyKeyDown = true;
    }
    else if ( action == GLFW_RELEASE )
    {
        s_keyStatus[key] = KEY_RELEASED;
        s_anyKeyUp = true;
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
        s_anyMouseButtonDown = true;
    }
    else if ( action == GLFW_RELEASE )
    {
        s_mouseButtonStatus[button] = KEY_RELEASED;
        s_anyMouseButtonUp = true;
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
        s_anyKeyDown = false;
        s_anyKeyUp = false;
        s_anyMouseButtonDown = false;
        s_anyMouseButtonUp = false;
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

    bool AnyKeyDown()
    {
        return s_anyKeyDown;
    }

    bool GetKeyUp( Key k )
    {
        return s_keyStatus[static_cast< int >( k )] == KEY_RELEASED;
    }

    bool AnyKeyUp()
    {
        return s_anyKeyUp;
    }

    bool GetKeyHeld( Key k )
    {
        return s_keyStatus[static_cast< int >( k )] == KEY_PRESSED || s_keyStatus[static_cast< int >( k )] == KEY_HELD;
    }

    bool GetMouseButtonDown( MouseButton b )
    {
        return s_mouseButtonStatus[static_cast< int >( b )] == KEY_PRESSED;
    }

    bool AnyMouseButtonDown()
    {
        return s_anyMouseButtonDown;
    }

    bool GetMouseButtonUp( MouseButton b )
    {
        return s_mouseButtonStatus[static_cast< int >( b )] == KEY_RELEASED;
    }

    bool AnyMouseButtonUp()
    {
        return s_anyMouseButtonUp;
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


    void RegisterLuaFunctions( lua_State* L )
    {
        sol::state_view lua( L );
        auto input = lua["Input"].get_or_create< sol::table >();
        input.set_function( "GetKeyDown", &GetKeyDown );
        input.set_function( "GetKeyUp", &GetKeyUp );
        input.set_function( "GetKeyHeld", &GetKeyHeld );
        input.set_function( "GetMouseButtonDown", &GetMouseButtonDown );
        input.set_function( "GetMouseButtonUp", &GetMouseButtonUp );
        input.set_function( "GetMouseButtonHeld", &GetMouseButtonHeld );
        input.set_function( "GetMousePosition", &GetMousePosition );
        input.set_function( "GetMouseChange", &GetMouseChange );
        input.set_function( "GetScrollChange", &GetScrollChange );

        std::initializer_list< std::pair< sol::string_view, Key > > keyItems =
        {
            { "A", Key::A },
            { "B", Key::B },
            { "C", Key::C },
            { "D", Key::D },
            { "E", Key::E },
            { "F", Key::F },
            { "H", Key::G },
            { "G", Key::H },
            { "I", Key::I },
            { "J", Key::J },
            { "K", Key::K },
            { "L", Key::L },
            { "M", Key::M },
            { "N", Key::N },
            { "O", Key::O },
            { "P", Key::P },
            { "Q", Key::Q },
            { "R", Key::R },
            { "S", Key::S },
            { "T", Key::T },
            { "U", Key::U },
            { "V", Key::V },
            { "W", Key::W },
            { "X", Key::X },
            { "Y", Key::Y },
            { "Z", Key::Z },
            { "UNKOWN", Key::UNKOWN },
            { "SPACE", Key::SPACE },
            { "ESC", Key::ESC },
            { "APOSTROPHE", Key::APOSTROPHE },
            { "COMMA", Key::COMMA },
            { "MINUS", Key::MINUS },
            { "PERIOD", Key::PERIOD },
            { "SLASH", Key::SLASH },
            { "SEMICOLON", Key::SEMICOLON },
            { "EQUAL", Key::EQUAL },
            { "LEFT_BRACKET", Key::LEFT_BRACKET },
            { "BACKSLASH", Key::BACKSLASH },
            { "RIGHT_BRACKET", Key::RIGHT_BRACKET },
            { "BACK_TICK", Key::BACK_TICK },
            { "ENTER", Key::ENTER },
            { "TAB", Key::TAB },
            { "BACKSPACE", Key::BACKSPACE },
            { "INSERT", Key::INSERT },
            { "DELETE", Key::DELETE },
            { "RIGHT", Key::RIGHT },
            { "LEFT", Key::LEFT },
            { "DOWN", Key::DOWN },
            { "UP", Key::UP },
            { "PAGE_UP", Key::PAGE_UP },
            { "PAGE_DOWN", Key::PAGE_DOWN },
            { "HOME", Key::HOME },
            { "END", Key::END },
            { "CAPS_LOCK", Key::CAPS_LOCK },
            { "SCROLL_LOCK", Key::SCROLL_LOCK },
            { "NUM_LOCK", Key::NUM_LOCK },
            { "PRINT_SCREEN", Key::PRINT_SCREEN },
            { "PAUSE", Key::PAUSE },
            { "LEFT_SHIFT", Key::LEFT_SHIFT },
            { "LEFT_CONTROL", Key::LEFT_CONTROL },
            { "LEFT_ALT", Key::LEFT_ALT },
            { "LEFT_SUPER", Key::LEFT_SUPER },
            { "RIGHT_SHIFT", Key::RIGHT_SHIFT },
            { "RIGHT_CONTROL", Key::RIGHT_CONTROL },
            { "RIGHT_ALT", Key::RIGHT_ALT },
            { "RIGHT_SUPER", Key::RIGHT_SUPER },
            { "MENU", Key::MENU },
            { "F1", Key::F1 },
            { "F2", Key::F2 },
            { "F3", Key::F3 },
            { "F4", Key::F4 },
            { "F5", Key::F5 },
            { "F6", Key::F6 },
            { "F7", Key::F7 },
            { "F8", Key::F8 },
            { "F9", Key::F9 },
            { "F10", Key::F10 },
            { "F11", Key::F11 },
            { "F12", Key::F12 },
            { "KP_0", Key::KP_0 },
            { "KP_1", Key::KP_1 },
            { "KP_2", Key::KP_2 },
            { "KP_3", Key::KP_3 },
            { "KP_4", Key::KP_4 },
            { "KP_5", Key::KP_5 },
            { "KP_6", Key::KP_6 },
            { "KP_7", Key::KP_7 },
            { "KP_8", Key::KP_8 },
            { "KP_9", Key::KP_9 },
            { "KP_DECIMAL",  Key::KP_DECIMAL },
            { "KP_DIVIDE",   Key::KP_DIVIDE },
            { "KP_MULTIPLY", Key::KP_MULTIPLY },
            { "KP_SUBTRACT", Key::KP_SUBTRACT },
            { "KP_ADD",      Key::KP_ADD },
            { "KP_ENTER",    Key::KP_ENTER },
            { "KP_EQUAL",    Key::KP_EQUAL }
        };
        lua.new_enum< Key, false >( "Key", keyItems ); // false makes it read/write in Lua, but its faster

        std::initializer_list< std::pair< sol::string_view, MouseButton > > mouseItems =
        {
            { "LEFT", MouseButton::LEFT },
            { "RIGHT", MouseButton::RIGHT },
            { "MIDDLE", MouseButton::MIDDLE },
        };
        lua.new_enum< MouseButton, false >( "MouseButton", mouseItems );
    }

} // namespace Input
} // namespace PG