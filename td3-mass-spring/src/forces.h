#pragma once

#include <Eigen/Dense>

class Particle;
class ParticleSystem;

class Force
{
public:
    inline virtual ~Force() {}
    virtual void addForces() = 0;
};

class GravityForce : public Force
{
public:
    inline GravityForce(ParticleSystem *ps, const Eigen::Vector3d &g) : ps(ps), g(g) {}
    inline ~GravityForce() {}
    void addForces();

    ParticleSystem *ps; // Particle System
    Eigen::Vector3d g;  // Gravity Acceleration Vector
};

class DragForce : public Force
{
public:
    inline DragForce(ParticleSystem *ps, double kd) : ps(ps), kd(kd) {}
    inline ~DragForce() {}
    void addForces();

    ParticleSystem *ps; // Particle System
    double kd;          // Damping Coefficient
};

class SpringForce : public Force
{
public:
    inline SpringForce(Particle *p0, Particle *p1, double ks, double kd, double l0) : p0(p0), p1(p1), ks(ks), kd(kd), l0(l0) {}
    inline ~SpringForce() {}
    void addForces();

    Particle *p0, *p1; // Particle Couple
    double ks;         // Spring Constant
    double kd;         // Damping Coefficient
    double l0;         // Rest Length
};

class AnchorForce : public Force
{
public:
    inline AnchorForce(Particle *p, Eigen::Vector3d x, double ks, double kd) : p(p), x(x), ks(ks), kd(kd) {}
    inline ~AnchorForce() {}
    void addForces();

    Particle *p;       // Particle
    Eigen::Vector3d x; // Anchor Point
    double ks;         // Spring Constant
    double kd;         // Damping Coefficient
};
