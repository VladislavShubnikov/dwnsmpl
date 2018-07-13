// ****************************************************************************
// File: dump.cpp
// Purpose: Save file to BMP and STL
// ****************************************************************************

// ****************************************************************************
// Includes
// ****************************************************************************

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <io.h>
#include <assert.h>

#include "memtrack.h"
#include "dump.h"

// ****************************************************************************
// Types
// ****************************************************************************
#pragma pack(push, 2)

struct BitmapHeader
{
  MUint16   bfType;
  MUint32   bfSize;
  MUint16   bfReserved1;
  MUint16   bfReserved2;
  MUint32   bfOffBits;
};

struct BitmapInfo
{
  MUint32   biSize;
  MInt32    biWidth;
  MInt32    biHeight;
  MUint16   biPlanes;
  MUint16   biBitCount;
  MUint32   biCompression;
  MUint32   biSizeImage;
  MInt32    biXPelsPerMeter;
  MInt32    biYPelsPerMeter;
  MUint32   biClrUsed;
  MUint32   biClrImportant;
};

#pragma pack(pop)

// ****************************************************************************
// Data
// ****************************************************************************


// ****************************************************************************
// Methods
// ****************************************************************************


static int _saveBmpBufferArgbToFile(const MUint8 *bufRGB, const int imageW, const int imageH, const char* fileName)
{
  BitmapHeader    head;
  BitmapInfo      info;

  int numPixels = imageW * imageH;
  int pixStride = 3 * imageW;
  pixStride = (pixStride + 3) & (~3);

  const int szHead = sizeof(BitmapHeader);
  const int szInfo = sizeof(BitmapInfo);

  head.bfType           = 0x4D42;      // "BM"
  head.bfSize           = szHead + szInfo + imageH * pixStride;
  head.bfReserved1      = 0;
  head.bfReserved2      = 0;
  head.bfOffBits        = szHead + szInfo;

  info.biSize           = szInfo;
  info.biWidth          = imageW;
  info.biHeight         = imageH;
  info.biPlanes         = 1;
  info.biBitCount       = 3 * 8;      // 3 color components as bytes
  info.biCompression    = 0;
  info.biSizeImage      = imageH * pixStride;
  info.biXPelsPerMeter  = 0;
  info.biYPelsPerMeter  = 0;
  info.biClrUsed        = 0;
  info.biClrImportant   = 0;

  FILE *file = fopen(fileName, "wb");
  if (!file)
    return 0;
  fwrite(&head, 1, sizeof(head), file);
  fwrite(&info, 1, sizeof(info), file);
  fwrite(bufRGB, 1, numPixels * 3, file);
  fclose(file);
  return 1;
}

int Dump::saveImageGreyToBitmapFile(
                                    const MUint32 *pixelsGrey,
                                    const int     imageW,
                                    const int     imageH,
                                    const char    *fileName
                                   )
{
  int pixStride = 3 * imageW;
  pixStride = (pixStride + 3) & (~3);

  // prepare RGB image from greyscale image
  MUint8 *bufRGB = M_NEW(MUint8[imageH * pixStride]);
  if (!bufRGB)
    return -1;
  for (int y = 0; y < imageH; y++)
  {
    // inverse y 
    const MUint32 *src = pixelsGrey + (y * imageW);
    MUint8        *dst = bufRGB + (imageH - 1 - y) * pixStride;

    for (int x = 0; x < imageW; x++)
    {
      MUint8 val = (MUint8)src[0];
      *dst++ = val;
      *dst++ = val;
      *dst++ = val;
      src++;
    }     // for (x)
  }       // for (y)

  int ok = _saveBmpBufferArgbToFile(bufRGB, imageW, imageH, fileName);
  delete[] bufRGB;
  return ok;
}

