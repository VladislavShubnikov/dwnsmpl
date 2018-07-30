// ****************************************************************************
// File: volume.h
// Purpose: Volume tools
// ****************************************************************************

#ifndef  __volume_h
#define  __volume_h

// ****************************************************************************
// Includes
// ****************************************************************************

#include "mtypes.h"
#include "image.h"

// ****************************************************************************
// Class
// ****************************************************************************

/**
* \class VolumeTools used for volume processing operations
*/


class VolumeTools
{
public:
  static int  performGaussSlow(
                                const MUint8 *volPixelsSrc,
                                const int     xDim,
                                const int     yDim,
                                const int     zDim,
                                MUint8        *volPixelsDst
                              );
  static int  performGaussFast(
                                const MUint8 *volPixelsSrc,
                                const int     xDim,
                                const int     yDim,
                                const int     zDim,
                                MUint8        *volPixelsDst
                              );
};

#endif
