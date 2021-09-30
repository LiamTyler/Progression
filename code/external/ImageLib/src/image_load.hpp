#pragma once

#include "glm/glm.hpp"
#include <string>

glm::u8vec4* Load2D_U8( const std::string& filename, int& width, int& height );

glm::vec4* Load2D_F32( const std::string& filename, int& width, int& height );

glm::u16vec4* Load2D_F16( const std::string& filename, int& width, int& height );