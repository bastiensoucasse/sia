#include "bsdf.h"
#include "frame.h"
#include "integrator.h"
#include "scene.h"
#include "warp.h"

class DirectMats : public Integrator
{
public:
    DirectMats(const PropertyList &props)
    {
        m_sampleCount = props.getInteger("sampleCount", 100);
        m_IS = props.getBoolean("IS", false);
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray &ray) const
    {
        // Check if a surface is found.
        Hit hit;
        scene->intersect(ray, hit);
        if (!hit.foundIntersection())
            return scene->backgroundColor(ray.direction);

        // Retrieve the surface point position and normal.
        const Point3f &position = ray.at(hit.t);
        const Normal3f &normal = hit.localFrame.n;
        const BSDF *bsdf = hit.shape->bsdf();

        // Initializes the radiance to 0.
        Color3f radiance = Color3f::Zero();

        // Cast rays from the surface towards the background.
        for (int sample = 0; sample < m_sampleCount; sample++)
        {
            if (m_IS)
            {
                BSDFQueryRecord bRec(hit.toLocal(-ray.direction));
                const Color3f sample = bsdf->sample(bRec, sampler->next2D());
                const Color3f intensity = scene->backgroundColor(hit.toWorld(bRec.wo));
                radiance += intensity * sample;
                continue;
            }

            const Vector3f lightDir = Warp::squareToCosineHemisphere(sampler->next2D());
            const float pdf = Warp::squareToCosineHemispherePdf(lightDir);
            const Color3f intensity = scene->backgroundColor(hit.toWorld(lightDir));

            const Ray lightRay(position + normal * Epsilon, hit.toWorld(lightDir), true);

            Hit lightHit;
            scene->intersect(lightRay, lightHit);
            if (!lightHit.foundIntersection())
            {
                const float cos_term = std::max(0.f, hit.toWorld(lightDir).dot(normal));
                const Color3f brdf = bsdf->eval(BSDFQueryRecord(hit.toLocal(-ray.direction), lightDir, ESolidAngle, hit.uv));

                radiance += intensity * cos_term * brdf / pdf;
            }
        }

        return radiance / m_sampleCount;
    }

    std::string toString() const { return "DirectMats[]"; }

private:
    int m_sampleCount;
    bool m_IS;
};

REGISTER_CLASS(DirectMats, "direct_mats")
