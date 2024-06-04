#pragma once

#include "shared/core_defines.hpp"

namespace PG::Input
{

enum class RawButton : uint16_t
{
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    K_0,
    K_1,
    K_2,
    K_3,
    K_4,
    K_5,
    K_6,
    K_7,
    K_8,
    K_9,

    UNKNOWN,
    SPACE,
    ESC,
    APOSTROPHE,
    COMMA,
    MINUS,
    PERIOD,
    SLASH,
    SEMICOLON,
    EQUALL,
    LEFT_BRACKET,
    BACKSLASH,
    RIGHT_BRACKET,
    BACK_TICK,
    ENTER,
    TAB,
    BACKSPACE,
    INSERT,
    DELETE,
    RIGHT,
    LEFT,
    DOWN,
    UP,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    CAPS_LOCK,
    SCROLL_LOCK,
    NUM_LOCK,
    PRINT_SCREEN,
    PAUSE,
    LEFT_SHIFT,
    LEFT_CONTROL,
    LEFT_ALT,
    LEFT_SUPER,
    RIGHT_SHIFT,
    RIGHT_CONTROL,
    RIGHT_ALT,
    RIGHT_SUPER,
    MENU,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    KP_0,
    KP_1,
    KP_2,
    KP_3,
    KP_4,
    KP_5,
    KP_6,
    KP_7,
    KP_8,
    KP_9,

    KP_DECIMAL,
    KP_DIVIDE,
    KP_MULTIPLY,
    KP_SUBTRACT,
    KP_ADD,
    KP_ENTER,
    KP_EQUAL,

    MB_LEFT,
    MB_MIDDLE,
    MB_RIGHT,

    COUNT
};

enum class RawButtonState : uint8_t
{
    PRESSED,  // only true the first frame pressed
    HELD,     // true for the 2nd frame the button is pressed and onwards
    RELEASED, // only true the first frame released
};

enum class RawAxis : uint8_t
{
    MOUSE_X,
    MOUSE_Y,

    COUNT
};

using RawAxisValue = float;

RawButton GlfwKeyToRawButton( int key );
RawButton GlfwMouseButtonToRawButton( int button );

} // namespace PG::Input
