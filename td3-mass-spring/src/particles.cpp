#include "particles.h"

#include <iostream>

#include "collider.h"
#include "forces.h"

using namespace Eigen;
using namespace std;

ParticleSystem::~ParticleSystem()
{
    if (_ready)
    {
        glDeleteBuffers(2, _vbo);
        glDeleteVertexArrays(2, _vao);
    }

    clear();
}

int ParticleSystem::getDimensions()
{
    return 6 * particles.size();
}

void ParticleSystem::getState(VectorXd &state)
{
    for (int i = 0; i < particles.size(); i++)
    {
        Particle *p = particles[i];
        if (p == nullptr)
        {
            fprintf(stderr, "[getState] Broken particle (null): %d.", i);
            continue;
        }

        state.segment(6 * i, 3) = p->x;
        state.segment(6 * i + 3, 3) = p->v;
    }
}

void ParticleSystem::setState(const VectorXd &state)
{
    for (int i = 0; i < particles.size(); i++)
    {
        Particle *p = particles[i];
        if (p == nullptr)
        {
            fprintf(stderr, "[setState] Broken particle (null): %d.", i);
            continue;
        }

        p->x = state.segment(6 * i, 3);
        p->v = state.segment(6 * i + 3, 3);
    }
}

void ParticleSystem::getDerivative(VectorXd &deriv)
{
    // Reset particle forces.
    for (int i = 0; i < particles.size(); i++)
    {
        Particle *p = particles[i];
        if (p == nullptr)
        {
            fprintf(stderr, "[getDerivative p1] Broken particle (null): %d.", i);
            continue;
        }

        p->f = Vector3d::Zero();
    }

    // Add forces to particles.
    for (int i = 0; i < forces.size(); i++)
    {
        Force *f = forces[i];
        if (f == nullptr)
        {
            fprintf(stderr, "[getDerivative p2] Broken force (null): %d.", i);
            continue;
        }

        f->addForces();
    }

    // Fill derivative vector.
    for (int i = 0; i < particles.size(); i++)
    {
        Particle *p = particles[i];
        if (p == nullptr)
        {
            fprintf(stderr, "[getDerivative p3] Broken particle (null): %d.", i);
            continue;
        }

        deriv.segment(6 * i, 3) = p->v;
        deriv.segment(6 * i + 3, 3) = p->f / p->m;
    }
}

void ParticleSystem::init()
{
    glGenVertexArrays(2, _vao);
    glGenBuffers(2, _vbo);

    _ready = true;
}

void ParticleSystem::clear()
{
    for (auto f : forces)
        delete f;
    forces.resize(0);

    _numSpringForces = 0;

    for (auto p : particles)
        delete p;
    particles.resize(0);

    for (auto c : colliders)
        delete c;
    colliders.resize(0);
}

void ParticleSystem::updateGL(bool first)
{
    vector<Vector3f> positions;
    positions.resize(particles.size());

    _bbox.setEmpty();
    for (size_t i = 0; i < particles.size(); i++)
    {
        positions[i] = particles[i]->x.cast<float>();
        _bbox.extend(positions[i]);
    }

    glBindVertexArray(_vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo[0]);
    if (first)
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3f) * particles.size(), positions.data(), GL_DYNAMIC_DRAW);
    else
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector3f) * particles.size(), positions.data());
    glBindVertexArray(0);

    positions.resize(0);
    positions.reserve(forces.size() * 2);
    for (size_t i = 0; i < forces.size(); i++)
    {
        SpringForce *s = dynamic_cast<SpringForce *>(forces[i]);
        if (s)
        {
            positions.push_back(s->p0->x.cast<float>());
            positions.push_back(s->p1->x.cast<float>());
        }

        AnchorForce *a = dynamic_cast<AnchorForce *>(forces[i]);
        if (a && a->p != nullptr)
        {
            positions.push_back(a->p->x.cast<float>());
            positions.push_back(a->x.cast<float>());
        }
    }

    glBindVertexArray(_vao[1]);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo[1]);
    if (first || _numSpringForces != positions.size())
    {
        _numSpringForces = positions.size();
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3f) * _numSpringForces, positions.data(), GL_DYNAMIC_DRAW);
    }
    else
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector3f) * _numSpringForces, positions.data());
    glBindVertexArray(0);
}

