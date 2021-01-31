e = ECS:CreateEntity()

mat = AssetManager.GetMaterial( "red" )
model = AssetManager.GetModel( "sphere" )

transform = ECS:Create_Transform( e )
transform.position = vec3.new( 0, 0, -5 );
modelRenderer = ECS:Create_ModelRenderer( e )
modelRenderer.model = model
modelRenderer.materials:add( mat )
