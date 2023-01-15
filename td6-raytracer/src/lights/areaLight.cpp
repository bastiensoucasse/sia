#include "lights/areaLight.h"
#include "core/bsdf.h"
#include "core/texture.h"
#include "core/warp.h"

AreaLight::AreaLight(const PropertyList &propList) {
  m_intensity = propList.getColor("radiance", Color3f(1.f));
  m_twoSided = propList.getBoolean("twoSided", false);
  m_texture = new Texture(m_intensity, propList);
  m_flags = (int)LightFlags::Area;
}

void AreaLight::addChild(Object *obj) {
  switch (obj->getClassType()) {
  case EShape:
    if (m_shape)
      throw RTException(
          "AreaLight: tried to register multiple shape instances!");
    m_shape = static_cast<Shape *>(obj);
    break;
  default:
    throw RTException("AreaLight::addChild(<%s>) is not supported!",
                      classTypeName(obj->getClassType()));
  }
}

Color3f AreaLight::intensity(const Hit &hit, const Vector3f &w) const {
  return (m_twoSided || hit.localFrame.n.dot(w) > 0.f)
             ? m_texture->lookUp(hit.uv)
             : Color3f(0.f);
}

Color3f AreaLight::sample(const Point3f &x, const Point2f &u, float &pdf,
                          Vector3f &wi, float &dist) const {
  Point3f y;
  Normal3f n;
  m_shape->sample(u, y, n, pdf);
  wi = y - x;
  float d2 = wi.squaredNorm();
  dist = std::sqrt(d2);
  wi /= dist;
  Hit h;
  h.localFrame = Frame(n);
  h.uv = u;
  // convert PDF value with respect to solid angle
  float absDot = std::max(0.f, n.dot(-wi));
  if (absDot < Epsilon)
    pdf = 0.f;
  else
    pdf *= d2 / absDot;
  return intensity(h, -wi);
}

// Photon AreaLight::samplePhoton(const Point2f &samplePos,
//                                const Point2f &sampleDir) const {
//   // Random position
//   Point3f pos;
//   Normal3f n;
//   float pdf;
//   m_shape->sample(samplePos, pos, n, pdf);

//   // Random direction
//   Vector3f d = Warp::squareToCosineHemisphere(sampleDir);
//   Hit h;
//   h.localFrame = Frame(n);
//   h.uv = samplePos;
//   Vector3f dir = h.toWorld(d);

//   Color3f power = M_PI * m_shape->area() * intensity(h, dir);

//   return Photon(pos, dir, power);
// }

std::string AreaLight::toString() const {
  return tfm::format("AreaLight[\n"
                     "  intensity = %s,\n"
                     "  two-sided = %f,\n"
                     "  shape = %f,\n"
                     "]",
                     m_intensity.toString(), m_twoSided ? "true" : "false",
                     indent(m_shape->toString()));
}

REGISTER_CLASS(AreaLight, "areaLight")
