#pragma once

#include <Eigen/Dense>

#include "particles.h"

class Mesh;

class Collider
{
public:
    inline virtual ~Collider() {}
    virtual void collision(Particle *p) = 0;
};

class PlaneCollider : public Collider
{
public:
    inline PlaneCollider(Eigen::Vector3d pos, Eigen::Vector3d n, double kr) : pos(pos), n(n), kr(kr) {}
    inline ~PlaneCollider() {}
    void collision(Particle *p) override;

    Eigen::Vector3d pos; // Position
    Eigen::Vector3d n;   // Normal
    double kr;           // Rebound Constant
};

class MeshCollider : public Collider
{
public:
    inline MeshCollider(Mesh *mesh, double kr) : mesh(mesh), kr(kr) {}
    inline ~MeshCollider() {}
    void collision(Particle *p) override;

    Mesh *mesh; // Mesh
    double kr;  // Rebound Constant
};
