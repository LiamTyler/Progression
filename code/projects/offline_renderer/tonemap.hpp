#pragma once

#include "shared/math.hpp"

namespace PT
{

glm::vec3 GammaCorrect( const glm::vec3& pixel, float gamma = 2.2 );
glm::vec3 PBRTGammaCorrect( const glm::vec3& pixel );
glm::vec3 ReinhardTonemap( const glm::vec3& pixel, float exposure = 1 );
glm::vec3 Uncharted2Tonemap( const glm::vec3& pixel, float exposure = 1 );

} // namepsace PT