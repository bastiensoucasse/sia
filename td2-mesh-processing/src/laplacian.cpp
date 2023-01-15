#include "laplacian.h"

#include <Eigen/SparseCholesky>

#include "mesh.h"

using namespace Eigen;
using namespace pmp;
using namespace std;

typedef SparseMatrix<float> SpMat;
typedef PermutationMatrix<Dynamic> Permutation;

static double cotan_weight(const SurfaceMesh &mesh, Halfedge he)
{
    const auto points = mesh.get_vertex_property<Point>("v:point");

    Vertex cot_vertices[4];
    cot_vertices[0] = mesh.from_vertex(he);
    cot_vertices[1] = mesh.to_vertex(he);
    cot_vertices[2] = mesh.to_vertex(mesh.next_halfedge(he));
    cot_vertices[3] = mesh.to_vertex(mesh.next_halfedge(mesh.opposite_halfedge(he)));

    Vector3f a = Vector3f(points[cot_vertices[0]] - points[cot_vertices[2]]).normalized();
    Vector3f b = Vector3f(points[cot_vertices[1]] - points[cot_vertices[2]]).normalized();
    Vector3f c = Vector3f(points[cot_vertices[0]] - points[cot_vertices[3]]).normalized();
    Vector3f d = Vector3f(points[cot_vertices[1]] - points[cot_vertices[3]]).normalized();

    double cotAlpha = 0.0;
    if (!mesh.is_boundary(he))
    {
        double cosAlpha = a.dot(b);
        double sinAlpha = a.cross(b).norm();
        cotAlpha = cosAlpha / sinAlpha;
    }
    
    double cotBeta = 0.0;
    if (!mesh.is_boundary(mesh.opposite_halfedge(he)))
    {
        double cosBeta = c.dot(d);
        double sinBeta = c.cross(d).norm();
        cotBeta = cosBeta / sinBeta;
    }

    return (cotAlpha + cotBeta) / 2;
}

void create_laplacian_matrix(const SurfaceMesh &mesh, SpMat &L, bool useCotWeights)
{
    vector<Triplet<double>> tripletList;

    const int n = mesh.n_vertices();

    const int estimation_of_entries = 7 * n;
    tripletList.reserve(estimation_of_entries);

    for (const Vertex &v : mesh.vertices())
    {
        double neighborWeightSum = 0.;
        Halfedge he = mesh.halfedge(v);
        do
        {
            const Vertex &vn = mesh.to_vertex(he);
            const double weight = useCotWeights ? cotan_weight(mesh, he) : 1.;

            neighborWeightSum += weight;
            tripletList.push_back(Triplet<double>((double)v.idx(), (double)vn.idx(), weight));

            he = mesh.next_halfedge(mesh.opposite_halfedge(he));
        } while (he != mesh.halfedge(v));
        tripletList.push_back(Triplet<double>((double)v.idx(), (double)v.idx(), (double)-neighborWeightSum));
    }

    L.setFromTriplets(tripletList.begin(), tripletList.end());
}

int create_permutation(const SurfaceMesh &mesh, Permutation &perm)
{
    const auto masks = mesh.get_vertex_property<int>("v:mask");

    const int n = mesh.n_vertices();
    perm.resize(n);

    int m = 0;
    int top_offset = 0, bottom_offset = n - 1;

    for (const Vertex &v : mesh.vertices())
        if (masks[v])
        {
            perm.indices()[v.idx()] = top_offset++;
            m++;
        }
        else
            perm.indices()[v.idx()] = bottom_offset--;

    return m;
}

void poly_harmonic_interpolation(const SurfaceMesh &mesh, const SpMat &L, const Permutation &perm, Ref<MatrixXf> u, int m, int k)
{
    const int n = mesh.n_vertices();

    // SpMat L(n, n);
    // create_laplacian_matrix(mesh, L, true);

    // Permutation perm;
    // int m = create_permutation(mesh, perm);

    SpMat L_prime;

    L_prime = L;
    for (int i = 1; i < k; i++)
        L_prime = L * L_prime;

    L_prime = L_prime.twistedBy(perm);

    MatrixXf u_prime;
    if (u.rows() == n)
        u_prime = perm * u;
    else
        u_prime = perm * u.transpose();

    SpMat L00 = L_prime.topLeftCorner(m, m);
    SpMat L01 = L_prime.topRightCorner(m, n - m);
    MatrixXf u1 = u_prime.bottomRows(n - m);

    SimplicialLDLT<SpMat> solver;

    solver.compute(L00);
    if (solver.info() != Success)
        return;

    MatrixXf x = solver.solve(-L01 * u1);
    if (solver.info() != Success)
        return;

    u_prime.topRows(m) = x;

    if (u.rows() == n)
        u = perm.inverse() * u_prime;
    else
        u = (perm.inverse() * u_prime).transpose();
}
