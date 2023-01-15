#pragma once

#include <vector>

#include <Eigen/Dense>

#include "integration.h"
#include "shader.h"

class Collider;
class Force;

class Particle
{
public:
    inline Particle(int i, double m, Eigen::Vector3d x, Eigen::Vector3d v) : i(i), x(x), v(v), m(m) {}

    int i;             // Index
    Eigen::Vector3d x; // Position
    Eigen::Vector3d v; // Velocity
    Eigen::Vector3d f; // Force
    double m;          // Mass
};

class ParticleSystem : public ODESystem
{
public:
    inline ParticleSystem() : _ready(false), _transformation(Eigen::Affine3f::Identity()) {}
    ~ParticleSystem();

    int getDimensions();
    void getState(Eigen::VectorXd &states);
    void setState(const Eigen::VectorXd &states);
    void getDerivative(Eigen::VectorXd &derivatives);

    void init();
    void clear();
    void updateGL(bool first = false);
    void draw(Shader &shader);
    void makeGrid(int m = 10, int n = 10);
    void step(double dt, Integration type);

    int getDOFs();                                                     // Not implemented.
    void getState(Eigen::VectorXd &x, Eigen::VectorXd &v);             // Not implemented.
    void setState(const Eigen::VectorXd &x, const Eigen::VectorXd &v); // Not implemented.
    void getForces(Eigen::VectorXd &f);                                // Not implemented.

    inline const Eigen::Affine3f &transformationMatrix() const { return _transformation; }
    inline Eigen::Affine3f &transformationMatrix() { return _transformation; }
    inline const Eigen::AlignedBox3f &boundingBox() const { return _bbox; }

    std::vector<Collider *> colliders;
    std::vector<Force *> forces;
    std::vector<Particle *> particles;

private:
    unsigned int _vao[2]; // OpenGL Vertex Array Objects
    unsigned int _vbo[2]; // OpenGL Vertex Buffer Objects
    bool _ready;          // OpenGL State

    size_t _numSpringForces;
    Eigen::AlignedBox3f _bbox;
    Eigen::Affine3f _transformation;
};
