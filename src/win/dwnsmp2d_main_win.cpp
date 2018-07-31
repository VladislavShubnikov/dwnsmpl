//  *****************************************************************
//  PURPOSE Advanced 2d image downsampling technique demonstation
//  NOTES
//  *****************************************************************

//  *****************************************************************
//  Includes
//  *****************************************************************

#define _CRT_SECURE_NO_WARNINGS

// System specific includes
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

// disable warning 'hides class member' for GdiPlus
#pragma warning( push )
#pragma warning( disable : 4458)

#include <gdiplus.h>

#pragma warning( pop ) 

#include <sys/stat.h>
#include <share.h>
#include <winbase.h>

#include <tchar.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// App specific includes
#include "mtypes.h"
#include "draw.h"
#include "dump.h"
#include "memtrack.h"

#include "dsample2d.h"


//  **********************************************************
//  Defines
//  **********************************************************


#define MAIN_TITLE                "Press Arrows to change Gauss koeffs"
#define MAIN_CLASS_NAME           "DownSamplerClass"

// screen size
#define W_SRC                     (3 * (512 + 8))
#define H_SRC                     (1 * (512 + 8))

// background color
#define SCREEN_BACK_COLOR         0xf808080

//  **********************************************************
//  Types
//  **********************************************************

//  **********************************************************
//  Static data for this module
//  **********************************************************

static  int                   s_imageW;
static  int                   s_imageH;
static  int                  *s_imageSrc;

static  int                   s_wDownsample;
static  int                   s_hDownsample;

// gdi+ stuff
static ULONG_PTR              s_gdiplusToken;


static  HWND                  _hWnd;
static  HINSTANCE				      _hInstance;          // App instance
static  HCURSOR               s_cursorWait;
static  int                   s_mousePressed = 0;
static  V2d                   s_mouse;
static  BOOL                  _bActive      = FALSE;
static  BOOL                  _bReady       = FALSE;
static  BOOL                  _bInUpdate    = FALSE;
static  RECT                  _rcWindow;           // Saves the window size & pos.
static  RECT                  _rcViewport;         // Pos. & size to blt from
static  RECT                  _rcScreen;           // Screen pos. for blt

// Bitmap name
static  char                  _caBitmapName[128];

// Windows rendering buffer
static  HBITMAP               _hbmp;
// static  unsigned char         *_npPixels;
static int                    s_wScreen;
static int                    s_hScreen;
static Image                  s_imageScreen;

static  ULONGLONG             s_startTime;

static int                    s_spacePressed    = 0;
static int                    s_enterPressed    = 0;
static int                    s_plusPressed     = 0;
static int                    s_minusPressed    = 0;
static int                    s_multiplyPressed = 0;
static int                    s_dividePressed   = 0;
static int                    s_updatePressed   = 0;


static int                    s_userModifyPressed = 0;

static Downsample2d          *s_downSampler = NULL;
static int                    s_initDownSampler = 0;


//  **********************************************************
//  Forward refs
//  **********************************************************

static int saveBitmap(const char *fileName, const unsigned char *bitmapPixels , int bitmapW, int bitmapH);

// **********************************************************
// Logs
// **********************************************************

static void _logString(const char *s)
{
  int               i;
  static char strOut[256];
  for (i = 0; s[i]; i++)
    strOut[i] = (char)s[i];
  strOut[i++] = (char)'\n';
  strOut[i++] = 0;
  OutputDebugString(strOut);
}

static int _memTrackCallbackPrint(
                                  const void   *pMem,
                                  const int     memSize,
                                  const char   *fileNameSrc,
                                  const int     fileLineNumber
                                )
{
  static char     strIn[256];

  USE_PARAM(pMem);
  sprintf(strIn, "Memory leak size = %d bytes, %s, line %d\n", memSize, fileNameSrc, fileLineNumber);
  _logString(strIn);
  return 1;
}


