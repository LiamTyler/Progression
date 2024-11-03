
NUM_LIGHTS = 4
MODELS_PER_LIGHT = 1
SPACING = 30

sphereModel = AssetManager.GetModel( "uv_sphere_4k" )
material = AssetManager.GetMaterial( "blue" )

function Start()
    start = -NUM_LIGHTS // 2
    if NUM_LIGHTS % 2 == 0 then
        start = start + 0.5
    end
    start = start * SPACING
    
    for lightIdx=0,NUM_LIGHTS-1 do
        light = scene:AddSpotLight()
        light.position = vec3.new( start + lightIdx * SPACING, 0, 20 )
        light.direction = vec3.new( 0, 0, -1 )
        light.radius = 50
        light.intensity = 1000
        light.castsShadow = true
        
        for modelIdx=1,MODELS_PER_LIGHT do
            e = ECS:CreateEntity()
            transform = ECS:Create_Transform( e )
            transform.position = light.position
            transform.position = transform.position - vec3.new( 0, 0, 20 )
            transform.scale = vec3.new( 5, 5, 5 )
            
            modelRenderer = ECS:Create_ModelRenderer( e )
            modelRenderer.model = sphereModel
            modelRenderer.materials:add( material )
        end
    end
end