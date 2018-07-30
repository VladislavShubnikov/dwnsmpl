// ****************************************************************************
// FILE: mtypes.h
// PURP: main basic and simple types definitions
// ****************************************************************************

#ifndef __mtypes_h
#define __mtypes_h

#pragma once

// ****************************************************************************
// Defines
// ****************************************************************************

#define USE_PARAM(s)          (s)

#ifndef M_PI
  #define M_PI                3.1415926535f
#endif

// ****************************************************************************
// Types
// ****************************************************************************

typedef unsigned long long    MUint64;
typedef unsigned int          MUint32;
typedef unsigned short int    MUint16;
typedef unsigned char         MUint8;

typedef long long             MInt64;
typedef int                   MInt32;
typedef short int             MInt16;
typedef char                  MInt8;


/** \class V2d
*  \brief Describes 2d point with integer coordinates
*/

class V2d
{
public:
  int x, y;

public:
  explicit V2d();
  V2d(const int xx, const int yy);
  V2d(const V2d &v);
  void set(const int xx, const int yy);

  int dotProduct(const V2d &v) const;
  float distTo(const V2d &v) const;
  V2d&  operator=(const V2d &v);
  void  scaleBy(const int s);
  void  add(const V2d &va, const V2d &vb);
  void  addWith(const V2d &v);
  void  sub(const V2d &va, const V2d &vb);
  void  subWith(const V2d &v);
};

/** \class V3d
*  \brief Describes 3d point with integer coordinates
*/

struct V3d
{
  int x, y, z;
};

/** \class V4d
*  \brief Describes 4d point with integer coordinates
*/

struct V4d
{
  int x, y, z, w;
};

/** \class V2f
*  \brief Describes 2d point with float coordinates
*/

class V2f
{
public:
  float x, y;

public:
  explicit V2f();
  V2f(const float xx, const float yy);
  explicit V2f(const V2f &v);
  explicit V2f(const V2d &v);
  void set(const float xx, const float yy);

  float dotProduct(const V2f &v) const;
  float distTo(const V2f &v) const;
  int   normalize();
  V2f&  operator=(const V2f &v);
  void  scaleBy(const float s);
  void  add(const V2f &va, const V2f &vb);
  void  addWith(const V2f &v);
  void  sub(const V2f &va, const V2f &vb);
  void  subWith(const V2f &v);
};

/** \class V3f
*  \brief Describes 3d point with float coordinates
*/

class V3f
{
public:
  float x, y, z;

public:
  explicit V3f();
  V3f(const float xx, const float yy, const float zz);
  V3f(const V3f &v);
  void set(const float xx, const float yy, const float zz);
  V3f& operator=(const V3f &v);

  float dotProduct(const V3f &v) const;
  float length() const;
  float distTo(const V3f &v) const;
  int   normalize();
  void  cross(const V3f &v0, const V3f &v1);

  void  scaleBy(const float s);
  void  add(const V3f &va, const V3f &vb);
  void  addWith(const V3f &v);
  void  sub(const V3f &va, const V3f &vb);
  void  subWith(const V3f &v);

};

/** \class TriangleIndices
*  \brief Describes integer triangle indices structre
*/

class TriangleIndices
{
public:
  int     m_idices[3];
public:
  TriangleIndices();
  TriangleIndices(const int ia, const int ib, const int ic);
};

/** \struct V4f
*  \brief Describes 4d point with float coordinates
*/

struct V4f
{
  float x, y, z, w;
};



#endif
