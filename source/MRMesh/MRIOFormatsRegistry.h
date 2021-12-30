#pragma once
#include "MRMeshFwd.h"
#include "MRIOFilters.h"
#include <tl/expected.hpp>
#include <filesystem>

namespace MR
{

namespace MeshLoad
{
using MeshLoader = tl::expected<MR::Mesh, std::string>( * )( const std::filesystem::path&, std::vector<Color>* );

struct NamedMeshLoader
{
    IOFilter filter;
    MeshLoader loader{nullptr};
};

// Finds expected loader from registry
MRMESH_API MeshLoader getMeshLoader( IOFilter filter );
// Gets all registered filters
MRMESH_API IOFilters getFilters();

// Register filter with loader function
// loader function signature: tl::expected<Mesh, std::string> fromFormat( const std::filesystem::path& path, std::vector<Color>* colors );
// example:
// ADD_MESH_LOADER( IOFilter("Name of filter (.ext)","*.ext"), fromFormat)
#define MR_ADD_MESH_LOADER( filter, loader ) \
MR::MeshLoad::MeshLoaderAdder __meshLoaderAdder_##loader(MR::MeshLoad::NamedMeshLoader{filter,static_cast<MR::MeshLoad::MeshLoader>(loader)});\

class MeshLoaderAdder
{
public:
    MRMESH_API MeshLoaderAdder( const NamedMeshLoader& loader );
};

}

}
