
function Start()
    --material = AssetManager.GetMaterial( "white" )
end

function Update()
    UI.OpenElement()
    e = UI.ElementDeclaration.new()
    e.id = UI.CreateID( "Parent" )
    e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_GROW, 0, UI.SIZING_TYPE_GROW, 0 )
    e.layout.childGap = 16
    e.layout.padding = UI.CreatePadding( 16 )
    e.layout.layoutDirection = UI.TOP_TO_BOTTOM;
    e.backgroundColor = UI.Color( 0, 200, 20, 255 )
    UI.ConfigureElement( e )
        UI.OpenElement()
        e.id = UI.CreateID( "Side1" )
        e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_FIXED, 400, UI.SIZING_TYPE_GROW, 0 )
        e.backgroundColor = UI.Color( 224, 215, 210, 255 )
        UI.ConfigureElement( e )
        UI.CloseElement()
        
        UI.OpenElement()
        e.id = UI.CreateID( "Side2" )
        e.layout.sizing = UI.CreateSizing( UI.SIZING_TYPE_FIXED, 400, UI.SIZING_TYPE_GROW, 0 )
        --e.backgroundColor = UI.Color( 128, 128, 210, 255 )
        e.image = UI.GetImage( "macaw" )
        UI.ConfigureElement( e )
        UI.CloseElement()
    UI.CloseElement()
end