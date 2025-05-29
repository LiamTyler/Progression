#include "raw_input_types.hpp"

namespace PG::Input
{

#define SWITCH_CASE( key, val ) \
    case key: return val;

RawButton SDL3KeyToRawButton( const SDL_KeyboardEvent& kEvent )
{
    switch ( kEvent.key )
    {
        SWITCH_CASE( SDLK_A, RawButton::A )
        // SWITCH_CASE( SDLK_b, RawButton::B )
        // SWITCH_CASE( SDLK_c, RawButton::C )
        // SWITCH_CASE( SDLK_d, RawButton::D )
        // SWITCH_CASE( SDLK_e, RawButton::E )
        // SWITCH_CASE( SDLK_f, RawButton::F )
        // SWITCH_CASE( SDLK_g, RawButton::G )
        // SWITCH_CASE( SDLK_h, RawButton::H )
        // SWITCH_CASE( SDLK_i, RawButton::I )
        // SWITCH_CASE( SDLK_j, RawButton::J )
        // SWITCH_CASE( SDLK_k, RawButton::K )
        // SWITCH_CASE( SDLK_l, RawButton::L )
        // SWITCH_CASE( SDLK_m, RawButton::M )
        // SWITCH_CASE( SDLK_n, RawButton::N )
        // SWITCH_CASE( SDLK_o, RawButton::O )
        // SWITCH_CASE( SDLK_p, RawButton::P )
        // SWITCH_CASE( SDLK_q, RawButton::Q )
        // SWITCH_CASE( SDLK_r, RawButton::R )
        // SWITCH_CASE( SDLK_s, RawButton::S )
        // SWITCH_CASE( SDLK_t, RawButton::T )
        // SWITCH_CASE( SDLK_u, RawButton::U )
        // SWITCH_CASE( SDLK_v, RawButton::V )
        // SWITCH_CASE( SDLK_w, RawButton::W )
        // SWITCH_CASE( SDLK_x, RawButton::X )
        // SWITCH_CASE( SDLK_y, RawButton::Y )
        // SWITCH_CASE( SDLK_z, RawButton::Z )

        SWITCH_CASE( SDLK_0, RawButton::K_0 )
        SWITCH_CASE( SDLK_1, RawButton::K_1 )
        SWITCH_CASE( SDLK_2, RawButton::K_2 )
        SWITCH_CASE( SDLK_3, RawButton::K_3 )
        SWITCH_CASE( SDLK_4, RawButton::K_4 )
        SWITCH_CASE( SDLK_5, RawButton::K_5 )
        SWITCH_CASE( SDLK_6, RawButton::K_6 )
        SWITCH_CASE( SDLK_7, RawButton::K_7 )
        SWITCH_CASE( SDLK_8, RawButton::K_8 )
        SWITCH_CASE( SDLK_9, RawButton::K_9 )

        SWITCH_CASE( SDLK_UNKNOWN, RawButton::UNKNOWN )
        SWITCH_CASE( SDLK_SPACE, RawButton::SPACE )
        SWITCH_CASE( SDLK_ESCAPE, RawButton::ESC )
        SWITCH_CASE( SDLK_APOSTROPHE, RawButton::APOSTROPHE )
        SWITCH_CASE( SDLK_COMMA, RawButton::COMMA )
        SWITCH_CASE( SDLK_MINUS, RawButton::MINUS )
        SWITCH_CASE( SDLK_PERIOD, RawButton::PERIOD )
        SWITCH_CASE( SDLK_SLASH, RawButton::SLASH )
        SWITCH_CASE( SDLK_SEMICOLON, RawButton::SEMICOLON )
        SWITCH_CASE( SDLK_EQUALS, RawButton::EQUALL )
        SWITCH_CASE( SDLK_LEFTBRACKET, RawButton::LEFT_BRACKET )
        SWITCH_CASE( SDLK_BACKSLASH, RawButton::BACKSLASH )
        SWITCH_CASE( SDLK_RIGHTBRACKET, RawButton::RIGHT_BRACKET )
        SWITCH_CASE( SDLK_GRAVE, RawButton::BACK_TICK )
        SWITCH_CASE( SDLK_RETURN, RawButton::ENTER )
        SWITCH_CASE( SDLK_TAB, RawButton::TAB )
        SWITCH_CASE( SDLK_BACKSPACE, RawButton::BACKSPACE )
        SWITCH_CASE( SDLK_INSERT, RawButton::INSERT )
        SWITCH_CASE( SDLK_DELETE, RawButton::DELETE )
        SWITCH_CASE( SDLK_RIGHT, RawButton::RIGHT )
        SWITCH_CASE( SDLK_LEFT, RawButton::LEFT )
        SWITCH_CASE( SDLK_DOWN, RawButton::DOWN )
        SWITCH_CASE( SDLK_UP, RawButton::UP )
        SWITCH_CASE( SDLK_PAGEUP, RawButton::PAGE_UP )
        SWITCH_CASE( SDLK_PAGEDOWN, RawButton::PAGE_DOWN )
        SWITCH_CASE( SDLK_HOME, RawButton::HOME )
        SWITCH_CASE( SDLK_END, RawButton::END )
        SWITCH_CASE( SDLK_CAPSLOCK, RawButton::CAPS_LOCK )
        SWITCH_CASE( SDLK_SCROLLLOCK, RawButton::SCROLL_LOCK )
        SWITCH_CASE( SDLK_NUMLOCKCLEAR, RawButton::NUM_LOCK )
        SWITCH_CASE( SDLK_PRINTSCREEN, RawButton::PRINT_SCREEN )
        SWITCH_CASE( SDLK_PAUSE, RawButton::PAUSE )
        SWITCH_CASE( SDLK_LSHIFT, RawButton::LEFT_SHIFT )
        SWITCH_CASE( SDLK_LCTRL, RawButton::LEFT_CONTROL )
        SWITCH_CASE( SDLK_LALT, RawButton::LEFT_ALT )
        SWITCH_CASE( SDLK_LGUI, RawButton::LEFT_SUPER )
        SWITCH_CASE( SDLK_RSHIFT, RawButton::RIGHT_SHIFT )
        SWITCH_CASE( SDLK_RCTRL, RawButton::RIGHT_CONTROL )
        SWITCH_CASE( SDLK_RALT, RawButton::RIGHT_ALT )
        SWITCH_CASE( SDLK_RGUI, RawButton::RIGHT_SUPER )
        SWITCH_CASE( SDLK_MENU, RawButton::MENU )

        SWITCH_CASE( SDLK_F1, RawButton::F1 )
        SWITCH_CASE( SDLK_F2, RawButton::F2 )
        SWITCH_CASE( SDLK_F3, RawButton::F3 )
        SWITCH_CASE( SDLK_F4, RawButton::F4 )
        SWITCH_CASE( SDLK_F5, RawButton::F5 )
        SWITCH_CASE( SDLK_F6, RawButton::F6 )
        SWITCH_CASE( SDLK_F7, RawButton::F7 )
        SWITCH_CASE( SDLK_F8, RawButton::F8 )
        SWITCH_CASE( SDLK_F9, RawButton::F9 )
        SWITCH_CASE( SDLK_F10, RawButton::F10 )
        SWITCH_CASE( SDLK_F11, RawButton::F11 )
        SWITCH_CASE( SDLK_F12, RawButton::F12 )

        SWITCH_CASE( SDLK_KP_0, RawButton::KP_0 )
        SWITCH_CASE( SDLK_KP_1, RawButton::KP_1 )
        SWITCH_CASE( SDLK_KP_2, RawButton::KP_2 )
        SWITCH_CASE( SDLK_KP_3, RawButton::KP_3 )
        SWITCH_CASE( SDLK_KP_4, RawButton::KP_4 )
        SWITCH_CASE( SDLK_KP_5, RawButton::KP_5 )
        SWITCH_CASE( SDLK_KP_6, RawButton::KP_6 )
        SWITCH_CASE( SDLK_KP_7, RawButton::KP_7 )
        SWITCH_CASE( SDLK_KP_8, RawButton::KP_8 )
        SWITCH_CASE( SDLK_KP_9, RawButton::KP_9 )

        SWITCH_CASE( SDLK_KP_PERIOD, RawButton::KP_DECIMAL )
        SWITCH_CASE( SDLK_KP_DIVIDE, RawButton::KP_DIVIDE )
        SWITCH_CASE( SDLK_KP_MULTIPLY, RawButton::KP_MULTIPLY )
        SWITCH_CASE( SDLK_KP_MINUS, RawButton::KP_SUBTRACT )
        SWITCH_CASE( SDLK_KP_PLUS, RawButton::KP_ADD )
        SWITCH_CASE( SDLK_KP_ENTER, RawButton::KP_ENTER )
        SWITCH_CASE( SDLK_KP_EQUALS, RawButton::KP_EQUAL )
    }

    return RawButton::UNKNOWN;
}

RawButton SDL3MouseButtonToRawButton( const SDL_MouseButtonEvent& mEvent )
{
    switch ( mEvent.button )
    {
        SWITCH_CASE( SDL_BUTTON_LEFT, RawButton::MB_LEFT )
        SWITCH_CASE( SDL_BUTTON_MIDDLE, RawButton::MB_MIDDLE )
        SWITCH_CASE( SDL_BUTTON_RIGHT, RawButton::MB_RIGHT )
    }

    return RawButton::UNKNOWN;
}

} // namespace PG::Input
