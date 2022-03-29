#pragma once

#include "glm/glm.hpp"
#include <string>

bool Save2D_U8( const std::string& filename, unsigned char* pixels, int width, int height, int channels );

bool Save2D_F16( const std::string& filename, unsigned short* pixels, int width, int height, int channels );

bool Save2D_F32( const std::string& filename, float* pixels, int width, int height, int channels );