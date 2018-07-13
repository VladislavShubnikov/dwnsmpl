//  *****************************************************************
//  PURPOSE 2d Image Advanced downsampling technique
//  NOTES
//
// Main down sampling method is based on article
// J.Diaz-Garcia, P.Brunet, I.Navazo, P.Vazquez, "Downsampling Methods for Medical Datasets", 2017
//
// web ref:
// https://upcommons.upc.edu/bitstream/handle/2117/111411/IADIS-CGVCV2017-.pdf;jsessionid=876F408154FD5973D28806BF5BDB635C?sequence=1
//
//  *****************************************************************

//  *****************************************************************
//  Includes
//  *****************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <assert.h>


#include "memtrack.h"
#include "dsample2d.h"

//  *****************************************************************
//  Defines
//  *****************************************************************

// for deep debug
//#define PERFORM_ONLY_DOWN_SAMPLING

//  *****************************************************************
//  Data
//  *****************************************************************


//  *****************************************************************
//  Methods
//  *****************************************************************

Downsample2d::Downsample2d()
{
  m_wSrc = m_hSrc = 0;
  m_wDst = m_hDst = 0;
  m_pixelsSrc = NULL;
  m_pixelsGauss = NULL;
  m_pixelsDownSampled = NULL;
  m_pixelsSubSample = NULL;
  m_pixelsBilateral = NULL;
  m_pixelsRestored = NULL;

  m_sigmaBilateralPos = 0.10f;
  m_sigmaBilateralVal = 0.51f;
  const float STRANGE = 55555.55555f;
  for (int i = 0; i < DS_MAX_NEIB_DIA * DS_MAX_NEIB_DIA; i++)
    m_filter[i] = STRANGE;
}
void    Downsample2d::destroy()
{
  if (m_pixelsSrc)
    delete [] m_pixelsSrc;
  if (m_pixelsGauss)
    delete [] m_pixelsGauss;
  if (m_pixelsDownSampled)
    delete [] m_pixelsDownSampled;
  if (m_pixelsSubSample)
    delete [] m_pixelsSubSample;
  if (m_pixelsBilateral)
    delete [] m_pixelsBilateral;
  if (m_pixelsRestored)
    delete [] m_pixelsRestored;

  m_pixelsSrc           = NULL;
  m_pixelsGauss         = NULL;
  m_pixelsDownSampled   = NULL;
  m_pixelsSubSample     = NULL;
  m_pixelsBilateral     = NULL;
  m_pixelsRestored      = NULL;
}

Downsample2d::~Downsample2d()
{
  destroy();
}

int Downsample2d::create(
                          const int       wSrc,
                          const int       hSrc,
                          const MUint32  *pixels,
                          const int       wDst,
                          const int       hDst
                        )
{
  if (m_pixelsSrc)
    delete[] m_pixelsSrc;
  m_wSrc = wSrc;
  m_hSrc = hSrc;

  m_wDst = wDst;
  m_hDst = hDst;

  const int numPixelsSrc = wSrc * hSrc;
  m_pixelsSrc = M_NEW(float[numPixelsSrc]);
  if (!m_pixelsSrc)
    return 0;
  m_pixelsRestored = M_NEW(float[numPixelsSrc]);
  if (!m_pixelsRestored)
    return 0;

  // convert source image ARGB format into greyscale image (float)
  int i;
  for (i = 0; i < numPixelsSrc; i++)
  {
    MUint32 val = pixels[i] & 0xff;
    m_pixelsSrc[i] = val * (1.0f / 255.0f);
  }
  // allocate memory for destination images
  const int numPixelsDst = wDst * hDst;
  m_pixelsGauss         = M_NEW(float[numPixelsDst]);
  m_pixelsDownSampled   = M_NEW(float[numPixelsDst]);
  m_pixelsSubSample     = M_NEW(float[numPixelsDst]);
  m_pixelsBilateral     = M_NEW(float[numPixelsDst]);
  if (!m_pixelsBilateral)
    return 0;
  return 1;
} // craete

int   Downsample2d::performDownSamplingAll()
{
#if !defined(PERFORM_ONLY_DOWN_SAMPLING)
  performSubSample();
  performGaussSlow(m_pixelsSrc, m_pixelsGauss);
  performBilateral();
#endif
  performDownSample();
  return 1;
}

int   Downsample2d::performSubSample()
{
  for (int cy = 0; cy < m_hDst; cy++)
  {
    const int cySrc = m_hSrc * cy / m_hDst;
    const int cySrcOff = cySrc * m_wSrc;
    const int cyDstOff = cy * m_wDst;

    for (int cx = 0; cx < m_wDst; cx++)
    {
      const int cxSrc = m_wSrc * cx / m_wDst;
      const float valSrc = m_pixelsSrc[cxSrc + cySrcOff];
      m_pixelsSubSample[cx + cyDstOff] = valSrc;
    } // for (cx)
  } // for (cy)
  return 1;
}

