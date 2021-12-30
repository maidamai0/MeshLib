#pragma once

#include "MRMeshFwd.h"

namespace MR
{

/**
 * \brief Segment voxels of given volume on two sets using graph-cut, returning source set
 * \ingroup VolumeSegmentationGroup
 * \param k - coefficient in the exponent of the metric affecting edge capacity:\n
 *        increasing k you force to find a higher steps in the density on the boundary, decreasing k you ask for smoother boundary
 * \param sourceSeeds - these voxels will be included in the result
 * \param sinkSeeds - these voxels will be excluded from the result
 * 
 * \sa \ref VolumeSegmenter
 */
MRMESH_API VoxelBitSet segmentVolumeByGraphCut( const SimpleVolume & densityVolume, float k, const VoxelBitSet & sourceSeeds, const VoxelBitSet & sinkSeeds );

} // namespace MR
