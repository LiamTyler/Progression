#ifndef __MESH_SHADING_DEFINES_H__
#define __MESH_SHADING_DEFINES_H__

#include "c_shared/defines.h"

#define INDIRECT_MAIN_DRAW PG_USE
#define TASK_SHADER_MAIN_DRAW PG_USE

#define MESH_SHADER_WORKGROUP_SIZE 64
#define TASK_SHADER_WORKGROUP_SIZE 32

#if PG_SHADER_CODE

#include "common.glsl"

struct TaskOutput
{
#if INDIRECT_MAIN_DRAW
    uint meshIndex;   
#endif // #if INDIRECT_MAIN_DRAW
    uint baseID;
    uint8_t deltaIDs[TASK_SHADER_WORKGROUP_SIZE];
};

#endif // #if PG_SHADER_CODE

#endif // #ifndef __MESH_SHADING_DEFINES_H__
