// ******************************************************************
// Unit tests
// ******************************************************************

#define _CRT_SECURE_NO_WARNINGS

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>


// disable warning 'hides class member' for GdiPlus
#pragma warning( push )
#pragma warning( disable : 4458)

#include <windows.h>
#include <gdiplus.h>

#pragma warning( pop ) 

// Cspec test framework includes
#include "cspec.h"
#include "cspec_output_header.h"
#include "cspec_output_verbose.h"
#include "cspec_output_unit.h"

// App specific includes
#include "dsample2d.h"
#include "image.h"
#include "ktxtexture.h"
#include "memtrack.h"

#include "imgload.h"

// ******************************************************************
// Data
// ******************************************************************

// gdi+ stuff
static ULONG_PTR              s_gdiplusToken;

// some interestimng pixels coordinates
const int X_BACK = 0;
const int Y_BACK = 0;

const int X_GREY = 63;
const int Y_GREY = 277;

const int X_WHITE = 86;
const int Y_WHITE = 314;

// ******************************************************************
// Test descriptions
// ******************************************************************


DESCRIBE(testLoadImage, "void testLoadImage()")
  IT("test simple load image from file")
  {
    const char *IMAGE_FILE_NAME = "data/lungs_000.jpg";
    int w, h;
    MUint32 *pixelsSrcImage = (MUint32*)ImageLoader::readBitmap(IMAGE_FILE_NAME, &w, &h);
    SHOULD_BE_TRUE(pixelsSrcImage != NULL);
    SHOULD_EQUAL(w, 512);
    SHOULD_EQUAL(h, 512);

    const MUint32 valBack = pixelsSrcImage[X_BACK + Y_BACK * w] & 0xff;
    SHOULD_EQUAL(valBack, 0);

    const MUint32 valGrey = pixelsSrcImage[X_GREY + Y_GREY * w] & 0xff;
    SHOULD_EQUAL(valGrey, 102);

    const MUint32 valWhite = pixelsSrcImage[X_WHITE + Y_WHITE * w] & 0xff;
    SHOULD_EQUAL(valWhite, 250);

    // SHOULD_EQUAL_DOUBLE(vInter.y, 0.0f, 0.000001f);
    delete [] pixelsSrcImage;
  }
  END_IT

  IT("test slowGauss")
  {
    const char *IMAGE_FILE_NAME = "data/lungs_000.jpg";
    int w, h;
    MUint32 *pixelsSrcImage = (MUint32*)ImageLoader::readBitmap(IMAGE_FILE_NAME, &w, &h);
    SHOULD_BE_TRUE(pixelsSrcImage != NULL);

    int wSmall, hSmall;

    static float KOEF_SIZE_DOWN = 0.35f;
    wSmall = (int)(w * KOEF_SIZE_DOWN);
    hSmall = (int)(h * KOEF_SIZE_DOWN);

    Downsample2d downSampler;
    int okCreate = downSampler.create(w, h, pixelsSrcImage, wSmall, hSmall);
    SHOULD_EQUAL(okCreate, 1);

    float *pixelsSrc        = downSampler.getImageSrc();
    float *pixelsGauss      = downSampler.getImageGauss();
    float *pixelsBilateral  = downSampler.getImageBilaterail();

    printf("   Perform Gauss slow...\n");
    downSampler.performGaussSlow(pixelsSrc, pixelsGauss);

    const int NUM_ITERATIONS = 4;
    clock_t timeStart, timeEnd;

    timeStart = clock();
    for (int iter = 0; iter < NUM_ITERATIONS; iter++)
    {
      downSampler.performGaussSlow(pixelsSrc, pixelsGauss);
    }
    timeEnd = clock();
    const float timeDeltaGaussSlow = (float)(timeEnd - timeStart) / NUM_ITERATIONS;

    printf("   Perform Gauss fast...\n");
    timeStart = clock();
    for (int iter = 0; iter < NUM_ITERATIONS; iter++)
    {
      downSampler.performGaussFast(pixelsSrc, pixelsBilateral);
    }
    timeEnd = clock();
    const float timeDeltaGaussFast = (float)(timeEnd - timeStart) / NUM_ITERATIONS;
    const float accelerationTime = timeDeltaGaussSlow / timeDeltaGaussFast;

    const MUint32 TIME_GAUSS_SLOW_MAX = 500;
    SHOULD_BE_TRUE(timeDeltaGaussSlow < TIME_GAUSS_SLOW_MAX);

    const float SLOW_PER_FAST_TIMES = 5.0f;
    SHOULD_BE_TRUE(accelerationTime > SLOW_PER_FAST_TIMES);

    // compate interesting pixels
    float valSlow, valFast, valDif;
    int off;

    const int xBack   = (int)(X_BACK * KOEF_SIZE_DOWN);
    const int yBack   = (int)(Y_BACK * KOEF_SIZE_DOWN);
    const int xGrey   = (int)(X_GREY * KOEF_SIZE_DOWN);
    const int yGrey   = (int)(Y_GREY * KOEF_SIZE_DOWN);
    const int xWhite  = (int)(X_WHITE * KOEF_SIZE_DOWN);
    const int yWhite  = (int)(Y_WHITE * KOEF_SIZE_DOWN);

    const float MIN_PIXEL_DIF = 1.0e-5f;
    off = xBack + yBack * wSmall;
    valSlow = pixelsGauss[off];
    valFast = pixelsBilateral[off];
    valDif = fabsf(valSlow - valFast);
    SHOULD_BE_TRUE(valDif < MIN_PIXEL_DIF);

    off = xGrey + yGrey * wSmall;
    valSlow = pixelsGauss[off];
    valFast = pixelsBilateral[off];
    valDif = fabsf(valSlow - valFast);
    SHOULD_BE_TRUE(valDif < MIN_PIXEL_DIF);

    off = xWhite + yWhite * wSmall;
    valSlow = pixelsGauss[off];
    valFast = pixelsBilateral[off];
    valDif = fabsf(valSlow - valFast);
    SHOULD_BE_TRUE(valDif < MIN_PIXEL_DIF);

    // random walk on image and compare fast VS slow gauss filtration
    const int NUM_RANDOM_COORDS = 8;
    for (int iter = 0; iter < NUM_RANDOM_COORDS; iter++)
    {
      const int xRnd = rand() & RAND_MAX;
      const int yRnd = rand() & RAND_MAX;
      const int xCoord = wSmall * xRnd / RAND_MAX;
      const int yCoord = hSmall * yRnd / RAND_MAX;
      off = xCoord + yCoord * wSmall;
      valSlow = pixelsGauss[off];
      valFast = pixelsBilateral[off];
      valDif = fabsf(valSlow - valFast);
      SHOULD_BE_TRUE(valDif < MIN_PIXEL_DIF);
    }

  }
  END_IT
