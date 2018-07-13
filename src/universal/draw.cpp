// ************************************
// Includes
// ************************************

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "draw.h"

// ************************************
// Defines
// ************************************

#define IABS(a)           (((a)>0)?(a):-(a))
#define FABS(a)           (((a)>0.0f)?(a):-(a))

#define   ACC_BITS                    10
#define   ACC_ONE                     (1<<ACC_BITS)

#define LINE_ACC    10
#define LINE_HALF   (1 << (LINE_ACC-1) )

// ************************************
// Methods
// ************************************

/*
void Draw::copyImageFloatToScreen(const float *pixelsSrc, const int w, const int h, Image &mageDst, const float scale)
{
  MUint32 *pixelsDst = imageDst.getPixels();
  const int wDst = mageDst.getWidth();
  const int hDst = mageDst.getHeight();

  int x, y;

  {
    float f = pixelsSrc[i] * scale;
    f = (f >= 0.0f) ? f : 0.0f;
    f = (f <= 255.0f) ? f : 255.0f;
    MUint32 val = (MUint32)(f);
    pixelsDst[i] = 0xff000000 | ((val >> 2) << 16) | (val << 8) | (val);
  }
}
void Draw::drawSignedImageFloatToScreen(const float *pixelsSrc, const int w, const int h, MUint32 *pixelsDst, const float scale)
{
  int numPixels = w * h;
  for (int i = 0; i < numPixels; i++)
  {
    float f = pixelsSrc[i] * scale;

    int numShift = 0;
    if (f >= 0.0f)
      numShift = 8; // green
    else
      numShift = 0; // blue
    f = (f >= 0.0f) ? f : (-f);
    f = (f <= 255.0f) ? f : 255.0f;
    MUint32 val = (MUint32)(f);
    pixelsDst[i] = 0xff000000 | (val << numShift);
  }
}

void Draw::drawFloatImageZeroToScreen(const float *pixelsSrc, const int w, const int h, MUint32 *pixelsDst, const float range)
{
  int numPixels = w * h;
  for (int i = 0; i < numPixels; i++)
  {
    const float f = pixelsSrc[i];
    MUint32 val = 0;
    if ((f >= -range) && (f <= +range))
      val = 255;
    pixelsDst[i] = 0xff000000 | ((val >> 2) << 16) | (val << 8) | (val);
  }
}

void Draw::copyImageGreyToScreen(const MUint32 *pixelsSrc, const int w, const int h, MUint32 *pixelsDst)
{
  int numPixels = w * h;
  for (int i = 0; i < numPixels; i++)
  {
    MUint32 val = pixelsSrc[i];
    val = (val > 255)? 255: val;
    pixelsDst[i] = 0xff000000 | (val << 16) | (val << 8) | (val);
  }
}
*/

void Draw::drawImageFloatZeroAreaToScreen(
                                          const float *pixelsSrc,
                                          const int w,
                                          const int h,
                                          MUint32 *pixelsDst,
                                          const float range
                                         )
{
  int offs[4];
  offs[0] = -1 - w;
  offs[1] = -1 + w;
  offs[2] = +1 - w;
  offs[3] = +1 + w;

  const float VMIN = -range;
  const float VMAX = +range;

  int cx, cy;
  int cyOff = 1 * w;
  for (cy = 1; cy < h - 1; cy++, cyOff += w)
  {
    for (cx = 1; cx < w - 1; cx++)
    {
      int offC = cx + cyOff;
      float vMin = pixelsSrc[offC];
      float vMax = vMin;
      for (int k = 0; k < 4; k++)
      {
        float v = pixelsSrc[offC + offs[k]];
        vMin = (v < vMin) ? v : vMin;
        vMax = (v > vMax) ? v : vMax;
      }
      if (vMin * vMax > 0.0f)
      {
        pixelsDst[offC] = 0xffffffff;
        continue;
      }
      // draw shade of blue
      float t = (pixelsSrc[offC] - VMIN) / (VMAX - VMIN);
      t = (t >= 0.0f) ? t : 0.0f;
      t = (t <= 1.0f) ? t : 1.0f;
      MUint32 val = (MUint32)(t * 255);
      pixelsDst[offC] = 0xff000000 | val;
    }
  }
}

