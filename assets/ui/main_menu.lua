
callbackHandle = nil

function InputCallback( cInput )    
    --if cInput:ActionJustPressed( Input.Action.TOGGLE_CAMERA_CONTROLS ) then
    --    active = not active
    --    GetMainWindow():SetRelativeMouse( active )        
    --end
    --if cInput:ActionBeingPressed( Input.Action.CAMERA_SPRINT ) then
    --    currentSpeed = boostSpeed
    --end
end

function Start()
    --callbackHandle = Input.AddCallback( Input.Context.MAIN_MENU, InputCallback )
end

function Update()
    local tConfig = UI.TextConfig.new()
    tConfig.fontId = UI.GetFont( "arial" )
    tConfig.textColor = UI.Color( 0, 0, 0, 255 )
    tConfig.fontSize = 36
            
    UI.OpenElement()
    local e = UI.ElementDeclaration.new()
    e.id = UI.CreateID( "root" )
    e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_GROW, 0, UI.SIZING_TYPE_GROW, 0 )
    e.layout.childGap = 16
    e.layout.padding = UI.CreatePaddingEqual( 16 )
    e.layout.layoutDirection = UI.TOP_TO_BOTTOM
    e.layout.childAlignment = UI.CreateAlignment( UI.ALIGN_X_CENTER, UI.ALIGN_Y_CENTER )
    e.image = UI.GetImage( "bubba_sleeping" )
    UI.ConfigureElement( e )
        e.image = UI.NullImage()
        UI.OpenElement()
        e.id = UI.CreateID( "title" )
        e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_GROW, 0, UI.SIZING_TYPE_GROW, 50 )
        UI.ConfigureElement( e )
            tConfig.fontSize = 128
            UI.AddText( "Progression", tConfig )
        UI.CloseElement()
        
        tConfig.fontSize = 40
        UI.OpenElement()
        e.id = UI.CreateID( "options" )
        e.layout.childAlignment = UI.CreateAlignment( UI.ALIGN_X_CENTER, UI.ALIGN_Y_TOP )
        e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_FIT, 0, UI.SIZING_TYPE_GROW, 0 )
        e.layout.childGap = 0
        e.layout.padding = UI.CreatePaddingEqual( 12 )
        UI.ConfigureElement( e )
            e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_FIT, 0, UI.SIZING_TYPE_FIT, 0 )
            --e.layout.childGap = 1
            UI.OpenElement()
            e.id = UI.CreateID( "level_select" )
            UI.ConfigureElement( e )
                UI.AddText( "Level Select", tConfig )
            UI.CloseElement()
            UI.OpenElement()
            e.id = UI.CreateID( "settings" )
            UI.ConfigureElement( e )
                UI.AddText( "Settings", tConfig )
            UI.CloseElement()
            UI.OpenElement()
            e.id = UI.CreateID( "quit" )
            UI.ConfigureElement( e )
                UI.AddText( "Quit", tConfig )
            UI.CloseElement()
        UI.CloseElement()
    UI.CloseElement()
    
    --UI.OpenElement()
    --e = UI.ElementDeclaration.new()
    --e.id = UI.CreateID( "Parent" )
    --e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_GROW, 0, UI.SIZING_TYPE_GROW, 0 )
    --e.layout.childGap = 16
    --e.layout.padding = UI.CreatePaddingEqual( 16 )
    --e.layout.layoutDirection = UI.TOP_TO_BOTTOM;
    --e.backgroundColor = UI.Color( 0, 200, 20, 255 )
    --UI.ConfigureElement( e )
    --    UI.OpenElement()
    --    e.id = UI.CreateID( "Side1" )
    --    e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_FIXED, 400, UI.SIZING_TYPE_GROW, 0 )
    --    e.layout.padding = UI.CreatePadding( 10, 40, 20, 2 )
    --    if UI.Hovered() then
    --        e.backgroundColor = UI.Color( 0, 200, 200, 255 )
    --    else
    --        e.backgroundColor = UI.Color( 200, 200, 200, 255 )
    --    end
    --    e.border.color = UI.Color( 50, 50, 255, 255 )
    --    e.border.width = UI.BorderWidth( 10, 40, 20, 2 )
    --    UI.ConfigureElement( e )
    --        local tConfig = UI.TextConfig.new()
    --        tConfig.fontId = UI.GetFont( "arial" )
    --        tConfig.textColor = UI.Color( 0, 0, 0, 255 )
    --        tConfig.fontSize = 16
    --        UI.AddText( "Hello", tConfig )
    --    UI.CloseElement()
    --    
    --    UI.OpenElement()
    --    e.id = UI.CreateID( "Side2" )
    --    e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_FIXED, 400, UI.SIZING_TYPE_GROW, 0 )
    --    e.layout.padding = UI.CreatePaddingEqual( 0 )
    --    e.border.width = UI.BorderWidth( 0, 0, 0, 0 )
    --    e.image = UI.GetImage( "macaw" )
    --    UI.ConfigureElement( e )
    --    UI.CloseElement()
    --UI.CloseElement()
end