void ParticleSystem::draw(Shader &shader)
{
    // Draw particles
    glBindVertexArray(_vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo[0]);
    GLuint vertex_loc = shader.getAttribLocation("vtx_position");
    glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(vertex_loc);
    glPointSize(5.f);
    glDrawArrays(GL_POINTS, 0, particles.size());
    glDisableVertexAttribArray(vertex_loc);
    glBindVertexArray(0);

    // Draw spring forces
    glBindVertexArray(_vao[1]);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo[1]);
    vertex_loc = shader.getAttribLocation("vtx_position");
    glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(vertex_loc);
    glLineWidth(2.f);
    glDrawArrays(GL_LINES, 0, _numSpringForces);
    glDisableVertexAttribArray(vertex_loc);
    glBindVertexArray(0);
}

void ParticleSystem::makeGrid(int m, int n)
{
    double mass = 1;                               // Total Mass
    double pmass = mass / ((m + 1) * (n + 1));     // Particle Mass
    double x0 = 0.3, x1 = 0.7, y0 = 0.3, y1 = 0.7; // Rectangle Extent
    double dx = (x1 - x0) / m, dy = (y1 - y0) / n; // Direct Spring Lengths
    double dd = sqrt(dx * dx + dy * dy);           // Diagonal Spring Lengths
    double ks = 200;                               // Direct Spring Constant
    double kd = 0.1;                               // Damping Constant
    double ks_diag = 50;                           // Diagonal Spring Constant

    // Particles
    Array<Particle *, Eigen::Dynamic, Eigen::Dynamic> particlesTab(m + 1, n + 1);
    for (int i = 0; i <= m; i++)
        for (int j = 0; j <= n; j++)
        {
            Vector3d x(x0 + dx * i, y0 + dy * j, 0);
            Vector3d v(0, 0, 0);
            particlesTab(i, j) = new Particle(particles.size(), pmass, x, v);
            particles.push_back(particlesTab(i, j));
        }

    // Anchors
    // forces.push_back(new AnchorForce(particlesTab(0, n), particlesTab(0, n)->x, 1000, 0.1));
    // forces.push_back(new AnchorForce(particlesTab(m, n), particlesTab(m, n)->x, 1000, 0.1));

    // Springs
    for (int i = 0; i <= m; i++)
    {
        for (int j = 0; j <= n; j++)
        {
            Particle *p0 = particlesTab(i, j);

            if (i < m)
            {
                Particle *p1 = particlesTab(i + 1, j);
                forces.push_back(new SpringForce(p0, p1, ks, kd, dx));
            }

            if (j < n)
            {
                Particle *p1 = particlesTab(i, j + 1);
                forces.push_back(new SpringForce(p0, p1, ks, kd, dy));
            }

            if (i < m && j < n)
            {
                Particle *p1 = particlesTab(i + 1, j);
                Particle *p2 = particlesTab(i, j + 1);
                Particle *p3 = particlesTab(i + 1, j + 1);
                forces.push_back(new SpringForce(p1, p2, ks_diag, kd, dd));
                forces.push_back(new SpringForce(p0, p3, ks_diag, kd, dd));
            }
        }
    }
}

void ParticleSystem::step(double dt, Integration type)
{
    if (type == Integration::EULER)
        explicitEulerStep(this, dt);

    if (type == Integration::MID_POINT)
        midPointStep(this, dt);

    for (Collider *c : colliders)
        for (Particle *p : particles)
            c->collision(p);

    updateGL();
}