const int   SIMPLE_GAUSS_RADIUS = 5;
const float SIMPLE_GAUSS_SIGMA = 0.2f;

int Downsample2d::performGaussSlow(const float *pixelsSrc, float *pixelsDst)
{
  // more sigma => more blurring
  const float SIMPLE_KOEF =
    1.0f / (2.0f * M_PI * SIMPLE_GAUSS_SIGMA * SIMPLE_GAUSS_SIGMA);

  for (int cy = 0; cy < m_hDst; cy++)
  {
    const int cySrc = m_hSrc * cy / m_hDst;
    const int cyDstOff = cy * m_wDst;

    for (int cx = 0; cx < m_wDst; cx++)
    {
      const int cxSrc = m_wSrc * cx / m_wDst;

      // accumulate sum around pixel [cxSrc, cySrc] with 
      // neighborhood SIMPLE_GAUSS_RADIUS
      float sum = 0.0f;
      float sumWeights = 0.0f;

      for (int dy = -SIMPLE_GAUSS_RADIUS; dy <= +SIMPLE_GAUSS_RADIUS; dy++)
      {
        const int y = cySrc + dy;
        if (y < 0)
          continue;
        if (y >= m_hSrc)
          break;
        const int yOff = y * m_wSrc;
        const float ty = (float)dy / SIMPLE_GAUSS_RADIUS;

        for (int dx = -SIMPLE_GAUSS_RADIUS; dx <= +SIMPLE_GAUSS_RADIUS; dx++)
        {
          const int x = cxSrc + dx;
          if (x < 0)
            continue;
          if (x >= m_wSrc)
            break;
          const float tx = (float)dx / SIMPLE_GAUSS_RADIUS;

          const float dist2 = tx * tx + ty * ty;
          const float gaussWeight = expf( -dist2 * SIMPLE_KOEF);

          const int offSrc = x + yOff;
          const float valSrc = pixelsSrc[offSrc];

          sum += valSrc * gaussWeight;
          sumWeights += gaussWeight;

        }  // for (dx)
      }  // for (dy)
      const float valDst = sum / sumWeights;
      pixelsDst[cx + cyDstOff] = valDst;
    } // for (cx)
  }  // for (cy)
  return 1;
}

int   Downsample2d::performGaussFast(const float *pixelsSrc, float *pixelsDst)
{
  const int SIMPLE_GAUSS_DIAMETER = (2 * SIMPLE_GAUSS_RADIUS + 1);
  static float gaussWeights[SIMPLE_GAUSS_DIAMETER * SIMPLE_GAUSS_DIAMETER];

  const float SIMPLE_KOEF =
    1.0f / (2.0f * M_PI * SIMPLE_GAUSS_SIGMA * SIMPLE_GAUSS_SIGMA);

  // fill weights
  int indexWeight = 0;
  for (int dy = -SIMPLE_GAUSS_RADIUS; dy <= +SIMPLE_GAUSS_RADIUS; dy++)
  {
    const float ty = (float)dy / SIMPLE_GAUSS_RADIUS;
    for (int dx = -SIMPLE_GAUSS_RADIUS; dx <= +SIMPLE_GAUSS_RADIUS; dx++)
    {
      const float tx = (float)dx / SIMPLE_GAUSS_RADIUS;
      const float dist2 = tx * tx + ty * ty;
      const float gaussWeight = expf(-dist2 * SIMPLE_KOEF);
      gaussWeights[indexWeight++] = gaussWeight;
    } // for (dx)
  }  // for (dy)

  for (int cy = 0; cy < m_hDst; cy++)
  {
    const int cySrc = m_hSrc * cy / m_hDst;
    const int cyDstOff = cy * m_wDst;

    for (int cx = 0; cx < m_wDst; cx++)
    {
      const int cxSrc = m_wSrc * cx / m_wDst;

      // accumulate sum around pixel [cxSrc, cySrc] with
      // neighborhood SIMPLE_GAUSS_RADIUS
      float sum = 0.0f;
      float sumWeights = 0.0f;

      for (int dy = -SIMPLE_GAUSS_RADIUS; dy <= +SIMPLE_GAUSS_RADIUS; dy++)
      {
        const int y = cySrc + dy;
        if (y < 0)
          continue;
        if (y >= m_hSrc)
          break;
        const int yOff = y * m_wSrc;

        for (int dx = -SIMPLE_GAUSS_RADIUS; dx <= +SIMPLE_GAUSS_RADIUS; dx++)
        {
          const int x = cxSrc + dx;
          if (x < 0)
            continue;
          if (x >= m_wSrc)
            break;

          const int indexWeightX = dx + SIMPLE_GAUSS_RADIUS;
          const int indexWeightY = dy + SIMPLE_GAUSS_RADIUS;
          const int offWeight =
            indexWeightX + indexWeightY * SIMPLE_GAUSS_DIAMETER;
          const float gaussWeight = gaussWeights[offWeight];

          const int offSrc = x + yOff;
          const float valSrc = pixelsSrc[offSrc];

          sum += valSrc * gaussWeight;
          sumWeights += gaussWeight;

        }  // for (dx)
      }  // for (dy)
      const float valDst = sum / sumWeights;
      pixelsDst[cx + cyDstOff] = valDst;
    } // for (cx)
  }  // for (cy)

  return 1;
}

