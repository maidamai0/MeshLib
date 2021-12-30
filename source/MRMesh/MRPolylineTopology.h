#pragma once

#include "MRMeshFwd.h"
#include "MRVector.h"
#include "MRId.h"
// for template implementation:
#include "MRBitSet.h"

namespace MR
{

// topology of one or several polylines (how line segments are connected in lines) common for 2D and 3D polylines
struct PolylineTopology
{
    // builds this topology from given contours,
    // also builds the vector of referenced points using two functors: reserve and add
    // if all even edges are consistently oriented, then the output contours will be oriented the same
    template<typename T, typename F1, typename F2>
    void buildFromContours( const std::vector<std::vector<T>> & contours, F1 && reservePoints, F2 && addPoint );

    // converts this topology into contours of given type using the functor returning point by its Id
    template<typename T, typename F>
    std::vector<std::vector<T>> convertToContours( F && getPoint ) const;

    // creates an edge not associated with any vertex
    MRMESH_API EdgeId makeEdge();
    // makes an edge from vertex a to b (both must be within reserved capacity for vertices);
    // if either of the vertices already has 2 incident edges, then makeEdge(a,b) fails and returns invalid edge
    MRMESH_API EdgeId makeEdge( VertId a, VertId b );
    // checks whether the edge is disconnected from all other edges and disassociated from all vertices (as if after makeEdge)
    MRMESH_API bool isLoneEdge( EdgeId a ) const;
    // returns last not lone edge id, or invalid id if no such edge exists
    MRMESH_API EdgeId lastNotLoneEdge() const;
    // returns the number of half-edge records including lone ones
    size_t edgeSize() const { return edges_.size(); }
    // returns the number of undirected edges (pairs of half-edges) including lone ones
    size_t undirectedEdgeSize() const { return edges_.size() >> 1; }
    // sets the capacity of half-edges vector
    void edgeReserve( size_t newCapacity ) { edges_.reserve( newCapacity ); }
    // returns true if given edge is within valid range and not-lone
    bool hasEdge( EdgeId e ) const { assert( e.valid() ); return e < (int)edgeSize() && !isLoneEdge( e ); }

    // given two half edges do either of two:
    // 1) if a and b were from distinct rings, puts them in one ring;
    // 2) if a and b were from the same ring, puts them in separate rings;
    // the cut in rings in both cases is made after a and b
    MRMESH_API void splice( EdgeId a, EdgeId b );
    
    // next (counter clock wise) half-edge in the origin ring
    EdgeId next( EdgeId he ) const { assert(he.valid()); return edges_[he].next; }
    // returns origin vertex of half-edge
    VertId org( EdgeId he ) const { assert(he.valid()); return edges_[he].org; }
    // returns destination vertex of half-edge
    VertId dest( EdgeId he ) const { assert(he.valid()); return edges_[he.sym()].org; }

    // sets new origin to the full origin ring including this edge;
    // edgePerVertex_ table is updated accordingly
    MRMESH_API void setOrg( EdgeId a, VertId v );

    // for all valid vertices this vector contains an edge with the origin there
    const Vector<EdgeId, VertId> & edgePerVertex() const { return edgePerVertex_; }
    // returns valid edge if given vertex is present in the mesh
    EdgeId edgeWithOrg( VertId a ) const { assert( a.valid() ); return a < int(edgePerVertex_.size()) && edgePerVertex_[a].valid() ? edgePerVertex_[a] : EdgeId(); }
    // returns true if given vertex is present in the mesh
    bool hasVert( VertId a ) const { return validVerts_.test( a ); }
    // returns 0 if given vertex does not exist, 1 if it has one incident edge, and 2 if it has two incident edges
    MRMESH_API int getVertDegree( VertId a ) const;
    // returns the number of valid vertices
    int numValidVerts() const { return numValidVerts_; }
    // returns last valid vertex id, or invalid id if no single valid vertex exists
    MRMESH_API VertId lastValidVert() const;
    // creates new vert-id not associated with any edge yet
    VertId addVertId() { edgePerVertex_.push_back( {} ); validVerts_.push_back( false ); return VertId( (int)edgePerVertex_.size() - 1 ); }
    // explicitly increases the size of verts vector
    void vertResize( size_t newSize ) { if ( edgePerVertex_.size() < newSize ) { edgePerVertex_.resize( newSize ); validVerts_.resize( newSize ); } }
    // sets the capacity of verts vector
    void vertReserve( size_t newCapacity ) { edgePerVertex_.reserve( newCapacity ); validVerts_.reserve( newCapacity ); }
    // returns the number of vertex records including invalid ones
    size_t vertSize() const { return edgePerVertex_.size(); }
     // returns cached set of all valid vertices
    const VertBitSet & getValidVerts() const { return validVerts_; }
    // if region pointer is not null then converts it in reference, otherwise returns all valid vertices in the mesh
    const VertBitSet & getVertIds( const VertBitSet * region ) const { return region ? *region : validVerts_; }

