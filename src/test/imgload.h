// ****************************************************************************
// File: img_load.h
// Purpose: Load image from jpg file
// ****************************************************************************

#ifndef  __imgload_h
#define  __imgload_h

// ****************************************************************************
// Includes
// ****************************************************************************

#include "mtypes.h"
#include "image.h"

// ****************************************************************************
// Class
// ****************************************************************************


class ImageLoader
{
public:
  static unsigned int *readBitmap(const char *fileName, int *imgOutW, int *imgOutH);
};

#endif
