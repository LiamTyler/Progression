#pragma once

#include "shared/math.hpp"

namespace PT
{

enum class TonemapOperator
{
    NONE,
    REINHARD,
    UNCHARTED2,
    ACES,

    COUNT
};

TonemapOperator TonemapOperatorFromString( const std::string& name );

using TonemapFunc = glm::vec3(*)( const glm::vec3& v );

TonemapFunc GetTonemapFunction( TonemapOperator op );

} // namepsace PT