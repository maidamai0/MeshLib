#pragma once
#include "MRMeshFwd.h"
#include "MRIOFilters.h"
#include <tl/expected.hpp>
#include <filesystem>

namespace MR
{

namespace VoxelsSave
{

MRMESH_API extern const IOFilters Filters;

MRMESH_API tl::expected<void, std::string> saveRAW( const std::filesystem::path& path, const ObjectVoxels& voxelsObject );

}

}
