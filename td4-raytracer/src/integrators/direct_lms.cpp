#include "bsdf.h"
#include "frame.h"
#include "integrator.h"
#include "scene.h"

class DirectLMS : public Integrator
{
public:
    DirectLMS(const PropertyList &props)
    {
        m_sampleCount = props.getInteger("sampleCount", 10);
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray &ray) const
    {
        Color3f radiance = Color3f::Zero();

        Hit hit;
        scene->intersect(ray, hit);
        if (!hit.foundIntersection())
            return scene->backgroundColor();

        Point3f pos = ray.at(hit.t);
        Normal3f normal = hit.localFrame.n;

        for (int i = 0; i < m_sampleCount; i++)
        {
            if (hit.shape->isAreaLight())
            {
                const Light *light = (Light *)hit.shape->areaLight();
                radiance += light->intensity(hit, (ray.origin - pos).normalized());
            }

            const BSDF *bsdf = hit.shape->bsdf();
            const LightList &lights = scene->lightList();
            for (LightList::const_iterator it = lights.begin(); it != lights.end(); ++it)
            {
                const Light *light = (*it);

                float dist, pdf;
                Vector3f lightDir;
                Color3f intensity = light->sample(pos, sampler->next2D(), pdf, lightDir, dist);
                lightDir = hit.toWorld(lightDir);

                Ray shadowRay(pos + normal * Epsilon, lightDir, true);
                Hit shadowHit;
                scene->intersect(shadowRay, shadowHit);
                if (!shadowHit.foundIntersection() || shadowHit.t > dist || shadowHit.shape == light->shape())
                {
                    float cos_term = std::max(0.f, lightDir.dot(normal));
                    Color3f brdf = bsdf->eval(BSDFQueryRecord(hit.toLocal(-ray.direction), hit.toLocal(lightDir), ESolidAngle, hit.uv));

                    if (pdf != 0.f)
                        radiance += intensity * cos_term * brdf / pdf;
                }
            }
        }

        return radiance / m_sampleCount;
    }

    std::string toString() const { return "DirectLMS[]"; }

private:
    int m_sampleCount;
};

REGISTER_CLASS(DirectLMS, "direct_lms")
