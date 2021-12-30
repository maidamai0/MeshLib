#include "MRMeshFixer.h"
#include "MRMesh.h"
#include "MRTimer.h"
#include "MRRingIterator.h"
#include "MRBitSetParallelFor.h"
#include "MRTriMath.h"
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>

namespace MR
{

// given a vertex, returns two edges with the origin in this vertex consecutive in the vertex ring without left faces both;
// both edges may be the same if there is only one edge without left face;
// or both edges can be invalid if all vertex edges have left face
static std::pair<EdgeId, EdgeId> getTwoSeqNoLeftAtVertex( const MeshTopology & m, VertId a )
{
    EdgeId e0 = m.edgeWithOrg( a );
    if ( !e0.valid() )
        return {}; //invalid vertex

    // find first hole edge
    EdgeId eh = e0;
    for (;;)
    {
        if ( !m.left( eh ).valid() )
            break;
        eh = m.next( eh );
        if ( eh == e0 )
            return {}; // no single hole near a
    }

    // find second hole edge
    for ( EdgeId e = m.next( eh ); e != e0; e = m.next( e ) )
    {
        if ( !m.left( e ).valid() )
            return { eh, e }; // another hole near a
    }

    return { eh, eh };
}

int duplicateMultiHoleVertices( Mesh & mesh )
{
    int duplicates = 0;
    const auto lastVert = mesh.topology.lastValidVert();
    for ( VertId v{0}; v <= lastVert; ++v )
    {
        auto ee = getTwoSeqNoLeftAtVertex( mesh.topology, v );
        if ( ee.first == ee.second )
            continue;

        EdgeId e1 = ee.first;
        EdgeId e0 = e1;
        while ( mesh.topology.right( e0 ).valid() )
            e0 = mesh.topology.prev( e0 );

        // unsplice [e0, e1] and create new vertex for it
        mesh.topology.splice( mesh.topology.prev( e0 ), e1 );
        assert( !mesh.topology.org( e0 ).valid() );

        auto vDup = mesh.addPoint( mesh.points[v] );
        mesh.topology.setOrg( e0, vDup );

        ++duplicates;
        --v;
    }

    return duplicates;
}

std::vector<MultipleEdge> findMultipleEdges( const MeshTopology & topology )
{
    MR_TIMER
    tbb::enumerable_thread_specific<std::vector<MultipleEdge>> threadData;
    const VertId lastValidVert = topology.lastValidVert();
    tbb::parallel_for( tbb::blocked_range<VertId>( VertId{0}, lastValidVert + 1 ), [&]( const tbb::blocked_range<VertId> & range )
    {
        auto & tls = threadData.local();
        std::vector<VertId> neis;
        for ( VertId v = range.begin(); v < range.end(); ++v )
        {
            if ( !topology.hasVert( v ) )
                continue;
            neis.clear();
            for ( auto e : orgRing( topology, v ) )
            {
                auto nv = topology.dest( e );
                if ( nv > v )
                    neis.push_back( nv );
            }
            std::sort( neis.begin(), neis.end() );
            auto it = neis.begin();
            for (;;)
            {
                it = std::adjacent_find( it, neis.end() );
                if ( it == neis.end() )
                    break;
                auto nv = *it;
                tls.emplace_back( v, nv );
                assert( nv == *( it + 1 ) );
                ++++it;
                while ( it != neis.end() && *it == nv )
                    ++it;
                if ( it == neis.end() )
                    break;
            }
        }
    } );

    std::vector<MultipleEdge> res;
    for ( const auto & ns : threadData )
        res.insert( res.end(), ns.begin(), ns.end() );
    // sort the result to make it independent of mesh distribution among threads
    std::sort( res.begin(), res.end() );

    return res;
}

FaceBitSet findDegenerateFaces( const Mesh& mesh, float criticalAspectRatio /*= FLT_MAX */ )
{
    FaceBitSet selection( mesh.topology.getValidFaces().size() );
    BitSetParallelFor( mesh.topology.getValidFaces(), [&] ( FaceId f )
    {
        Vector3f vp[3];
        mesh.getTriPoints( f, vp[0], vp[1], vp[2] );
        if ( triangleAspectRatio( vp[0], vp[1], vp[2] ) >= criticalAspectRatio )
            selection.set( f );
    } );
    return selection;
}

} //namespace MR
