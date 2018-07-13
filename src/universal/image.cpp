// ****************************************************************************
// File: image.cpp
// Purpose: Image 2d definition
// ****************************************************************************

#include <assert.h>

#include "image.h"
#include "memtrack.h"

Image::Image()
{
  m_format = IMAGE_FORMAT_NA;
  m_needDestroy = 0;
  m_pixelsInt = NULL;
  m_pixelsFloat = NULL;
  m_width = 0;
  m_height = 0;
}

Image::Image(const int w, const int h, MUint32 *pixels)
{
  m_format = IMAGE_FORMAT_UINT32;
  m_needDestroy = 1;
  m_pixelsInt = M_NEW(MUint32[w * h]);
  m_pixelsFloat = NULL;
  memcpy(m_pixelsInt, pixels, w * h * sizeof(MUint32));
  m_width = w;
  m_height = h;
}
Image::Image(const int w, const int h, float *pixels)
{
  m_format = IMAGE_FORMAT_FLOAT;
  m_needDestroy = 1;
  m_pixelsInt = NULL;
  m_pixelsFloat = M_NEW(float[w * h]);
  memcpy(m_pixelsFloat, pixels, w * h * sizeof(float));
  m_width = w;
  m_height = h;
}

Image::Image(const int w, const int h, const ImageFormat fmt)
{
  m_format = fmt;
  m_needDestroy = 1;
  if (fmt == IMAGE_FORMAT_UINT32)
  {
    m_pixelsInt = M_NEW(MUint32[w * h]);
    m_pixelsFloat = NULL;
  }
  else if (fmt == IMAGE_FORMAT_FLOAT)
  {
    m_pixelsInt = NULL;
    m_pixelsFloat = M_NEW(float[w * h]);
  }
  m_width = w;
  m_height = h;
}
Image::Image(Image &img)
{
  m_needDestroy = 1;
  const int w = img.getWidth();
  const int h = img.getHeight();
  m_format = img.getFormat();
  if (m_format == IMAGE_FORMAT_UINT32)
  {
    m_pixelsInt = M_NEW(MUint32[w * h]);
    memcpy(m_pixelsInt, img.getPixelsInt(), w * h * sizeof(MUint32));
  }
  else if (m_format == IMAGE_FORMAT_FLOAT)
  {
    m_pixelsFloat = M_NEW(float[w * h]);
    memcpy(m_pixelsInt, img.getPixelsFloat(), w * h * sizeof(float));
  }
  
  m_width = w;
  m_height = h;
  
}
Image&  Image::operator=(const Image &img)
{
  if (&img != this)
  {
    m_needDestroy = 1;
    const int w = img.getWidth();
    const int h = img.getHeight();
    m_format = img.getFormat();

    if (m_format == IMAGE_FORMAT_UINT32)
    {
      m_pixelsInt = M_NEW(MUint32[w * h]);
      memcpy(m_pixelsInt, img.getPixelsInt(), w * h * sizeof(MUint32));
    }
    else if (m_format == IMAGE_FORMAT_FLOAT)
    {
      m_pixelsFloat = M_NEW(float[w * h]);
      memcpy(m_pixelsInt, img.getPixelsFloat(), w * h * sizeof(float));
    }
    m_width = w;
    m_height = h;
  }
  return *this;
}
void Image::setAllocated(const int w, const int h, MUint32 *pixels)
{
  m_format = IMAGE_FORMAT_UINT32;
  m_needDestroy = 0;
  m_width = w;
  m_height = h;
  m_pixelsInt = pixels;
}
void Image::setAllocated(const int w, const int h, float   *pixels)
{
  m_format = IMAGE_FORMAT_FLOAT;
  m_needDestroy = 0;
  m_width = w;
  m_height = h;
  m_pixelsFloat = pixels;
}

Image::~Image()
{
  destroy();
}

void Image::destroy()
{
  if ((m_pixelsInt != NULL) && (m_needDestroy))
    delete [] m_pixelsInt;
  if ((m_pixelsFloat != NULL) && (m_needDestroy))
    delete[] m_pixelsFloat;

  m_pixelsInt = NULL;
  m_pixelsFloat = NULL;
  m_needDestroy = 0;
  m_width = 0;
  m_height = 0;
}