// **********************************************************
// Functions 
// **********************************************************


// NAME       
// PURPOSE    
// ARGUMENTS  None
// RETURNS    None
// NOTES      None
//
static  BOOL _mainCreate(HWND hWnd)
{
  HDC            hdc;
  BYTE           bmiBuf[56];
  BITMAPINFO     *bmi;
  DWORD          *bmiCol;

  hdc   = GetDC(hWnd);
  if (!hdc)
    return FALSE;
  bmi   = ( BITMAPINFO * ) bmiBuf ;
  ZeroMemory( bmi , sizeof(BITMAPINFO) ) ;
  bmi->bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
  bmi->bmiHeader.biWidth        = +s_wScreen;
  bmi->bmiHeader.biHeight       = -s_hScreen;
  bmi->bmiHeader.biPlanes       = 1;
  bmi->bmiHeader.biBitCount     = 4 * 8;
  bmi->bmiHeader.biCompression  = BI_RGB;
  bmi->bmiHeader.biSizeImage    = s_wScreen * s_hScreen * 4;
  bmiCol                        = ( DWORD * )( bmiBuf + sizeof(BITMAPINFOHEADER) ) ;
  bmiCol[0]                     = 0 ;
  bmiCol[1]                     = 0 ;
  bmiCol[2]                     = 0 ;

  MUint32 *pixelsScreen;
  _hbmp  = CreateDIBSection( 
                            hdc,
                            bmi,
                            DIB_RGB_COLORS,
                            (void**)&pixelsScreen,
                            NULL,
                            0
                         );
  ReleaseDC(hWnd, hdc);
  if (!_hbmp)
    return FALSE;

  s_imageScreen.setAllocated(s_wScreen, s_hScreen, pixelsScreen);
  // fill screen with back color
  {
    MUint32 *pixelsScrDst = s_imageScreen.getPixelsInt();
    const int w = s_imageScreen.getWidth();
    const int h = s_imageScreen.getHeight();
    const int numPixelsScreen = w * h;
    for (int i = 0; i < numPixelsScreen; i++)
      pixelsScrDst[i] = SCREEN_BACK_COLOR;
  }

  return TRUE;
}

// NAME       
// PURPOSE    
// ARGUMENTS  None
// RETURNS    None
// NOTES      None
//
static  BOOL  _mainDestroy()
{
  if (_hbmp)
  {
    DeleteObject(_hbmp);
    _hbmp = 0;
  }
  return TRUE;

}


