
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
