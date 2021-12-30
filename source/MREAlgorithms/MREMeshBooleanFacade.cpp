#include "MREMeshBooleanFacade.h"
#include "MREMeshBoolean.h"
#include "MRMesh/MRObjectMesh.h"
#include "MRMesh/MRGTest.h"

namespace MRE
{
using namespace MR;

TransformedMesh MeshMeshConverter::operator() ( const ObjectMesh & obj ) const
{
    return TransformedMesh( *obj.mesh(), obj.xf() );
}

TransformedMesh & operator += ( TransformedMesh & a, const TransformedMesh& b )
{
    auto b2a = a.xf.inverse() * b.xf;
    auto res = boolean( a.mesh, b.mesh, BooleanOperation::Union, &b2a );
    assert( res.valid() );
    if ( res.valid() )
        a.mesh = std::move( res.mesh );
    return a;
}

TransformedMesh & operator -= ( TransformedMesh & a, const TransformedMesh& b )
{
    auto b2a = a.xf.inverse() * b.xf;
    auto res = boolean( a.mesh, b.mesh, BooleanOperation::DifferenceAB, &b2a );
    assert( res.valid() );
    if ( res.valid() )
        a.mesh = std::move( res.mesh );
    return a;
}

TransformedMesh & operator *= ( TransformedMesh & a, const TransformedMesh& b )
{
    auto b2a = a.xf.inverse() * b.xf;
    auto res = boolean( a.mesh, b.mesh, BooleanOperation::Intersection, &b2a );
    assert( res.valid() );
    if ( res.valid() )
        a.mesh = std::move( res.mesh );
    return a;
}

TEST( MREAlgorithms, MeshBooleanFacade )
{
    Mesh gingivaCopy;
    Mesh combinedTooth;
    MeshMeshConverter convert;

    auto gingivaGrid = convert( gingivaCopy );
    auto toothGrid = convert( combinedTooth );
    toothGrid -= gingivaGrid;
    auto tooth = std::make_shared<MR::Mesh>( convert( toothGrid ) );
}

} //namespace MR
