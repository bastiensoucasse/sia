#include "collider.h"

#include "mesh.h"
#include "ray.h"

using namespace Eigen;

static const double epsilon = 1e-3;

void PlaneCollider::collision(Particle *p)
{
    if ((p->x - pos).dot(n) < epsilon && n.dot(p->v) < 0)
    {
        const Vector3d vn = n.dot(p->v) * n;
        const Vector3d vt = p->v - vn;

        p->v = vt - kr * vn;

        if ((p->x - pos).dot(n) < 0)
            p->x += (-(p->x - pos).dot(n) + epsilon) * n;
    }

    if (abs((p->x - pos).dot(n)) < epsilon && abs(n.dot(p->v)) < epsilon && n.dot(p->f) < 0)
    {
        const Vector3d fc = -(n.dot(p->f)) * n;

        p->f = fc;
    }
}

void MeshCollider::collision(Particle *p)
{
    Ray ray = Ray(p->x.cast<float>(), p->v.normalized().cast<float>());

    Hit hit = Hit();
    if (!mesh->intersect(ray, hit))
        return;

    Vector3d n = hit.getNormal().cast<double>();
    if (hit.t() < epsilon && n.dot(p->v) < 0)
    {
        const Vector3d vn = n.dot(p->v) * n;
        const Vector3d vt = p->v - vn;
        p->v = vt - kr * vn;
    }

    if (hit.t() < epsilon && abs(n.dot(p->v)) < epsilon && n.dot(p->f) < 0)
    {
        const Vector3d fc = -(n.dot(p->f)) * n;
        p->f = fc;
    }

    ray = Ray(p->x.cast<float>(), -p->v.normalized().cast<float>());

    hit = Hit();
    if (!mesh->intersect(ray, hit))
        return;

    n = hit.getNormal().cast<double>();
    if (hit.t() < epsilon && n.dot(p->v) < 0)
    {
        const Vector3d vn = n.dot(p->v) * n;
        const Vector3d vt = p->v - vn;
        p->v = vt - kr * vn;
    }

    if (hit.t() < epsilon && abs(n.dot(p->v)) < epsilon && n.dot(p->f) < 0)
    {
        const Vector3d fc = -(n.dot(p->f)) * n;
        p->f = fc;
    }
}
