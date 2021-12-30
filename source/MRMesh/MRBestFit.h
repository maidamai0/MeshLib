#pragma once

#include "MRMatrix3.h"
#include "MRPlane3.h"
#include "MRLine3.h"
#include "MRAffineXf3.h"

namespace MR
{
 
class PointAccumulator
{
public:
    MRMESH_API void addPoint( const Vector3d & pt );
    MRMESH_API void addPoint( const Vector3d & pt, double weight );
    void addPoint( const Vector3f& pt ) { addPoint( Vector3d( pt ) ); }
    void addPoint( const Vector3f& pt, float weight ) { addPoint( Vector3d( pt ), weight ); }

    // computes the best approximating plane from the accumulated points
    MRMESH_API Plane3d getBestPlane() const;
    Plane3f getBestPlanef() const { return Plane3f( getBestPlane() ); }
    // computes the best approximating line from the accumulated points
    MRMESH_API Line3d getBestLine() const;
    Line3f getBestLinef() const { return Line3f( getBestLine() ); }

    // computes centroid and eigenvectors/eigenvalues of the covariance matrix of the accumulated points
    MRMESH_API bool getCenteredCovarianceEigen( Vector3d & centroid, Matrix3d & eigenvectors, Vector3d & eigenvalues ) const;
    MRMESH_API bool getCenteredCovarianceEigen( Vector3f& centroid, Matrix3f& eigenvectors, Vector3f& eigenvalues ) const;

    // returns the transformation that maps (0,0,0) into point centroid,
    // and maps vectors (1,0,0), (0,1,0), (0,0,1) into first, second, third eigenvectors
    MRMESH_API AffineXf3d getBasicXf() const;
    AffineXf3f getBasicXf3f() const { return AffineXf3f( getBasicXf() ); }

    bool valid() { return sumWeight_ > 0; };

private:
    double sumWeight_ = 0;
    Vector3d momentum1_;
    Matrix3d momentum2_ = Matrix3d::zero();
};

// This function accumulate all mesh face centers added there with the weight equal to face area in existing `PointAccumulator`
MRMESH_API void accumulateFaceCenters( PointAccumulator& accum, const MeshPart& mp, const AffineXf3f* xf = nullptr );

} //namespace MR
