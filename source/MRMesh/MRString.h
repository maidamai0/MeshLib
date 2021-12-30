#pragma once
#include "MRMeshFwd.h"
#include <string>
#include <typeinfo>

namespace MR
{

// finding the substring in the string. Returns false if substring is not exists
MRMESH_API bool findSubstring( const std::string& string, const std::string& substring );

}
