
material = nil

function Start()
    material = AssetManager.GetMaterial( "white" )
    -- remove because default roughness and metalness are not 1.0,
    --   so the multiply tint would be messed up
    material.albedoMetalnessImage = nil
    material.normalRoughnessImage = nil
    material.albedoTint = vec3.new( 1 )
    material.roughnessTint = 0
    material.metalnessTint = 0
end

function Update()
    if Input.GetKeyUp( Key.RIGHT ) then
        material.roughnessTint = math.min( 1.0, material.roughnessTint + 0.1 )
    end
    if Input.GetKeyUp( Key.LEFT ) then
        material.roughnessTint = math.max( 0.0, material.roughnessTint - 0.1 )
    end
    
    if Input.GetKeyUp( Key.UP ) then
        material.metalnessTint = math.min( 1.0, material.metalnessTint + 0.1 )
    end
    if Input.GetKeyUp( Key.DOWN ) then
        material.metalnessTint = math.max( 0.0, material.metalnessTint - 0.1 )
    end
end