void Image::drawIntImage(const Image &imageSrc, const int xc, const int yc)
{
  assert(imageSrc.getFormat() == IMAGE_FORMAT_UINT32);
  assert(getFormat() == IMAGE_FORMAT_UINT32);

  const int wSrc = imageSrc.getWidth();
  const int hSrc = imageSrc.getHeight();
  const MUint32 *pixelsSrc = imageSrc.getPixelsInt();

  for (int y = 0; y < hSrc; y++)
  {
    const int yDst = yc + y;
    if (yDst < 0)
      continue;
    if (yDst >= m_height)
      break;
    const int yDstOff = yDst * m_width;
    const int ySrcOff = y * wSrc;
    for (int x = 0; x < wSrc; x++)
    {
      const int xDst = xc + x;
      if (xDst < 0)
        continue;
      if (xDst >= m_width)
        break;
      const MUint32 valSrc = pixelsSrc[ySrcOff + x] & 0x00ffffff;
      m_pixelsInt[xDst + yDstOff] = 0xff000000 | valSrc;
    } // for (x)
  } // for (y)
}

void Image::drawIntImageWithSignedFloatMask(
                                            const Image &imageSrc,
                                            const Image &imageMask,
                                            const int xc,
                                            const int yc
                                           )
{
  assert(imageSrc.getFormat() == IMAGE_FORMAT_UINT32);
  assert(imageMask.getFormat() == IMAGE_FORMAT_FLOAT);
  assert(getFormat() == IMAGE_FORMAT_UINT32);

  const int wSrc = imageSrc.getWidth();
  const int hSrc = imageSrc.getHeight();
  const MUint32 *pixelsSrc = imageSrc.getPixelsInt();

  const int wMask = imageMask.getWidth();
  const int hMask = imageMask.getHeight();
  const float *pixelsMask = imageMask.getPixelsFloat();
  if (wMask != wSrc)
    return;
  if (hMask != hSrc)
    return;

  for (int y = 0; y < hSrc; y++)
  {
    const int yDst = yc + y;
    if (yDst < 0)
      continue;
    if (yDst >= m_height)
      break;
    const int yDstOff = yDst * m_width;
    const int ySrcOff = y * wSrc;
    for (int x = 0; x < wSrc; x++)
    {
      const int xDst = xc + x;
      if (xDst < 0)
        continue;
      if (xDst >= m_width)
        break;
      MUint32 valSrc = pixelsSrc[ySrcOff + x] & 0x00ffffff;
      const float maskSrc = pixelsMask[ySrcOff + x];
      if (maskSrc > 0.0f) // out of object
      {
        MUint32 valB = (valSrc >>  0) & 0xff;
        MUint32 valG = (valSrc >>  8) & 0xff;
        MUint32 valR = (valSrc >> 16) & 0xff;
        // keep red component
        valB >>= 2;
        valG >>= 2;
        valSrc = (valR << 16) | (valG << 8) | valB;
      }

      m_pixelsInt[xDst + yDstOff] = 0xff000000 | valSrc;
    } // for (x)
  } // for (y)

}


void Image::drawFloatImage(
                            const Image &imageSrc,
                            const int xc,
                            const int yc,
                            const float scaleSrc
                          )
{
  assert( imageSrc.getFormat() == IMAGE_FORMAT_FLOAT);
  assert(getFormat() == IMAGE_FORMAT_UINT32);

  const int wSrc = imageSrc.getWidth();
  const int hSrc = imageSrc.getHeight();
  const float *pixelsSrc = imageSrc.getPixelsFloat();

  for (int y = 0; y < hSrc; y++)
  {
    const int yDst = yc + y;
    if (yDst < 0)
      continue;
    if (yDst >= m_height)
      break;
    const int yDstOff = yDst * m_width;
    const int ySrcOff = y * wSrc;
    for (int x = 0; x < wSrc; x++)
    {
      const int xDst = xc + x;
      if (xDst < 0)
        continue;
      if (xDst >= m_width)
        break;
      float valSrc = pixelsSrc[ySrcOff + x] * scaleSrc;
      valSrc = (valSrc >= 0.0f) ? valSrc : 0.0f;
      valSrc = (valSrc <= 255.0f) ? valSrc: 255.0f;
      MUint32 ival = (MUint32)valSrc;
      m_pixelsInt[xDst + yDstOff] =
        0xff000000 | (ival << 16) | (ival << 8) | (ival);
    } // for (x)
  } // for (y)
}

