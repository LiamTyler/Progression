#pragma once

#include "core/input_types.hpp"
#include "core/lua.hpp"
#include "core/raw_input_types.hpp"
#include "shared/math_vec.hpp"
#include <unordered_map>

struct lua_State;

namespace PG::Input
{

enum class InputContextBlockLevel : u8
{
    NOT_BLOCKING,
    BLOCK_MAPPED_CONTROLS,
    BLOCK_ALL_CONTROLS,

    COUNT
};

enum class InputContextID : u8
{
#if !USING( SHIP_BUILD )
    DEV_CONTROLS,
#endif // #if !USING( SHIP_BUILD )
    CAMERA_CONTROLS,

    COUNT
};

const char* InputContextIDToString( InputContextID id );

class CallbackInput
{
public:
    bool HasInput() const;
    bool ActionJustPressed( Action action ) const;
    bool ActionBeingPressed( Action action ) const;
    bool ActionReleased( Action action ) const;
    AxisValue GetAxisValue( Axis axis ) const;

    std::vector<ActionStatePair> actions;
    std::vector<AxisValuePair> axes;
};

using InputCodeCallback = void ( * )( const CallbackInput& cInput );

class InputContext
{
    friend class RawInputTracker;

public:
    InputContext( InputContextID id ) : m_id( id ) {}

    void AddRawButtonToAction( RawButton rawButton, Action action );
    void AddRawButtonToAxis( RawButton rawButton, AxisValuePair axisValuePair );
    void AddRawAxisToAxis( RawAxis rawAxis, Axis axis );

    u32 AddCallback( sol::function callback );
    u32 AddCallback( InputCodeCallback callback );
    void RemoveCallback( u32 id );
    void ProcessCallbacks( const CallbackInput& cInput );

    InputContextBlockLevel GetBlockLevel() const;
    void SetBlockLevel( InputContextBlockLevel level );

private:
    std::unordered_map<RawButton, Action> m_rawButtonToActionMap;
    std::unordered_map<RawButton, AxisValuePair> m_rawButtonToAxisMap;
    std::unordered_map<RawAxis, Axis> m_rawAxisToAxisMap;
    InputContextBlockLevel m_blockLevel = InputContextBlockLevel::BLOCK_MAPPED_CONTROLS;
    InputContextID m_id;

    std::unordered_map<u32, sol::function> m_scriptCallbacks;
    std::unordered_map<u32, InputCodeCallback> m_codeCallbacks;
    u32 m_callbackID = 0;
};

class InputContextManager
{
public:
    InputContextManager();

    bool LoadContexts();
    void Update();

    void ClearAll();
    void PushContext_Front( InputContextID contextID );
    void PopContext_Front();
    void PushContext_Back( InputContextID contextID );
    void PopContext_Back();
    void PushLayer( InputContextID contextID );
    void PopLayer();

private:
    struct ContextLayer
    {
        std::vector<InputContext*> contexts;
    };
    ContextLayer& CurrentLayer() { return m_layers.back(); }
    std::vector<ContextLayer> m_layers;
};

bool Init();
void Shutdown();

InputContext* GetContext( InputContextID contextID );

// should be called once per frame to update inputs
void PollEvents();
void ResetMousePosition();
void MouseCursorChange();

void RegisterLuaFunctions( lua_State* L );

} // namespace PG::Input
