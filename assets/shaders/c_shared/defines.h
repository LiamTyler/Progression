#define PG_SHADER_EPSILON 0.00001

#define PG_MAX_NUM_TEXTURES 2048
#define PG_INVALID_TEXTURE_INDEX 65535

#define PG_SCENE_CONSTANT_BUFFER_SET 0
#define PG_2D_TEXTURES_SET 1
#define PG_BONE_TRANSFORMS_SET 2

#define PG_MATERIAL_PUSH_CONSTANT_OFFSET 192

#define PG_SSAO_KERNEL_SIZE 32

#define PG_SHADER_DEBUG_LAYER_REGULAR 0
#define PG_SHADER_DEBUG_LAYER_NO_SSAO 1
#define PG_SHADER_DEBUG_LAYER_SSAO_ONLY 2
#define PG_SHADER_DEBUG_LAYER_AMBIENT 3
#define PG_SHADER_DEBUG_LAYER_LIT_DIFFUSE 4
#define PG_SHADER_DEBUG_LAYER_LIT_SPECULAR 5
#define PG_SHADER_DEBUG_LAYER_POSITIONS 6
#define PG_SHADER_DEBUG_LAYER_NORMALS 7
#define PG_SHADER_DEBUG_LAYER_GBUFFER_DIFFUSE 8
#define PG_SHADER_DEBUG_LAYER_GBUFFER_SPECULAR 9

#ifdef PG_SHADER_CODE

#define VEC2 vec2
#define VEC3 vec3
#define VEC4 vec4
#define UVEC2 uvec2
#define UVEC3 uvec3
#define UVEC4 uvec4
#define IVEC2 ivec2
#define IVEC3 ivec3
#define IVEC4 ivec4
#define MAT3 mat3
#define MAT4 mat4
#define UINT uint

#else // #ifdef PG_CPP_CODE


#define VEC2 glm::vec2
#define VEC3 glm::vec3
#define VEC4 glm::vec4
#define UVEC2 glm::uvec2
#define UVEC3 glm::uvec3
#define UVEC4 glm::uvec4
#define IVEC2 glm::ivec2
#define IVEC3 glm::ivec3
#define IVEC4 glm::ivec4
#define MAT3 glm::mat3
#define MAT4 glm::mat4
#define UINT uint32_t

#endif // #else // #ifdef PG_CPP_CODE