int   Downsample2d::performBilateral()
{
  const int NEIB_RADIUS = 8;
  // more sigma => more blurring
  const float POS_SIGMA   = m_sigmaBilateralPos;
  const float POS_KOEF    = 1.0f / (2.0f * M_PI * POS_SIGMA * POS_SIGMA);

  const float VAL_SIGMA   = m_sigmaBilateralVal;
  const float VAL_KOEF    = 1.0f / (2.0f * M_PI * VAL_SIGMA * VAL_SIGMA);

  for (int cy = 0; cy < m_hDst; cy++)
  {
    const int cySrc = m_hSrc * cy / m_hDst;
    const int cySrcOff = cySrc * m_wSrc;
    const int cyDstOff = cy * m_wDst;

    for (int cx = 0; cx < m_wDst; cx++)
    {
      const int cxSrc = m_wSrc * cx / m_wDst;

      // accumulate sum around pixel [cxSrc, cySrc]
      // with neighborhood SIMPLE_GAUSS_RADIUS
      float sum = 0.0f;
      float sumWeights = 0.0f;

      // !!!!!!!!!!!!!!! TEST !!!!!!!!!!!
      // if ( (cx == 153) && (cy == 131))
      // sum = 0.0f; // break here

      const float valSrcCenter = m_pixelsSrc[cxSrc + cySrcOff];

      for (int dy = -NEIB_RADIUS; dy <= +NEIB_RADIUS; dy++)
      {
        const int y = cySrc + dy;
        if (y < 0)
          continue;
        if (y >= m_hSrc)
          break;
        const int yOff = y * m_wSrc;
        const float ty = (float)dy / NEIB_RADIUS;

        for (int dx = -NEIB_RADIUS; dx <= +NEIB_RADIUS; dx++)
        {
          const int x = cxSrc + dx;
          if (x < 0)
            continue;
          if (x >= m_wSrc)
            break;
          const float tx = (float)dx / NEIB_RADIUS;

          const int offSrc = x + yOff;
          const float valSrc = m_pixelsSrc[offSrc];

          const float dist2 = tx * tx + ty * ty;
          const float posWeight = expf(-dist2 * POS_KOEF);

          const float deltaVal = valSrc - valSrcCenter;
          const float valWeight = expf(-deltaVal * deltaVal * VAL_KOEF);

          sum += valSrc * posWeight * valWeight;
          sumWeights += posWeight * valWeight;

        }  // for (dx)
      }  // for (dy)
      const float valDst = sum / sumWeights;
      m_pixelsBilateral[cx + cyDstOff] = valDst;

    } // for (cx)
  }  // for (cy)
  return 1;
}