// NAME       
// PURPOSE    
// ARGUMENTS  None
// RETURNS    None
// NOTES      None
//
static  BOOL  _mainUpdateFrame(HWND hWnd)
{
  HDC             hdc;
  HDC             memDC;
  HBITMAP         hOldBitmap;
  ULONGLONG       curTime;
  ULONGLONG       dt;

  curTime = GetTickCount64();
  dt = (unsigned int)(curTime - s_startTime);


  const int X_SRC = 4;
  const int Y_SRC = 4;

  if (dt < 500)
  {
    // draw src image
    Image imgIntSource(s_imageW, s_imageH, (MUint32*)s_imageSrc);
    s_imageScreen.drawIntImage(imgIntSource, X_SRC, Y_SRC);

  }
  else
  {
    if (!s_userModifyPressed)
    {
    }

    // Update data
    if (!s_initDownSampler)
    {
      s_initDownSampler = 1;
      HCURSOR hCursorOld = SetCursor(s_cursorWait);
      {
        s_downSampler->performDownSamplingAll();
      }
      SetCursor(hCursorOld);
    } // update data

    if (s_updatePressed)
    {
      HCURSOR hCursorOld = SetCursor(s_cursorWait);
      {
        s_updatePressed = 0;
        // update
        s_downSampler->performDownSamplingAll();
      }
      SetCursor(hCursorOld);
    } // if need to update


    // fill screen with back color
    {
      MUint32 *pixelsScrDst = s_imageScreen.getPixelsInt();
      const int w = s_imageScreen.getWidth();
      const int h = s_imageScreen.getHeight();
      const int numPixelsScreen = w * h;
      for (int i = 0; i < numPixelsScreen; i++)
        pixelsScrDst[i] = SCREEN_BACK_COLOR;
    }

    // -------------- ROW 0 -----------------------------------
    Image imgIntSource(s_imageW, s_imageH, (MUint32*)s_imageSrc);
    Image imgFloatSubSample(s_wDownsample, s_hDownsample, s_downSampler->getImageSubSample() );
    Image imgFloatGauss(s_wDownsample, s_hDownsample, s_downSampler->getImageGauss() );
    Image imgFloatBila(s_wDownsample, s_hDownsample, s_downSampler->getImageBilaterail() );
    Image imgFloatDownsampled(s_wDownsample, s_hDownsample, s_downSampler->getImageDownSampled() );

    // draw src image
    s_imageScreen.drawIntImage(imgIntSource, X_SRC, Y_SRC);

    // draw sub smaple
    const int X_SUBSAMPLE = X_SRC + s_downSampler->getWidthSrc() + 4;
    const int Y_SUBSAMPLE = Y_SRC;
    const float SCALE_TO_256 = 256.0f;
    s_imageScreen.drawFloatImage(imgFloatSubSample, X_SUBSAMPLE, Y_SUBSAMPLE, SCALE_TO_256);

    // draw gauss
    const int X_GAUSS = X_SUBSAMPLE + s_wDownsample + 4;
    const int Y_GAUSS = Y_SUBSAMPLE;
    s_imageScreen.drawFloatImage(imgFloatGauss, X_GAUSS, Y_GAUSS, SCALE_TO_256);

    // draw bila
    const int X_BILA = X_GAUSS + s_wDownsample + 4;
    const int Y_BILA = Y_GAUSS;
    s_imageScreen.drawFloatImage(imgFloatBila, X_BILA, Y_BILA, SCALE_TO_256);

    // downsampled
    const int X_DWNS = X_BILA + s_wDownsample + 4;
    const int Y_DWNS = Y_BILA;
    s_imageScreen.drawFloatImage(imgFloatDownsampled, X_DWNS, Y_DWNS, SCALE_TO_256);


    if (s_multiplyPressed)
    {
    }
    else if (s_dividePressed)
    {
    }
    else if (s_enterPressed)
    {
    }
    else if (s_spacePressed)
    {
    }
    else if (s_plusPressed)
    {
    }
    else if (s_minusPressed)
    {
    }
    else
    {
    }

    // draw level set

  }     // if (dt > 0.5 sec)
  
  // Put to screen
  hdc      = GetDC(hWnd);
  if (!hdc)
    return FALSE;

  memDC    = CreateCompatibleDC(hdc);
  if (!memDC)
    return FALSE;
  hOldBitmap  = (HBITMAP)SelectObject(memDC, _hbmp);

  const int wScr = s_imageScreen.getWidth();
  const int hScr = s_imageScreen.getHeight();
  BitBlt( 
          hdc,
          0,                      // XDest
          0,                      // YDest
          wScr,                   // Width
          hScr,                   // Height
          memDC,                  // DC
          0,                      // XSrc
          0,                      // YSrc
          SRCCOPY                 // OpCode
       );

  SelectObject(  memDC , hOldBitmap);
  DeleteDC(      memDC);
  ReleaseDC(     hWnd, hdc);

  return TRUE;
}


static void _getMouseRel(const V2d &vAbs, V2d &vRel)
{
  int x = vAbs.x - 4;
  int y = vAbs.y - 4;
  int col = x / (s_imageW + 4);
  int row = y / (s_imageH + 4);

  vRel.x = x - col * (s_imageW + 4);
  vRel.y = y - row * (s_imageH + 4);
}


