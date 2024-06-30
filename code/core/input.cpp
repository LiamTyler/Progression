#include "core/input.hpp"
#include "core/engine_globals.hpp"
#include "core/lua.hpp"
#include "core/window.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <unordered_set>

#if USING( DEVELOPMENT_BUILD )
#include "imgui/imgui.h"
#endif // #if USING( DEVELOPMENT_BUILD )

namespace PG::Input
{

static vec2 s_prevMousePos;
static bool s_cursorStateChanged = false;

class RawInputTracker
{
    friend class InputContextManager;

    std::unordered_map<RawButton, RawButtonState> m_activeButtons;
    std::vector<u32> m_characters; // unicode
    std::unordered_map<RawAxis, RawAxisValue> m_activeAxes;

public:
    RawInputTracker() = default;

    bool HasInput() const { return !m_activeButtons.empty() || !m_characters.empty() || !m_activeAxes.empty(); }

    void AddOrUpdateButton( RawButton button, RawButtonState state ) { m_activeButtons[button] = state; }

    void AddCharacter( u32 c ) { m_characters.push_back( c ); }

    void AddAxisValue( RawAxis axis, RawAxisValue value )
    {
        // Seems like the mouse cursor callback can happen multiple times per frame. Combined the values if so
        if ( value )
        {
            if ( m_activeAxes.contains( axis ) )
                m_activeAxes[axis] += value;
            else
                m_activeAxes[axis] = value;
        }
    }

    void StartFrame()
    {
        m_characters.clear();
        m_activeAxes.clear();
        for ( auto it = m_activeButtons.begin(); it != m_activeButtons.end(); )
        {
            auto nextIt = std::next( it );
            if ( it->second == RawButtonState::PRESSED )
                it->second = RawButtonState::HELD;
            if ( it->second == RawButtonState::RELEASED )
                nextIt = m_activeButtons.erase( it );

            it = nextIt;
        }
    }

    void CreateCallbackInput( InputContext* context, CallbackInput& cInput )
    {
        cInput.actions.clear();
        cInput.axes.clear();

        for ( const auto [rawButton, rawState] : m_activeButtons )
        {
            if ( context->m_rawButtonToActionMap.contains( rawButton ) )
            {
                Action action      = context->m_rawButtonToActionMap[rawButton];
                ActionState aState = (ActionState)rawState;
                cInput.actions.emplace_back( action, aState );
            }
            else if ( context->m_rawButtonToAxisMap.contains( rawButton ) )
            {
                AxisValuePair axisValuePair = context->m_rawButtonToAxisMap[rawButton];
                if ( rawState == RawButtonState::RELEASED )
                    axisValuePair.value = 0.0f;
                // if two actions are present that map to the same axis, add them
                size_t i = 0;
                while ( i < cInput.axes.size() && cInput.axes[i].axis != axisValuePair.axis )
                    ++i;
                if ( i == cInput.axes.size() )
                    cInput.axes.emplace_back( axisValuePair );
                else
                    cInput.axes[i].value += axisValuePair.value;
            }
        }

        for ( const auto [rawAxis, rawValue] : m_activeAxes )
        {
            if ( context->m_rawAxisToAxisMap.contains( rawAxis ) )
            {
                Axis axis        = context->m_rawAxisToAxisMap[rawAxis];
                AxisValue aValue = (AxisValue)rawValue;
                cInput.axes.emplace_back( axis, aValue );
            }
        }
    }