int Dump::saveImageArgbToBitmapFile(
                                    const MUint32 *pixelsArgb,
                                    const int imageW,
                                    const int imageH,
                                    const char* fileName
                                   )
{
  // Prepare buffer
  int pixStride = 3 * imageW;
  pixStride = (pixStride + 3) & (~3);
  MUint8 *bufRGB = M_NEW(MUint8[imageH * pixStride]);
  if (!bufRGB)
    return -1;

  for (int y = 0; y < imageH; y++)
  {
    // inverse y 
    const MUint32 *src = pixelsArgb + (y * imageW);
    MUint8        *dst = bufRGB + (imageH - 1 - y) * pixStride;

    for (int x = 0; x < imageW; x++, src++)
    {
      MUint32 val = *src;
      MUint32 bCol = val & 0xff; val >>= 8;
      MUint32 gCol = val & 0xff; val >>= 8;
      MUint32 rCol = val & 0xff;

      *dst++ = (MUint8)bCol;
      *dst++ = (MUint8)gCol;
      *dst++ = (MUint8)rCol;
    }     // for (x)
  }       // for (y)
  int ok = _saveBmpBufferArgbToFile(bufRGB, imageW, imageH, fileName);
  delete[] bufRGB;
  return ok;
}

int Dump::saveFieldToBitmapFile(
                                const float *field,
                                const int   w,
                                const int   h,
                                const char  *fileName
                               )
{
  int numPixels = w * h;
  MUint32 *pixels = M_NEW(MUint32[numPixels]);
  if (!pixels)
    return -1;
  // find min, max
  float valMin = +1.0e12f;
  float valMax = -1.0e12f;
  int i;
  for (i = 0; i < numPixels; i++)
  {
    valMin = (field[i] < valMin) ? field[i] : valMin;
    valMax = (field[i] > valMax) ? field[i] : valMax;
  }
  valMax += 0.9f;
  const float scale = 255.0f / (valMax - valMin);
  for (i = 0; i < numPixels; i++)
  {
    float val = (field[i] - valMin) * scale;
    pixels[i] = (MUint32)(val);
  }

  int ok = Dump::saveImageGreyToBitmapFile(pixels, w, h, fileName);
  delete[] pixels;
  return ok;
}

