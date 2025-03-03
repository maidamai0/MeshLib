#if !defined( __EMSCRIPTEN__) && !defined( MRMESH_NO_VOXEL )
#include "MRObjectVoxels.h"
#include "MRObjectFactory.h"
#include "MRMesh.h"
#include "MRVDBConversions.h"
#include "MRFloatGrid.h"
#include "MRSimpleVolume.h"
#include "MRVoxelsSave.h"
#include "MRVoxelsLoad.h"
#include "MRSerializer.h"
#include "MRMeshNormals.h"
#include "MRTimer.h"
#include "MRPch/MRJson.h"
#include "MRSceneColors.h"
#include "MRStringConvert.h"
#include "MRPch/MRTBB.h"
#include "MRPch/MRAsyncLaunchType.h"
#include <filesystem>

namespace MR
{

MR_ADD_CLASS_FACTORY( ObjectVoxels )

constexpr size_t cVoxelsHistogramBinsNumber = 256;

void ObjectVoxels::construct( const SimpleVolume& volume, const ProgressCallback& cb )
{
    mesh_.reset();
    grid_ = simpleVolumeToDenseGrid( volume, cb );
    dimensions_ = volume.dims;
    indexer_ = VolumeIndexer( dimensions_ );
    activeBox_ = Box3i( Vector3i(), dimensions_ );
    voxelSize_ = volume.voxelSize;
    reverseVoxelSize_ = { 1 / voxelSize_.x,1 / voxelSize_.y,1 / voxelSize_.z };
    updateHistogram_( volume.min, volume.max );
}

void ObjectVoxels::construct( const FloatGrid& grid, const Vector3f& voxelSize, const ProgressCallback& cb )
{
    if ( !grid )
        return;
    grid_ = grid;

    auto vdbDims = grid_->evalActiveVoxelDim();
    dimensions_ = {vdbDims.x(),vdbDims.y(),vdbDims.z()};
    indexer_ = VolumeIndexer( dimensions_ );
    activeBox_ = Box3i( Vector3i(), dimensions_ );
    voxelSize_ = voxelSize;
    reverseVoxelSize_ = { 1 / voxelSize_.x,1 / voxelSize_.y,1 / voxelSize_.z };

    updateHistogramAndSurface( cb );
}

void ObjectVoxels::updateHistogramAndSurface( const ProgressCallback& cb )
{
    if ( !grid_ )
        return;

    float min{0.0f}, max{0.0f};

#if OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER >= 9 && (OPENVDB_LIBRARY_MINOR_VERSION_NUMBER >= 1 || OPENVDB_LIBRARY_PATCH_VERSION_NUMBER >= 1)
    auto minMax = openvdb::tools::minMax(grid_->tree());
    min = minMax.min();
    max = minMax.max();
#else
    grid_->evalMinMax( min, max );
#endif

    updateHistogram_( min, max );

    if ( mesh_ )
    {
        mesh_.reset();
        setIsoValue( isoValue_, cb );
    }
}

bool ObjectVoxels::setIsoValue( float iso, const ProgressCallback& cb, bool updateSurface )
{
    if ( !grid_ )
        return false; // no volume presented in this
    if ( mesh_ && iso == isoValue_ )
        return false; // current iso surface represents required iso value

    isoValue_ = iso;
    if ( updateSurface )
        updateIsoSurface( recalculateIsoSurface( isoValue_, cb ) );
    return updateSurface;
}

std::shared_ptr<Mesh> ObjectVoxels::updateIsoSurface( std::shared_ptr<Mesh> mesh )
{
    if ( mesh != mesh_ )
    {
        mesh_.swap( mesh );
        setDirtyFlags( DIRTY_ALL );
        isoSurfaceChangedSignal();
    }
    return mesh;

}

std::shared_ptr<Mesh> ObjectVoxels::recalculateIsoSurface( float iso, const ProgressCallback& cb /*= {} */ )
{
    if ( !grid_ )
        return {};
    auto meshRes = gridToMesh( grid_, voxelSize_, maxSurfaceTriangles_, iso, 0.0f, cb );

    FloatGrid downsampledGrid = grid_;
    while ( !meshRes.has_value() )
    {
        downsampledGrid = resampled( downsampledGrid, 2.0f );
        meshRes = gridToMesh( downsampledGrid, 2.0f * voxelSize_, maxSurfaceTriangles_, iso, 0.0f, cb );
    }
    return std::make_shared<Mesh>( std::move( meshRes.value() ) );
}

void ObjectVoxels::setActiveBounds( const Box3i& activeBox, const ProgressCallback& cb, bool updateSurface )
{
    if ( !grid_ )
        return;
    if ( !activeBox.valid() )
        return;

    activeBox_ = activeBox;
    auto accessor = grid_->getAccessor();

    int counter = 0;
    float volume = float( dimensions_.x ) * dimensions_.y * dimensions_.z;
    float cbModifier = updateSurface ? 0.5f : 1.0f;

    bool insideX = false;
    bool insideY = false;
    bool insideZ = false;
    for ( int z = 0; z < dimensions_.z; ++z )
    for ( int y = 0; y < dimensions_.y; ++y )
    for ( int x = 0; x < dimensions_.x; ++x )
    {
        insideX = ( x >= activeBox_.min.x && x < activeBox_.max.x );
        insideY = ( y >= activeBox_.min.y && y < activeBox_.max.y );
        insideZ = ( z >= activeBox_.min.z && z < activeBox_.max.z );
        accessor.setActiveState( {x,y,z}, insideX && insideY && insideZ );
        ++counter;
        if ( cb && ( counter % 256 == 0 ) )
            cb( cbModifier * float( counter ) / volume );
    }
    if ( updateSurface )
    {
        ProgressCallback isoProgressCallback;
        if ( cb )
            isoProgressCallback = [&] ( float p )->bool
        {
            return cb( cbModifier + ( 1.0f - cbModifier ) * p );
        };

        updateIsoSurface( recalculateIsoSurface( isoValue_, isoProgressCallback ) );
    }
}

VoxelId ObjectVoxels::getVoxelIdByCoordinate( const Vector3i& coord ) const
{
    return indexer_.toVoxelId( coord );
}

VoxelId ObjectVoxels::getVoxelIdByPoint( const Vector3f& point ) const
{
    return getVoxelIdByCoordinate( Vector3i( mult( point, reverseVoxelSize_ ) ) );
}

Vector3i ObjectVoxels::getCoordinateByVoxelId( VoxelId id ) const
{
    return indexer_.toPos( id );
}

void ObjectVoxels::setMaxSurfaceTriangles( int maxFaces )
{
    if ( maxFaces == maxSurfaceTriangles_ )
        return;
    maxSurfaceTriangles_ = maxFaces;
    if ( !mesh_ || mesh_->topology.numValidFaces() <= maxSurfaceTriangles_ )
        return;
    mesh_.reset();
    setIsoValue( isoValue_ );
}

std::shared_ptr<Object> ObjectVoxels::clone() const
{
    auto res = std::make_shared<ObjectVoxels>( ProtectedStruct{}, *this );
    if ( mesh_ )
        res->mesh_ = std::make_shared<Mesh>( *mesh_ );
    if ( grid_ )
        res->grid_ = MakeFloatGrid( grid_->deepCopy() );
    return res;
}

std::shared_ptr<Object> ObjectVoxels::shallowClone() const
{
    auto res = std::make_shared<ObjectVoxels>( ProtectedStruct{}, *this );
    if ( mesh_ )
        res->mesh_ = mesh_;
    if ( grid_ )
        res->grid_ = grid_;
    return res;
}

void ObjectVoxels::setDirtyFlags( uint32_t mask )
{
    VisualObject::setDirtyFlags( mask );

    if ( ( mask & DIRTY_POSITION || mask & DIRTY_FACE ) && mesh_ )
        mesh_->invalidateCaches();
}

size_t ObjectVoxels::heapBytes() const
{
    return ObjectMeshHolder::heapBytes()
        + ( grid_ ? sizeof( *grid_ ) + grid_->memUsage() : 0 )
        + histogram_.heapBytes();
}

void ObjectVoxels::swapBase_( Object& other )
{
    if ( auto otherVoxels = other.asType<ObjectVoxels>() )
        std::swap( *this, *otherVoxels );
    else
        assert( false );
}

void ObjectVoxels::swapSignals_( Object& other )
{
    ObjectMeshHolder::swapSignals_( other );
    if ( auto otherVoxels = other.asType<ObjectVoxels>() )
        std::swap( isoSurfaceChangedSignal, otherVoxels->isoSurfaceChangedSignal );
    else
        assert( false );
}

class HistogramCalc
{
public:
    HistogramCalc( const ObjectVoxels& obj, float min, float max ) :
        hist{Histogram( min,max,cVoxelsHistogramBinsNumber )}, grid_{obj.grid()}, indexer_{obj.getVolumeIndexer()}
    {
    }
    HistogramCalc( HistogramCalc& x, tbb::split ) : 
        hist{Histogram( x.hist.getMin(),x.hist.getMax(),cVoxelsHistogramBinsNumber )}, grid_{x.grid_}, indexer_{x.indexer_}
    {
    }
    void join( const HistogramCalc& y )
    {
        hist.addHistogram( y.hist );
    }

