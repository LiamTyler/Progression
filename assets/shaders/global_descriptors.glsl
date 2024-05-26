#ifndef __GLOBAL_DESCRIPTORS_GLSL__
#define __GLOBAL_DESCRIPTORS_GLSL__

#include "common.glsl"
#include "c_shared/bindless.h"
#include "c_shared/structs.h"

#define DEFINE_GLOBAL_DESCRIPTORS() \
    layout( set = 0, binding = PG_BINDLESS_READ_ONLY_TEXTURE_DSET_BINDING ) uniform readonly image2D readOnlyImages[]; \
    layout( set = 0, binding = PG_BINDLESS_RW_TEXTURE_DSET_BINDING ) uniform image2D rwImages[]; \
    layout( set = 0, binding = PG_SCENE_GLOBALS_DSET_BINDING ) uniform SceneGlobalsUBO \
    { \
        SceneGlobals globals; \
    }; \
    layout( scalar, set = 0, binding = PG_BINDLESS_BUFFER_DSET_BINDING ) readonly buffer BindlessBuffers \
    { \
        uint64_t bindlessBuffers[]; \
    }

#endif // #ifndef __GLOBAL_DESCRIPTORS_GLSL__