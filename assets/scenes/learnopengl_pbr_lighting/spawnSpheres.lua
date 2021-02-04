local model = AssetManager.GetModel( "sphere" )
local USE_RED_SPHERES = true

if USE_RED_SPHERES then

	materials = {}
	local numRows = 7
	local numCols = 7
	local spacing = 2.5
	for row=0,numRows-1 do
		local metalness = row / numRows
		for col=0,numCols-1 do
			local roughness = math.max( 0.05, math.min( 1.0, col / numCols ) )
			local mat = Material.new()
			mat.name = tostring( row * 10 + col )
			mat.albedoTint = vec3.new( 0.5, 0, 0 )
			mat.metalnessTint = metalness
			mat.roughnessTint = roughness
			materials[row*numRows + col] = mat
			
			local e = ECS:CreateEntity()
			local transform = ECS:Create_Transform( e )
			transform.position = vec3.new( (col - (numCols / 2)) * spacing, (row - (numRows / 2)) * spacing, 0 );
			local modelRenderer = ECS:Create_ModelRenderer( e )
			modelRenderer.model = model
			modelRenderer.materials:add( mat )
		end
	end
	
else
	local mat = AssetManager.GetMaterial( "rusted_iron" )
	local e = ECS:CreateEntity()
	local transform = ECS:Create_Transform( e )
	transform.position = vec3.new( 0, 0, 0 );
	local modelRenderer = ECS:Create_ModelRenderer( e )
	modelRenderer.model = model
	modelRenderer.materials:add( mat )
	
	scene.camera.position = vec3.new( 0, 0, 5 )
end