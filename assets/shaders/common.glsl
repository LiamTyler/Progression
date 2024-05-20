#ifndef _COMMON_INCLUDE
#define _COMMON_INCLUDE

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_image_load_formatted : enable
#extension GL_ARB_shader_image_size : enable

#define DEFINE_BUFFER_REFERENCE( alignment ) \
    layout(scalar, buffer_reference, buffer_reference_align = alignment) buffer
    
#endif // #ifndef _COMMON_INCLUDE