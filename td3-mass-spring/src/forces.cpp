#include "forces.h"

#include "particles.h"

using namespace Eigen;

void GravityForce::addForces()
{
    if (ps == nullptr)
    {
        fprintf(stderr, "[GravityForce::addForces] Broken particle system (null).\n");
        return;
    }

    for (int i = 0; i < ps->particles.size(); i++)
    {
        Particle *p = ps->particles[i];
        if (p == nullptr)
        {
            fprintf(stderr, "[GravityForce::addForces] Broken particle (null): %d.\n", i);
            continue;
        }

        p->f += p->m * g;
    }
}

void DragForce::addForces()
{
    if (ps == nullptr)
    {
        fprintf(stderr, "[DragForce::addForces] Broken particle system (null).\n");
        return;
    }

    for (int i = 0; i < ps->particles.size(); i++)
    {
        Particle *p = ps->particles[i];
        if (p == nullptr)
        {
            fprintf(stderr, "[DragForce::addForces] Broken particle (null): %d.\n", i);
            continue;
        }

        p->f += -kd * p->v;
    }
}

void SpringForce::addForces()
{
    if (p0 == nullptr || p1 == nullptr)
    {
        fprintf(stderr, "[SpringForce::addForces] Broken particle couple (null).\n");
        return;
    }

    Vector3d dist = p1->x - p0->x;
    Vector3d fs = -ks * (dist.norm() - l0) * dist.normalized();
    Vector3d fd = -kd * (p1->v - p0->v).dot(dist) / dist.norm() * dist.normalized();
    p1->f += (fs + fd);
    p0->f += -(fs + fd);
}

void AnchorForce::addForces()
{
    if (p == nullptr)
        return;

    p->f += -ks * (p->x - x) - kd * p->v;
}
