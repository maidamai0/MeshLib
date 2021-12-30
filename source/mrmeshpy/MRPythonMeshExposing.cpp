#include "MRMesh/MRPython.h"
#include "MRMesh/MRMesh.h"
#include "MRMesh/MRId.h"
#include "MRMesh/MRVector.h"
#include "MRMesh/MRObjectsAccess.h"
#include "MRMesh/MRSceneRoot.h"
#include "MRMesh/MRObjectMesh.h"
#include "MRMesh/MRMeshFillHole.h"
#include "MRMesh/MRMeshComponents.h"
#include "MRMesh/MRCube.h"
#include "MRMesh/MRTorus.h"
#include "MRMesh/MRBoolean.h"
#include "MRMesh/MRMeshMetrics.h"
#include "MRMesh/MRMeshBuilder.h"


using namespace MR;

Mesh pythonGetSelectedMesh()
{
    auto selected = MR::getAllObjectsInTree<ObjectMesh>( &SceneRoot::get(), ObjectSelectivityType::Selected );
    if ( selected.size() != 1 )
        return {};
    if ( !selected[0] || !selected[0]->mesh() )
        return {};
    return *selected[0]->mesh();
}

void pythonSetMeshToSelected( Mesh mesh )
{
    auto selected = MR::getAllObjectsInTree<ObjectMesh>( &SceneRoot::get(), ObjectSelectivityType::Selected );
    if ( selected.size() != 1 )
        return;
    if ( !selected[0] )
        return;
    selected[0]->setMesh( std::make_shared<Mesh>( std::move( mesh ) ) );
    selected[0]->setDirtyFlags( DIRTY_ALL );
}

MR_ADD_PYTHON_FUNCTION( mrmeshpy, get_selected_mesh, pythonGetSelectedMesh, "gets mesh from selected ObjectMesh" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, set_mesh_to_selected, pythonSetMeshToSelected, "sets mesh to selected ObjectMesh" )

MR_ADD_PYTHON_CUSTOM_DEF( mrmeshpy, MeshTopology, [] ( pybind11::module_& m )
{
    pybind11::class_<MR::MeshTopology>( m, "MeshTopology" ).
        def( pybind11::init<>() ).
        def( "getValidFaces", &MR::MeshTopology::getValidFaces, pybind11::return_value_policy::copy ).
        def( "getValidVerts", &MR::MeshTopology::getValidVerts, pybind11::return_value_policy::copy ).
        def( "findBoundaryVerts", &MR::MeshTopology::findBoundaryVerts ).
        def( "findHoleRepresentiveEdges", &MR::MeshTopology::findHoleRepresentiveEdges ).
        def( "getTriVerts", ( void( MR::MeshTopology::* )( FaceId, VertId&, VertId&, VertId& )const )& MR::MeshTopology::getTriVerts );
} )

MR_ADD_PYTHON_CUSTOM_DEF( mrmeshpy, VertCoords, [] ( pybind11::module_& m )
{
    pybind11::class_<MR::VertCoords>( m, "VertCoords" ).
        def( pybind11::init<>() ).
        def_readwrite( "vec", &MR::VertCoords::vec_ );
} )

MR_ADD_PYTHON_CUSTOM_DEF( mrmeshpy, MeshBuilder, [] ( pybind11::module_& m )
{
    pybind11::class_<MR::MeshBuilder::Triangle>( m, "MeshBuilderTri").
        def( pybind11::init<VertId, VertId, VertId, FaceId>() );
    m.def( "topologyFromTriangles", &MR::MeshBuilder::fromTriangles, "constructs topology from given vecMeshBuilderTri" );
} )

MR_ADD_PYTHON_VEC( mrmeshpy, vecMeshBuilderTri, MR::MeshBuilder::Triangle )

MR_ADD_PYTHON_CUSTOM_DEF( mrmeshpy, Mesh, [] ( pybind11::module_& m )
{
    pybind11::class_<MR::Mesh>( m, "Mesh" ).
        def( pybind11::init<>() ).
        def( "computeBoundingBox", ( Box3f( MR::Mesh::* )( const FaceBitSet*, const AffineXf3f* ) const )& MR::Mesh::computeBoundingBox ).
        def_readwrite( "topology", &MR::Mesh::topology ).
        def_readwrite( "points", &MR::Mesh::points ).
        def( "invalidateCaches", &MR::Mesh::invalidateCaches ).
        def( "transform", ( void( MR::Mesh::* ) ( const AffineXf3f& ) ) &MR::Mesh::transform );
} )