void Image::drawSignedFloatImage(
                                  const Image &imageSrc,
                                  const int xc,
                                  const int yc,
                                  const float scaleSrc
                                )
{
  assert(imageSrc.getFormat() == IMAGE_FORMAT_FLOAT);
  assert(getFormat() == IMAGE_FORMAT_UINT32);

  const int wSrc = imageSrc.getWidth();
  const int hSrc = imageSrc.getHeight();
  const float *pixelsSrc = imageSrc.getPixelsFloat();

  for (int y = 0; y < hSrc; y++)
  {
    const int yDst = yc + y;
    if (yDst < 0)
      continue;
    if (yDst >= m_height)
      break;
    const int yDstOff = yDst * m_width;
    const int ySrcOff = y * wSrc;
    for (int x = 0; x < wSrc; x++)
    {
      const int xDst = xc + x;
      if (xDst < 0)
        continue;
      if (xDst >= m_width)
        break;
      float valSrc = pixelsSrc[ySrcOff + x] * scaleSrc;
      int numShift = 0;
      if (valSrc >= 0.0f)
        numShift = 8; // green
      else
        numShift = 0; // blue
      valSrc = (valSrc >= 0.0f) ? valSrc: (-valSrc);
      valSrc = (valSrc <= 255.0f) ? valSrc : 255.0f;

      MUint32 ival = (MUint32)valSrc;
      m_pixelsInt[xDst + yDstOff] = 0xff000000 | (ival << numShift);
    } // for (x)
  } // for (y)
}

void Image::drawSignedFloatImageAutoScale(
                                          const Image &imageSrc,
                                          const int xc,
                                          const int yc
                                         )
{
  assert(imageSrc.getFormat() == IMAGE_FORMAT_FLOAT);
  assert(getFormat() == IMAGE_FORMAT_UINT32);

  const int wSrc = imageSrc.getWidth();
  const int hSrc = imageSrc.getHeight();
  const float *pixelsSrc = imageSrc.getPixelsFloat();

  float valMin = +1.0e12f, valMax = -1.0e12f;
  const int numPixels = wSrc * hSrc;
  int i;
  for (i = 0; i < numPixels; i++)
  {
    const float val = pixelsSrc[i];
    valMin = (val < valMin) ? val : valMin;
    valMax = (val > valMax) ? val : valMax;
  } // for (i)
  const float valRange = (-valMin > valMax) ? (-valMin) : valMax;

  for (int y = 0; y < hSrc; y++)
  {
    const int yDst = yc + y;
    if (yDst < 0)
      continue;
    if (yDst >= m_height)
      break;
    const int yDstOff = yDst * m_width;
    const int ySrcOff = y * wSrc;
    for (int x = 0; x < wSrc; x++)
    {
      const int xDst = xc + x;
      if (xDst < 0)
        continue;
      if (xDst >= m_width)
        break;
      float valSrc = pixelsSrc[ySrcOff + x];
      int numShift = 0;

      if (valMin * valMax >= 0.0f) 
      {
        // same sign for all values
        const float t = (valSrc - valMin) / (valMax - valMin);
        valSrc = t * 255.0f;
        numShift = 16; // red
      }
      else
      {
        // different signs
        const float t = (valSrc - 0.0f) / (valRange);
        if (valSrc > 0.0f)
        {
          numShift = 8; // green
          valSrc = 255.0f * t;
        }
        else
        {
          numShift = 0; // blue
          valSrc = -255.0f * t;
        }
      }
      

      valSrc = (valSrc >= 0.0f)   ? valSrc : (-valSrc);
      valSrc = (valSrc <= 255.0f) ? valSrc : 255.0f;

      MUint32 ival = (MUint32)valSrc;
      m_pixelsInt[xDst + yDstOff] = 0xff000000 | (ival << numShift);
    } // for (x)
  } // for (y)

}
