from helper import *

torusMesh = mrmesh.make_torus(2,1,32,32,None)
torusPointCloud = mrmesh.mesh_to_points(torusMesh, True, None)

params = mrealgorithms.TriangulationParameters()
restored = mrealgorithms.triangulate_point_cloud(torusPointCloud, params)

assert( len(restored.points.vec) == 1024 )
assert( restored.topology.getValidVerts().count() == 1024 )
assert( restored.topology.findHoleRepresentiveEdges().size() == 0)
