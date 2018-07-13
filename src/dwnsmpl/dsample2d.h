//  *****************************************************************
//  PURPOSE 2d Image Advanced downsampling technique
//  NOTES
//  *****************************************************************

#ifndef   __downsample2d_h
#define   __downsample2d_h

//  *****************************************************************
//  Includes
//  *****************************************************************

#include "mtypes.h"

//  *****************************************************************
//  Defines
//  *****************************************************************

// total colors
#define DSMPL_NUM_COLORS   256

// gauss neighbourhood area
#define DS_MAX_NEIB_RAD      12
#define DS_MAX_NEIB_DIA      (2 * DS_MAX_NEIB_RAD + 1)


//  *****************************************************************
//  Types
//  *****************************************************************

//  *****************************************************************
//  Classes
//  *****************************************************************

/**
* \class Downsample2d used for different 2d image downsampling
* approaches
*/

class Downsample2d
{
public:
  Downsample2d();
  ~Downsample2d();

  int     create(const int wSrc, const int hSrc, const MUint32 *pixels, const int wDst, const int hDst);
  void    destroy();

  int     getWidthSrc() const {
    return m_wSrc;
  }
  float *getImageSrc() const {
    return m_pixelsSrc;
  }

  int     getHeightSrc() const {
    return m_hSrc;
  }

  float *getImageSubSample() const {
    return m_pixelsSubSample;
  }

  float   *getImageGauss() const {
    return m_pixelsGauss;
  }
  float   *getImageBilaterail() const {
    return m_pixelsBilateral;
  }
  float   *getImageDownSampled() const {
    return m_pixelsDownSampled;
  }

  float     getSigmaBilateralPos() const {
    return m_sigmaBilateralPos;
  }
  float     getSigmaBilateralVal() const {
    return m_sigmaBilateralVal;
  }
  void      setSigmaBilateralPos(const float sigma) {
    m_sigmaBilateralPos = sigma;
  }
  void      setSigmaBilateralVal(const float sigma) {
    m_sigmaBilateralVal = sigma;
  }

  int   performDownSamplingAll();
  int   performGaussSlow(const float *pixelsSrc, float *pixelsDst);
  int   performGaussFast(const float *pixelsSrc, float *pixelsDst);

protected:
  int   performSubSample();
  int   performBilateral();
  int   performDownSample();

private:
  int       m_wSrc;
  int       m_hSrc;

  int       m_wDst;
  int       m_hDst;

  // source image float repsentation [0..1]
  float    *m_pixelsSrc;

  float    *m_pixelsSubSample;
  float    *m_pixelsGauss;
  float    *m_pixelsBilateral;
  float    *m_pixelsDownSampled;
  float    *m_pixelsRestored;

  float     m_sigmaBilateralPos;
  float     m_sigmaBilateralVal;

  // store precalculated neib pixel weights here
  float     m_filter[DS_MAX_NEIB_DIA * DS_MAX_NEIB_DIA];

};

#endif
