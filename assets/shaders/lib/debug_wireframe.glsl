#ifndef __DEBUG_WIREFRAME_GLSL__
#define __DEBUG_WIREFRAME_GLSL__

#extension GL_EXT_fragment_shader_barycentric : require

// https://wunkolo.github.io/post/2022/07/gl_ext_fragment_shader_barycentric-wireframe/
float WireFrame( in float Thickness, in float Falloff )
{
	const vec3 BaryCoord = gl_BaryCoordEXT;

	const vec3 dBaryCoordX = dFdxFine(BaryCoord);
	const vec3 dBaryCoordY = dFdyFine(BaryCoord);
	const vec3 dBaryCoord  = sqrt(dBaryCoordX*dBaryCoordX + dBaryCoordY*dBaryCoordY);

	const vec3 dFalloff   = dBaryCoord * Falloff;
	const vec3 dThickness = dBaryCoord * Thickness;

	const vec3 Remap = smoothstep(dThickness, dThickness + dFalloff, BaryCoord);
	const float ClosestEdge = min(min(Remap.x, Remap.y), Remap.z);

	return 1.0 - ClosestEdge;
}

#ifdef __GLOBAL_DESCRIPTORS_GLSL__
vec4 ProcessStandardWireframe( vec4 color )
{
    const vec4 wireframeColor = vec4( globals.debug_wireframeData.rgb, 1 );
    const float wireframeThickness = globals.debug_wireframeData.a;
    float wireframeVal = WireFrame( wireframeThickness, 0.0 );
    color = mix( color, wireframeColor, wireframeVal );
    return color;
}
#endif // __GLOBAL_DESCRIPTORS_GLSL__

#endif // #ifndef __DEBUG_WIREFRAME_GLSL__