// NAME       
// PURPOSE    
// ARGUMENTS  None
// RETURNS    None
// NOTES      Noneh
//
static   ULONGLONG    s_lastUpdateFrameTime = 0xffffffffffff;
static   long         s_lastRenderNumber    = 0;
static   long         s_renderNumber        = 0;

static  BOOL _mainShowFrameRate(HWND hWnd)
{
  ULONGLONG   time = GetTickCount64() ;
  if (s_lastUpdateFrameTime == 0xffffffffffff)
  {
    s_lastUpdateFrameTime   = time;
    s_lastRenderNumber      = 0;
    s_renderNumber          = 0;
    static char *strMsg     = MAIN_TITLE;
    SetWindowText(hWnd, strMsg);
  }

  const ULONGLONG TIME_UPDATE_TITLE = 2000;
  if ((time - s_lastUpdateFrameTime > TIME_UPDATE_TITLE) && (s_renderNumber > 0))
  {

    V2d mouseRel;
    _getMouseRel(s_mouse, mouseRel);

    const float sigmaPos = s_downSampler->getSigmaBilateralPos();
    const float sigmaVal = s_downSampler->getSigmaBilateralVal();

    static char str[150];
    sprintf_s(
                str , 96,"SigmaPos = %5.2f | SigmaVal = %5.2f | FPS = %5.2f | MS=(%3d, %3d)",
                sigmaPos, sigmaVal,
                (float)( s_renderNumber - s_lastRenderNumber ) * 1000.0 / (float)(time - s_lastUpdateFrameTime ),
                mouseRel.x, mouseRel.y
                
             );
    SetWindowText(hWnd , str) ;
    s_lastUpdateFrameTime = time;
    s_lastRenderNumber    = s_renderNumber;
  }
  s_renderNumber++;
  return(TRUE);
}

static int _getWindowW(const int wImage)
{

  const int padSizeX = GetSystemMetrics(SM_CXSIZEFRAME);
  const int padBorder = GetSystemMetrics(SM_CXPADDEDBORDER);

  const int addHor = padSizeX + padBorder;
  const int SCR_W = wImage;
  int cx = SCR_W + 2 * addHor;
  return cx;
}

static int _getWindowH(const int hImage)
{
  const int padSizeY = GetSystemMetrics(SM_CYSIZEFRAME);
  const int padBorder = GetSystemMetrics(SM_CXPADDEDBORDER);
  const int padCap = GetSystemMetrics(SM_CYCAPTION);

  const int addTop = padSizeY + padBorder + padCap;
  const int addBot = padSizeY + padBorder;

  const int SCR_H = hImage;
  int cy = SCR_H + addTop + addBot;
  return cy;
}

