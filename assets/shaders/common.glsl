#ifndef __COMMON_GLSL__
#define __COMMON_GLSL__

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_image_load_formatted : require
#extension GL_ARB_shader_image_size : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#define DEFINE_BUFFER_REFERENCE( alignment ) layout( scalar, buffer_reference, buffer_reference_align = alignment ) buffer

#define DEFINE_STANDARD_BUFFER_REFERENCE( alignment, name, type ) layout( scalar, buffer_reference, buffer_reference_align = alignment ) buffer name \
{ \
    type data[]; \
}

DEFINE_STANDARD_BUFFER_REFERENCE( 8, FloatBuffer, float );
DEFINE_STANDARD_BUFFER_REFERENCE( 8, Vec2Buffer, vec2 );
DEFINE_STANDARD_BUFFER_REFERENCE( 8, Vec3Buffer, vec3 );
DEFINE_STANDARD_BUFFER_REFERENCE( 8, Vec4Buffer, vec4 );
DEFINE_STANDARD_BUFFER_REFERENCE( 8, ByteBuffer, uint8_t );
DEFINE_STANDARD_BUFFER_REFERENCE( 8, UshortBuffer, uint16_t );
DEFINE_STANDARD_BUFFER_REFERENCE( 8, UintBuffer, uint );
    
#endif // #ifndef __COMMON_GLSL__