#include "warp.h"

#include "frame.h"
#include "vector.h"

Point2f Warp::squareToUniformSquare(const Point2f &sample)
{
    return sample;
}

float Warp::squareToUniformSquarePdf(const Point2f &sample)
{
    // return ((sample.array() >= 0).all() && (sample.array() <= 1).all()) ? 1.0f : 0.0f;

    return 1.0f;
}

Vector3f Warp::squareToUniformSphere(const Point2f &sample)
{
    float z = 1.f - 2.f * sample.x();
    float r = std::sqrt(std::max(0.f, 1.f - z * z));
    float phi = 2.f * M_PI * sample.y();
    return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}

float Warp::squareToUniformSpherePdf(const Vector3f &v)
{
    return INV_FOURPI;
}

Point2f Warp::squareToUniformDisk(const Point2f &sample)
{
    float r = std::sqrt(sample.x());
    float phi = 2.f * M_PI * sample.y();
    return Point2f(r * std::cos(phi), r * std::sin(phi));
}

float Warp::squareToUniformDiskPdf(const Point2f &p)
{
    return std::pow(p.x(), 2) + std::pow(p.y(), 2) <= 1 ? INV_PI : 0.f;
}

Vector3f Warp::squareToUniformHemisphere(const Point2f &sample)
{
    float phi = 2.f * M_PI * sample.x();
    float theta = std::acos(sample.y());
    return Vector3f(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
}

float Warp::squareToUniformHemispherePdf(const Vector3f &v)
{
    return v.z() >= 0 ? INV_TWOPI : 0.f;
}

Vector3f Warp::squareToCosineHemisphere(const Point2f &sample)
{
    float phi = 2.f * M_PI * sample.x();
    float theta = std::acos(std::sqrt(1 - sample.y()));
    return Vector3f(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
}

float Warp::squareToCosineHemispherePdf(const Vector3f &v)
{
    return v.z() >= 0 ? v.z() / M_PI : 0.f;
}

Point2f Warp::squareToUniformTriangle(const Point2f &sample)
{
    return Point2f(1 - sqrt(sample.x()), sample.y() * sqrt(sample.x()));
}

float Warp::squareToUniformTrianglePdf(const Point2f &p)
{
    return p.x() + p.y() < 1 ? .5f : .0f;
}

Vector3f Warp::squareToBeckmann(const Point2f &sample, float alpha)
{
    float phi = 2.f * M_PI * sample.x();
    float theta = atan(sqrt(-powf(alpha, 2.f) * log(1.f - sample.y())));

    float cosPhi = cos(phi);
    float sinPhi = sin(phi);
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    return Vector3f(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

float Warp::squareToBeckmannPdf(const Vector3f &m, float alpha)
{
    if (m.z() <= 0.f)
        return 0.f;

    float alpha2 = powf(alpha, 2.f);
    float cosTheta = Frame::cosTheta(m);
    float tanTheta2 = powf(Frame::tanTheta(m), 2.f);
    float cosTheta3 = powf(cosTheta, 3.f);

    float azimuthal = INV_TWOPI;
    float longitudinal = 2.f * exp(-tanTheta2 / alpha2) / (alpha2 * cosTheta3);

    return azimuthal * longitudinal;
}