int Dump::saveHeightMapGeoToObjFile(
                                    const float *heights,
                                    const int w,
                                    const int h,
                                    const char* fileNameMtl,
                                    const char* fileNameObj
                                  )
{
  FILE    *file;

  static char *strHeadComment = "# LevelSet engine exporter \n";
  static char *strMtlGreen    = "Green";
  static char *strMtlBlue     = "Blue";
  static char *strMtlGrey     = "Grey";

  // write MTL file
  file = fopen(fileNameMtl, "wt");
  if (!file)
    return -1;
  fprintf(file, "%s", strHeadComment);
  fprintf(file, "\n");

  // green
  fprintf(file, "newmtl %s\n", strMtlGreen);
  fprintf(file, "d 1\n");
  fprintf(file, "Tr 1\n");
  fprintf(file, "Tf 1 1 1\n");
  fprintf(file, "illum 2\n");
  fprintf(file, "Ka 0.0 0.1 0.0\n");
  fprintf(file, "Kd 0.3 0.7 0.3\n");
  fprintf(file, "Ks 0.2 0.2 0.2\n");
  fprintf(file, "\n");

  // blue
  fprintf(file, "newmtl %s\n", strMtlBlue);
  fprintf(file, "d 1\n");
  fprintf(file, "Tr 1\n");
  fprintf(file, "Tf 1 1 1\n");
  fprintf(file, "illum 2\n");
  fprintf(file, "Ka 0.0 0.0 0.1\n");
  fprintf(file, "Kd 0.3 0.3 0.7\n");
  fprintf(file, "Ks 0.2 0.2 0.2\n");
  fprintf(file, "\n");

  // grey
  fprintf(file, "newmtl %s\n", strMtlGrey);
  fprintf(file, "d 1\n");
  fprintf(file, "Tr 0.6\n");
  fprintf(file, "Tf 0.6 0.6 0.6\n");
  fprintf(file, "illum 2\n");
  fprintf(file, "Ka 0.1 0.1 0.1\n");
  fprintf(file, "Kd 0.6 0.6 0.6\n");
  fprintf(file, "Ks 0.2 0.2 0.2\n");
  fprintf(file, "\n");

  fclose(file);

  // write OBJ file

  int numPixels = w * h;
  int numFaces = (w - 1) * (h - 1) * 2;

  int sizeVertices = 34 * numPixels;
  int sizeNormals = 34 * numPixels;
  int sizeFaces   = 52 * numFaces;
  // estimation
  int sizeBuffer = sizeVertices + sizeNormals + sizeFaces + 128;
  char *strBuffer = M_NEW(char[sizeBuffer]);
  if (!strBuffer)
    return -1;
  memset(strBuffer, 0, sizeBuffer);


  char *s = strBuffer;
  sprintf(s, "%s", strHeadComment);
  s += strlen(s);

  sprintf(s, "\n");
  s += strlen(s);

  const char *sLib = strrchr(fileNameMtl, '/');
  sLib++;
  static char strMtlLib[96];
  sprintf(strMtlLib, "mtllib %s\n", sLib);

  sprintf(s, "%s", strMtlLib);
  s += strlen(s);

  sprintf(s, "#\n");
  s += strlen(s);
  sprintf(s, "# Object objField\n");
  s += strlen(s);
  sprintf(s, "#\n");
  s += strlen(s);

  // Get min max
  float valMin = +1.0e12f;
  float valMax = -1.0e12f;
  int i;
  for (i = 0; i < numPixels; i++)
  {
    float v = heights[i];
    #ifdef _DEBUG
    if (isinf(v))
    {
      assert(!isinf(v));
      v = 0.0f;
    }
    if (isnan(v))
    {
      assert(!isnan(v));
      v = 0.0f;
    }
    #endif

    valMin = (v < valMin) ? v : valMin;
    valMax = (v > valMax) ? v : valMax;
  }
  valMax += 0.9f;
  const float VOL_DIM = 100.0f;
  const float zScale = VOL_DIM / (valMax - valMin);
  const float xScale = VOL_DIM / w;
  const float yScale = VOL_DIM / h;

  float fx, fy, fz;
  int x, y;
  // write all vertices
  i = 0;
  for (y = 0; y < h; y++)
  {
    fy = y * yScale;
    for (x = 0; x < w; x++)
    {
      fx = x * xScale;
      float val = heights[i++];
      fz = (val - valMin) * zScale;

      sprintf(s, "v  %f %f %f\n", fx, fy, fz);
      s += strlen(s);
    }   // for (x)
  }     // for (y)

  // write additional rect pixels
  int indexRect = i;
  {
    fx = (-w * 0.2f) * xScale;
    fy = (-h * 0.2f) * yScale;
    fz = (0.0f - valMin) * zScale;
    i++;

    sprintf(s, "v  %f %f %f\n", fx, fy, fz);
    s += strlen(s);

    fx = (w * 1.2f) * xScale;
    fy = (-h * 0.2f) * yScale;
    fz = (0.0f - valMin) * zScale;
    i++;

    sprintf(s, "v  %f %f %f\n", fx, fy, fz);
    s += strlen(s);

    fx = (-w * 0.2f) * xScale;
    fy = (h * 1.2f) * yScale;
    fz = (0.0f - valMin) * zScale;
    i++;

    sprintf(s, "v  %f %f %f\n", fx, fy, fz);
    s += strlen(s);

    fx = (w * 1.2f) * xScale;
    fy = (h * 1.2f) * yScale;
    fz = (0.0f - valMin) * zScale;
    i++;

    sprintf(s, "v  %f %f %f\n", fx, fy, fz);
    s += strlen(s);
  }


  // write final num verts
  sprintf(s, "# %d vertices\n", i);
  s += strlen(s);

  // write normals
  i = 0;
  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      float dx, dy;
      if ((x > 0) && (x < w - 1))
      {
        dx = 0.5f * (heights[i + 1] - heights[i - 1]);
      }
      else if (x == 0)
      {
        dx = heights[i + 1] - heights[i];
      }
      else
      {
        dx = heights[i] - heights[i - 1];
      }

      if ((y > 0) && (y < h - 1))
      {
        dy = 0.5f * (heights[i + w] - heights[i - w]);
      }
      else if (y == 0)
      {
        dy = heights[i + w] - heights[i];
      }
      else
      {
        dy = heights[i] - heights[i - w];
      }
      V3f vX(1.0f, 0.0f, dx);
      V3f vY(0.0f, 1.0f, dy);
      V3f vNormal;
      vNormal.cross(vX, vY);
      vNormal.normalize();
      sprintf(s, "vn %f %f %f\n", vNormal.x, vNormal.y, vNormal.z);
      s += strlen(s);

      i++;
    }
  }
  // write quad normals
  for (int k = 0; k < 4; k++)
  {
    V3f vN(0.0f, 0.0f, 1.0f);
    sprintf(s, "vn %f %f %f\n", vN.x, vN.y, vN.z);
    s += strlen(s);
    i++;
  }


  sprintf(s, "# %d normals\n", i);
  s += strlen(s);

  // write geo
  sprintf(s, "g objField\n");
  s += strlen(s);
  sprintf(s, "usemtl %s\n", strMtlGreen);
  s += strlen(s);

  assert(s - strBuffer < sizeBuffer);

  i = 0;
  for (y = 0; y < h - 1; y++)
  {
    int yOff = y * w;
    for (x = 0; x < w - 1; x++)
    {
      //
      //  A     B
      //  +-----+
      //  |    /|
      //  |  /  |
      //  |/    |
      //  +-----+
      //  C     D
      //
      int offA = yOff + x;
      int offB = offA + 1;
      int offC = offA + w;
      int offD = offC + 1;

      int i0, i1, i2;

      i0 = 1 + offC;
      i1 = 1 + offB;
      i2 = 1 + offA;
      sprintf(s, "f %d//%d %d//%d %d//%d\n", i0, i0, i1, i1, i2, i2);
      s += strlen(s);
      i++;

      i0 = 1 + offB;
      i1 = 1 + offC;
      i2 = 1 + offD;
      sprintf(s, "f %d//%d %d//%d %d//%d\n", i0, i0, i1, i1, i2, i2);
      s += strlen(s);
      i++;
    }   // for (x) on grid
  }     // for (y) on grid


  // write rect
  {
    sprintf(s, "g objRect\n");
    s += strlen(s);
    sprintf(s, "usemtl %s\n", strMtlGrey);
    s += strlen(s);

    int offA = indexRect + 0;
    int offB = indexRect + 1;
    int offC = indexRect + 2;
    int offD = indexRect + 3;

    int i0, i1, i2;

    i0 = 1 + offC;
    i1 = 1 + offB;
    i2 = 1 + offA;
    sprintf(s, "f %d//%d %d//%d %d//%d\n", i0, i0, i1, i1, i2, i2);
    s += strlen(s);
    i++;

    i0 = 1 + offB;
    i1 = 1 + offC;
    i2 = 1 + offD;
    sprintf(s, "f %d//%d %d//%d %d//%d\n", i0, i0, i1, i1, i2, i2);
    s += strlen(s);
    i++;
  }

  sprintf(s, "# %d faces\n", i);
  s += strlen(s);
  assert(s - strBuffer < sizeBuffer);


  // save str buffer to file
  file = fopen(fileNameObj, "wt");
  if (!file)
  {
    delete [] strBuffer;
    return -1;
  }
  fwrite(strBuffer, 1, strlen(strBuffer), file);
  fclose(file);
  delete [] strBuffer;

  return 1;
}

