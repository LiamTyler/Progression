#pragma once

#include <string>
#include <vector>

namespace Progression
{

// returns true if dependency's last modified time is newer than file's last modified time
// if the dependency doesnt exist, will always return false
bool IsFileOutOfDate( const std::string& file, const std::string& dependency );

bool IsFileOutOfDate( const std::string& file, const std::vector< std::string >& dependencies );

} // namespace Progression