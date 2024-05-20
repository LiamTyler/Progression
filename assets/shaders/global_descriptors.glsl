#ifndef _GLOBAL_DESCRIPTORS_INCLUDE
#define _GLOBAL_DESCRIPTORS_INCLUDE

#include "common.glsl"
#include "c_shared/structs.h"

#define DEFINE_GLOBAL_DESCRIPTORS() \
    layout( set = 0, binding = 0 ) uniform readonly image2D readOnlyImages[]; \
    layout( set = 0, binding = 1 ) uniform image2D rwImages[]; \
    layout( set = 0, binding = 2 ) uniform SceneGlobalsUBO \
    { \
        SceneGlobals globals; \
    }

#endif // #ifndef _GLOBAL_DESCRIPTORS_INCLUDE