static int _getVertexNormals(
                              const V3f *vertices,
                              const int numVertices,
                              const TriangleIndices *indices,
                              const int numTriangles,
                              V3f *vertNormals
                            )
{
  V3f *triNormals;

  triNormals = M_NEW(V3f[numTriangles]);
  if (!triNormals)
    return 0;

  const int IND_A = 0;
  const int IND_B = 1;
  const int IND_C = 2;
  int   t;
  for (t = 0; t < numTriangles; t++)
  {
    const TriangleIndices *tri = &indices[t];
    const int ia = tri->m_idices[IND_A];
    const int ib = tri->m_idices[IND_B];
    const int ic = tri->m_idices[IND_C];
    #ifdef _DEBUG
    {
      assert(ia >= 0);
      assert(ib >= 0);
      assert(ic >= 0);
      assert(ia < numVertices);
      assert(ib < numVertices);
      assert(ic < numVertices);
      assert(ia != ib);
      assert(ia != ic);
    }
    #endif
    V3f va = vertices[ia];
    V3f vb = vertices[ib];
    V3f vc = vertices[ic];

    V3f vab, vbc;
    vab.sub(vb, va);
    vbc.sub(vc, vb);

    V3f vTriNormal;
    vTriNormal.cross(vab, vbc);
    int ok = vTriNormal.normalize();
    if (ok < 1)
    {
      delete[] triNormals;
      return ok;
    }
    triNormals[t] = vTriNormal;

  } // for (t) all triangles

  // init vert normals with 0 vector
  int i;
  for (i = 0; i < numVertices; i++)
  {
    vertNormals[i].set(0.0f, 0.0f, 0.0f);
  }

  // get additions
  for (t = 0; t < numTriangles; t++)
  {
    const TriangleIndices *tri = &indices[t];
    const int ia = tri->m_idices[IND_A];
    const int ib = tri->m_idices[IND_B];
    const int ic = tri->m_idices[IND_C];
    V3f vTriNormal = triNormals[t];

    vertNormals[ia].addWith(vTriNormal);

    vertNormals[ib].addWith(vTriNormal);

    vertNormals[ic].addWith(vTriNormal);
  }

  // normalize vert normals
  for (i = 0; i < numVertices; i++)
  {
    float len2 = vertNormals[i].dotProduct(vertNormals[i]);
    if (len2 < 1.0e-9f)
      vertNormals[i].set(0.0f, 0.0f, 0.0f);
    else
      vertNormals[i].normalize();
  }

  delete[] triNormals;
  return 1;
}


