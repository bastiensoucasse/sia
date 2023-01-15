#pragma once

#include <pmp/SurfaceMesh.h>

// Returns the list of all vertices connected to v within a distance 'dist'
std::vector<pmp::Vertex> select_neighbors(const pmp::SurfaceMesh &mesh,
                                          pmp::Vertex v, float dist);
