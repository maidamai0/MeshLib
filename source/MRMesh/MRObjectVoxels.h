#pragma once
#if !defined( __EMSCRIPTEN__) && !defined( MRMESH_NO_VOXEL )
#include "MRMeshFwd.h"
#include "MRObjectMeshHolder.h"
#include "MRProgressCallback.h"
#include "MRHistogram.h"
#include "MRVolumeIndexer.h"

namespace MR
{

/// This class stores information about voxels object
/// \ingroup DataModelGroup
class MRMESH_CLASS ObjectVoxels : public ObjectMeshHolder
{
public:
    MRMESH_API ObjectVoxels();
    ObjectVoxels& operator = ( ObjectVoxels&& ) noexcept = default;
    ObjectVoxels( ObjectVoxels&& ) noexcept = default;
    virtual ~ObjectVoxels() = default;

    constexpr static const char* TypeName() noexcept { return "ObjectVoxels"; }
    virtual const char* typeName() const override { return TypeName(); }

    MRMESH_API virtual void applyScale( float scaleFactor ) override;

    /// Returns iso surface, empty if iso value is not set
    const std::shared_ptr<Mesh>& surface() const { return mesh_; }

    /// Returns Float grid which contains voxels data, see more on openvdb::FloatGrid
    const FloatGrid& grid() const
    { return grid_; }
    /// Returns dimensions of voxel objects
    const Vector3i& dimensions() const
    { return dimensions_; }
    /// Returns current iso value
    float getIsoValue() const
    { return isoValue_; }
    /// Returns histogram
    const Histogram& histogram() const
    { return histogram_; }

    const Vector3f& voxelSize() const
    { return voxelSize_; }

    MRMESH_API virtual std::vector<std::string> getInfoLines() const override;
    virtual std::string getClassName() const override { return "Voxels"; }

    /// Clears all internal data and then creates grid and calculates histogram
    MRMESH_API void construct( const SimpleVolume& volume, const ProgressCallback& cb = {} );
    /// Clears all internal data and calculates histogram
    MRMESH_API void construct( const FloatGrid& grid, const Vector3f& voxelSize, const ProgressCallback& cb = {} );
    /// Updates histogram, by stored grid (evals min and max values from grid)
    /// rebuild iso surface if it is present
    MRMESH_API void updateHistogramAndSurface( const ProgressCallback& cb = {} );

    /// Sets iso value and updates iso-surfaces if needed: 
    /// Returns true if iso-value was updated, false - otherwise
    MRMESH_API virtual bool setIsoValue( float iso, const ProgressCallback& cb = {}, bool updateSurface = true );

    /// Sets external surface mesh for this object
    /// and returns back previous mesh of this
    MRMESH_API std::shared_ptr<Mesh> updateIsoSurface( std::shared_ptr<Mesh> mesh );

    /// Calculates and return new mesh
    /// returns empty pointer if no volume is present
    MRMESH_API std::shared_ptr<Mesh> recalculateIsoSurface( float iso, const ProgressCallback& cb = {} );

    /// Sets active bounds for some simplifications (max excluded)
    /// active bounds is box in voxel coordinates, note that voxels under (0,0,0) and voxels over (dimensions) are empty 
    MRMESH_API virtual void setActiveBounds( const Box3i& activeBox, const ProgressCallback& cb = {}, bool updateSurface = true );
    /// Returns active bounds (max excluded)
    /// active bounds is box in voxel coordinates, note that voxels under (0,0,0) and voxels over (dimensions) are empty 
    const Box3i& getActiveBounds() const
    { return activeBox_; }

    const VoxelBitSet& getSelectedVoxels() const { return selectedVoxels_; }
    void selectVoxels( const VoxelBitSet& selectedVoxels ) { selectedVoxels_ = selectedVoxels; }

    /// VoxelId is numerical representation of voxel
    /// Coordinate is {x,y,z} indices of voxels in box (base dimensions space, NOT active dimensions)
    /// Point is local space coordinate of point in scene
    MRMESH_API VoxelId getVoxelIdByCoordinate( const Vector3i& coord ) const;
    MRMESH_API VoxelId getVoxelIdByPoint( const Vector3f& point ) const;
    MRMESH_API Vector3i getCoordinateByVoxelId( VoxelId id ) const;

    /// Returns indexer with more options
    const VolumeIndexer& getVolumeIndexer() const { return indexer_; }

    MRMESH_API void setMaxSurfaceTriangles( int maxFaces );
    int getMaxSurfaceTriangles() const { return maxSurfaceTriangles_; }

    MRMESH_API virtual std::shared_ptr<Object> clone() const override;
    MRMESH_API virtual std::shared_ptr<Object> shallowClone() const override;

    MRMESH_API virtual void setDirtyFlags( uint32_t mask ) override;

    /// \note this ctor is public only for std::make_shared used inside clone()
    ObjectVoxels( ProtectedStruct, const ObjectVoxels& obj ) : ObjectVoxels( obj ) {}

    /// returns the amount of memory this object occupies on heap
    [[nodiscard]] MRMESH_API virtual size_t heapBytes() const override;

    /// signal about ISO changing, triggered in when iso surface updates (setIsoValue, updateIsoSurface)
    using IsoChangedSignal = boost::signals2::signal<void()>;
    IsoChangedSignal isoSurfaceChangedSignal;

private:
    int maxSurfaceTriangles_{ 10000000 };
    FloatGrid grid_;
    Vector3i dimensions_;
    float isoValue_{0.0f};
    Histogram histogram_;
    Vector3f voxelSize_;
    Box3i activeBox_;

    /// Service data
    VolumeIndexer indexer_ = VolumeIndexer( dimensions_ );
    Vector3f reverseVoxelSize_;

    void updateHistogram_( float min, float max );


    /// this is private function to set default colors of this type (ObjectVoxels) in constructor only
    void setDefaultColors_();

protected:
    VoxelBitSet selectedVoxels_;

    MRMESH_API ObjectVoxels( const ObjectVoxels& other );

    /// swaps this object with other
    MRMESH_API virtual void swapBase_( Object& other ) override;
    /// swaps signals, used in `swap` function to return back signals after `swapBase_`
    /// pls call Parent::swapSignals_ first when overriding this function
    MRMESH_API virtual void swapSignals_( Object& other ) override;

    MRMESH_API virtual void serializeFields_( Json::Value& root ) const override;

    MRMESH_API void deserializeFields_( const Json::Value& root ) override;

#ifndef MRMESH_NO_DICOM
    MRMESH_API tl::expected<void, std::string> deserializeModel_( const std::filesystem::path& path, ProgressCallback progressCb = {} ) override;
#endif

    MRMESH_API virtual tl::expected<std::future<void>, std::string> serializeModel_( const std::filesystem::path& path ) const override;
};


}
#endif