//
// Based on article
// J.Diaz-Garcia, P.Brunet, I.Navazo, P.Vazquez, 
// "Downsampling Methods for Medical Datasets", 2017
//
// web ref:
// upcommons.upc.edu/bitstream/handle/2117/111411/IADIS-CGVCV2017-.pdf;jsessionid=876F408154FD5973D28806BF5BDB635C?sequence=1
//
int   Downsample2d::performDownSample()
{
  performGaussFast(m_pixelsSrc, m_pixelsGauss);

  // restore original resolution image via bilinear
  // interpolation from diminished gauss image

  int ind = 0;
  for (int yLar = 0; yLar < m_hSrc; yLar++)
  {
    const float ySmall = (float)m_hDst * yLar / m_hSrc;
    const int iySmall = (int)(ySmall);
    const float ty = ySmall - (float)iySmall;
    const int iySmallNext = (iySmall + 1 < m_hDst) ?
      (iySmall + 1) : (m_hDst - 1);

    for (int xLar = 0; xLar < m_wSrc; xLar++)
    {
      const float xSmall = (float)m_wDst * xLar / m_wSrc;
      const int ixSmall = (int)(xSmall);
      const float tx = xSmall - (float)ixSmall;
      const int ixSmallNext = (ixSmall + 1 < m_wDst) ?
        (ixSmall + 1) : (m_wDst - 1);

      // Neibs valued for bilinear interpolatoin
      //
      // A B
      // C D
      //
      const float valA = m_pixelsGauss[ixSmall + iySmall * m_wDst];
      const float valB = m_pixelsGauss[ixSmallNext + iySmall * m_wDst];
      const float valC = m_pixelsGauss[ixSmall + iySmallNext * m_wDst];
      const float valD = m_pixelsGauss[ixSmallNext + iySmallNext * m_wDst];

      const float valL = valA * (1.0f - ty) + valC * ty;
      const float valR = valB * (1.0f - ty) + valD * ty;
      const float val = valL * (1.0f - tx) + valR * tx;

      m_pixelsRestored[ind++] = val;
    } // for (xLar)
  } // for (yLar)

  // process large image => small image

  const int DS_RADIUS = 8;
  const float DS_GAUSS_SIGMA = 1.5f;
  const float DS_KOEF_GAUSS = 
    1.0f / (2.0f * M_PI * DS_GAUSS_SIGMA * DS_GAUSS_SIGMA);

  assert(DS_RADIUS <= DS_MAX_NEIB_RAD);
  const int DS_NUM_ELEMS_FILTER = (2 * DS_RADIUS + 1) * (2 * DS_RADIUS + 1);
  int indDstSmall = 0;
  for (int ySmall = 0; ySmall < m_hDst; ySmall++)
  {
    const int ySrc = m_hSrc * ySmall / m_hDst;
    for (int xSmall = 0; xSmall < m_wDst; xSmall++)
    {
      const int xSrc = m_wSrc * xSmall / m_wDst;
      // center in point (xSrc, ySrc) from large image

      // clear filter
      for (int i = 0; i < DS_NUM_ELEMS_FILTER; i++)
        m_filter[i] = 0.0f;

      float weightsSum = 0.0f;

      int dx, dy;
      // create image val weights
      for (dy = -DS_RADIUS; dy <= +DS_RADIUS; dy++)
      {
        const int y = ySrc + dy;
        if (y < 0)
          continue;
        if (y >= m_hSrc)
          break;
        const int yOff = y * m_wSrc;
        for (dx = -DS_RADIUS; dx <= +DS_RADIUS; dx++)
        {
          const int x = xSrc + dx;
          if (x < 0)
            continue;
          if (x >= m_wSrc)
            break;
          const float valSrc = m_pixelsSrc[x + yOff];
          const float valSmo = m_pixelsRestored[x + yOff];
          const float deltaVal = (valSrc - valSmo >= 0.0f) ?
            (valSrc - valSmo) : -(valSrc - valSmo);
          const float weight = deltaVal * deltaVal;

          weightsSum += weight;
          const int indFilterY = dy + DS_RADIUS;
          const int indFilterX = dx + DS_RADIUS;
          m_filter[indFilterX + indFilterY * (2 * DS_RADIUS + 1)] = weight;
        } // for (dx)
      } // for (dy)

      // normalize filter
      const float scaleFilter = 1.0f / weightsSum;
      for (int i = 0; i < DS_NUM_ELEMS_FILTER; i++)
        m_filter[i] *= scaleFilter;

      // get source pixels around (xSrc, ySrc) using filter and
      // gaussian smoothing
      float sum = 0.0f;
      float sumW = 0.0f;
      for (dy = -DS_RADIUS; dy <= +DS_RADIUS; dy++)
      {
        const int y = ySrc + dy;
        if (y < 0)
          continue;
        if (y >= m_hSrc)
          break;
        const int yOff = y * m_wSrc;
        const float ty = (float)dy / DS_RADIUS;
        for (dx = -DS_RADIUS; dx <= +DS_RADIUS; dx++)
        {
          const int x = xSrc + dx;
          if (x < 0)
            continue;
          if (x >= m_wSrc)
            break;
          const float tx = (float)dx / DS_RADIUS;

          const float dist2 = tx * tx + ty *ty;
          const float gaussWeight = expf(-dist2 / DS_KOEF_GAUSS);

          const int indFilterY = dy + DS_RADIUS;
          const int indFilterX = dx + DS_RADIUS;
          const int offWe = indFilterX + indFilterY * (2 * DS_RADIUS + 1);
          const float filterWeight = m_filter[offWe];

          const float val = m_pixelsSrc[x + yOff];
          sum += val * gaussWeight * filterWeight;
          sumW += gaussWeight * filterWeight;
        } // for (dx)
      } // for (dy)

      const float valFiltered = sum / sumW;
      m_pixelsDownSampled[indDstSmall++] = valFiltered;
    } // for (xSmall)
  } // for (ySmall)

  return 1;
}
