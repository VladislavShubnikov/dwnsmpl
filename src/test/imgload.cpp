// ****************************************************************************
// File: imgload.cpp
// Purpose: Load image
// ****************************************************************************

// ****************************************************************************
// Includes
// ****************************************************************************

// disable warning 'hides class member' for GdiPlus
#pragma warning( push )
#pragma warning( disable : 4458)

#include <windows.h>
#include <gdiplus.h>

#pragma warning( pop ) 

#include "imgload.h"
#include "memtrack.h"

// ****************************************************************************
// Methods
// ****************************************************************************

unsigned int *ImageLoader::readBitmap(const char *fileName, int *imgOutW, int *imgOutH)
{
  WCHAR                 uFileName[64];
  Gdiplus::Bitmap      *bitmap;
  int                   i, j;
  int                   imageWidth, imageHeight;
  unsigned int          *pixels;
  unsigned int          *src, *dst;


  // File name to uniclode
  for (i = 0; fileName[i]; i++)
    uFileName[i] = (WCHAR)fileName[i];
  uFileName[i] = 0;

  bitmap = Gdiplus::Bitmap::FromFile(uFileName);
  if (bitmap == NULL)
    return NULL;

  *imgOutW = imageWidth = bitmap->GetWidth();
  *imgOutH = imageHeight = bitmap->GetHeight();
  if (imageWidth == 0)
    return NULL;

  pixels = M_NEW(unsigned int[imageWidth * imageHeight]);
  if (pixels == NULL)
    return NULL;

  Gdiplus::Rect rect(0, 0, imageWidth, imageHeight);
  Gdiplus::BitmapData* bitmapData = M_NEW(Gdiplus::BitmapData);
  Gdiplus::Status status = bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);
  if (status != Gdiplus::Ok)
  {
    delete[] pixels;
    return NULL;
  }

  src = (unsigned int *)bitmapData->Scan0;
  dst = (unsigned int *)pixels;

  for (j = 0; j < imageHeight; j++)
  {
    // Read in inverse lines order
    //src = (unsigned int *)bitmapData->Scan0 + (imageHeight - 1 - j) * imageWidth;

    // Read in normal lines order
    src = (unsigned int *)bitmapData->Scan0 + j * imageWidth;

    for (i = 0; i < imageWidth; i++)
    {
      *dst = *src;
      src++; dst++;
    } // for all pixels
  }

  bitmap->UnlockBits(bitmapData);
  delete bitmapData;
  delete bitmap;

  return pixels;
}