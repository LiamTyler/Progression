#pragma once

#include "shared/math_vec.hpp"
#include <string>

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

using TonemapFunc = vec3(*)( const vec3& v );

TonemapFunc GetTonemapFunction( TonemapOperator op );

} // namepsace PT