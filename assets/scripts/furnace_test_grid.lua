
sphereModel = AssetManager.GetModel( "uv_sphere_4k" )
materials = {}

for metalness = 0,1 do
    for i=0,10 do
        e = ECS:CreateEntity()
        transform = ECS:Create_Transform( e )
        transform.position = vec3.new( 2 * i - 10, 0, 1 - 2 * metalness )
        
        material = Material.new()
        material.albedoTint = vec3.new( 1, 1, 1 )
        material.metalnessTint = metalness
        material.roughnessTint = i / 10.0
        materials[i] = material
        
        modelRenderer = ECS:Create_ModelRenderer( e )
        modelRenderer.model = sphereModel
        modelRenderer.materials = { material }
    end
end