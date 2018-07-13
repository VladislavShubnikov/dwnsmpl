// ****************************************************************************
// Includes
// ****************************************************************************

#include <math.h>

#include "mtypes.h"

// ****************************************************************************
// V2d
// ****************************************************************************

V2d::V2d()
{
  x = y = 0;
}

V2d::V2d(const int xx, const int yy)
{
  x = xx; y = yy;
}

V2d::V2d(const V2d &v)
{
  x = v.x; y = v.y;
}
void V2d::set(const int xx, const int yy)
{
  x = xx; y = yy;
}

V2d& V2d::operator=(const V2d &v)
{
  x = v.x;
  y = v.y;
  return *this;
}

int V2d::dotProduct(const V2d &v) const
{
  return (x * v.x + y * v.y);
}

float V2d::distTo(const V2d &v) const
{
  float dx = (float)x - v.x;
  float dy = (float)y - v.y;
  float d = dx * dx + dy * dy;
  d = sqrtf(d);
  return d;
}
void  V2d::scaleBy(const int s)
{
  x *= s;
  y *= s;
}
void  V2d::add(const V2d &va, const V2d &vb)
{
  x = va.x + vb.x;
  y = va.y + vb.y;
}
void  V2d::addWith(const V2d &v)
{
  x += v.x;
  y += v.y;
}

void  V2d::sub(const V2d &va, const V2d &vb)
{
  x = va.x - vb.x;
  y = va.y - vb.y;
}
void  V2d::subWith(const V2d &v)
{
  x -= v.x;
  y -= v.y;
}


// ****************************************************************************
// V2f
// ****************************************************************************

V2f::V2f()
{
  x = y = 0.0f;
}

V2f::V2f(const float xx, const float yy)
{
  x = xx; y = yy;
}

V2f::V2f(const V2f &v)
{
  x = v.x; y = v.y;
}
V2f::V2f(const V2d &v)
{
  x = (float)v.x; y = (float)v.y;
}

void V2f::set(const float xx, const float yy)
{
  x = xx; y = yy;
}

V2f& V2f::operator=(const V2f &v)
{
  x = v.x;
  y = v.y;
  return *this;
}

float V2f::dotProduct(const V2f &v) const
{
  return (x * v.x + y * v.y);
}

float V2f::distTo(const V2f &v) const
{
  float dx = x - v.x;
  float dy = y - v.y;
  float d = dx * dx + dy * dy;
  d = sqrtf(d);
  return d;
}
int   V2f::normalize()
{
  float len2 = x * x + y * y;
  if (len2 > 1.0e-9f)
  {
    const float scale = 1.0f / sqrtf(len2);
    x *= scale;
    y *= scale;
    return 1;
  }
  else
    return 0;
}
void  V2f::scaleBy(const float s)
{
  x *= s;
  y *= s;
}
void  V2f::add(const V2f &va, const V2f &vb)
{
  x = va.x + vb.x;
  y = va.y + vb.y;
}
void  V2f::addWith(const V2f &v)
{
  x += v.x;
  y += v.y;
}

void  V2f::sub(const V2f &va, const V2f &vb)
{
  x = va.x - vb.x;
  y = va.y - vb.y;
}
void  V2f::subWith(const V2f &v)
{
  x -= v.x;
  y -= v.y;
}

// ****************************************************************************
// V3f
// ****************************************************************************

V3f::V3f()
{
  x = y = z = 0.0f;
}

V3f::V3f(const float xx, const float yy, const float zz)
{
  x = xx; y = yy;
  z = zz;
}

V3f::V3f(const V3f &v)
{
  x = v.x; y = v.y;
  z = v.z;
}
void V3f::set(const float xx, const float yy, const float zz)
{
  x = xx; y = yy;
  z = zz;
}
V3f& V3f::operator=(const V3f &v)
{
  x = v.x;
  y = v.y;
  z = v.z;
  return *this;
}


float V3f::dotProduct(const V3f &v) const
{
  return (x * v.x + y * v.y + z * v.z);
}

float V3f::distTo(const V3f &v) const
{
  float dx = x - v.x;
  float dy = y - v.y;
  float dz = z - v.z;
  float d = dx * dx + dy * dy + dz * dz;
  d = sqrtf(d);
  return d;
}
float V3f::length() const
{
  float len = sqrtf(x * x + y * y + z * z);
  return len;
}
int   V3f::normalize()
{
  float len2 = x * x + y * y + z * z;
  if (len2 > 1.0e-14f)
  {
    const float scale = 1.0f / sqrtf(len2);
    x *= scale;
    y *= scale;
    z *= scale;
    return 1;
  }
  else
    return 0;
}

void  V3f::cross(const V3f &v0, const V3f &v1)
{
  x = v0.y * v1.z - v0.z * v1.y;
  y = v0.z * v1.x - v0.x * v1.z;
  z = v0.x * v1.y - v0.y * v1.x;
}

void  V3f::scaleBy(const float s)
{
  x *= s;
  y *= s;
  z *= s;
}

void  V3f::add(const V3f &va, const V3f &vb)
{
  x = va.x + vb.x;
  y = va.y + vb.y;
  z = va.z + vb.z;
}

void  V3f::addWith(const V3f &v)
{
  x += v.x;
  y += v.y;
  z += v.z;
}

void  V3f::sub(const V3f &va, const V3f &vb)
{
  x = va.x - vb.x;
  y = va.y - vb.y;
  z = va.z - vb.z;
}

void  V3f::subWith(const V3f &v)
{
  x -= v.x;
  y -= v.y;
  z -= v.z;
}

//  **********************************************************
// TriangleIndices
//  **********************************************************

TriangleIndices::TriangleIndices()
{
  m_idices[0] = m_idices[1] = m_idices[2] = -1;
}
TriangleIndices::TriangleIndices(const int ia, const int ib, const int ic)
{
  m_idices[0] = ia;
  m_idices[1] = ib;
  m_idices[2] = ic;
}