void Draw::drawLine(
                    MUint32 *pixelsDst,
                    const int w,
                    const int h,
                    const V2d *vS,
                    const V2d *vE,
                    const MUint32 valDraw
                   )
{
  int  xs, ys, xe, ye, adx, ady;

  xs = vS->x; ys = vS->y;
  xe = vE->x; ye = vE->y;
  adx = IABS(xe - xs);
  ady = IABS(ye - ys);
  if (adx > ady)
  {
    if (adx == 0)
      return;
    // hor line
    if (xs > xe)
    {
      int t;
      t = xs; xs = xe; xe = t; t = ys; ys = ye; ye = t;
    }
    // Hor line tendention
    int dx = xe - xs;
    int stepAcc = ((ye - ys) << LINE_ACC) / dx;
    int yAcc = (ys << LINE_ACC) + LINE_HALF;
    for (int x = xs; x < xe; x++, yAcc += stepAcc)
    {
      int y = yAcc >> LINE_ACC;
      if ((x >= 0) && (x < w) && (y >= 0) && (y < h))
      {
        int off = x + y * w;
        pixelsDst[off] = valDraw;
      }
    }

  } // hor line
  else
  {
    if (ady == 0)
      return;
    // vert line
    if (ys > ye)
    {
      int t;
      t = xs; xs = xe; xe = t; t = ys; ys = ye; ye = t;
    }
    // Hor line tendention
    int dy = ye - ys;
    int stepAcc = ((xe - xs) << LINE_ACC) / dy;
    int xAcc = (xs << LINE_ACC) + LINE_HALF;
    for (int y = ys; y < ye; y++, xAcc += stepAcc)
    {
      int x = xAcc >> LINE_ACC;
      if ((x >= 0) && (x < w) && (y >= 0) && (y < h))
      {
        int off = x + y * w;
        pixelsDst[off] = valDraw;
      }
    }
  }   // vert line
}

void Draw::drawHistogram(
                          MUint32 *pixelsDst,
                          const int w,
                          const int h,
                          const float *hist256
                        )
{
  int i;
  // get min max
  float valMin = +1.0e12f;
  float valMax = -1.0e12f;
  for (i = 0; i < 256; i++)
  {
    float val = hist256[i];
    valMin = (val < valMin) ? val : valMin;
    valMax = (val > valMax) ? val : valMax;
  }
  valMax += 0.00001f;
  const float scale = h / (valMax - valMin);

  int numPixels = w * h;
  // init with white color
  for (i = 0; i < numPixels; i++)
    pixelsDst[i] = 0xffffffff;
  int xPrev = -1, yPrev = -1;
  const MUint32 colDraw = 0xff000000;
  V2d va, vb;
  int x = 0, y = 0;
  for (i = 0; i < 256; i++)
  {
    x = (int)(i * w / 256);
    y = (int)((hist256[i] - valMin) * scale);
    if (i > 0)
    {
      va.x = xPrev;
      va.y = yPrev;
      vb.x = x;
      vb.y = y;
      Draw::drawLine(pixelsDst, w, h, &va, &vb, colDraw );
    }
    xPrev = x; yPrev = y;
  }
  // draw last
  va.x = xPrev;
  va.y = yPrev;
  vb.x = x;
  vb.y = y;
  Draw::drawLine(pixelsDst, w, h, &va, &vb, colDraw);
}

void Draw::drawPoint(
                      MUint32         *pixelsDst,
                      const int       w,
                      const int       h,
                      const V2d       &point,
                      const MUint32   valDraw
                    )
{
  int dx, dy;

  for (dy = -1; dy <= +1; dy++)
  {
    int y = point.y + dy;
    if ((y < 0) || (y >= h))
      continue;
    for (dx = -1; dx <= +1; dx++)
    {
      int x = point.x + dx;
      if ((x < 0) || (x >= w))
        continue;
      int off = x + y * w;
      pixelsDst[off] = valDraw;
    }
  }
}