// NAME       
// PURPOSE    
// ARGUMENTS  None
// RETURNS    None
// NOTES      None
//
LRESULT CALLBACK  MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_ACTIVATEAPP:
        {
          // Pause if minimized or not the top window
        	_bActive = (wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE);
          return 0L;
        }

        case WM_LBUTTONDOWN:
        {
          s_mousePressed = 1;
          s_mouse.x = LOWORD(lParam);
          s_mouse.y = HIWORD(lParam);
          SetCapture(hWnd);
          break;
        }
        case WM_LBUTTONUP:
        {
          s_mousePressed = 0;
          ReleaseCapture();
          break;
        }

        case WM_MOUSEMOVE:
        {
          s_mouse.x = LOWORD(lParam);
          s_mouse.y = HIWORD(lParam);

          if (s_mousePressed)
          {
            //WaScnAddSource( nX , nY );
          }

          break;
        }
        case WM_KEYDOWN:
        {
          const float SIGMA_STEP_POS = 0.05f;
          const float SIGMA_STEP_VAL = 0.5f;
          if ( wParam == VK_ESCAPE )
          {
            DestroyWindow(hWnd);
            return 0L;
          }
          else if (wParam == VK_LEFT)
          {
            float sigma = s_downSampler->getSigmaBilateralPos();
            sigma -= SIGMA_STEP_POS;
            if (sigma <= 0.0f)
              sigma += SIGMA_STEP_POS;
            s_downSampler->setSigmaBilateralPos(sigma);
            s_updatePressed = 1;
          }
          else if (wParam == VK_RIGHT)
          {
            float sigma = s_downSampler->getSigmaBilateralPos();
            sigma += SIGMA_STEP_POS;
            s_downSampler->setSigmaBilateralPos(sigma);
            s_updatePressed = 1;
          }
          else if (wParam == VK_DOWN)
          {
            float sigma = s_downSampler->getSigmaBilateralVal();
            sigma -= SIGMA_STEP_VAL;
            if (sigma <= 0.0f)
              sigma += SIGMA_STEP_VAL;
            s_downSampler->setSigmaBilateralVal(sigma);
            s_updatePressed = 1;
          }
          else if (wParam == VK_UP)
          {
            float sigma = s_downSampler->getSigmaBilateralVal();
            sigma += SIGMA_STEP_VAL;
            s_downSampler->setSigmaBilateralVal(sigma);
            s_updatePressed = 1;
          }

          else if (wParam == '1')
          {
            HCURSOR hCursorOld = SetCursor(s_cursorWait);
            {
            }
            SetCursor(hCursorOld);
          }
          else if (wParam == '2')
          {
            HCURSOR hCursorOld = SetCursor(s_cursorWait);
            {
            }
            SetCursor(hCursorOld);
          }
          else if (wParam == 'U')
          {
            // s_updatePressed = 1;
          }
          else if (wParam == VK_ADD)
          {
            s_plusPressed = 1 - s_plusPressed;
          }
          else if (wParam == VK_SUBTRACT)
          {
            s_minusPressed = 1 - s_minusPressed;
          }
          else if (wParam == VK_MULTIPLY)
          {
            s_multiplyPressed = 1 - s_multiplyPressed;
          }
          else if (wParam == VK_DIVIDE)
          {
            s_dividePressed = 1 - s_dividePressed;
          }
          else if ( wParam == VK_RETURN)
          {
            s_enterPressed = 1 - s_enterPressed;
          }
          else if (wParam == VK_SPACE)
          {
            s_spacePressed = 1 - s_spacePressed;
          }
          else if ( wParam == 'S')
          {
            /*
            MUint8 *pixelsScr = (MUint8*)s_imageScreen.getPixelsInt();
            int wScreen = s_imageScreen.getWidth();
            int hScreen = s_imageScreen.getHeight();
            saveBitmap("dump.png", pixelsScr, wScreen, hScreen);
            */
          }
          else if (wParam == 'H')
          {
            /*
            const float *heights = s_downSampler->getFieldSrc();
            const int w = s_downSampler->getWidth();
            const int h = s_downSampler->getHeight();
            const char *FIELD_MTL_NAME = "dump/field.mtl";
            const char *FIELD_OBJ_NAME = "dump/field.obj";
            Dump::saveHeightMapGeoToObjFile(heights, w, h, FIELD_MTL_NAME, FIELD_OBJ_NAME);
            */
          }
          else
          {
            //WaScnSetMode( (WaInt32)(wParam - '1') );
          }
          break;
        }

        case WM_KEYUP:
        {
          break;
        }


        case WM_DESTROY:
            // Clean up and close the app
            PostQuitMessage(0);
            return 0L;

        case WM_MOVE:
            // Retrieve the window position after a move
            if (_bActive && _bReady)
            {
              GetWindowRect(hWnd, &_rcWindow);
            	GetClientRect(hWnd, &_rcViewport);
            	GetClientRect(hWnd, &_rcScreen);
            	ClientToScreen(hWnd, (POINT*)&_rcScreen.left);
            	ClientToScreen(hWnd, (POINT*)&_rcScreen.right);
            }
            break;

        case WM_SETCURSOR:
            // Display the cursor in the window if windowed
            if (_bActive && _bReady)
            {
                //SetCursor(NULL);
                //return TRUE;
            }
            break;

        case WM_SIZE:
            // Check to see if we are losing our window...
            if (SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam)
                _bActive = FALSE;
            else
                _bActive = TRUE;
            break;
        case WM_GETMINMAXINFO:
          // Fix the size of the window to client size
          MINMAXINFO *pMinMax = (MINMAXINFO *)lParam;
          pMinMax->ptMinTrackSize.x = _getWindowW(s_wScreen);
          pMinMax->ptMinTrackSize.y = _getWindowH(s_hScreen);
          break;

    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