MR_ADD_PYTHON_VEC( mrmeshpy, vectorMesh, MR::Mesh )

void pythonSetFillHolePlaneMetric( MR::FillHoleParams& params, const Mesh& mesh, EdgeId e )
{
    params.metric = std::make_unique<PlaneFillMetric>( mesh, e );
}

void pythonSetFillHoleEdgeLengthMetric( MR::FillHoleParams& params, const Mesh& mesh )
{
    params.metric = std::make_unique<EdgeLengthFillMetric>( mesh );
}

void pythonSetFillHoleCircumscribedMetric( MR::FillHoleParams& params, const Mesh& mesh )
{
    params.metric = std::make_unique<CircumscribedFillMetric>( mesh );
}

MR_ADD_PYTHON_CUSTOM_DEF( mrmeshpy, FillHole, [] ( pybind11::module_& m )
{
    pybind11::class_<MR::FillHoleParams>( m, "FillHoleParams" ).
        def( pybind11::init<>() ).
        def_readwrite( "avoidMultipleEdges", &MR::FillHoleParams::avoidMultipleEdges ).
        def_readwrite( "makeDegenerateBand", &MR::FillHoleParams::makeDegenerateBand ).
        def_readwrite( "outNewFaces", &MR::FillHoleParams::outNewFaces );

    m.def( "set_fill_hole_metric_plane", pythonSetFillHolePlaneMetric, "set plane metric to fill hole parameters" );
    m.def( "set_fill_hole_metric_edge_length", pythonSetFillHoleEdgeLengthMetric, "set edge length metric to fill hole parameters" );
    m.def( "set_fill_hole_metric_circumscribed", pythonSetFillHoleCircumscribedMetric, "set circumscribed metric to fill hole parameters" );
    m.def( "fill_hole", MR::fillHole, "fills hole represented by edge" );
} )

std::vector<Vector3f> pythonComputePerVertNormals( const Mesh& mesh )
{
    auto res = computePerVertNormals( mesh );
    return res.vec_;
}

std::vector<Vector3f> pythonComputePerFaceNormals( const Mesh& mesh )
{
    auto res = computePerFaceNormals( mesh );
    return res.vec_;
}

std::vector<Mesh> pythonGetMeshComponents( const Mesh& mesh )
{
    auto components = MeshComponents::getAllComponents( mesh, MeshComponents::FaceIncidence::PerVertex );
    std::vector<Mesh> res( components.size() );
    for ( int i = 0; i < res.size(); ++i )
        res[i].addPartByMask( mesh, components[i] );
    return res;
}

Mesh pythonMergeMehses( const pybind11::list& meshes )
{
    Mesh res;
    for ( int i = 0; i < pybind11::len( meshes ); ++i )
        res.addPart( pybind11::cast<Mesh>( meshes[i] ) );
    return res;
}

void pythonHealSelfIntersections( Mesh& mesh, float voxelSize )
{
    MeshVoxelsConverter convert;
    convert.voxelSize = voxelSize;
    mesh = convert( convert( mesh ) );
}

MR_ADD_PYTHON_FUNCTION( mrmeshpy, self_intersections_heal, pythonHealSelfIntersections, "heals self intersections by converting mesh to voxels and back" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, merge_meshes, pythonMergeMehses, "merge python list of meshes to one mesh" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, compute_per_vert_normals, pythonComputePerVertNormals, "returns vector that contains normal for each valid vert" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, compute_per_face_normals, pythonComputePerFaceNormals, "returns vector that contains normal for each valid face" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, build_bottom, MR::buildBottom, "prolongs hole represented by edge to lowest point by dir, returns new EdgeId corresponding givven one" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, get_mesh_components, pythonGetMeshComponents, "find all disconnecteds components of mesh, return them as vector of meshes" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, make_cube, MR::makeCube, "creates simple cube mesh" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, make_torus, MR::makeTorus, "creates simple torus mesh" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, make_outer_half_test_torus, MR::makeOuterHalfTorus, "creates spetial torus without inner faces" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, make_undercut_test_torus, MR::makeTorusWithUndercut, "creates spetial torus with undercut" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, make_spikes_test_torus, MR::makeTorusWithSpikes, "creates spetial torus with some spikes" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, make_components_test_torus, MR::makeTorusWithComponents, "creates spetial torus without some segments" )

MR_ADD_PYTHON_FUNCTION( mrmeshpy, make_selfintersect_test_torus, MR::makeTorusWithSelfIntersections, "creates spetial torus with self-intersections" )