    void operator()( const tbb::blocked_range<VoxelId>& r )
    {
        auto accessor = grid_->getConstAccessor();
        for ( VoxelId v = r.begin(); v < r.end(); ++v )
        {
            auto pos = indexer_.toPos( v );
            hist.addSample( accessor.getValue( {pos.x,pos.y,pos.z} ) );
        }
    }

    Histogram hist;
private:
    const FloatGrid& grid_;
    const VolumeIndexer& indexer_;
};

void ObjectVoxels::updateHistogram_( float min, float max )
{
    MR_TIMER;
    auto size = indexer_.sizeXY()* dimensions_.z;

    HistogramCalc calc( *this, min, max );
    parallel_reduce( tbb::blocked_range<VoxelId>( VoxelId( 0 ), VoxelId( size ) ), calc );
    histogram_ = std::move( calc.hist );
}

void ObjectVoxels::setDefaultColors_()
{
    setFrontColor( SceneColors::get( SceneColors::SelectedObjectVoxels ), true );
    setFrontColor( SceneColors::get( SceneColors::UnselectedObjectVoxels ), false );
}

ObjectVoxels::ObjectVoxels( const ObjectVoxels& other ) :
    ObjectMeshHolder( other )
{
    dimensions_ = other.dimensions_;
    isoValue_ = other.isoValue_;
    histogram_ = other.histogram_;
    voxelSize_ = other.voxelSize_;
    activeBox_ = other.activeBox_;

    indexer_ = other.indexer_;
    reverseVoxelSize_ = other.reverseVoxelSize_;
}

ObjectVoxels::ObjectVoxels()
{
    setDefaultColors_();
}

void ObjectVoxels::applyScale( float scaleFactor )
{
    voxelSize_ *= scaleFactor;

    ObjectMeshHolder::applyScale( scaleFactor );
}

void ObjectVoxels::serializeFields_( Json::Value& root ) const
{
    VisualObject::serializeFields_( root );
    serializeToJson( voxelSize_, root["VoxelSize"] );

    serializeToJson( dimensions_, root["Dimensions"] );
    serializeToJson( activeBox_.min, root["MinCorner"] );
    serializeToJson( activeBox_.max, root["MaxCorner"] );
    serializeToJson( selectedVoxels_, root["SelectionVoxels"] );

    root["IsoValue"] = isoValue_;
    root["Type"].append( ObjectVoxels::TypeName() );
}

tl::expected<std::future<void>, std::string> ObjectVoxels::serializeModel_( const std::filesystem::path& path ) const
{
    if ( ancillary_ || !grid_ )
        return {};

    return std::async( getAsyncLaunchType(),
        [this, filename = utf8string( path ) + ".raw"]() { MR::VoxelsSave::saveRAW( filename, *this ); } );
}

void ObjectVoxels::deserializeFields_( const Json::Value& root )
{
    VisualObject::deserializeFields_( root );
    if ( root["VoxelSize"].isDouble() )
        voxelSize_ = Vector3f::diagonal( ( float )root["VoxelSize"].asDouble() );
    else
        deserializeFromJson( root["VoxelSize"], voxelSize_ );

    deserializeFromJson( root["Dimensions"], dimensions_ );

    deserializeFromJson( root["MinCorner"], activeBox_.min );

    deserializeFromJson( root["MaxCorner"], activeBox_.max );

    deserializeFromJson( root["SelectionVoxels"], selectedVoxels_ );

    if ( root["IsoValue"].isNumeric() )
        isoValue_ = root["IsoValue"].asFloat();

    if ( !activeBox_.valid() )
        activeBox_ = Box3i( Vector3i(), dimensions_ );

    if ( activeBox_.min != Vector3i() || activeBox_.max != dimensions_ )
        setActiveBounds( activeBox_ );
    else 
        setIsoValue( isoValue_ );
}

#ifndef MRMESH_NO_DICOM
tl::expected<void, std::string> ObjectVoxels::deserializeModel_( const std::filesystem::path& path, ProgressCallback progressCb )
{
    auto res = VoxelsLoad::loadRaw( utf8string( path ) + ".raw", progressCb );
    if ( !res.has_value() )
        return tl::make_unexpected( res.error() );
    
    construct( res.value() );
    if ( !grid_ )
        return tl::make_unexpected( "No grid loaded" );

    return {};
}
#endif

std::vector<std::string> ObjectVoxels::getInfoLines() const
{
    std::vector<std::string> res = ObjectMeshHolder::getInfoLines();
    res.push_back( "dims: (" + std::to_string( dimensions_.x ) + ", " + std::to_string( dimensions_.y ) + ", " + std::to_string( dimensions_.z ) + ")" );
    res.push_back( "voxel size: (" + std::to_string( voxelSize_.x ) + ", " + std::to_string( voxelSize_.y ) + ", " + std::to_string( voxelSize_.z ) + ")" );
    res.push_back( "iso-value: " + std::to_string( isoValue_ ) );
    return res;
}

}
#endif