unsigned int *readBitmap(char *fileName, int *imgOutW, int *imgOutH)
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

  *imgOutW = imageWidth   = bitmap->GetWidth();
  *imgOutH = imageHeight  = bitmap->GetHeight();
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
    delete [] pixels;
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
/*
static int _getEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
  UINT  num = 0;          // number of image encoders
  UINT  size = 0;         // size of the image encoder array in bytes
  Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
  Gdiplus::GetImageEncodersSize(&num, &size);
  if(size == 0)
  return -1;  // Failure
  pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
  if(pImageCodecInfo == NULL)
  return -1;  // Failure
  GetImageEncoders(num, size, pImageCodecInfo);
  for(UINT j = 0; j < num; ++j)
  {
    if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
    {
      *pClsid = pImageCodecInfo[j].Clsid;
      free(pImageCodecInfo);
      return j;  // Success
    }
  }
  free(pImageCodecInfo);
  return -1;  // Failure
}

static int saveBitmap(const char *fileName, const unsigned char *bitmapPixels , int bitmapW, int bitmapH)
{
  WCHAR             uFileName[64];
  int               x, y, i;

  for (i = 0; fileName[i]; i++)
    uFileName[i] = (WCHAR)fileName[i];
  uFileName[i] = 0;

  Gdiplus::Bitmap *image  = new Gdiplus::Bitmap(bitmapW, bitmapH);
  for (y = 0; y < bitmapH; y++)
  {
    // save in inverse Y order
    //src = (unsigned int*)bitmapPixels + (bitmapH - 1 - y)* bitmapW;

    // save in normal order
    unsigned int *src = (unsigned int*)bitmapPixels + y * bitmapW;

    for (x = 0; x < bitmapW; x++, src++)
    {
      unsigned int color = *src;
      image->SetPixel(x, y, color);
    } // for all x
  } // for all y


  CLSID clsid;
	//_getEncoderClsid(L"image/jpeg", &clsid);
  _getEncoderClsid(L"image/png", &clsid);
  int ok = image->Save(uFileName, &clsid);
  delete image;
  return ok;
}
*/


