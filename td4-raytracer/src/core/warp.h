#pragma once

#include "common.h"
#include "vector.h"

/// A collection of useful warping functions for importance sampling.
class Warp
{
public:
    /// Dummy warping function: takes uniformly distributed points in a square and just returns them.
    static Point2f squareToUniformSquare(const Point2f &sample);

    /// Probability density of squareToUniformSquare().
    static float squareToUniformSquarePdf(const Point2f &p);

    /// Uniformly sample a vector on the unit sphere with respect to solid angles.
    static Vector3f squareToUniformSphere(const Point2f &sample);

    /// Probability density of squareToUniformSphere().
    static float squareToUniformSpherePdf(const Vector3f &v);

    /// Uniformly sample a vector on a 2D disk with radius 1, centered around the origin.
    static Point2f squareToUniformDisk(const Point2f &sample);

    /// Probability density of squareToUniformDisk().
    static float squareToUniformDiskPdf(const Point2f &p);

    /// Uniformly sample a vector on the unit hemisphere around the pole (0,0,1) with respect to solid angles.
    static Vector3f squareToUniformHemisphere(const Point2f &sample);

    /// Probability density of squareToUniformHemisphere().
    static float squareToUniformHemispherePdf(const Vector3f &v);

    /// Uniformly sample a vector on the unit hemisphere around the pole (0,0,1) with respect to projected solid angles.
    static Vector3f squareToCosineHemisphere(const Point2f &sample);

    /// Probability density of squareToCosineHemisphere().
    static float squareToCosineHemispherePdf(const Vector3f &v);

    /// Uniformly sample a vector on a 2D isosceles right triangle of area 1/2.
    static Point2f squareToUniformTriangle(const Point2f &sample);

    /// Probability density of squareToUniformTriangle().
    static float squareToUniformTrianglePdf(const Point2f &p);

    /// Warp a uniformly distributed square sample to a Beckmann distribution * cosine for the given 'alpha' parameter.
    static Vector3f squareToBeckmann(const Point2f &sample, float alpha);

    /// Probability density of squareToBeckmann().
    static float squareToBeckmannPdf(const Vector3f &m, float alpha);
};
