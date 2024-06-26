#ifndef __DEFINES_H__
#define __DEFINES_H__

#if PG_SHADER_CODE

#define PI 3.1415926535f

#define BEGIN_GPU_DATA_NAMESPACE()
#define END_GPU_DATA_NAMESPACE()

#else // #if PG_SHADER_CODE

#include "shared/math.hpp"

#define BEGIN_GPU_DATA_NAMESPACE() namespace GpuData {
#define END_GPU_DATA_NAMESPACE() } // namespace GpuData

#endif // #if PG_SHADER_CODE

#endif // #ifndef __DEFINES_H__
