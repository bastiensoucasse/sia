#include "neighborhood.h"
#include "mesh.h"
#include <stack>
#include <vector>

using namespace pmp;
using namespace Eigen;
using namespace std;

std::vector<pmp::Vertex> select_neighbors(const SurfaceMesh &mesh, Vertex v,
                                          float dist) {
  std::vector<Vertex> neighbors;

  neighbors.push_back(v);

  Vector3f q = mesh.position(v);
  std::vector<bool> visited(mesh.n_vertices(), false);
  visited[v.idx()] = true;
  std::stack<pmp::Vertex> stack;
  stack.push(v);
  while (!stack.empty()) {
    pmp::Vertex vj = stack.top();
    stack.pop();

    // pour tous les voisins vi de vj
    pmp::Halfedge h = mesh.halfedge(vj), h0 = h;
    do {
      pmp::Vertex vi = mesh.to_vertex(h);
      if (!visited[vi.idx()]) // si on ne l'a pas déjà rencontré
      {
        if ((static_cast<Eigen::Vector3f>(mesh.position(vi)) - q).norm() <
            dist) // si on n'est pas sortie de la boule
        {
          neighbors.push_back(vi);
          stack.push(vi);
        }
      }
      visited[vi.idx()] = true;
      h = mesh.next_halfedge(mesh.opposite_halfedge(h));
    } while (h != h0);
  }

  return neighbors;
}
