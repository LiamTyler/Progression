#ifndef __COMMON_GLSL__
#define __COMMON_GLSL__

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_image_load_formatted : require
#extension GL_ARB_shader_image_size : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#define DEFINE_BUFFER_REFERENCE( alignment ) layout(scalar, buffer_reference, buffer_reference_align = alignment) buffer
    
#endif // #ifndef __COMMON_GLSL__