int Dump::saveTriangleGeoToObjFile(
                                    const V3f *vertices,
                                    const int numVertices,
                                    const TriangleIndices *indices,
                                    const int numTriangles,
                                    const char* fileNameMtl,
                                    const char* fileNameObj
                                  )
{
  FILE    *file;

  static char *strHeadComment = "# Active Volume engine exporter \n";
  static char *strMtlGreen = "Green";

  // write MTL file
  file = fopen(fileNameMtl, "wt");
  if (!file)
    return -1;
  fprintf(file, "%s", strHeadComment);
  fprintf(file, "\n");

  // green
  fprintf(file, "newmtl %s\n", strMtlGreen);
  fprintf(file, "d 1\n");
  fprintf(file, "Tr 1\n");
  fprintf(file, "Tf 1 1 1\n");
  fprintf(file, "illum 2\n");
  fprintf(file, "Ka 0.0 0.1 0.0\n");
  fprintf(file, "Kd 0.3 0.7 0.3\n");
  fprintf(file, "Ks 0.2 0.2 0.2\n");
  fprintf(file, "\n");

  fclose(file);

  // write OBJ file
  file = fopen(fileNameObj, "wt");
  if (!file)
    return -1;

  fprintf(file, "%s", strHeadComment);
  fprintf(file, "\n");

  const char *sLib = strrchr(fileNameMtl, '/');
  sLib++;
  fprintf(file, "mtllib %s\n", sLib);

  fprintf(file, "#\n");
  fprintf(file, "# Object objField\n");
  fprintf(file, "#\n");

  int i;

  // create vertex normals
  V3f *normals = M_NEW(V3f[numVertices]);
  if (!normals)
  {
    fclose(file);
    return -1;
  }
  int ok = _getVertexNormals(vertices, numVertices,
    indices, numTriangles, normals);
  if (ok < 1)
  {
    delete [] normals;
    fclose(file);
    return ok;
  }


  // write vertices
  for (i = 0; i < numVertices; i++)
  {
    const float fx = vertices[i].x;
    const float fy = vertices[i].y;
    const float fz = vertices[i].z;
    fprintf(file, "v  %f %f %f\n", fx, fy, fz);
  }
  // total num verts
  fprintf(file, "# %d vertices\n", numVertices);

  // write normals
  for (i = 0; i < numVertices; i++)
  {
    const float fx = normals[i].x;
    const float fy = normals[i].y;
    const float fz = normals[i].z;
    fprintf(file, "vn %f %f %f\n", fx, fy, fz);
  }
  // total num normals
  fprintf(file, "# %d normals\n", numVertices);


  // free vert normals
  delete [] normals;

  // write tri indices
  fprintf(file, "g objField\n");
  fprintf(file, "usemtl %s\n", strMtlGreen);
  for (i = 0; i < numTriangles; i++)
  {
    int offA = indices[i].m_idices[0];
    int offB = indices[i].m_idices[1];
    int offC = indices[i].m_idices[2];

    int i0, i1, i2;

    i0 = 1 + offA;
    i1 = 1 + offB;
    i2 = 1 + offC;
    fprintf(file, "f %d//%d %d//%d %d//%d\n", i0, i0, i1, i1, i2, i2);
  }
  // total num tris
  fprintf(file, "# %d triangles\n", numTriangles);

  fclose(file);

  return 1;
}