    void UpdateBasedOnContext( InputContext* context )
    {
        if ( context->m_blockLevel == InputContextBlockLevel::NOT_BLOCKING )
            return;

        if ( context->m_blockLevel == InputContextBlockLevel::BLOCK_ALL_CONTROLS )
        {
            m_activeAxes.clear();
            m_activeButtons.clear();
            m_characters.clear();
            return;
        }

        for ( auto it = m_activeButtons.begin(); it != m_activeButtons.end(); )
        {
            const RawButton button = it->first;
            bool isMapped          = context->m_rawButtonToActionMap.contains( button ) || context->m_rawButtonToAxisMap.contains( button );
            it                     = isMapped ? m_activeButtons.erase( it ) : std::next( it );
        }

        for ( auto it = m_activeAxes.begin(); it != m_activeAxes.end(); ++it )
        {
            bool isMapped = context->m_rawAxisToAxisMap.contains( it->first );
            it            = isMapped ? m_activeAxes.erase( it ) : std::next( it );
        }
    }
};

static std::vector<InputContext*> s_allInputContexts;
static InputContextManager s_contextManager;
static RawInputTracker s_inputTracker;

static void KeyCallback( GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods )
{
    if ( action != GLFW_PRESS && action != GLFW_RELEASE )
        return;

    RawButton button = GlfwKeyToRawButton( key );
    if ( button == RawButton::UNKNOWN )
    {
        LOG_WARN( "Unknown key pressed!" );
        return;
    }

    RawButtonState state = action == GLFW_PRESS ? RawButtonState::PRESSED : RawButtonState::RELEASED;
    s_inputTracker.AddOrUpdateButton( button, state );
}

static void CharCallback( GLFWwindow* window, u32 c ) { s_inputTracker.AddCharacter( c ); }

static void MousePosCallback( GLFWwindow* window, f64 x, f64 y )
{
    vec2 pos = vec2( (f32)x, (f32)y );
    // LOG( "%f %f", pos.x, pos.y );
    if ( !s_cursorStateChanged )
    {
        s_inputTracker.AddAxisValue( RawAxis::MOUSE_X, pos.x - s_prevMousePos.x );
        s_inputTracker.AddAxisValue( RawAxis::MOUSE_Y, pos.y - s_prevMousePos.y );
    }
    s_prevMousePos = pos;
}

static void MouseButtonCallback( GLFWwindow* window, i32 button, i32 action, i32 mods )
{
    if ( action != GLFW_PRESS && action != GLFW_RELEASE )
        return;

    RawButton rawButton = GlfwMouseButtonToRawButton( button );
    if ( rawButton == RawButton::UNKNOWN )
    {
        LOG_WARN( "Unknown key pressed!" );
        return;
    }

    RawButtonState state = action == GLFW_PRESS ? RawButtonState::PRESSED : RawButtonState::RELEASED;
    s_inputTracker.AddOrUpdateButton( rawButton, state );
}

bool Init()
{
    PGP_ZONE_SCOPEDN( "Input::Init" );
    s_contextManager.ClearAll();
    if ( !s_contextManager.LoadContexts() )
    {
        LOG_ERR( "Failed to load all input contexts" );
        return false;
    }

    s_contextManager.PushLayer( InputContextID::CAMERA_CONTROLS );

    GLFWwindow* window = GetMainWindow()->GetGLFWHandle();
    glfwSetKeyCallback( window, KeyCallback );
    glfwSetCharCallback( window, CharCallback );
    glfwSetMouseButtonCallback( window, MouseButtonCallback );

    ResetMousePosition();
    glfwSetCursorPosCallback( window, MousePosCallback );

    return true;
}

void Shutdown()
{
    for ( InputContext* context : s_allInputContexts )
        delete context;

    glfwPollEvents();
}

void PollEvents()
{
    PGP_ZONE_SCOPEDN( "Input::PollEvents" );
    s_inputTracker.StartFrame();
    glfwPollEvents();
    s_contextManager.Update();
    s_cursorStateChanged = false;
}

void ResetMousePosition()
{
    f64 mouse_x, mouse_y;
    glfwGetCursorPos( GetMainWindow()->GetGLFWHandle(), &mouse_x, &mouse_y );
    s_prevMousePos = vec2( (f32)mouse_x, (f32)mouse_y );
}

void MouseCursorChange() { s_cursorStateChanged = true; }

bool CallbackInput::HasInput() const { return !axes.empty() || !actions.empty(); }

bool CallbackInput::ActionJustPressed( Action action ) const
{
    for ( size_t i = 0; i < actions.size(); ++i )
    {
        if ( actions[i].action == action )
            return actions[i].state == ActionState::PRESSED;
    }

    return false;
}

bool CallbackInput::ActionBeingPressed( Action action ) const
{
    for ( size_t i = 0; i < actions.size(); ++i )
    {
        if ( actions[i].action == action )
            return actions[i].state == ActionState::PRESSED || actions[i].state == ActionState::HELD;
    }

    return false;
}

bool CallbackInput::ActionReleased( Action action ) const
{
    for ( size_t i = 0; i < actions.size(); ++i )
    {
        if ( actions[i].action == action )
            return actions[i].state == ActionState::RELEASED;
    }

    return false;
}

AxisValue CallbackInput::GetAxisValue( Axis axis ) const
{
    for ( size_t i = 0; i < axes.size(); ++i )
    {
        if ( axes[i].axis == axis )
            return axes[i].value;
    }

    return 0;
}

void InputContext::AddRawButtonToAction( RawButton rawButton, Action action ) { m_rawButtonToActionMap[rawButton] = action; }

void InputContext::AddRawButtonToAxis( RawButton rawButton, AxisValuePair axisValuePair )
{
    m_rawButtonToAxisMap[rawButton] = axisValuePair;
}

void InputContext::AddRawAxisToAxis( RawAxis rawAxis, Axis axis ) { m_rawAxisToAxisMap[rawAxis] = axis; }

u32 InputContext::AddCallback( sol::function callback )
{
    u32 id                = m_callbackID++;
    m_scriptCallbacks[id] = callback;
    return id;
}

u32 InputContext::AddCallback( InputCodeCallback callback )
{
    u32 id              = m_callbackID++;
    m_codeCallbacks[id] = callback;
    return id;
}

void InputContext::RemoveCallback( u32 id )
{
    if ( m_scriptCallbacks.contains( id ) )
    {
        m_scriptCallbacks.erase( id );
        return;
    }

    PG_ASSERT( m_codeCallbacks.contains( id ) );
    m_codeCallbacks.erase( id );
}

InputContextBlockLevel InputContext::GetBlockLevel() const { return m_blockLevel; }
void InputContext::SetBlockLevel( InputContextBlockLevel level ) { m_blockLevel = level; }

void InputContext::ProcessCallbacks( const CallbackInput& cInput )
{
    if ( !cInput.HasInput() )
        return;

    for ( const auto& [id, callback] : m_scriptCallbacks )
    {
        CHECK_SOL_FUNCTION_CALL( callback( cInput ) );
    }
    for ( const auto& [id, callback] : m_codeCallbacks )
    {
        callback( cInput );
    }
}

InputContextManager::InputContextManager()
{
    ClearAll();
    m_layers.reserve( 8 );
    for ( auto& layer : m_layers )
        layer.contexts.reserve( 8 );
}

static void GlobalControlsInputCallback( const CallbackInput& cInput )
{
    if ( cInput.ActionJustPressed( Action::QUIT_GAME ) )
        eg.shutdown = true;
}

bool InputContextManager::LoadContexts()
{
    s_allInputContexts.resize( Underlying( InputContextID::COUNT ) );
    InputContext* context;

    context = new InputContext( InputContextID::GLOBAL_CONTROLS );
    context->SetBlockLevel( InputContextBlockLevel::BLOCK_MAPPED_CONTROLS );
    context->AddRawButtonToAction( RawButton::ESC, Action::QUIT_GAME );
    context->AddCallback( GlobalControlsInputCallback );

    s_allInputContexts[Underlying( InputContextID::GLOBAL_CONTROLS )] = context;

    context = new InputContext( InputContextID::CAMERA_CONTROLS );
    context->SetBlockLevel( InputContextBlockLevel::BLOCK_MAPPED_CONTROLS );
    context->AddRawButtonToAxis( RawButton::A, { Axis::CAMERA_VEL_X, -1.0f } );
    context->AddRawButtonToAxis( RawButton::D, { Axis::CAMERA_VEL_X, 1.0f } );
    context->AddRawButtonToAxis( RawButton::W, { Axis::CAMERA_VEL_Y, 1.0f } );
    context->AddRawButtonToAxis( RawButton::S, { Axis::CAMERA_VEL_Y, -1.0f } );
    context->AddRawButtonToAxis( RawButton::MB_LEFT, { Axis::CAMERA_VEL_Z, 1.0f } );
    context->AddRawButtonToAxis( RawButton::MB_RIGHT, { Axis::CAMERA_VEL_Z, -1.0f } );
    context->AddRawButtonToAction( RawButton::X, Action::TOGGLE_CAMERA_CONTROLS );
    context->AddRawButtonToAction( RawButton::LEFT_SHIFT, Action::CAMERA_SPRINT );
    context->AddRawAxisToAxis( RawAxis::MOUSE_X, Axis::CAMERA_YAW );
    context->AddRawAxisToAxis( RawAxis::MOUSE_Y, Axis::CAMERA_PITCH );

    s_allInputContexts[Underlying( InputContextID::CAMERA_CONTROLS )] = context;

    return true;
}

void InputContextManager::Update()
{
    if ( !s_inputTracker.HasInput() )
        return;

    RawInputTracker inputTracker = s_inputTracker;
    CallbackInput cInput;

    // always process the global controls + handle imgui right after (in developement builds)
    InputContext* devContext = s_allInputContexts[Underlying( InputContextID::GLOBAL_CONTROLS )];
    inputTracker.CreateCallbackInput( devContext, cInput );
    devContext->ProcessCallbacks( cInput );
    inputTracker.UpdateBasedOnContext( devContext );

#if USING( DEVELOPMENT_BUILD )
    ImGuiIO& io = ImGui::GetIO();
    if ( io.WantCaptureKeyboard )
        inputTracker.m_activeButtons.clear();
    if ( io.WantTextInput )
        inputTracker.m_characters.clear();
    if ( io.WantCaptureMouse )
        inputTracker.m_activeAxes.clear();

    if ( !inputTracker.HasInput() )
        return;
#endif // #if USING( DEVELOPMENT_BUILD )

    // make copy, in case the callbacks edit the context list/order
    static std::vector<InputContext*> scratchContexts;
    scratchContexts = CurrentLayer().contexts;
    for ( size_t cIdx = scratchContexts.size(); cIdx-- > 0; )
    {
        InputContext* context = scratchContexts[cIdx];
        inputTracker.CreateCallbackInput( context, cInput );

        context->ProcessCallbacks( cInput );

        inputTracker.UpdateBasedOnContext( context );
        if ( !inputTracker.HasInput() )
            return;
    }
}

void InputContextManager::ClearAll() { m_layers.clear(); }
void InputContextManager::PushContext_Front( InputContextID contextID )
{
    CurrentLayer().contexts.insert( CurrentLayer().contexts.begin(), GetContext( contextID ) );
}
void InputContextManager::PopContext_Front() { CurrentLayer().contexts.erase( CurrentLayer().contexts.begin() ); }
void InputContextManager::PushContext_Back( InputContextID contextID ) { CurrentLayer().contexts.push_back( GetContext( contextID ) ); }
void InputContextManager::PopContext_Back() { CurrentLayer().contexts.pop_back(); }
void InputContextManager::PushLayer( InputContextID contextID )
{
    m_layers.emplace_back();
    PushContext_Back( contextID );
}
void InputContextManager::PopLayer() { m_layers.pop_back(); }

InputContext* GetContext( InputContextID id ) { return s_allInputContexts[Underlying( id )]; }

static u32 AddCallback( InputContextID contextID, sol::function callback )
{
    return s_allInputContexts[Underlying( contextID )]->AddCallback( callback );
}

static void RemoveCallback( InputContextID contextID, u32 callbackHandle )
{
    return s_allInputContexts[Underlying( contextID )]->RemoveCallback( callbackHandle );
}

void RegisterLuaFunctions( lua_State* L )
{
    sol::state_view lua( L );
    auto input = lua["Input"].get_or_create<sol::table>();
    input.set_function( "AddCallback", &AddCallback );
    input.set_function( "RemoveCallback", &RemoveCallback );

    sol::table actionEnum = input.create( (i32)Underlying( Action::COUNT ) );
    for ( u16 i = 0; i < Underlying( Action::COUNT ); ++i )
    {
        Action action = (Action)i;
        actionEnum.set( ActionToString( action ), action );
    }
    input.create_named( "Action", sol::metatable_key,
        input.create_with( sol::meta_function::new_index, sol::detail::fail_on_newindex, sol::meta_function::index, actionEnum ) );

    sol::table axisEnum = input.create( (i32)Underlying( Axis::COUNT ) );
    for ( u16 i = 0; i < Underlying( Axis::COUNT ); ++i )
    {
        Axis axis = (Axis)i;
        axisEnum.set( AxisToString( axis ), axis );
    }
    input.create_named( "Axis", sol::metatable_key,
        input.create_with( sol::meta_function::new_index, sol::detail::fail_on_newindex, sol::meta_function::index, axisEnum ) );

    sol::table contextEnum = input.create( (i32)Underlying( InputContextID::COUNT ) );
    for ( u16 i = 0; i < Underlying( InputContextID::COUNT ); ++i )
    {
        InputContextID id = (InputContextID)i;
        contextEnum.set( InputContextIDToString( id ), id );
    }
    input.create_named( "Context", sol::metatable_key,
        input.create_with( sol::meta_function::new_index, sol::detail::fail_on_newindex, sol::meta_function::index, contextEnum ) );

    sol::usertype<CallbackInput> cInput = lua.new_usertype<CallbackInput>( "CallbackInput" );
    cInput["ActionJustPressed"]         = &CallbackInput::ActionJustPressed;
    cInput["ActionBeingPressed"]        = &CallbackInput::ActionBeingPressed;
    cInput["ActionReleased"]            = &CallbackInput::ActionReleased;
    cInput["GetAxisValue"]              = &CallbackInput::GetAxisValue;
}

const char* InputContextIDToString( InputContextID id )
{
    static const char* names[] = {
        "GLOBAL_CONTROLS",
        "CAMERA_CONTROLS",
    };
    static_assert( Underlying( InputContextID::COUNT ) == ARRAY_COUNT( names ), "don't forget to update" );

    return names[Underlying( id )];
}

} // namespace PG::Input