// NAME       
// PURPOSE    
// ARGUMENTS  None
// RETURNS    None
// NOTES      None
//
int PASCAL  WinMain(
                      HINSTANCE   hInstance,
                      HINSTANCE   hPrevInstance,
                      LPSTR       cpCmdLine,
                      int         nCmdShow
                   )
{
  WNDCLASS	      wc;
  MSG			        msg;
  int             cx, cy;
  char            *fileName;

  // command line: load file name
  {
    char *s;

    if (cpCmdLine[0] == 0)
    {
      MessageBox(NULL, "Ls.exe <ImageFileName>", "Error", MB_OK);
      return -1;
    }
    s = cpCmdLine;
    while(s[0] == ' ')
      s++;
    if (s[0] == 0)
    {
      MessageBox(NULL, "Ls.exe <ImageFileName>", "Error", MB_OK);
      return -1;
    }
    fileName = s;
    while ( (s[0] != 0) && (s[0] != ' ') ) 
      s++;
  }

  MemTrackStart();

  // Init GDI+
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&s_gdiplusToken, &gdiplusStartupInput, NULL);

  USE_PARAM(cpCmdLine);
  if (!hPrevInstance)
  {
    // Register the Window Class
    wc.lpszClassName = MAIN_CLASS_NAME;
    wc.lpfnWndProc = MainWndProc;
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance = hInstance;
    wc.hIcon = NULL; //LoadIcon( hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL ; // MAKEINTRESOURCE(IDR_MENU);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    RegisterClass(&wc);

    s_cursorWait = LoadCursor(NULL, IDC_WAIT);
  }

  // fileName = FILE_NAME_BRAIN;
  s_imageSrc = (int*)readBitmap(fileName , &s_imageW, &s_imageH);
  if (!s_imageSrc)
  {
    static char strText[128];
    sprintf(strText, "cant load file: %s", fileName);
    MessageBox(NULL, strText, "Error load bitmap", MB_OK );
    return -1;
  }

  // proposal for downsample size
  //const float DOWNSAMPLE_SCALE_DOWN = 0.58f;
  const float DOWNSAMPLE_SCALE_DOWN = 0.35f;
  s_wDownsample = (int)(s_imageW * DOWNSAMPLE_SCALE_DOWN);
  s_hDownsample = (int)(s_imageH * DOWNSAMPLE_SCALE_DOWN);

  s_downSampler = M_NEW(Downsample2d());
  s_downSampler->create(s_imageW, s_imageH, (MUint32*)s_imageSrc, s_wDownsample, s_hDownsample);


  // Calculate the proper size for the window given a client 

  s_wScreen = W_SRC;
  s_hScreen = H_SRC;

  cx = _getWindowW(s_wScreen);
  cy = _getWindowH(s_hScreen);

  _hInstance = hInstance;
  // Create and Show the Main Window
  _hWnd = CreateWindowEx(0,
                          MAIN_CLASS_NAME,
                          MAIN_TITLE,
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
  	                      cx,
                          cy,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);
  if (_hWnd == NULL)
    return FALSE;
  ShowWindow(_hWnd, nCmdShow);
  UpdateWindow(_hWnd);

  s_wScreen = W_SRC;
  s_hScreen = H_SRC;
  if (!_mainCreate(_hWnd))
  {
    return FALSE;
  }

  _bReady = TRUE;

  s_startTime = GetTickCount64();

  //--------------------------------------------------------------
  //           The Message Loop
  //--------------------------------------------------------------
  msg.wParam = 0;
  while (_bReady)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
      // if QUIT message received quit app
      if (!GetMessage(&msg, NULL, 0, 0 ))
        break;
      // Translate and dispatch the message
      TranslateMessage(&msg); 
      DispatchMessage(&msg);
    }
    else
    if (_bActive && _bReady)
    {
      _bInUpdate = TRUE;

      // Update the background and flip every time the timer ticks
      _mainUpdateFrame(_hWnd);
      _mainShowFrameRate(_hWnd) ;

      _bInUpdate = FALSE;
    } // Update app
  } // while ( TRUE )

  _mainDestroy();

  s_downSampler->destroy();
  delete s_downSampler;
  s_downSampler = NULL;

  delete [] s_imageSrc;
  s_imageSrc = NULL;

  int memAllocatedSize = MemTrackGetSize(NULL);
  MemTrackStop();
  if (memAllocatedSize > 0)
  {
    // printf("Allocation leak found with %d bytes!!!", memAllocatedSize);
    static char strErr[120];
    sprintf(strErr, "XXXXXXXXXXXXXXXXXXXXXXXXXXX Memory Leak ! %d bytes XXXXXXXXXXXXXXXXXXXXXXXXXXX", memAllocatedSize);
    _logString(strErr);
    MemTrackForAll(_memTrackCallbackPrint);
  }
  return (int)msg.wParam;
}