END_DESCRIBE

DESCRIBE(testLoadVolume, "void testLoadVolume()")
  IT("test simple load image from file")
  {
    const char *KTX_TEXTURE_FILE_NAME = "data/lungs_000.ktx";

    FILE *file;
    file = fopen(KTX_TEXTURE_FILE_NAME, "rb");
    SHOULD_BE_TRUE(file != NULL);

    KtxTexture *volTexture = M_NEW(KtxTexture);
    KtxError errLoad = volTexture->loadFromFileContent(file);
    fclose(file);
    SHOULD_BE_TRUE(errLoad == KTX_ERROR_OK);

    const int xDim = volTexture->getWidth();
    const int yDim = volTexture->getHeight();
    const int zDim = volTexture->getDepth();

    SHOULD_EQUAL(xDim, 512);
    SHOULD_EQUAL(yDim, 512);
    SHOULD_EQUAL(zDim, 128);

    delete volTexture;
  }
  END_IT
END_DESCRIBE


// ****************************************************************************
// Main test launcher
// ****************************************************************************

DEFINE_DESCRIPTION(testLoadImage)
DEFINE_DESCRIPTION(testLoadVolume)

int  main(int argc, char *argv)
{
  USE_PARAM(argc);
  USE_PARAM(argv);

  // Init GDI+
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&s_gdiplusToken, &gdiplusStartupInput, NULL);

  MemTrackStart();

  int res = 0;
  res += CSpec_Run(DESCRIPTION(testLoadImage),  CSpec_NewOutputVerbose());
  res += CSpec_Run(DESCRIPTION(testLoadVolume), CSpec_NewOutputVerbose());

  int memAllocatedSize = MemTrackGetSize(NULL);
  MemTrackStop();
  if (memAllocatedSize > 0)
  {
    printf("Allocation leak found with %d bytes!!!", memAllocatedSize);
    // static char strErr[120];
    // sprintf(strErr, "XXXXXXXXXXXXXXXXXXXXXXXXXXX Memory Leak ! %d bytes XXXXXXXXXXXXXXXXXXXXXXXXXXX", memAllocatedSize);
    // _logString(strErr);
    // MemTrackForAll(_memTrackCallbackPrint);
  }
  return res;
}
 