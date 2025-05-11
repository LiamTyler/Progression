
function Start()
    --material = AssetManager.GetMaterial( "white" )
end

function Update()
    UI.OpenElement()
    e = UI.ElementDeclaration.new()
    e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_GROW, 0, UI.SIZING_TYPE_GROW, 0 )
    e.layout.padding = UI.CreatePadding( 16 )
    e.backgroundColor = UI.Color( 0, 0, 255, 255 )
    UI.ConfigureElement( e )
        UI.OpenElement()
        e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_FIXED, 400, UI.SIZING_TYPE_GROW, 0 )
        e.layout.padding = UI.CreatePadding( 16 )
        e.backgroundColor = UI.Color( 224, 215, 210, 255 )
        UI.ConfigureElement( e )
        UI.CloseElement()
    UI.CloseElement()
end