int Dump::saveRenderGeoToObjFile(
                                  const float   *verticesXYZW,
                                  const float   *normalsXYZ,
                                  const int     numVertices,
                                  const MUint32 *indices,
                                  const int     numTriangles,
                                  const char    *fileNameMtl,
                                  const char    *fileNameObj
                                )
{
  FILE    *file;

  static char *strHeadComment =
    "# Active Volume engine exporter from rendered geometry\n";
  static char *strMtlGreen = "Green";

  // write MTL file
  file = fopen(fileNameMtl, "wt");
  if (!file)
    return -1;
  fprintf(file, "%s", strHeadComment);
  fprintf(file, "\n");

  // green
  fprintf(file, "newmtl %s\n", strMtlGreen);
  fprintf(file, "d 1\n");
  fprintf(file, "Tr 1\n");
  fprintf(file, "Tf 1 1 1\n");
  fprintf(file, "illum 2\n");
  fprintf(file, "Ka 0.0 0.1 0.0\n");
  fprintf(file, "Kd 0.3 0.7 0.3\n");
  fprintf(file, "Ks 0.2 0.2 0.2\n");
  fprintf(file, "\n");

  fclose(file);

  // write OBJ file
  file = fopen(fileNameObj, "wt");
  if (!file)
    return -1;

  fprintf(file, "%s", strHeadComment);
  fprintf(file, "\n");

  const char *sLib = strrchr(fileNameMtl, '/');
  sLib++;
  fprintf(file, "mtllib %s\n", sLib);

  fprintf(file, "#\n");
  fprintf(file, "# Object objField\n");
  fprintf(file, "#\n");

  int i, i4;

  // write vertices
  for (i = 0, i4 = 0; i < numVertices; i++, i4 += 4)
  {
    const float fx = verticesXYZW[i4 + 0];
    const float fy = verticesXYZW[i4 + 1];
    const float fz = verticesXYZW[i4 + 2];
    fprintf(file, "v  %f %f %f\n", fx, fy, fz);
  }
  // total num verts
  fprintf(file, "# %d vertices\n", numVertices);

  // write normals
  int i3;
  for (i = 0, i3 = 0; i < numVertices; i++, i3 += 3)
  {
    const float fx = normalsXYZ[i3 + 0];
    const float fy = normalsXYZ[i3 + 1];
    const float fz = normalsXYZ[i3 + 2];
    fprintf(file, "vn %f %f %f\n", fx, fy, fz);
  }
  // total num normals
  fprintf(file, "# %d normals\n", numVertices);

  // write tri indices
  fprintf(file, "g objField\n");
  fprintf(file, "usemtl %s\n", strMtlGreen);
  for (i = 0, i3 = 0; i < numTriangles; i++, i3 += 3)
  {
    MUint32 offA = indices[i3 + 0];
    MUint32 offB = indices[i3 + 1];
    MUint32 offC = indices[i3 + 2];

    int i0, i1, i2;

    i0 = (int)(1 + offA);
    i1 = (int)(1 + offB);
    i2 = (int)(1 + offC);
    fprintf(file, "f %d//%d %d//%d %d//%d\n", i0, i0, i1, i1, i2, i2);
  }
  // total num tris
  fprintf(file, "# %d triangles\n", numTriangles);

  fclose(file);

  return 1;
}
