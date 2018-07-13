// ****************************************************************************
// File: dump.h
// Purpose: Save file to BMP and STL
// ****************************************************************************

#ifndef  __dump_h
#define  __dump_h

// ****************************************************************************
// Includes
// ****************************************************************************

#include "mtypes.h"

// ****************************************************************************
// Class
// ****************************************************************************

/**
* \class Dump utility class to save images and geometry data into files
*/

class Dump
{
public:

  static int saveImageGreyToBitmapFile(
                                        const MUint32 *pixelsGrey,
                                        const int w,
                                        const int h,
                                        const char* fileName
                                       );
  static int saveImageArgbToBitmapFile(
                                        const MUint32 *pixelsGrey,
                                        const int w,
                                        const int h,
                                        const char* fileName
                                      );
  static int saveFieldToBitmapFile(
                                        const float *field,
                                        const int w,
                                        const int h,
                                        const char* fileName
                                  );

  static int saveHeightMapGeoToObjFile(
                                        const float *heights,
                                        const int w,
                                        const int h,
                                        const char* fileNameMtl,
                                        const char* fileNameObj
                                      );
  static int saveTriangleGeoToObjFile(
                                        const V3f *vertices,
                                        const int numVertices,
                                        const TriangleIndices *indices,
                                        const int numTriangles,
                                        const char* fileNameMtl,
                                        const char* fileNameObj
                                     );
  static int saveRenderGeoToObjFile(
                                        const float *verticesXYZW,
                                        const float *normalsXYZ,
                                        const int numVertices,
                                        const MUint32 *indices,
                                        const int numTriangles,
                                        const char* fileNameMtl,
                                        const char* fileNameObj
                                      );
};

#endif
