#pragma once

#include "common.h"
#include "opengl.h"
#include "ray.h"
#include "particles.h"

#include <Eigen/Core>
#include <pmp/SurfaceMesh.h>
#include <string>
#include <vector>

class BVH;
class Shader;
class ParticleSystem;

class Mesh : public pmp::SurfaceMesh {
  using Vector4f = Eigen::Vector4f;
  using Vector3f = Eigen::Vector3f;
  using Vector2f = Eigen::Vector2f;

public:
  Mesh()
      : _ready(false), _transformation(Eigen::Affine3f::Identity()),
        _bvh(nullptr) {}
  ~Mesh();

  /// Loads the mesh from file
  void load(const std::string &filename);

  /// Creates a mesh representing a uniform paramteric grids with m x n vertices
  /// over the [0,1]^2 range.
  void createGrid(int m, int n);

  /// Creates a mesh representing a UV sphere
  void createSphere(float radius = 1.f, int nU = 10, int nV = 10);

  /// Populates a particle system from the current mesh
  void instanciateParticleSystem(ParticleSystem &psys);

  /// Updates vertex positons from the particles
  void updatePos(const ParticleSystem &psys);

  void init();

  /// Draws the mesh using OpenGL
  void draw(Shader &shader);

  const Eigen::Affine3f &transformationMatrix() const {
    return _transformation;
  }
  Eigen::Affine3f &transformationMatrix() { return _transformation; }

  /// returns face indices as a 3xN matrix of integers
  Eigen::Map<Eigen::Matrix<int, 3, Eigen::Dynamic>> faceIndices() {
    return Eigen::Map<Eigen::Matrix<int, 3, Eigen::Dynamic>>(_indices.data(), 3,
                                                             n_faces());
  }

  Eigen::Map<const Eigen::Matrix<int, 3, Eigen::Dynamic>> faceIndices() const {
    return Eigen::Map<const Eigen::Matrix<int, 3, Eigen::Dynamic>>(
        _indices.data(), 3, n_faces());
  }

  // Accessors to vertex attributes as Eigen's matrices:
  Eigen::Map<Eigen::Matrix3Xf> positions() {
    auto &vertices = get_vertex_property<pmp::Point>("v:point").vector();
    return Eigen::Map<Eigen::Matrix3Xf>(vertices[0].data(), 3, vertices.size());
  }

  Eigen::Map<Eigen::Matrix3Xf> colors() {
    auto &vcolors = get_vertex_property<pmp::Color>("v:color").vector();
    return Eigen::Map<Eigen::Matrix3Xf>(vcolors[0].data(), 3, vcolors.size());
  }

  Eigen::Map<Eigen::Matrix3Xf> normals() {
    auto &vnormals = get_vertex_property<pmp::Normal>("v:normal").vector();
    return Eigen::Map<Eigen::Matrix3Xf>(vnormals[0].data(), 3, vnormals.size());
  }

  Eigen::Map<Eigen::Matrix2Xf> texcoords() {
    auto &texcoords = get_vertex_property<pmp::TexCoord>("v:texcoord").vector();
    return Eigen::Map<Eigen::Matrix2Xf>(texcoords[0].data(), 2,
                                        texcoords.size());
  }

  /// Call all update* functions below.
  void updateAll();

  /// Re-compute vertex normals (needs to be called after editing vertex
  /// positions)
  void updateNormals();

  /// Re-compute the aligned bounding box (needs to be called after editing
  /// vertex positions)
  void updateBoundingBox();

  /// Re-compute the BVH for fast ray-mesh intersections (needs to be called
  /// after editing vertex positions)
  void updateBVH();

  /// Copy vertex attributes from the CPU to GPU memory (needs to be called
  /// after editing any vertex attributes: positions, normals, texcoords, masks,
  /// etc.)
  void updateVBO();

  /// Computes the first intersection between the ray and the mesh in hit (if
  /// any)
  bool intersect(const Ray &ray, Hit &hit) const;

  /// compute the intersection between a ray and a given triangular face
  bool intersectFace(const Ray &ray, Hit &hit, int faceId) const;

  const Eigen::AlignedBox3f &boundingBox() const { return _bbox; }

private:
  bool _ready;
  Eigen::AlignedBox3f _bbox;
  Eigen::Affine3f _transformation;

  std::vector<int> _indices;

  BVH *_bvh;

  GLuint _vao;
  const int VBO_IDX_FACE = 0;
  const int VBO_IDX_POSITION = 1;
  const int VBO_IDX_NORMAL = 2;
  const int VBO_IDX_COLOR = 3;
  const int VBO_IDX_TEXCOORD = 4;
  GLuint _vbo[7];
};
