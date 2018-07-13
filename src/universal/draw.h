// ****************************************************************************
// File: draw.h
// Purpose: Draw utils
// ****************************************************************************

#ifndef  __draw_h
#define  __draw_h

// ****************************************************************************
// Includes
// ****************************************************************************

#include "mtypes.h"
#include "image.h"

// ****************************************************************************
// Class
// ****************************************************************************

/**
* \class Draw utility class to draw images in different pixel formats on screen
*/

class Draw
{
private:
public:
  static void copyImageFloatToScreen(
    const float *pixelsSrc, const int w, const int h,
    Image &mageDst, const float scale = 1.0f);
  static void drawFloatImageZeroToScreen(
    const float *pixelsSrc, const int w, const int h,
    Image &mageDst, const float scale = 1.0f);
  static void drawSignedImageFloatToScreen(
    const float *pixelsSrc, const int w, const int h,
    Image &mageDst, const float scale = 1.0f);
  static void copyImageGreyToScreen(
    const MUint32 *pixelsSrc, const int w, const int h,
    MUint32 *pixelsDst);
  static void drawImageFloatZeroAreaToScreen(
    const float *pixelsSrc, const int w, const int h,
    MUint32 *pixelsDst, const float scale = 1.0f);

  static void drawLine(
    MUint32 *pixelsDst, const int w, const int h,
    const V2d *vS, const V2d *vE, const MUint32 valDraw);

  static void drawHistogram(MUint32 *pixelsDst, const int w, const int h,
    const float *hist256);
  static void drawPoint(MUint32 *pixelsDst, const int w, const int h,
    const V2d &point, const MUint32 valDraw);
};

#endif
