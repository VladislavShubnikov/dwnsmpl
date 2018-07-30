// ****************************************************************************
// File: volume.cpp
// Purpose: Volume tools
// ****************************************************************************

// ****************************************************************************
// Includes
// ****************************************************************************

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "volume.h"

// ****************************************************************************
// Methods
// ****************************************************************************

int  VolumeTools::performGaussSlow(
                                  const MUint8 *volPixelsSrc,
                                  const int     xDim,
                                  const int     yDim,
                                  const int     zDim,
                                  MUint8        *volPixelsDst
                                )
{
  const int   VOL_GAUSS_RADIUS = 1;
  const float VOL_GAUSS_SIGMA = 0.8f;
  // more sigma => more blurring
  const float VOL_KOEF =
    1.0f / (3.0f * M_PI * VOL_GAUSS_SIGMA * VOL_GAUSS_SIGMA);
  int cx, cy, cz;
  for (cz = 0; cz < zDim; cz++)
  {
    for (cy = 0; cy < yDim; cy++)
    {
      for (cx = 0; cx < xDim; cx++)
      {

        int dx, dy, dz;

        float sum = 0.0f;
        float sumWeights = 0.0f;

        // !!!!!! TEST !!!!!!!!!
        if ( (cx == 72) && (cy == 53) && (cz == 32) )
          sum = 0.0f;

        for (dz = -VOL_GAUSS_RADIUS; dz <= +VOL_GAUSS_RADIUS; dz++)
        {
          const int z = cz + dz;
          if (z < 0)
            continue;
          if (z >= zDim)
            break;
          const float tz = (float)dz / VOL_GAUSS_RADIUS;
          for (dy = -VOL_GAUSS_RADIUS; dy <= +VOL_GAUSS_RADIUS; dy++)
          {
            const int y = cy + dy;
            if (y < 0)
              continue;
            if (y >= yDim)
              break;
            const float ty = (float)dy / VOL_GAUSS_RADIUS;

            for (dx = -VOL_GAUSS_RADIUS; dx <= +VOL_GAUSS_RADIUS; dx++)
            {
              const int x = cx + dx;
              if (x < 0)
                continue;
              if (x >= xDim)
                break;
              const float tx = (float)dx / VOL_GAUSS_RADIUS;

              const int off = x + y * xDim + z * xDim * yDim;
              const MUint8 val = volPixelsSrc[off];

              const float dist2 = tx * tx + ty * ty + tz * tz;
              const float weight = expf(-dist2 * VOL_KOEF);
              sum += val * weight;
              sumWeights += weight;
            }   // for (dx)
          }     // for (dy)
        }       // for (dz)

        float valSmoothed = sum / sumWeights;
        valSmoothed = (valSmoothed <= 255.0f) ? valSmoothed : 255.0f;
        const int offDst = cx + (cy * xDim) + (cz * xDim * yDim);
        volPixelsDst[offDst] = (MUint8)valSmoothed;
      }   // for (cx)
    }     // for (cy)
  }       // for (cz)
  return 1;
}

int  VolumeTools::performGaussFast(
                                    const MUint8 *volPixelsSrc,
                                    const int     xDim,
                                    const int     yDim,
                                    const int     zDim,
                                    MUint8        *volPixelsDst
                                  )
{
  // TODO: this function should be improved in terms of performance
  // 1) Gauss koefficients should be calculated before main volume cycle
  // 2) Constants should be calculated out of internal cycles

  const int   VOL_GAUSS_RADIUS  = 1;
  const float VOL_GAUSS_SIGMA   = 0.8f;
  // more sigma => more blurring
  const float VOL_KOEF =
    1.0f / (3.0f * M_PI * VOL_GAUSS_SIGMA * VOL_GAUSS_SIGMA);
  int cx, cy, cz;
  for (cz = 0; cz < zDim; cz++)
  {
    for (cy = 0; cy < yDim; cy++)
    {
      for (cx = 0; cx < xDim; cx++)
      {

        int dx, dy, dz;

        float sum = 0.0f;
        float sumWeights = 0.0f;

        for (dz = -VOL_GAUSS_RADIUS; dz <= +VOL_GAUSS_RADIUS; dz++)
        {
          const int z = cz + dz;
          if (z < 0)
            continue;
          if (z >= zDim)
            break;
          const float tz = (float)dz / VOL_GAUSS_RADIUS;
          for (dy = -VOL_GAUSS_RADIUS; dy <= +VOL_GAUSS_RADIUS; dy++)
          {
            const int y = cy + dy;
            if (y < 0)
              continue;
            if (y >= yDim)
              break;
            const float ty = (float)dy / VOL_GAUSS_RADIUS;

            for (dx = -VOL_GAUSS_RADIUS; dx <= +VOL_GAUSS_RADIUS; dx++)
            {
              const int x = cx + dx;
              if (x < 0)
                continue;
              if (x >= xDim)
                break;
              const float tx = (float)dx / VOL_GAUSS_RADIUS;

              // TODO: volume offset should be calculated
              // more effective (in terms of constants
              // relative to internal cycle)
              const int off = x + y * xDim + z * xDim * yDim;
              const MUint8 val = volPixelsSrc[off];

              // TODO: dist2, weight should be calcuated
              // more efficient in terms of constant related to internal cycles
              // very time-expensive opertaion "expf" should be
              // completely avoided here
              const float dist2 = tx * tx + ty * ty + tz * tz;
              const float weight = expf(-dist2 * VOL_KOEF);

              sum += val * weight;
              sumWeights += weight;

            }   // for (dx)
          }     // for (dy)
        }       // for (dz)

        float valSmoothed = sum / sumWeights;
        valSmoothed = (valSmoothed <= 255.0f) ? valSmoothed : 255.0f;
        // TODO: offDst should be calculated in a more efficient way:
        // remove repeated constants calculation
        const int offDst = cx + (cy * xDim) + (cz * xDim * yDim);
        volPixelsDst[offDst] = (MUint8)valSmoothed;
      }   // for (cx)
    }     // for (cy)
  }       // for (cz)
  return 1;
}
