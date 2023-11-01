
material = nil

function Start()
    material = AssetManager.GetMaterial( "white" )
    -- remove because default roughness texture is 0.8, not 1.0. Instead uses roughness tint directly
    material.normalRoughnessImage = nil
end

function Update()
    if Input.GetKeyUp( Key.RIGHT ) then
        material.roughnessTint = math.min( 1.0, material.roughnessTint + 0.1 )
    end
    if Input.GetKeyUp( Key.LEFT ) then
        material.roughnessTint = math.max( 0.0, material.roughnessTint - 0.1 )
    end
end