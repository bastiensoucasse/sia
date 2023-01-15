#include "bsdf.h"
#include "core/warp.h"
#include "integrator.h"
#include "scene.h"

class AO : public Integrator
{
public:
    AO(const PropertyList &props)
    {
        m_sampleCount = props.getInteger("sampleCount", 10);
        m_cosineWeighted = props.getBoolean("cosineWeighted", true);
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray &ray) const
    {
        Hit hit;
        scene->intersect(ray, hit);
        if (!hit.foundIntersection())
            return Color3f(0.f);

        float L = 0.f;
        for (int i = 0; i < m_sampleCount; i++)
        {
            Point2f p = sampler->next2D();
            Vector3f d = m_cosineWeighted ? Warp::squareToCosineHemisphere(p) : Warp::squareToUniformHemisphere(p);
            float pdf = m_cosineWeighted ? Warp::squareToCosineHemispherePdf(d) : Warp::squareToUniformHemispherePdf(d);

            Ray occRay(ray.at(hit.t) + hit.localFrame.n * Epsilon, hit.toWorld(d));
            Hit occHit;

            scene->intersect(occRay, occHit);
            if (!occHit.foundIntersection())
                L += d.z() / pdf;
        }
        L /= (M_PI * m_sampleCount);

        return Color3f(L);
    }

    std::string toString() const
    {
        return tfm::format("AO[\n  samples = %f\n  cosine-weighted = %s\n ]", m_sampleCount, m_cosineWeighted ? "true" : "false");
    }

private:
    int m_sampleCount;
    bool m_cosineWeighted;
};

REGISTER_CLASS(AO, "ao")
