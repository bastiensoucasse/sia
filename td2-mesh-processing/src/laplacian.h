#pragma once

#include <Eigen/SparseCore>
#include <pmp/SurfaceMesh.h>

/// Computes the Laplacian matrix in matrix \a L using cotangent weights or the graph Laplacian if useCotWeights==false.
void create_laplacian_matrix(const pmp::SurfaceMesh &mesh, Eigen::SparseMatrix<float> &L, bool useCotWeights = true);

/// Computes the permutation putting selected vertices (mask==1) first, and the others at the end. It returns the number of selected vertices.
int create_permutation(const pmp::SurfaceMesh &mesh, Eigen::PermutationMatrix<Eigen::Dynamic> &perm);

/// Performs the poly-harmonic interpolation (order k) over the selected vertices (mask==1) of the vertex attributes u. For each vertex V of index i, if  V.mask!=1 then u.col(i) is used as input constraints, otherwise, V.mask==1, and u.col(i) is replaced by the poly-harmonic interpolation of the fixed values.
void poly_harmonic_interpolation(const pmp::SurfaceMesh &mesh, const Eigen::SparseMatrix<float> &L, const Eigen::PermutationMatrix<Eigen::Dynamic> &perm, Eigen::Ref<Eigen::MatrixXf> u, int m, int k = 1);
