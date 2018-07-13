// ****************************************************************************
// File: image.h
// Purpose: Image 2d definition
// ****************************************************************************

#ifndef  __image_h
#define  __image_h

// ****************************************************************************
// Includes
// ****************************************************************************

#include "mtypes.h"

// ****************************************************************************
// Class
// ****************************************************************************

enum ImageFormat
{
  IMAGE_FORMAT_NA       =-1,
  IMAGE_FORMAT_UINT32   = 0,
  IMAGE_FORMAT_FLOAT    = 1
};

/**
* \class Image to keep 2d image properties
*/

class Image
{
public:

  Image();
  Image(const int w, const int h, MUint32 *pixels);
  Image(const int w, const int h, float *pixels);
  Image(const int w, const int h, const ImageFormat fmt);
  Image(Image &img);
  Image&  operator=(const Image &img);
  ~Image();

  void setAllocated(const int w, const int h, MUint32 *pixels);
  void setAllocated(const int w, const int h, float   *pixels);

  void drawIntImage(const Image &imageSrc, const int xc, const int yc);
  void drawIntImageWithSignedFloatMask(const Image &imageSrc, const Image &imageMask, const int xc, const int yc);
  void drawFloatImage(const Image &imageSrc, const int xc, const int yc, const float scaleSrc = 1.0f);
  void drawSignedFloatImage(const Image &imageSrc, const int xc, const int yc, const float scaleSrc = 1.0f);
  void drawSignedFloatImageAutoScale(const Image &imageSrc, const int xc, const int yc);

public:
  int getWidth() const
  {
    return m_width;
  }
  int getHeight() const
  {
    return m_height;
  }
  ImageFormat getFormat() const
  {
    return m_format;
  }
  MUint32 *getPixelsInt() const
  {
    return m_pixelsInt;
  }
  float *getPixelsFloat() const
  {
    return m_pixelsFloat;
  }

protected:
  void destroy();

private:
  int           m_needDestroy;
  ImageFormat   m_format;
  MUint32      *m_pixelsInt;
  float        *m_pixelsFloat;
  int           m_width;
  int           m_height;
};

#endif