    // finds and returns edge from o to d in the mesh; returns invalid edge otherwise
    MRMESH_API EdgeId findEdge( VertId o, VertId d ) const;

    // returns all vertices incident to path edges
    MRMESH_API VertBitSet getPathVertices( const EdgePath & path ) const;

    // split given edge on two parts, with e pointing on the second part with the same destination vertex but new origin vertex (which is returned)
    MRMESH_API VertId splitEdge( EdgeId e );

    // adds polyline in this topology passing progressively via vertices *[vs, vs+num);
    // if vs[0] == vs[num-1] then a closed polyline is created;
    // return the edge from first to second vertex
    MRMESH_API EdgeId makePolyline( const VertId * vs, size_t num );

    // comparison via edges (all other members are considered as not important caches)
    bool operator ==( const PolylineTopology & b ) const { return edges_ == b.edges_; }
    bool operator !=( const PolylineTopology & b ) const { return edges_ != b.edges_; }

    // returns true if for each edge e: e == e.next() || e.odd() == next( e ).sym().odd()
    MRMESH_API bool isConsistentlyOriented() const;
    // verifies that all internal data structures are valid
    MRMESH_API bool checkValidity() const;

private:
    // sets new origin to the full origin ring including this edge, without updating edgePerVertex_ table
    void setOrg_( EdgeId a, VertId v );

    // data of every half-edge
    struct HalfEdgeRecord
    {
        EdgeId next; // next counter clock wise half-edge in the origin ring
        VertId org;  // vertex at the origin of the edge

        bool operator ==( const HalfEdgeRecord & b ) const = default;
    };

    // edges_: EdgeId -> edge data
    Vector<HalfEdgeRecord, EdgeId> edges_;

    // edgePerVertex_: VertId -> one edge id of one of edges with origin there
    Vector<EdgeId, VertId> edgePerVertex_;
    VertBitSet validVerts_; // each true bit here corresponds to valid element in edgePerVertex_
    int numValidVerts_ = 0; // the number of valid elements in edgePerVertex_ or set bits in validVerts_
};

template<typename T, typename F1, typename F2>
void PolylineTopology::buildFromContours( const std::vector<std::vector<T>> & contours, F1 && reservePoints, F2 && addPoint )
{
    *this = {};
    size_t size = 0;
    std::vector<bool> closed;
    closed.reserve( contours.size() );
    int numNotEmpty = 0;
    int numClosed = 0;
    for ( const auto& c : contours )
    {
        if ( c.size() > 2 )
        {
            ++numNotEmpty;
            closed.push_back( c.front() == c.back() );
        }
        else
        {
            closed.push_back( false );
        }
        size += c.size();
        if ( closed.back() )
            ++numClosed;
    }

    reservePoints( size - numClosed );
    vertResize( size - numClosed );

    for ( int i = 0; i < contours.size(); ++i )
    {
        const auto& c = contours[i];
        if ( c.empty() )
            continue;
        const auto e0 = makeEdge();
        const auto v0 = addPoint( c[0] );
        setOrg( e0, v0 );
        auto e = e0;
        for ( int j = 1; j + 1 < c.size(); ++j )
            {
            const auto ej = makeEdge();
            splice( ej, e.sym() );
            const auto vj = addPoint( c[j] );
            setOrg( ej, vj );
            e = ej;
        }
        if ( closed[i] )
        {
            splice( e0, e.sym() );
        }
        else
        {
            const auto vx = addPoint( c.back() );
            setOrg( e.sym(), vx );
        }
    }
    assert( isConsistentlyOriented() );
}

template<typename T, typename F>
std::vector<std::vector<T>> PolylineTopology::convertToContours( F && getPoint ) const
{
    std::vector<std::vector<T>> res;

    UndirectedEdgeBitSet linesUsed;
    linesUsed.autoResizeSet( UndirectedEdgeId{ undirectedEdgeSize() } );
    linesUsed.flip();
    for ( EdgeId e0 : linesUsed )
    {
        if ( isLoneEdge( e0 ) )
            continue;

        EdgeId curLine = e0;
        while ( curLine != next( curLine ) ) // until contour start is reached
        {
            curLine = next( curLine ).sym();
            if ( curLine == e0 )
                break; // the contour is closed
        }

        linesUsed.set( curLine.undirected(), false );

        EdgeId e = curLine;
        std::vector<T> cont;
        cont.push_back( getPoint( org( e ) ) );
        for ( ;; )
        {
            e = e.sym();
            cont.push_back( getPoint( org( e ) ) );
            e = next( e );
            if ( !linesUsed.test_set( e.undirected(), false ) )
                break;
        }
        res.push_back( std::move( cont ) );
    }

    return res;
}

} //namespace MR
