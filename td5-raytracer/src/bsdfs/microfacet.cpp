/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob
*/

#include "bsdf.h"
#include "core/warp.h"
#include "frame.h"

static Vector3f n = Vector3f(0.f, 0.f, 1.f);

static float xi_plus(float c) { return c > 0.f ? 1.f : 0.f; }
static float xi_plus_k(float b) { return b < 1.6f ? (3.535f * b + 2.181f * powf(b, 2.f)) / (1.f + 2.276f * b + 2.577f * powf(b, 2.f)) : 1.f; }

class Microfacet : public BSDF
{
public:
    Microfacet(const PropertyList &propList)
    {
        /* RMS surface roughness */
        m_alpha = propList.getFloat("alpha", 0.1f);

        /* Interior IOR (default: BK7 borosilicate optical glass) */
        m_intIOR = propList.getFloat("intIOR", 1.5046f);

        /* Exterior IOR (default: air) */
        m_extIOR = propList.getFloat("extIOR", 1.000277f);

        /* Albedo of the diffuse base material (a.k.a "kd") */
        m_kd = propList.getColor("kd", Color3f(0.5f));

        /* To ensure energy conservation, we must scale the
           specular component by 1-kd.

           While that is not a particularly realistic model of what
           happens in reality, this will greatly simplify the
           implementation. */
        m_ks = 1 - m_kd.maxCoeff();
    }

    /// Evaluate the BRDF for the given pair of directions
    Color3f eval(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle || Frame::cosTheta(bRec.wi) <= 0.f || Frame::cosTheta(bRec.wo) <= 0.f)
            return Color3f(0.f);

        const Vector3f wh = (bRec.wi + bRec.wo).normalized();

        const float D = Warp::squareToBeckmannPdf(wh, m_alpha);

        float cosThetaT;
        const float F = fresnel(bRec.wi.dot(wh), m_extIOR, m_intIOR, cosThetaT);

        float gi = xi_plus(bRec.wi.dot(wh) / bRec.wi.dot(n));
        float bi = powf(m_alpha * Frame::tanTheta(bRec.wi), -1);
        float ki = xi_plus_k(bi);

        float go = xi_plus(bRec.wo.dot(wh) / bRec.wo.dot(n));
        float bo = powf(m_alpha * Frame::tanTheta(bRec.wo), -1);
        float ko = xi_plus_k(bo);

        float G = gi * ki * go * ko;

        return m_kd / M_PI + m_ks * (D * F * G) / (4.f * bRec.wi.dot(n) * bRec.wo.dot(n) * wh.dot(n));
    }

    /// Evaluate the sampling density of \ref sample() wrt. solid angles
    float pdf(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle || Frame::cosTheta(bRec.wi) <= 0.f || Frame::cosTheta(bRec.wo) <= 0.f)
            return 0.f;
        
        const Vector3f wh = (bRec.wi + bRec.wo).normalized();

        const float D = Warp::squareToBeckmannPdf(wh, m_alpha);
        
        const float J = powf(4.f * bRec.wo.dot(wh), -1);
        return m_ks * D * J + (1 - m_ks) * Frame::cosTheta(bRec.wo) / M_PI;
    }

    /// Sample the BRDF
    Color3f sample(BSDFQueryRecord &bRec, const Point2f &sample) const
    {
        if (Frame::cosTheta(bRec.wi) <= 0)
            return Color3f(0.f);

        bRec.measure = ESolidAngle;

        Point2f transformed_sample(sample);
        if (sample.x() < m_ks)
        {
            // Specular
            transformed_sample.x() /= m_ks;
            Vector3f normal = (Warp::squareToBeckmann(transformed_sample, m_alpha)).normalized();
            bRec.wo = 2.f * bRec.wi.dot(normal) * normal - bRec.wi;
        }
        else
        {
            // Diffuse
            transformed_sample.x() = (sample.x() - m_ks) / (1 - m_ks);
            bRec.wo = Warp::squareToCosineHemisphere(transformed_sample);
        }

        bRec.eta = 1.f;

        if (pdf(bRec) == 0)
            return Color3f(0.f);

        return eval(bRec) / pdf(bRec) * Frame::cosTheta(bRec.wo);
    }

    bool isDiffuse() const
    {
        /* While microfacet BRDFs are not perfectly diffuse, they can be
           handled by sampling techniques for diffuse/non-specular materials,
           hence we return true here */
        return true;
    }

    std::string toString() const
    {
        return tfm::format("Microfacet[\n"
                           "  alpha = %f,\n"
                           "  intIOR = %f,\n"
                           "  extIOR = %f,\n"
                           "  kd = %s,\n"
                           "  ks = %f\n"
                           "]",
                           m_alpha, m_intIOR, m_extIOR, m_kd.toString(), m_ks);
    }

private:
    float m_alpha;
    float m_intIOR, m_extIOR;
    float m_ks;
    Color3f m_kd;
};

REGISTER_CLASS(Microfacet, "microfacet");
