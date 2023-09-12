#pragma once

namespace PG
{

enum class PModelVersionNum : unsigned int
{
    BITANGENT_SIGNS = 1,

    TOTAL,
    CURRENT_VERSION = TOTAL - 1
};

} // namespace PG