#include "bsdf.h"
#include "integrator.h"
#include "lights/areaLight.h"
#include "scene.h"
#include "warp.h"

class PathMats : public Integrator
{
public:
    PathMats(const PropertyList &props) {}

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray &ray) const
    {
        // Initialize the radiance value.
        Color3f radiance = Color3f::Zero();

        // Initialize the ray with the primary ray for the first iteration (it will be overwritten for next iterations).
        Ray r(ray);

        // Initialize the eta and the flow.
        float eta = 1.f;
        Color3f flow = Color3f::Ones();

        // Initialize the iteration count and start the loop.
        int iterations = 0;
        while (true)
        {
            // Compute the scene intersection.
            Hit hit;
            scene->intersect(r, hit);

            // Break the loop if no rebound is possible.
            if (!hit.foundIntersection())
            {
                radiance += flow * scene->backgroundColor(r.direction);
                break;
            }

            // Retreive the intersection data.
            const Point3f position = r.at(hit.t);
            const Normal3f normal = hit.localFrame.n;
            const BSDF *bsdf = hit.shape->bsdf();

            // Add the light intensity to the radiance if the intersection is with a light.
            if (hit.shape->isAreaLight())
            {
                const Light *light = (Light *)hit.shape->areaLight();
                radiance += flow * light->intensity(hit, (r.origin - position).normalized());
            }

            // Compute the BRDF and the output direction.
            BSDFQueryRecord query(hit.toLocal(-r.direction));
            query.uv = hit.uv;
            Color3f weighted_brdf = bsdf->sample(query, sampler->next2D());
            Vector3f outputDirection = hit.toWorld(query.wo);

            // Increase the eta.
            eta *= query.eta;

            if (iterations < 3)
            {
                // Increase the flow.
                flow *= weighted_brdf;
            }
            else
            {
                // Compute the probability to continue the path.
                float probability = fmin(fmax(fmax(flow.x(), flow.y()), flow.z()) * eta * eta, .99f);

                // Increase the flow.
                flow *= weighted_brdf / probability;

                // Break along probability.
                if (sampler->next1D() > probability)
                    break;
            }

            // Overwrite the current ray for the next iteration.
            if (outputDirection.dot(normal) < 0)
                r = Ray(position - normal * Epsilon, outputDirection);
            else
                r = Ray(position + normal * Epsilon, outputDirection);

            // Increment the iteration count.
            iterations++;
        }

        // Return the computed radiance.
        return radiance;
    }

    std::string toString() const { return tfm::format("PathMats[]"); }
};

REGISTER_CLASS(PathMats, "path_mats")
