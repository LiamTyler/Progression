#ifndef __DEFINES_H__
#define __DEFINES_H__

#if PG_SHADER_CODE

#define PI 3.1415926535f

#define BEGIN_GPU_DATA_NAMESPACE()
#define END_GPU_DATA_NAMESPACE()

#define PG_USE 1
#define PG_DONT_USE 0

#else // #if PG_SHADER_CODE

#include "shared/math.hpp"

#define BEGIN_GPU_DATA_NAMESPACE() namespace GpuData {
#define END_GPU_DATA_NAMESPACE() } // namespace GpuData

#define FloatBuffer  u64
#define Vec2Buffer   u64
#define Vec3Buffer   u64
#define Vec4Buffer   u64
#define ByteBuffer   u64
#define UshortBuffer u64
#define UintBuffer   u64

#define PG_USE IN_USE
#define PG_DONT_USE NOT_IN_USE

#endif // #else // #if PG_SHADER_CODE

#endif // #ifndef __DEFINES_H__
