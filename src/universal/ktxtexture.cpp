// ****************************************************************************
//
// ****************************************************************************

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <assert.h>


#include "memtrack.h"
#include "ktxtexture.h"

// ****************************************************************************
// Defines
// ****************************************************************************

#define GAUSS_SMOOTH_MAX_SIDE        (11*2+1)

// ****************************************************************************
// Vars
// ****************************************************************************

static float  s_gaussWeights[GAUSS_SMOOTH_MAX_SIDE * GAUSS_SMOOTH_MAX_SIDE * GAUSS_SMOOTH_MAX_SIDE];

// KTX id sign
static unsigned char s_ktxIdFile[12] =
{
  0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

static char *s_ktxTextureErrMessages[KTX_ERROR_COUNT] =
{
  "OK",                     // [0] KTX_ERROR_OK
  "No memory",              // [1] KTX_ERROR_NO_MEMORY
  "Wrong format",           // [2] KTX_ERROR_WRONG_FORMAT
  "Wrong size",             // [3] KTX_ERROR_WRONG_SIZE
  "Broken data content",    // [4] KTX_ERROR_BROKEN_CONTENT
  "Error write to file"     // [5] KTX_ERROR_WRITE
};


// ****************************************************************************
// Methods
// ****************************************************************************

const char *KtxTextureGetErrorString(const KtxError err)
{
  return s_ktxTextureErrMessages[ (int)err ];
}

KtxTexture::KtxTexture()
{
  memset(&m_header, 0, sizeof(m_header) );
  m_data            = NULL;
  m_dataSize        = 0;
  m_isCompressed    = 0;
  m_boxSize.x = m_boxSize.y = m_boxSize.z = 0.0f;
}

KtxTexture::~KtxTexture()
{
  destroy();
}

void   KtxTexture::destroy()
{
  if (m_data != NULL)
    delete [] m_data;
  m_data      = NULL;
  m_dataSize  = 0;
  memset((char*)&m_header, 0, sizeof(m_header));
}

KtxError KtxTexture::createAs1ByteCopy(const KtxTexture *tex)
{
  int   sizeVolume;
  const MUint32 *src;

  // create 1 byte per pixel texture from 1/3/4 bpp texture
  memcpy(&m_header.m_id, &tex->m_header, sizeof(KtxHeader));

  // copy user key data
  memcpy(&m_keyData, &tex->m_keyData, sizeof(KtxKeyData) );

  // Created 1 byte per pixel texture
  m_header.m_glFormat              = KTX_GL_RED;
  // GL_R8_EXT, GL_R8 (0x8229)
  m_header.m_glInternalFormat      = KTX_GL_R8_EXT;
  m_header.m_glBaseInternalFormat  = KTX_GL_RED;

  sizeVolume = m_header.m_pixelWidth;
  if (m_header.m_pixelHeight > 0)
    sizeVolume *= m_header.m_pixelHeight;
  if (m_header.m_pixelDepth > 0)
    sizeVolume *= m_header.m_pixelDepth;

  if (m_data != NULL)
   delete [] m_data;

  m_data = M_NEW( MUint8[sizeVolume] );
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  m_isCompressed    = 0;
  m_dataSize        = sizeVolume;
  
  // Convert 4 -> 1
  if (tex->m_header.m_glFormat == KTX_GL_RGBA)
  {
    src = (const MUint32 *)tex->m_data;
    MUint8 *dst = (MUint8*)m_data;
    for (int i = 0; i < (int)sizeVolume; i++)
    {
      MUint32 val32 = src[i];
      val32 >>= 24;
      val32 &= 0xff;
      dst[i] = (MUint8)val32;
    }
  }
  // Convert 1 -> 1
  if (tex->m_header.m_glFormat == KTX_GL_RED)
  {
    memcpy(m_data, tex->m_data, sizeVolume);
  }

  return KTX_ERROR_OK;
}

KtxError KtxTexture::createAsCopy(const KtxTexture *tex)
{
  int   sizeVolume;

  // create 1 byte per pixel texture from 1/3/4 bpp texture
  memcpy(&m_header, &tex->m_header, sizeof(KtxHeader));

  sizeVolume = m_header.m_pixelWidth;
  if (m_header.m_pixelHeight > 0)
    sizeVolume *= m_header.m_pixelHeight;
  if (m_header.m_pixelDepth > 0)
    sizeVolume *= m_header.m_pixelDepth;

  if (tex->m_header.m_glFormat == KTX_GL_RED)
    sizeVolume *= 1;
  if (tex->m_header.m_glFormat == KTX_GL_RGB)
    sizeVolume *= 3;
  if (tex->m_header.m_glFormat == KTX_GL_RGBA)
    sizeVolume *= 4;

  m_isCompressed = 0;
  m_dataSize = sizeVolume;


  // delete old data
  if (m_data != NULL)
    delete[] m_data;

  m_data = M_NEW(MUint8[sizeVolume]);
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;

  // do copy
  memcpy(m_data, tex->m_data, sizeVolume);

  return KTX_ERROR_OK;
}

KtxError KtxTexture::create1D(const int xDim, const int bytesPerPixel)
{
  int     sizeVolume;

  memcpy(m_header.m_id, s_ktxIdFile, sizeof(s_ktxIdFile));
  m_header.m_endianness             = 0x04030201;
  m_header.m_glType                 = KTX_GL_UNSIGNED_BYTE;
  m_header.m_glTypeSize             = 1;

  m_header.m_pixelWidth             = xDim;
  m_header.m_pixelHeight            = 0;
  m_header.m_pixelDepth             = 0;

  m_header.m_numberOfArrayElements  = 0;  // for non-array textures is 0.
  m_header.m_numberOfFaces          = 1;
  m_header.m_numberOfMipmapLevels   = 1;
  //m_header.m_bytesOfKeyValueData   = sizeof(s_keyValues);
  m_header.m_bytesOfKeyValueData    = 0;

  switch (bytesPerPixel)
  {
    case 1:
    {
      m_header.m_glFormat              = KTX_GL_RED;
      // GL_R8_EXT, GL_R8 (0x8229)
      m_header.m_glInternalFormat      = KTX_GL_RED;
      m_header.m_glBaseInternalFormat  = KTX_GL_RED;
      break;
    }
    case 3:
    {
      m_header.m_glFormat              = KTX_GL_RGB;
      // GL_RGB8_OES, GL_RGB8_EXT (0x8051)
      m_header.m_glInternalFormat      = KTX_GL_RGB;
      m_header.m_glBaseInternalFormat  = KTX_GL_RGB;
      break;
    }
    case 4:
    {
      m_header.m_glFormat              = KTX_GL_RGBA;
      // GL_RGBA8_OES, GL_RGBA8_EXT (0x8058)
      m_header.m_glInternalFormat      = KTX_GL_RGBA;
      m_header.m_glBaseInternalFormat  = KTX_GL_RGBA;
      break;
    }
    default:
    {
      // impossible pixel format
      assert(bytesPerPixel < -5555);
      return KTX_ERROR_WRONG_FORMAT;
    }
  }       // switch

  sizeVolume = xDim * bytesPerPixel;
  m_data = M_NEW( MUint8[sizeVolume] );
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  memset(m_data, 0, sizeVolume);
  m_isCompressed    = 0;
  m_dataSize        = sizeVolume;
  return KTX_ERROR_OK;
}

KtxError KtxTexture::create2D(
                              const int xDim,
                              const int yDim,
                              const int bytesPerPixel
                             )
{
  int     sizeVolume;

  memcpy(m_header.m_id, s_ktxIdFile, sizeof(s_ktxIdFile));
  m_header.m_endianness             = 0x04030201;
  m_header.m_glType                 = KTX_GL_UNSIGNED_BYTE;
  m_header.m_glTypeSize             = 1;

  m_header.m_pixelWidth             = xDim;
  m_header.m_pixelHeight            = yDim;
  m_header.m_pixelDepth             = 0;

  m_header.m_numberOfArrayElements  = 0;  // for non-array textures is 0.
  m_header.m_numberOfFaces          = 1;
  m_header.m_numberOfMipmapLevels   = 1;
  //m_header.m_bytesOfKeyValueData   = sizeof(s_keyValues);
  m_header.m_bytesOfKeyValueData    = 0;

  switch (bytesPerPixel)
  {
    case 1:
    {
      m_header.m_glFormat              = KTX_GL_RED;
      // GL_R8_EXT, GL_R8 (0x8229)
      m_header.m_glInternalFormat      = KTX_GL_RED;
      m_header.m_glBaseInternalFormat  = KTX_GL_RED;
      break;
    }
    case 3:
    {
      m_header.m_glFormat              = KTX_GL_RGB;
      // GL_RGB8_OES, GL_RGB8_EXT (0x8051)
      m_header.m_glInternalFormat      = KTX_GL_RGB;
      m_header.m_glBaseInternalFormat  = KTX_GL_RGB;
      break;
    }
    case 4:
    {
      m_header.m_glFormat              = KTX_GL_RGBA;
      // GL_RGBA8_OES, GL_RGBA8_EXT (0x8058)
      m_header.m_glInternalFormat      = KTX_GL_RGBA;
      m_header.m_glBaseInternalFormat  = KTX_GL_RGBA;
      break;
    }
    default:
    {
      // impossible pixel format
      assert(bytesPerPixel < -5555);
      return KTX_ERROR_WRONG_FORMAT;
    }
  }       // switch

  sizeVolume = xDim * yDim * bytesPerPixel;
  m_data = M_NEW( MUint8[sizeVolume] );
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  memset(m_data, 0, sizeVolume);
  m_dataSize        = sizeVolume;
  m_isCompressed    = 0;
  return KTX_ERROR_OK;
}

KtxError KtxTexture::create3D(
                              const int xDim,
                              const int yDim,
                              const int zDim,
                              const int bytesPerPixel
                             )
{
  int     sizeVolume;

  memcpy(m_header.m_id, s_ktxIdFile, sizeof(s_ktxIdFile));
  m_header.m_endianness             = 0x04030201;
  m_header.m_glType                 = KTX_GL_UNSIGNED_BYTE;
  m_header.m_glTypeSize             = 1;

  m_header.m_pixelWidth             = xDim;
  m_header.m_pixelHeight            = yDim;
  m_header.m_pixelDepth             = zDim;

  m_header.m_numberOfArrayElements  = 0;  // for non-array textures is 0.
  m_header.m_numberOfFaces          = 1;
  m_header.m_numberOfMipmapLevels   = 1;
  //m_header.m_bytesOfKeyValueData   = sizeof(s_keyValues);
  m_header.m_bytesOfKeyValueData    = 0;

  switch (bytesPerPixel)
  {
    case 1:
    {
      m_header.m_glFormat              = KTX_GL_RED;
      // GL_R8_EXT, GL_R8 (0x8229)
      m_header.m_glInternalFormat      = KTX_GL_RED;
      m_header.m_glBaseInternalFormat  = KTX_GL_RED;
      break;
    }
    case 3:
    {
      m_header.m_glFormat              = KTX_GL_RGB;
      // GL_RGB8_OES, GL_RGB8_EXT (0x8051)
      m_header.m_glInternalFormat      = KTX_GL_RGB;
      m_header.m_glBaseInternalFormat  = KTX_GL_RGB;
      break;
    }
    case 4:
    {
      m_header.m_glFormat              = KTX_GL_RGBA;
      // GL_RGBA8_OES, GL_RGBA8_EXT (0x8058)
      m_header.m_glInternalFormat      = KTX_GL_RGBA;
      m_header.m_glBaseInternalFormat  = KTX_GL_RGBA;
      break;
    }
    default:
    {
      // impossible pixel format
      assert(bytesPerPixel < -5555);
      return KTX_ERROR_WRONG_FORMAT;
    }
  }       // switch

  sizeVolume = xDim * yDim * zDim * bytesPerPixel;
  m_data = M_NEW( MUint8[sizeVolume] );
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  memset(m_data, 0, sizeVolume);
  m_dataSize        = sizeVolume;
  m_isCompressed    = 0;
  return KTX_ERROR_OK;
}

KtxError KtxTexture::saveToFileContent(FILE *file)
{
  int   sizeVolume, bytesPerPixel;
  int   numBytesWritten;
  static char strKeyBuf[sizeof(int) * 2 + 8 * 2 + sizeof(V3f) * 2 + 8];

  if ((unsigned char)m_header.m_id[0] != s_ktxIdFile[0])
    return KTX_ERROR_BROKEN_CONTENT;

  // Add bytes for key values
  if (m_keyData.m_dataType == KTX_KEY_DATA_BBOX)
  {
    m_header.m_bytesOfKeyValueData = 0;
    char *dst = strKeyBuf;
    int pairSize = 8 + sizeof(V3f);
    assert((pairSize & 3) == 0);


    V3f *vBoxMin = reinterpret_cast<V3f*>(m_keyData.m_buffer);
    V3f *vBoxMax = reinterpret_cast<V3f*>(m_keyData.m_buffer) + 1;

    *((int*)dst) = pairSize;
    dst += sizeof(pairSize);
    strcpy(dst, "fBoxMin");
    dst += strlen("fBoxMin") +  1;

    memcpy(dst, vBoxMin, sizeof(V3f) );
    dst += sizeof(V3f);

    *((int*)dst) = pairSize;
    dst += sizeof(pairSize);
    strcpy(dst, "fBoxMax");
    dst += strlen("fBoxMax") + 1;
    memcpy(dst, vBoxMax, sizeof(V3f));
    dst += sizeof(V3f);

    m_header.m_bytesOfKeyValueData = (MUint32)(dst - strKeyBuf);
    assert(m_header.m_bytesOfKeyValueData <= sizeof(strKeyBuf));
  }

  if (m_keyData.m_dataType == KTX_KEY_DATA_MIN_SIZE)
  {
    m_header.m_bytesOfKeyValueData = 0;
    char *dst = strKeyBuf;
    int pairSize = 8 + sizeof(V3d);
    assert((pairSize & 3) == 0);

    V3d *vBoxMin  = reinterpret_cast<V3d*>(m_keyData.m_buffer);
    V3d *vBoxSize = reinterpret_cast<V3d*>(m_keyData.m_buffer) + 1;

    *((int*)dst) = pairSize;
    dst += sizeof(pairSize);
    strcpy(dst, "dBoxMin");
    dst += strlen("dBoxMin") + 1;

    memcpy(dst, vBoxMin, sizeof(V3d));
    dst += sizeof(V3f);

    *((int*)dst) = pairSize;
    dst += sizeof(pairSize);
    strcpy(dst, "dBoxSize");
    dst += strlen("dBoxSize") + 1;
    memcpy(dst, vBoxSize, sizeof(V3d));
    dst += sizeof(V3d);

    m_header.m_bytesOfKeyValueData = (MUint32)(dst - strKeyBuf);
    assert(m_header.m_bytesOfKeyValueData <= sizeof(strKeyBuf));
  }


  bytesPerPixel   = 
    (m_header.m_glFormat == KTX_GL_RED)?
    1 : ((m_header.m_glFormat == KTX_GL_RGB)? 3: 4);
  sizeVolume      = m_header.m_pixelWidth * bytesPerPixel;
  if (m_header.m_pixelHeight > 0)
    sizeVolume *= m_header.m_pixelHeight;
  if (m_header.m_pixelDepth > 0)
    sizeVolume *= m_header.m_pixelDepth;

  // write header to dest buffer
  numBytesWritten = (int)fwrite(&m_header, 1, sizeof(m_header), file );
  if (numBytesWritten != sizeof(m_header))
    return KTX_ERROR_WRITE;

  // write key values, if present in texture
  if (
        (m_keyData.m_dataType != KTX_KEY_DATA_NA) &&
        m_header.m_bytesOfKeyValueData
     )
  {
    numBytesWritten = (int)fwrite(strKeyBuf, 1, m_header.m_bytesOfKeyValueData, file);

    if (numBytesWritten != (int)m_header.m_bytesOfKeyValueData)
      return KTX_ERROR_WRITE;
  }   // if exists key data

  // write volume size to dest buffer
  numBytesWritten = (int)fwrite(&sizeVolume, 1, sizeof(sizeVolume), file);
  if (numBytesWritten != sizeof(sizeVolume))
    return KTX_ERROR_WRITE;


  // write image bits
  assert(m_data != NULL);
  numBytesWritten = (int)fwrite(m_data, 1, sizeVolume, file);
  if (numBytesWritten != sizeVolume)
    return KTX_ERROR_WRITE;

  return KTX_ERROR_OK;
}

KtxError KtxTexture::loadFromFileContent(FILE *file)
{
  int numReadedBytes, xDim, yDim, zDim, bytesPerVoxel;

  // numReadedBytes = (int)file.read( (char*)&m_header, sizeof(m_header)  );
  numReadedBytes = (int)fread(&m_header, 1, sizeof(m_header), file);
  if (numReadedBytes != sizeof(m_header))
    return KTX_ERROR_WRONG_SIZE;

  // check is it KTX file header
  if (memcmp(m_header.m_id, s_ktxIdFile, sizeof(s_ktxIdFile)) != 0)
    return KTX_ERROR_BROKEN_CONTENT;
  if (m_header.m_endianness != 0x04030201)
    return KTX_ERROR_WRONG_FORMAT;

  xDim = (int)m_header.m_pixelWidth;
  yDim = (int)m_header.m_pixelHeight;
  zDim = (int)m_header.m_pixelDepth;
  USE_PARAM(xDim);
  USE_PARAM(yDim);
  USE_PARAM(zDim);

  m_isCompressed = 0;
  bytesPerVoxel = 0;
  if (m_header.m_glFormat == KTX_GL_RED)
    bytesPerVoxel = 1;
  else if (m_header.m_glFormat == KTX_GL_RGB)
    bytesPerVoxel = 3;
  else if (m_header.m_glFormat == KTX_GL_RGBA)
    bytesPerVoxel = 4;
  else if ( 
    (m_header.m_glInternalFormat == KTX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT) &&
    (m_header.m_glFormat == 0) 
          )
  {
    bytesPerVoxel = 1;
    m_isCompressed  = 1;
  }
  else if ( 
    (m_header.m_glInternalFormat == KTX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) &&
    (m_header.m_glFormat == 0) 
          )
  {
    bytesPerVoxel = 1;
    m_isCompressed  = 1;
  }
  else
    return KTX_ERROR_WRONG_FORMAT;
  //volSize = xDim * yDim * zDim * bytesPerVoxel;
  USE_PARAM(bytesPerVoxel);

  if (m_header.m_bytesOfKeyValueData > 0)
  {
    char *userData;

    userData = M_NEW(char[m_header.m_bytesOfKeyValueData + 8] );
    if (!userData)
      return KTX_ERROR_NO_MEMORY;
    numReadedBytes = (int)fread(userData, 1, m_header.m_bytesOfKeyValueData, file);
    if (numReadedBytes != (int)m_header.m_bytesOfKeyValueData)
    {
      delete [] userData;
      return KTX_ERROR_WRONG_FORMAT;
    }

    V3f vBoxMin, vBoxMax;

    // parse user data
    static char strName[64];
    char *src;
    src = userData;
    while (src - userData < (int)m_header.m_bytesOfKeyValueData)
    {
      int pairLen;
      pairLen = *((int*)src);
      USE_PARAM(pairLen);
      src += sizeof(int);
      char *dst;
      for (
            dst = strName;
            (src[0] != 0) && (dst - strName < sizeof(strName)-1);
            src++, dst++
          )
      {
        *dst = *src;
      }
      *dst = 0;
      src++;
      if (strcmp(strName, "fBoxMin") == 0)
      {
        V3f *vertDstMin = reinterpret_cast<V3f*>(m_keyData.m_buffer);

        m_keyData.m_dataType = KTX_KEY_DATA_BBOX;
        V3f *vertData = reinterpret_cast<V3f*>(src);
        *vertDstMin  = *vertData;
        vBoxMin = *vertData;
        src += sizeof(V3f);
      }
      if (strcmp(strName, "fBoxMax") == 0)
      {
        V3f *vertDstMax = reinterpret_cast<V3f*>(m_keyData.m_buffer) + 1;

        m_keyData.m_dataType = KTX_KEY_DATA_BBOX;
        V3f *vertData = reinterpret_cast<V3f*>(src);
        *vertDstMax = *vertData;
        vBoxMax = *vertData;
        src += sizeof(V3f);

        m_boxSize.x = vBoxMax.x - vBoxMin.x;
        m_boxSize.y = vBoxMax.y - vBoxMin.y;
        m_boxSize.z = vBoxMax.z - vBoxMin.z;
      }
      if (strcmp(strName, "dBoxMin") == 0)
      {
        V3d *vertDstMin = reinterpret_cast<V3d*>(m_keyData.m_buffer);

        m_keyData.m_dataType = KTX_KEY_DATA_MIN_SIZE;
        V3d *vertData = reinterpret_cast<V3d*>(src);
        *vertDstMin = *vertData;
        src += sizeof(V3d);
      }
      if (strcmp(strName, "dBoxSize") == 0)
      {
        V3d *vertDstSize = reinterpret_cast<V3d*>(m_keyData.m_buffer) + 1;

        m_keyData.m_dataType = KTX_KEY_DATA_MIN_SIZE;
        V3d *vertData = reinterpret_cast<V3d*>(src);
        *vertDstSize =  *vertData;
        src += sizeof(V3d);
      }
    }     // while ! end of user key pairs
    delete[] userData;
  }

  // read size
  //numReadedBytes = (int)file.read( (char*)&m_dataSize, sizeof(m_dataSize) );
  numReadedBytes = (int)fread(&m_dataSize, 1, sizeof(m_dataSize), file);
  if (numReadedBytes != sizeof(m_dataSize))
    return KTX_ERROR_WRONG_FORMAT;
  if (m_dataSize > 1024 * 1024 * 512)
    return KTX_ERROR_WRONG_FORMAT;

  if (m_data)
    delete [] m_data;

  m_data = M_NEW(MUint8[m_dataSize]);
  if (m_data == NULL)
    return KTX_ERROR_NO_MEMORY;
  //numReadedBytes = (int)file.read( (char*)m_data, m_dataSize);
  numReadedBytes = (int)fread(m_data, 1, m_dataSize, file);
  if (numReadedBytes != m_dataSize)
    return KTX_ERROR_WRONG_SIZE;

  
  if (m_keyData.m_dataType == KTX_KEY_DATA_MIN_SIZE)
  {
    KtxError err = enlargeByMinSize();
    return err;
  }
  
  return KTX_ERROR_OK;
}

KtxError    KtxTexture::enlargeByMinSize()
{
  if (m_keyData.m_dataType != KTX_KEY_DATA_MIN_SIZE)
    return KTX_ERROR_OK;
  if (m_header.m_glFormat != KTX_GL_RGBA)
    return KTX_ERROR_WRONG_FORMAT;

  V3d *vBoxMin   = reinterpret_cast<V3d*>(m_keyData.m_buffer);
  V3d *vBoxSize  = reinterpret_cast<V3d*>(m_keyData.m_buffer) + 1;

  int xDimDst = vBoxSize->x;
  int yDimDst = vBoxSize->y;
  int zDimDst = vBoxSize->z;

  int xDimSrc = getWidth();
  int yDimSrc = getHeight();
  int zDimSrc = getDepth();

  int numPixelsDst = xDimDst * yDimDst * zDimDst;
  MUint32 *pixelsDst = M_NEW(MUint32[numPixelsDst]);
  if (pixelsDst == NULL)
    return KTX_ERROR_NO_MEMORY;

  // Clear with background
  MUint32 *pixelsSrc = (MUint32*)m_data;
  MUint32 valBackground = pixelsSrc[0];
  for (int i = 0; i < numPixelsDst; i++)
  {
    pixelsDst[i] = valBackground;
  }

  // Copy part of volume
  int     x, y, z;

  for (z = 0; z < zDimSrc; z++)
  {
    int zDst = z + vBoxMin->z;
    int zDstOff = zDst * xDimDst * yDimDst;
    for (y = 0; y < yDimSrc; y++)
    {
      int yDst = y + vBoxMin->y;
      int yDstOff = yDst * xDimDst;
      for (x = 0; x < xDimSrc; x++)
      {
        MUint32 val = *pixelsSrc++;
        int xDst = x + vBoxMin->x;
        int off = xDst + yDstOff + zDstOff;
        pixelsDst[off] = val;
      }   // for (x)
    }     // for (y)
  }       // for (z)

  // setup new dimensions
  m_header.m_pixelWidth   = xDimDst;
  m_header.m_pixelHeight  = yDimDst;
  m_header.m_pixelDepth   = zDimDst;

  // assign new pixels array
  delete[] m_data;
  m_data = (MUint8*)pixelsDst;
  return KTX_ERROR_OK;
}


static __inline const char *_readUntil(
                                        const char *strStart,
                                        const int   bufSize,
                                        const char *strBuf,
                                        const char symMatch
                                      )
{
  const char *str = strBuf;
  while (str[0] != symMatch)
  {
    str++;
    if (str - strStart >= bufSize)
      return NULL;
  }
  str++;
  return str;
}

#define _IS_DIGIT(ch)   ((((ch) >= '0') && ((ch) <= '9')) ||\
  ((ch) == 'e') || ((ch) == '.') || ((ch) == '-') || ((ch) == '+'))

static int _closestPowerOfTwoGreatOrEqual(const int val)
{
  int   i;

  for (i = 1; i < 31; i++)
  {
    int dim = 1 << i;
    if (val <= dim)
      return dim;
  }
  return -1;
}

static void _copyAndAlignTexture(
                                  const MUint8 *pixelsSrc,
                                  const int     xDimSrc,
                                  const int     yDimSrc,
                                  const int     zDimSrc,
                                  MUint8       *pixelsDst,
                                  const int     xDimDst,
                                  const int     yDimDst,
                                  const int     zDimDst
                                )
{
  int   xyzDimDst;
  int   x, y, z;

  int xDimSrc2 = xDimSrc / 2;
  int yDimSrc2 = yDimSrc / 2;
  int zDimSrc2 = zDimSrc / 2;

  int xDimDst2 = xDimDst / 2;
  int yDimDst2 = yDimDst / 2;
  int zDimDst2 = zDimDst / 2;

  xyzDimDst = xDimDst * yDimDst * zDimDst;

  int xyzDimSrc = xDimSrc * yDimSrc * zDimSrc;
  int numBlacks = 0;
  int numWhites = 0;
  for (x = 0; x < xyzDimSrc; x++)
  {
    MUint32 val = (MUint32)pixelsSrc[x];
    if (val <= 32)
      numBlacks++;
    if (val >= 256 - 32)
      numWhites++;

  }
  MUint32 valBackground;
  if (numBlacks > numWhites)
    valBackground = 0;
  else
    valBackground = 255;

  memset((char*)pixelsDst, valBackground, xyzDimDst);

  int indSrc = 0;
  for (z = 0; z < zDimSrc; z++)
  {
    int zDst = z - zDimSrc2 + zDimDst2;
    int zOff = zDst * (xDimDst * yDimDst);
    for (y = 0; y < yDimSrc; y++)
    {
      int yDst = y - yDimSrc2 + yDimDst2;
      int yOff = yDst * xDimDst;
      for (x = 0; x < xDimSrc; x++)
      {
        int   offDst;

        MUint8 val = pixelsSrc[indSrc++];
        int xDst = x - xDimSrc2 + xDimDst2;
        offDst = xDst + yOff + zOff;
        pixelsDst[offDst] = val;
      }       // for (x)
    }         // for (y)
  }           // for (z)
}

KtxError  KtxTexture::createAs1ByteCopyPowerOfTwo(const KtxTexture *tex)
{
  int   sizeVolume;

  // create 1 byte per pixel texture from 1/3/4 bpp texture
  memcpy(&m_header.m_id, &tex->m_header, sizeof(KtxHeader));
  // Make texture dimension as power of two.
  m_header.m_pixelWidth   = 
    _closestPowerOfTwoGreatOrEqual(tex->m_header.m_pixelWidth);
  m_header.m_pixelHeight  = 
    _closestPowerOfTwoGreatOrEqual(tex->m_header.m_pixelHeight);
  m_header.m_pixelDepth   = 
    _closestPowerOfTwoGreatOrEqual(tex->m_header.m_pixelDepth);

  // copy user key data
  memcpy(&m_keyData, &tex->m_keyData, sizeof(KtxKeyData));

  // Created 1 byte per pixel texture
  m_header.m_glFormat             = KTX_GL_RED;
  // GL_R8_EXT, GL_R8 (0x8229)
  m_header.m_glInternalFormat     = KTX_GL_R8_EXT;
  m_header.m_glBaseInternalFormat = KTX_GL_RED;

  sizeVolume = m_header.m_pixelWidth;
  if (m_header.m_pixelHeight > 0)
    sizeVolume *= m_header.m_pixelHeight;
  if (m_header.m_pixelDepth > 0)
    sizeVolume *= m_header.m_pixelDepth;

  if (m_data != NULL)
    delete[] m_data;

  m_data = M_NEW(MUint8[sizeVolume]);
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  m_isCompressed = 0;
  m_dataSize = sizeVolume;

  // Convert 1 -> 1
  assert(tex->m_header.m_glFormat == KTX_GL_RED);
  //memcpy(m_data, tex->m_data, sizeVolume);

  int xDimSrc = tex->m_header.m_pixelWidth;
  int yDimSrc = tex->m_header.m_pixelHeight;
  int zDimSrc = tex->m_header.m_pixelDepth;

  int xDimDst = m_header.m_pixelWidth;
  int yDimDst = m_header.m_pixelHeight;
  int zDimDst = m_header.m_pixelDepth;

  const MUint8 *pixelsSrc = tex->getData();
  MUint8 *pixelsDst = getData();

  _copyAndAlignTexture(
                        pixelsSrc,
                        xDimSrc, yDimSrc, zDimSrc,
                        pixelsDst,
                        xDimDst, yDimDst, zDimDst
                      );

  return KTX_ERROR_OK;
}

KtxError  KtxTexture::createAs1ByteCopyScaledDown(
                                                   const KtxTexture *tex,
                                                   const int scaleDownTimes
                                                 )
{
  int   sizeVolume;

  // create 1 byte per pixel texture from 1/3/4 bpp texture
  memcpy(&m_header.m_id, &tex->m_header, sizeof(KtxHeader));
  // Make texture dimension as power of two.
  m_header.m_pixelWidth   = tex->m_header.m_pixelWidth / scaleDownTimes;
  m_header.m_pixelHeight  = tex->m_header.m_pixelHeight / scaleDownTimes;
  m_header.m_pixelDepth   = tex->m_header.m_pixelDepth / scaleDownTimes;

  // copy user key data
  memcpy(&m_keyData, &tex->m_keyData, sizeof(KtxKeyData));

  // Created 1 byte per pixel texture
  m_header.m_glFormat = KTX_GL_RED;
  m_header.m_glInternalFormat = KTX_GL_R8_EXT;        // GL_R8_EXT, GL_R8 (0x8229)
  m_header.m_glBaseInternalFormat = KTX_GL_RED;

  sizeVolume = m_header.m_pixelWidth;
  if (m_header.m_pixelHeight > 0)
    sizeVolume *= m_header.m_pixelHeight;
  if (m_header.m_pixelDepth > 0)
    sizeVolume *= m_header.m_pixelDepth;

  if (m_data != NULL)
    delete[] m_data;

  m_data = M_NEW(MUint8[sizeVolume]);
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  m_isCompressed = 0;
  m_dataSize = sizeVolume;

  // Convert 1 -> 1
  assert(tex->m_header.m_glFormat == KTX_GL_RED);

  int xDimSrc = tex->m_header.m_pixelWidth;
  int yDimSrc = tex->m_header.m_pixelHeight;
  //int zDimSrc = tex->m_header.m_pixelDepth;

  int xDimDst = m_header.m_pixelWidth;
  int yDimDst = m_header.m_pixelHeight;
  int zDimDst = m_header.m_pixelDepth;

  const MUint8 *pixelsSrc = tex->getData();
  MUint8 *pixelsDst = getData();

  int x, y, z;
  int indDst = 0;
  for (z = 0; z < zDimDst; z++)
  {
    int zSrcMin = z * scaleDownTimes;
    int zSrcMax = (z + 1) * scaleDownTimes;
    for (y = 0; y < yDimDst; y++)
    {
      int ySrcMin = y * scaleDownTimes;
      int ySrcMax = (y + 1) * scaleDownTimes;
      for (x = 0; x < xDimDst; x++)
      {
        int xSrcMin = x * scaleDownTimes;
        int xSrcMax = (x + 1) * scaleDownTimes;

        MUint32 sum = 0;
        int numPixels = 0;
        int xx, yy, zz;
        for (zz = zSrcMin; zz < zSrcMax; zz++)
        {
          for (yy = ySrcMin; yy < ySrcMax; yy++)
          {
            for (xx = xSrcMin; xx < xSrcMax; xx++)
            {
              int offSrc = xx + (yy * xDimSrc) + (zz * xDimSrc * yDimSrc);
              sum += (MUint32)pixelsSrc[offSrc];
              numPixels++;
            }
          }
        }
        sum = sum / numPixels;

        pixelsDst[indDst] = (MUint8)sum;
        indDst++;
      }
    }
  }
  return KTX_ERROR_OK;
}

static int V3dEqual(const V3d &va, const V3d &vb)
{
  V3d v;
  v.x = va.x - vb.x;
  v.y = va.y - vb.y;
  v.z = va.z - vb.z;
  return ((v.x | v.y | v.z) == 0) ? 1: 0;
}

static void _addValueToArray(
                              V3d *arr,
                              const int maxArrElements,
                              const V3d &vecAdd,
                              int &numElements
                            )
{
  int i;
  for (i = 0; i < numElements; i++)
  {
    if (V3dEqual(arr[i], vecAdd))
      return;
  }
  // add
  assert(numElements < maxArrElements);
  if (numElements < maxArrElements)
  {
    arr[numElements++] = vecAdd;
  }
}

KtxError  KtxTexture::createMask(
                                  const KtxTexture *tex,
                                  const int valBarrier,
                                  const int numAddVoxels
                                 )
{
  KtxError err;
  err = createAs1ByteCopy(tex);
  if (err != KTX_ERROR_OK)
    return err;
  
  int xDim = getWidth();
  int yDim = getHeight();
  int zDim = getDepth();
  int xyDim = xDim * yDim;
  int xyzDim = xDim * yDim* zDim;

  MUint8 *pixelsBina = M_NEW(MUint8[xyzDim]);
  if (!pixelsBina)
    return KTX_ERROR_NO_MEMORY;

  static V3d offRing[128];
  int             numPixelsInRing;
  int             i, j;
  int             x, y, z;
  int             radius = numAddVoxels;

  // build ring offsets
  numPixelsInRing = 0;
  for (j = 0; j < 16; j++)
  {
    float angleTeta = -3.1415926f / 2 + 3.1415926f * (float)j / 16.0f;
    for (i = 0; i < 32; i++)
    {
      float anglePhi = 3.1415926f * 2.0f * (float)i / 32.0f;
      z = (int)(radius * sinf(angleTeta));
      float xy = radius * cosf(angleTeta);
      x = (int)(xy * cosf(anglePhi));
      y = (int)(xy * sinf(anglePhi));

      V3d v;
      v.x = x;
      v.y = y;
      v.z = z;
      _addValueToArray(offRing, sizeof(offRing) / sizeof(offRing[0]),
        v, numPixelsInRing);
    }
  }     // for (j)


  const MUint8  *pixelsSrc = tex->getData();
  MUint8        *pixelsDst = getData();
  int indDst;

  indDst = 0;
  for (z = 0; z < zDim; z++)
  {
    for (y = 0; y < yDim; y++)
    {
      for (x = 0; x < xDim; x++)
      {
        MUint32 valDst;
        MUint32 val = (MUint32)pixelsSrc[indDst];

        // binarize by barrier
        valDst = (val <= (MUint32)valBarrier)? 0: 255;

        pixelsBina[indDst] = (MUint8)valDst;
        indDst++;
      }   // for (x)
    }     // for (y)
  }       // for (z)

  // Dilatation

  indDst = 0;
  for (z = 0; z < zDim; z++)
  {
    for (y = 0; y < yDim; y++)
    {
      for (x = 0; x < xDim; x++)
      {
        if (pixelsBina[indDst])
        {
          pixelsDst[indDst] = pixelsBina[indDst];
          indDst++;
          continue;
        }

        // check neighbours
        int hasNeibObject = 0;
        for (i = 0; i < numPixelsInRing; i++)
        {
          int xDst = x + offRing[i].x;
          int yDst = y + offRing[i].y;
          int zDst = z + offRing[i].z;
          if ( (xDst < 0) || (xDst >= xDim) )
           continue;
          if ( (yDst < 0) || (yDst >= yDim) )
            continue;
          if ( (zDst < 0) || (zDst >= zDim) )
            continue;
          int offNeigh = xDst + (yDst * xDim) + (zDst * xyDim);
          if (pixelsBina[offNeigh])
          {
            hasNeibObject = 1;
            break;
          }
        }     // for (i) all neighbours in 3d sphere

        pixelsDst[indDst] = (hasNeibObject)? 255: 0;
        indDst++;
      }   // for (x)
    }     // for (y)
  }       // for (z)
  delete[] pixelsBina;

  return KTX_ERROR_OK;
}

// Perform smoothing for 1-byte 2d image
#define   MAX_BOX_LEN     32

static void _boxFilter3d(
                          MUint8 *pixels,
                          const int xDim,
                          const int yDim,
                          const int zDim,
                          const int radius
                        )
{
  static MUint32  history[MAX_BOX_LEN];
  int             boxLen = radius + radius + 1;
  int             x, y, z, dr, offLine, xyDim, zOff;
  int             multFast;

  assert(boxLen <= MAX_BOX_LEN);

  xyDim = xDim * yDim;
  multFast = (int)(512.0f * 1.0f / (float)boxLen);
  // horizontals processing
  for (z = 0, zOff = 0; z < zDim; z++, zOff += xyDim)
  {
    for (y = 0, offLine = 0; y < yDim; y++, offLine += xDim)
    {
      MUint32 sum = 0;
      for (dr = -radius; dr <= +radius; dr++)
      {
        x = (dr >= 0) ? dr : 0;
        MUint32 val = (MUint32)pixels[zOff + offLine + x];
        sum += val;
        history[dr + radius] = val;
      }
      int indHistory = radius;
      for (x = 0; x < xDim; x++)
      {
        // (x * 170 / 512) is almost the same as (x / 3)
        pixels[zOff + offLine + x] = (MUint8)((sum * multFast) >> 9);
        // update sum
        int indMin = indHistory - radius;
        indMin = (indMin >= 0) ? indMin : (indMin + boxLen);

        sum -= history[indMin];

        // update central history index
        indHistory++;
        if (indHistory >= boxLen)
          indHistory = 0;

        // add new element to history and sum
        int indMax = indHistory + radius;
        indMax = (indMax < boxLen) ? indMax : (indMax - boxLen);
        int off = (x + radius + 1 < xDim) ? (x + radius + 1) : (xDim - 1);
        history[indMax] = (MUint32)pixels[zOff + offLine + off];
        sum += history[indMax];
      }     // for (x)
    }       // for (y)
  }         // for (z)

            // vertical processing
  for (z = 0, zOff = 0; z < zDim; z++, zOff += xyDim)
  {
    for (x = 0; x < xDim; x++)
    {
      MUint32 sum = 0;
      for (dr = -radius; dr <= +radius; dr++)
      {
        y = (dr >= 0) ? dr : 0;
        MUint32 val = (MUint32)pixels[x + y * xDim + zOff];
        sum += val;
        history[dr + radius] = val;
      }
      int indHistory = radius;
      for (y = 0, offLine = 0; y < yDim; y++, offLine += xDim)
      {
        // (x * 170 / 512) is almost the same as (x / 3)
        pixels[offLine + x + zOff] = (MUint8)((sum * multFast) >> 9);
        // update sum
        int indMin = indHistory - radius;
        indMin = (indMin >= 0) ? indMin : (indMin + boxLen);

        sum -= history[indMin];

        // update central history index
        indHistory++;
        if (indHistory >= boxLen)
          indHistory = 0;

        // add new element to history and sum
        int indMax = indHistory + radius;
        indMax = (indMax < boxLen) ? indMax : (indMax - boxLen);
        int yMax = (y + radius + 1 < yDim) ? (y + radius + 1) : (yDim - 1);
        history[indMax] = (MUint32)pixels[x + yMax * xDim + zOff];
        sum += history[indMax];
      }     // for (y)
    }       // for (x)
  }         // for (z)


  // depth (z) processing
  for (y = 0, offLine = 0; y < yDim; y++, offLine += xDim)
  {
    for (x = 0; x < xDim; x++)
    {
      MUint32 sum = 0;
      for (dr = -radius; dr <= +radius; dr++)
      {
        z = (dr >= 0) ? dr : 0;
        MUint32 val = (MUint32)pixels[x + offLine + z * xyDim];
        sum += val;
        history[dr + radius] = val;
      }
      int indHistory = radius;
      for (z = 0, zOff = 0; z < zDim; z++, zOff += xyDim)
      {
        // (x * 170 / 512) is almost the same as (x / 3)
        pixels[offLine + x + zOff] = (MUint8)((sum * multFast) >> 9);
        // update sum
        int indMin = indHistory - radius;
        indMin = (indMin >= 0) ? indMin : (indMin + boxLen);

        sum -= history[indMin];

        // update central history index
        indHistory++;
        if (indHistory >= boxLen)
          indHistory = 0;

        // add new element to history and sum
        int indMax = indHistory + radius;
        indMax = (indMax < boxLen) ? indMax : (indMax - boxLen);
        int zMax = (z + radius + 1 < zDim) ? (z + radius + 1) : (zDim - 1);
        history[indMax] = (MUint32)pixels[x + offLine + zMax * xyDim];
        sum += history[indMax];
      }     // for (z)
    }       // for (x)
  }         // for (y)
}

void  KtxTexture::boxFilter3d(const int radius)
{
  _boxFilter3d(m_data,
    m_header.m_pixelWidth,
    m_header.m_pixelHeight,
    m_header.m_pixelDepth,
    radius);
}

void  KtxTexture::clearBorder()
{
  int   x, y, z;

  int xDim = m_header.m_pixelWidth;
  int yDim = m_header.m_pixelHeight;
  int zDim = m_header.m_pixelDepth;
  int xyzDim = xDim * yDim * zDim;

  int numBlacks = 0;
  int numWhites = 0;
  for (x = 0; x < xyzDim; x++)
  {
    MUint32 val = (MUint32)m_data[x];
    if (val <= 32)
      numBlacks++;
    if (val >= 256 - 32)
      numWhites++;

  }
  MUint32 valBackground;
  if (numBlacks > numWhites)
    valBackground = 0;
  else
    valBackground = 255;


  int indDst = 0;
  for (z = 0; z < zDim; z++)
  {
    int isBorderZ = ((z == 0) || (z == zDim -1))? 1: 0;
    for (y = 0; y < yDim; y++)
    {
      int isBorderY = ((y == 0) || (y == yDim - 1)) ? 1: 0;
      for (x = 0; x < xDim; x++)
      {
        int isBorderX = ((x == 0) || (x == xDim - 1)) ? 1 : 0;
        int isBorder = isBorderX | isBorderY | isBorderZ;
        if (isBorder)
          m_data[indDst] = (MUint8)valBackground;
        indDst++;
      }   // for (x)
    }     // for (y)
  }       // for (z)
}

void  KtxTexture::binarizeByBarrier(
                                    const MUint8 valBarrier,
                                    const MUint8 valLess,
                                    const MUint8 valGreat
                                   )
{
  int   xyzDim = m_header.m_pixelWidth *
    m_header.m_pixelHeight *
    m_header.m_pixelDepth;
  int i;
  for (i = 0; i < xyzDim; i++)
  {
    MUint8 val = m_data[i];
    m_data[i] = (val <= valBarrier)? valLess: valGreat;
  }
}

static int _getLargeEqualPowerOfTwo(const int val)
{
  int i;
  for (i = 1; i < 30; i++)
  {
    int pwr = 1 << i;
    if (val <= pwr)
      return pwr;
  }
  return -1;
}

static __inline float _cubicHermite(
                                    const float aIn,
                                    const float bIn,
                                    const float cIn,
                                    const float dIn,
                                    const float t
                                   )
{
  float a = -aIn / 2.0f + (3.0f * bIn) / 2.0f - (3.0f * cIn) / 2.0f +
    dIn / 2.0f;
  float b = +aIn - (5.0f * bIn) / 2.0f + 2.0f * cIn - dIn / 2.0f;
  float c = -aIn / 2.0f + cIn / 2.0f;
  float d = bIn;
  return a * t * t * t + b * t * t + c * t + d;
}

static __inline float _getIntensityClamped(
                                            const MUint8   *volData,
                                            const int       xDimSrc,
                                            const int       yDimSrc,
                                            const int       zDimSrc,
                                            const int       x,
                                            const int       y,
                                            const int       z
                                          )
{
  int   xx = (x < 0) ? 0 : ((x >= xDimSrc) ? (xDimSrc - 1) : x);
  int   yy = (y < 0) ? 0 : ((y >= yDimSrc) ? (yDimSrc - 1) : y);
  int   zz = (z < 0) ? 0 : ((z >= zDimSrc) ? (zDimSrc - 1) : z);
  return (float)volData[xx + yy * xDimSrc + zz * xDimSrc * yDimSrc];
}


static void _getBicubicIntensity(
                                  const MUint8   *volIntensity,
                                  const int       xDimSrc,
                                  const int       yDimSrc,
                                  const int       zDimSrc,
                                  const float     tx,
                                  const float     ty,
                                  const float     tz,
                                  MUint8         *valIntensity
                                )
{
  float     x, y, z, xFract, yFract, zFract;
  int       xInt, yInt, zInt;

  x = (tx * xDimSrc) - 0.5f;
  y = (ty * yDimSrc) - 0.5f;
  z = (tz * zDimSrc) - 0.5f;

  xInt = (int)x;
  yInt = (int)y;
  zInt = (int)z;

  xFract = x - floorf(x);       // in [0..1]
  yFract = y - floorf(y);
  zFract = z - floorf(z);

  float p000 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt - 1, zInt - 1);
  float p100 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt - 1, zInt - 1);
  float p200 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt - 1, zInt - 1);
  float p300 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt - 1, zInt - 1);

  float p010 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 0, zInt - 1);
  float p110 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 0, zInt - 1);
  float p210 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 0, zInt - 1);
  float p310 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 0, zInt - 1);

  float p020 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 1, zInt - 1);
  float p120 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 1, zInt - 1);
  float p220 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 1, zInt - 1);
  float p320 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 1, zInt - 1);

  float p030 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 2, zInt - 1);
  float p130 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 2, zInt - 1);
  float p230 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 2, zInt - 1);
  float p330 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 2, zInt - 1);


  float p001 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt - 1, zInt + 0);
  float p101 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt - 1, zInt + 0);
  float p201 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt - 1, zInt + 0);
  float p301 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt - 1, zInt + 0);

  float p011 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 0, zInt + 0);
  float p111 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 0, zInt + 0);
  float p211 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 0, zInt + 0);
  float p311 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 0, zInt + 0);

  float p021 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 1, zInt + 0);
  float p121 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 1, zInt + 0);
  float p221 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 1, zInt + 0);
  float p321 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 1, zInt + 0);

  float p031 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 2, zInt + 0);
  float p131 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 2, zInt + 0);
  float p231 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 2, zInt + 0);
  float p331 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 2, zInt + 0);

  float p002 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt - 1, zInt + 1);
  float p102 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt - 1, zInt + 1);
  float p202 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt - 1, zInt + 1);
  float p302 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt - 1, zInt + 1);

  float p012 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 0, zInt + 1);
  float p112 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 0, zInt + 1);
  float p212 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 0, zInt + 1);
  float p312 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 0, zInt + 1);

  float p022 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 1, zInt + 1);
  float p122 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 1, zInt + 1);
  float p222 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 1, zInt + 1);
  float p322 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 1, zInt + 1);

  float p032 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 2, zInt + 1);
  float p132 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 2, zInt + 1);
  float p232 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 2, zInt + 1);
  float p332 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 2, zInt + 1);

  float p003 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt - 1, zInt + 2);
  float p103 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt - 1, zInt + 2);
  float p203 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt - 1, zInt + 2);
  float p303 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt - 1, zInt + 2);

  float p013 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 0, zInt + 2);
  float p113 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 0, zInt + 2);
  float p213 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 0, zInt + 2);
  float p313 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 0, zInt + 2);

  float p023 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 1, zInt + 2);
  float p123 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 1, zInt + 2);
  float p223 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 1, zInt + 2);
  float p323 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 1, zInt + 2);

  float p033 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt - 1, yInt + 2, zInt + 2);
  float p133 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 0, yInt + 2, zInt + 2);
  float p233 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 1, yInt + 2, zInt + 2);
  float p333 = _getIntensityClamped(volIntensity, xDimSrc, yDimSrc, zDimSrc, xInt + 2, yInt + 2, zInt + 2);


  float col0, col1, col2, col3, row0, row1, row2, row3, res;

  col0 = _cubicHermite(p000, p100, p200, p300, xFract);
  col1 = _cubicHermite(p010, p110, p210, p310, xFract);
  col2 = _cubicHermite(p020, p120, p220, p320, xFract);
  col3 = _cubicHermite(p030, p130, p230, p330, xFract);
  row0 = _cubicHermite(col0, col1, col2, col3, yFract);

  col0 = _cubicHermite(p001, p101, p201, p301, xFract);
  col1 = _cubicHermite(p011, p111, p211, p311, xFract);
  col2 = _cubicHermite(p021, p121, p221, p321, xFract);
  col3 = _cubicHermite(p031, p131, p231, p331, xFract);
  row1 = _cubicHermite(col0, col1, col2, col3, yFract);

  col0 = _cubicHermite(p002, p102, p202, p302, xFract);
  col1 = _cubicHermite(p012, p112, p212, p312, xFract);
  col2 = _cubicHermite(p022, p122, p222, p322, xFract);
  col3 = _cubicHermite(p032, p132, p232, p332, xFract);
  row2 = _cubicHermite(col0, col1, col2, col3, yFract);

  col0 = _cubicHermite(p003, p103, p203, p303, xFract);
  col1 = _cubicHermite(p013, p113, p213, p313, xFract);
  col2 = _cubicHermite(p023, p123, p223, p323, xFract);
  col3 = _cubicHermite(p033, p133, p233, p333, xFract);
  row3 = _cubicHermite(col0, col1, col2, col3, yFract);

  res = _cubicHermite(row0, row1, row2, row3, zFract);
  res = (res < 0.0f) ? 0.0f : ((res > 255.0f) ? 255.0f : res);

  *valIntensity = (MUint8)res;
}

static void _scaleDownTextureSmooth(
                              const MUint8 *pixelsSrc,
                              const int xDimSrc, const int yDimSrc, const int zDimSrc,
                              MUint8 *pixelsDst,
                              const int xDimDst, const int yDimDst, const int zDimDst
                            )
{
  int x, y, z;

  float xStep = 1.0f / xDimDst;
  float yStep = 1.0f / yDimDst;
  float zStep = 1.0f / zDimDst;
  int indDst = 0;
  for (z = 0; z < zDimDst; z++)
  {
    float tz = z * zStep;
    for (y = 0; y < yDimDst; y++)
    {
      float ty = y * yStep;
      for (x = 0; x < xDimDst; x++)
      {
        float tx = x * xStep;
        MUint8 val;
        _getBicubicIntensity(pixelsSrc, xDimSrc, yDimSrc, zDimSrc, tx, ty, tz, &val);
        pixelsDst[indDst++] = val;
      }
    }
  }
}

void  _scaleUpTextureSmooth(
                        const MUint8    *volTextureSrc,
                        const int       xDimSrc,
                        const int       yDimSrc,
                        const int       zDimSrc,
                        MUint8          *volTextureDst,
                        const int       xDimDst,
                        const int       yDimDst,
                        const int       zDimDst
                      )
{
  int   offDst, x, y, z;

  assert( (xDimSrc < xDimDst) || (yDimSrc < yDimDst) || (zDimSrc < zDimDst));

  offDst = 0;
  for (z = 0; z < zDimDst; z++)
  {
    float tz = (float)z / zDimDst;
    for (y = 0; y < yDimDst; y++)
    {
      float ty = (float)y / yDimDst;
      for (x = 0; x < xDimDst; x++)
      {
        float tx = (float)x / xDimDst;
        MUint8 val;
        _getBicubicIntensity(volTextureSrc, xDimSrc, yDimSrc, zDimSrc, tx, ty, tz, &val);
        volTextureDst[offDst] = val;
        // next voxel dst
        offDst++;
      }     // for (x)
    }       // for (y)
  }         // for (z)
}

static void _scaleDownTextureRough(
                                    const MUint8 *pixelsSrc,
                                    const int xDimSrc, const int yDimSrc, const int zDimSrc,
                                    MUint8 *pixelsDst,
                                    const int xDimDst, const int yDimDst, const int zDimDst
                                  )
{
  int x, y, z;

  assert(xDimDst <= xDimSrc);
  assert(yDimDst <= yDimSrc);
  assert(zDimDst <= zDimSrc);

  float xScale = (float)xDimSrc / xDimDst;
  float yScale = (float)yDimSrc / yDimDst;
  float zScale = (float)zDimSrc / zDimDst;

  int indDst = 0;
  for (z = 0; z < zDimDst; z++)
  {
    int zSrcMin = (int)((z + 0) * zScale);
    int zSrcMax = (int)((z + 1) * zScale);
    for (y = 0; y < yDimDst; y++)
    {
      int ySrcMin = (int)((y + 0) * yScale);
      int ySrcMax = (int)((y + 1) * yScale);
      for (x = 0; x < xDimDst; x++)
      {
        int xSrcMin = (int)((x + 0) * xScale);
        int xSrcMax = (int)((x + 1) * xScale);

        int xx, yy, zz;
        MUint32 sum = 0;
        int numPixels = 0;
        for (zz = zSrcMin; zz < zSrcMax; zz++)
        {
          int zSrcOff = zz * xDimSrc * yDimSrc;
          for (yy = ySrcMin; yy < ySrcMax; yy++)
          {
            int ySrcOff = yy * xDimSrc;
            for (xx = xSrcMin; xx < xSrcMax; xx++)
            {
              int offSrc = xx + ySrcOff + zSrcOff;
              MUint32 valSrc = (MUint32)pixelsSrc[offSrc];
              if (valSrc == 0)
                continue;
              sum += valSrc;
              numPixels++;
            }
          }
        }
        MUint8 val = 0;
        if (numPixels > 0)
          val = (MUint8)(sum / (MUint32)numPixels);
        pixelsDst[indDst++] = val;
      }
    }
  }
}

static void _scaleUpTextureRough(
                                  const MUint8 *pixelsSrc,
                                  const int xDimSrc, const int yDimSrc, const int zDimSrc,
                                  MUint8 *pixelsDst,
                                  const int xDimDst, const int yDimDst, const int zDimDst
                                )
{
  int x, y, z;

  assert(xDimDst >= xDimSrc);
  assert(yDimDst >= yDimSrc);
  assert(zDimDst >= zDimSrc);

  float xScale = (float)xDimSrc / xDimDst;
  float yScale = (float)yDimSrc / yDimDst;
  float zScale = (float)zDimSrc / zDimDst;

  int indDst = 0;
  for (z = 0; z < zDimDst; z++)
  {
    int zSrc = (int)((z + 0) * zScale);
    int zOffSrc = zSrc * xDimSrc * yDimSrc;
    for (y = 0; y < yDimDst; y++)
    {
      int ySrc = (int)((y + 0) * yScale);
      int yOffSrc = ySrc * xDimSrc;
      for (x = 0; x < xDimDst; x++)
      {
        int xSrc = (int)((x + 0) * xScale);
        int offSrc = xSrc + yOffSrc + zOffSrc;
        MUint8 val = pixelsSrc[offSrc];
        pixelsDst[indDst++] = val;
      }
    }
  }
}


KtxError  KtxTexture::createAs1BytePowerByMaxSide(
                                                    const KtxTexture   *tex,
                                                    const int           maxDstDim,
                                                    const int           isDstCubeTexture,
                                                    const int           useSmoothInterpolation
                                                )
{
  int xDimSrc = tex->getWidth();
  int yDimSrc = tex->getHeight();
  int zDimSrc = tex->getDepth();

  int maxSrcDim = xDimSrc;
  if (yDimSrc > maxSrcDim)
    maxSrcDim = yDimSrc;
  if (zDimSrc > maxSrcDim)
    maxSrcDim = zDimSrc;
  
  float xKoef = (float)xDimSrc / maxSrcDim;
  float yKoef = (float)yDimSrc / maxSrcDim;
  float zKoef = (float)zDimSrc / maxSrcDim;

  int xDimDst = (int)(maxDstDim * xKoef);
  int yDimDst = (int)(maxDstDim * yKoef);
  int zDimDst = (int)(maxDstDim * zKoef);

  if (isDstCubeTexture)
    yDimDst = zDimDst = xDimDst;

  MUint8 *pixelsSmall = M_NEW(MUint8[xDimDst * yDimDst * zDimDst]);
  if (pixelsSmall == NULL)
    return KTX_ERROR_NO_MEMORY;

  const MUint8 *pixelsSrc = tex->getData();
  if (
        (xDimSrc >= xDimDst) && 
        (yDimSrc >= yDimDst) &&
        (zDimSrc >= zDimDst) 
    )
  {
    if (useSmoothInterpolation)
      _scaleDownTextureSmooth(
                        pixelsSrc,
                        xDimSrc, yDimSrc, zDimSrc,
                        pixelsSmall,
                        xDimDst, yDimDst, zDimDst
                      );
    else
      _scaleDownTextureRough(
                        pixelsSrc,
                        xDimSrc, yDimSrc, zDimSrc,
                        pixelsSmall,
                        xDimDst, yDimDst, zDimDst
                      );
  }
  else
  {
    if (useSmoothInterpolation)
      _scaleUpTextureSmooth(
                            pixelsSrc,
                            xDimSrc, yDimSrc, zDimSrc,
                            pixelsSmall,
                            xDimDst, yDimDst, zDimDst
                          );
    else
      _scaleUpTextureRough(
                            pixelsSrc,
                            xDimSrc, yDimSrc, zDimSrc,
                            pixelsSmall,
                            xDimDst, yDimDst, zDimDst
                          );
  }

  int xDimFinal = _getLargeEqualPowerOfTwo(xDimDst);
  int yDimFinal = _getLargeEqualPowerOfTwo(yDimDst);
  int zDimFinal = _getLargeEqualPowerOfTwo(zDimDst);

  MUint8 *pixelsFinal = M_NEW(MUint8[xDimFinal * yDimFinal* zDimFinal]);
  if (pixelsFinal == NULL)
    return KTX_ERROR_NO_MEMORY;
  _copyAndAlignTexture(
                        pixelsSmall,
                        xDimDst, yDimDst, zDimDst,
                        pixelsFinal,
                        xDimFinal, yDimFinal, zDimFinal
                      );
  delete[] pixelsSmall;

  // create 1 byte per pixel texture from 1/3/4 bpp texture
  memcpy(&m_header.m_id, &tex->m_header, sizeof(KtxHeader));
  // Make texture dimension as power of two.
  m_header.m_pixelWidth   = xDimFinal;
  m_header.m_pixelHeight  = yDimFinal;
  m_header.m_pixelDepth   = zDimFinal;

  // copy user key data
  memcpy(&m_keyData, &tex->m_keyData, sizeof(KtxKeyData));

  // Created 1 byte per pixel texture
  m_header.m_glFormat = KTX_GL_RED;
  m_header.m_glInternalFormat = KTX_GL_R8_EXT;        // GL_R8_EXT, GL_R8 (0x8229)
  m_header.m_glBaseInternalFormat = KTX_GL_RED;


  int sizeVolume = m_header.m_pixelWidth;
  if (m_header.m_pixelHeight > 0)
    sizeVolume *= m_header.m_pixelHeight;
  if (m_header.m_pixelDepth > 0)
    sizeVolume *= m_header.m_pixelDepth;

  m_data = pixelsFinal;
  m_isCompressed = 0;
  m_dataSize = sizeVolume;

  return KTX_ERROR_OK;
}

KtxError  KtxTexture::createAs1ByteLessSize(
                                            const KtxTexture *tex,
                                            const int xDimDst,
                                            const int yDimDst,
                                            const int zDimDst
                                          )
{
  int   sizeVolume;

  // create 1 byte per pixel texture from 1/3/4 bpp texture
  memcpy(&m_header.m_id, &tex->m_header, sizeof(KtxHeader));
  // Make texture dimension as power of two.
  m_header.m_pixelWidth = xDimDst;
  m_header.m_pixelHeight = yDimDst;
  m_header.m_pixelDepth = zDimDst;

  // copy user key data
  memcpy(&m_keyData, &tex->m_keyData, sizeof(KtxKeyData));

  // Created 1 byte per pixel texture
  m_header.m_glFormat = KTX_GL_RED;
  m_header.m_glInternalFormat = KTX_GL_R8_EXT;        // GL_R8_EXT, GL_R8 (0x8229)
  m_header.m_glBaseInternalFormat = KTX_GL_RED;

  sizeVolume = m_header.m_pixelWidth;
  if (m_header.m_pixelHeight > 0)
    sizeVolume *= m_header.m_pixelHeight;
  if (m_header.m_pixelDepth > 0)
    sizeVolume *= m_header.m_pixelDepth;

  if (m_data != NULL)
    delete[] m_data;

  m_data = M_NEW(MUint8[sizeVolume]);
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  m_isCompressed = 0;
  m_dataSize = sizeVolume;

  // Convert 1 -> 1
  assert(tex->m_header.m_glFormat == KTX_GL_RED);
  //memcpy(m_data, tex->m_data, sizeVolume);

  int xDimSrc = tex->m_header.m_pixelWidth;
  int yDimSrc = tex->m_header.m_pixelHeight;
  int zDimSrc = tex->m_header.m_pixelDepth;

  const MUint8 *pixelsSrc = tex->getData();
  MUint8 *pixelsDst = getData();

  _scaleDownTextureSmooth(
                          pixelsSrc,
                          xDimSrc, yDimSrc, zDimSrc,
                          pixelsDst,
                          xDimDst, yDimDst, zDimDst
                        );

  return KTX_ERROR_OK;
}

KtxError  KtxTexture::createAsSingleSphere(const int dim)
{
  int   sizeVolume;

  int xDim = dim;
  int yDim = dim;
  int zDim = dim;

  // Make texture dimension as power of two.
  memcpy(m_header.m_id, s_ktxIdFile, sizeof(s_ktxIdFile));
  m_header.m_endianness = 0x04030201;
  m_header.m_glType = KTX_GL_UNSIGNED_BYTE;
  m_header.m_glTypeSize = 1;

  m_header.m_pixelWidth = xDim;
  m_header.m_pixelHeight = yDim;
  m_header.m_pixelDepth = zDim;

  m_header.m_numberOfArrayElements = 0;  // for non-array textures is 0.
  m_header.m_numberOfFaces = 1;
  m_header.m_numberOfMipmapLevels = 1;
  //m_header.m_bytesOfKeyValueData   = sizeof(s_keyValues);
  m_header.m_bytesOfKeyValueData = 0;

  m_header.m_glFormat = KTX_GL_RED;
  m_header.m_glInternalFormat = KTX_GL_RED;  // GL_R8_EXT, GL_R8 (0x8229)
  m_header.m_glBaseInternalFormat = KTX_GL_RED;

  // copy user key data
  char *keyDataPtr =  (char*)&m_keyData;  // for happy cppcheck
  int memSize = sizeof(KtxKeyData);       // for happy cppcheck
  memset(keyDataPtr, 0, memSize);

  sizeVolume = m_header.m_pixelWidth;
  if (m_header.m_pixelHeight > 0)
    sizeVolume *= m_header.m_pixelHeight;
  if (m_header.m_pixelDepth > 0)
    sizeVolume *= m_header.m_pixelDepth;

  if (m_data != NULL)
    delete[] m_data;

  m_data = M_NEW(MUint8[sizeVolume]);
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  m_isCompressed = 0;
  m_dataSize = sizeVolume;

  int xCenter = xDim >> 1;
  int yCenter = yDim >> 1;
  int zCenter = zDim >> 1;

  const float KOEF_ROUGH = 0.05f;

  int x, y, z;
  int indDst = 0;
  float radius = (float)(dim / 2);
  for (z = 0; z < zDim; z++)
  {
    int dz = z - zCenter;
    for (y = 0; y < yDim; y++)
    {
      int dy = y - yCenter;
      for (x = 0; x < xDim; x++)
      {
        int dx = x - xCenter;
        float r = sqrtf( (float)dx * dx + dy * dy + dz * dz) + 0.001f;
        //float fVal = KOEF_ROUGH *  256.0f * (radius / r);
        float fVal = (radius - r) * 256.0f * KOEF_ROUGH;
        int val = (int)fVal;
        if (val > 255)
          val = 255;
        if (val < 0)
          val = 0;
        m_data[indDst++] = (MUint8)(val);
      }     // for (x)
    }       // for (y)
  }         // for (z)
  return KTX_ERROR_OK;
}


KtxError  KtxTexture::createMinSizeTexture(const KtxTexture *tex, const int indexChannel)
{
  //
  // Store original texture size
  // and top left corner offset
  // in the user data area
  //

  int xDimSrc = tex->getWidth();
  int yDimSrc = tex->getHeight();
  int zDimSrc = tex->getDepth();
  MUint32 format = tex->getGlFormat();
  int bpp = (format == KTX_GL_RED)? 1: ((format == KTX_GL_RGB)? 3: 4);
  if (bpp != 4)
  {
    return KTX_ERROR_WRONG_FORMAT;
  }

  // bbox
  V3d    vBoxMin, vBoxMax;

  vBoxMin.x = vBoxMin.y = vBoxMin.z = 1<<24;
  vBoxMax.x = vBoxMax.y = vBoxMax.z = 0;

  int bitShift = indexChannel * 8;
  int x, y, z;
  const MUint32 *pixelsSrc = (MUint32*)tex->getData();
  for (z = 0; z < zDimSrc; z++)
  {
    for (y = 0; y < yDimSrc; y++)
    {
      for (x = 0; x < xDimSrc; x++)
      {
        MUint32 val = *pixelsSrc;
        // check G component
        if ((val >> bitShift) & 0xff)
        {
          if (x < vBoxMin.x)
            vBoxMin.x = x;
          if (y < vBoxMin.y)
            vBoxMin.y = y;
          if (z < vBoxMin.z)
            vBoxMin.z = z;

          if (x > vBoxMax.x)
            vBoxMax.x = x;
          if (y > vBoxMax.y)
            vBoxMax.y = y;
          if (z > vBoxMax.z)
            vBoxMax.z = z;
        }

        pixelsSrc++;
      }     // for (x)
    }       // for (y)
  }         // for (z)

  int xDimDst = vBoxMax.x - vBoxMin.x + 1;
  int yDimDst = vBoxMax.y - vBoxMin.y + 1;
  int zDimDst = vBoxMax.z - vBoxMin.z + 1;


  memcpy(&m_header.m_id, &tex->m_header, sizeof(KtxHeader));
  m_header.m_pixelWidth   = xDimDst;
  m_header.m_pixelHeight  = yDimDst;
  m_header.m_pixelDepth   = zDimDst;

  int sizeVolume = xDimDst * yDimDst * zDimDst;
  sizeVolume *= bpp;

  if (m_data != NULL)
    delete[] m_data;

  m_data = M_NEW(MUint8[sizeVolume]);
  if (!m_data)
    return KTX_ERROR_NO_MEMORY;
  m_isCompressed = 0;
  m_dataSize = sizeVolume;

  // Copy pixels
  MUint32 *pixelsDst = (MUint32*)m_data;
  pixelsSrc = (MUint32*)tex->getData();

  for (z = 0; z < zDimDst; z++)
  {
    int zSrc = z + vBoxMin.z;
    int zSrcOff = zSrc * xDimSrc * yDimSrc;
    for (y = 0; y < yDimDst; y++)
    {
      int ySrc = y + vBoxMin.y;
      int ySrcOff = ySrc * xDimSrc;
      for (x = 0; x < xDimDst; x++)
      {
        int xSrc = x + vBoxMin.x;

        int off = xSrc + ySrcOff + zSrcOff;
        *pixelsDst = pixelsSrc[off];
        pixelsDst++;
      }     // for (x)
    }       // for (y)
  }         // for (z)

  // Setup user data
  V3d vSize;
  vSize.x = xDimSrc;
  vSize.y = yDimSrc;
  vSize.z = zDimSrc;

  setKeyDataMinSize(vBoxMin, vSize);

  return KTX_ERROR_OK;
}


void KtxTexture::setKeyDataBbox(const V3f &vMin, const V3f &vMax)
{
  m_keyData.m_dataType = KTX_KEY_DATA_BBOX;
  V3f *dst = reinterpret_cast<V3f*>(m_keyData.m_buffer);
  dst[0] = vMin;
  dst[1] = vMax;
}

void KtxTexture::setKeyDataMinSize(const V3d &vMin, const V3d &vSize)
{
  m_keyData.m_dataType = KTX_KEY_DATA_MIN_SIZE;
  V3d *dst = reinterpret_cast<V3d*>(m_keyData.m_buffer);
  dst[0] = vMin;
  dst[1] = vSize;
}

void  KtxTexture::fillValZGreater(const int zTop, const MUint8 valToFill)
{
  const int xDim = getWidth();
  const int yDim = getHeight();
  const int xyDim = xDim * yDim;
  const int zDim = getDepth();
  int z;
  for (z = zTop; z < zDim; z++)
  {
    int zOff = z * xDim * yDim;
    for (int i = 0; i < xyDim; i++)
    {
      m_data[zOff + i] = valToFill;
    }
  }
}

void  KtxTexture::fillZeroYGreater(const int yClipMax)
{
  // only 1 byte texture are suppoirted for this operation now
  assert(getGlFormat() == KTX_GL_RED);
  int xDim = getWidth();
  int yDim = getHeight();
  int zDim = getDepth();
  assert(yClipMax < yDim);
  for (int z = 0; z < zDim; z++)
  {
    int zOff = z * xDim * yDim;
    for (int y = 0; y < yDim; y++)
    {
      int yOff = y * xDim;
      for (int x = 0; x < xDim; x++)
      {
        int off = x + yOff + zOff;
        if (y >= yClipMax)
          m_data[off] = 0;
      }   // for (x)
    }     // for (y)
  }       // for (z)
}

void  KtxTexture::gaussSmooth(const int gaussNeigh, const float gaussSigma)
{
  int xDim          = getWidth();
  int yDim          = getHeight();
  int zDim          = getDepth();
  int xyzDim        = xDim * yDim * zDim;
  MUint8 *pixelsDst = M_NEW(MUint8[xyzDim]);
  memset(pixelsDst, 0, xyzDim);

  const   float gaussKoef = 1.0f / (2.0f * gaussSigma * gaussSigma);

  const   int   MAX_NEIGHS = 2 * 6 + 1;
  static  float koefs[MAX_NEIGHS * MAX_NEIGHS * MAX_NEIGHS];

  int     dx, dy, dz;
  int     offKoef = 0;
  float   wSum    = 0.0f;
  for (dz = -gaussNeigh; dz <= +gaussNeigh; dz++)
  {
    float kz = (float)dz / (float)gaussNeigh;
    for (dy = -gaussNeigh; dy <= +gaussNeigh; dy++)
    {
      float ky = (float)dy / (float)gaussNeigh;
      for (dx = -gaussNeigh; dx <= +gaussNeigh; dx++)
      {
        float kx = (float)dx / (float)gaussNeigh;
        float w = expf(-(kx * kx + ky * ky + kz * kz) * gaussKoef);
        koefs[offKoef ++] = w;
        wSum += w;
      }   // for (dx)
    }     // for (dy)
  }       // for (dz)
  // normalize koefs
  int numElemsKoefs = 2 * gaussNeigh + 1;
  numElemsKoefs = numElemsKoefs * numElemsKoefs * numElemsKoefs;
  assert(numElemsKoefs == offKoef);
  float scale = 1.0f / wSum;
  for (int i = 0; i < numElemsKoefs; i++)
  {
    koefs[i] = koefs[i] * scale;
  } // for (i)

  int cx, cy, cz;
  for (cz = gaussNeigh; cz < zDim - gaussNeigh; cz++)
  {
    int zOff = cz * xDim * yDim;
    for (cy = gaussNeigh; cy < yDim - gaussNeigh; cy++)
    {
      int yOff = cy * xDim;
      for (cx = gaussNeigh; cx < xDim - gaussNeigh; cx++)
      {
        int off = zOff + yOff + cx;
        float valSum = 0.0f;

        offKoef = 0;
        for (dz = -gaussNeigh; dz <= +gaussNeigh; dz++)
        {
          int dzOff = dz * xDim * yDim;
          for (dy = -gaussNeigh; dy <= +gaussNeigh; dy++)
          {
            int dyOff = dy * xDim;
            for (dx = -gaussNeigh; dx <= +gaussNeigh; dx++)
            {
              float w = koefs[offKoef ++];
              valSum += w * (float)m_data[off + dx + dyOff + dzOff];
            }   // for (dx)
          }     // for (dy)
        }       // for (dz)
        valSum = (valSum <= 255.0f) ? valSum: 255.0f;
        MUint8 vSum = (MUint8)valSum;
        pixelsDst[off] = vSum;
      }   // for (cx)
    }     // for (cy)
  }       // for (cz)
  memcpy(m_data, pixelsDst, xyzDim);
  delete [] pixelsDst;
}

static int _scaleTextureDown(
                              const MUint8    *volTextureSrc,
                              const int       xDimSrc,
                              const int       yDimSrc,
                              const int       zDimSrc,
                              const int       xDimDst,
                              const int       yDimDst,
                              const int       zDimDst,
                              MUint8          *volTextureDst
                            )
{
  const int ACC_BITS = 10;
  const int ACC_HALF = 1 << (ACC_BITS - 1);
  int       xStep, yStep, zStep;
  int       indDst;
  int       xDst, yDst, zDst;
  int       zSrcAccL, zSrcAccH;
  int       x, y, z, zOff, yOff;
  int       xyDimSrc;

  assert(xDimSrc > xDimDst);
  assert(yDimSrc > yDimDst);
  assert(zDimSrc > zDimDst);

  xyDimSrc = xDimSrc * yDimSrc;

  xStep = (xDimSrc << ACC_BITS) / xDimDst;
  yStep = (yDimSrc << ACC_BITS) / yDimDst;
  zStep = (zDimSrc << ACC_BITS) / zDimDst;
  indDst = 0;
  zSrcAccL = ACC_HALF;
  zSrcAccH = zSrcAccL + zStep;
  for (zDst = 0; zDst < zDimDst; zDst++, zSrcAccL += zStep, zSrcAccH += zStep)
  {
    int zSrcL = zSrcAccL >> ACC_BITS;
    int zSrcH = zSrcAccH >> ACC_BITS;

    int ySrcAccL = ACC_HALF;
    int ySrcAccH = ySrcAccL + yStep;
    for (yDst = 0; yDst < yDimDst; yDst++, ySrcAccL += yStep, ySrcAccH += yStep)
    {
      int ySrcL = ySrcAccL >> ACC_BITS;
      int ySrcH = ySrcAccH >> ACC_BITS;

      int xSrcAccL = ACC_HALF;
      int xSrcAccH = xSrcAccL + xStep;
      for (xDst = 0; xDst < xDimDst; xDst++, xSrcAccL += xStep, xSrcAccH += xStep)
      {
        int xSrcL = xSrcAccL >> ACC_BITS;
        int xSrcH = xSrcAccH >> ACC_BITS;

        // Accumulate sum
        MUint32 sum = 0;
        int     numPixels = 0;
        for (z = zSrcL, zOff = zSrcL * xyDimSrc; z < zSrcH; z++, zOff += xyDimSrc)
        {
          for (y = ySrcL, yOff = ySrcL * xDimSrc; y < ySrcH; y++, yOff += xDimSrc)
          {
            for (x = xSrcL; x < xSrcH; x++)
            {
              int offSrc = x + yOff + zOff;
              sum += (MUint32)volTextureSrc[offSrc];
              numPixels++;
            }   // for (x)
          }     // for (y)
        }       // for (z)
        sum = sum / numPixels;
        volTextureDst[indDst++] = (MUint8)sum;
      }       // for (xDst)
    }         // for (yDst)
  }           // for (zDst)

  return 1;
}

int KtxTexture::scaleDownToSize(const int xDimDst, const int yDimDst, const int zDimDst)
{
  int xDimSrc = getWidth();
  int yDimSrc = getHeight();
  int zDimSrc = getDepth();
  int numPixelsDst = xDimDst * yDimDst * zDimDst;
  MUint8 *pixelsDst = M_NEW(MUint8[numPixelsDst]);
  if (!pixelsDst)
    return -1;
  _scaleTextureDown(m_data, xDimSrc, yDimSrc, zDimSrc, xDimDst, yDimDst, zDimDst, pixelsDst);
  delete [] m_data;
  m_data = pixelsDst;
  m_header.m_pixelWidth   = xDimDst;
  m_header.m_pixelHeight  = yDimDst;
  m_header.m_pixelDepth   = zDimDst;
  return 1;
}

int KtxTexture::convertTo4bpp()
{
  int numPixels = getWidth() * getHeight() * getDepth();
  MUint8 *dataNew = M_NEW(MUint8[numPixels * 4]);
  if (!dataNew)
    return -1;
  MUint32 *pixelsDst = (MUint32*)dataNew;
  for (int i = 0; i < numPixels; i++)
  {
    MUint32 val = (MUint32)m_data[i];
    val = val | (val << 8) | (val << 16) | (val << 24);
    pixelsDst[i] = val;
  } // for (i)
  delete [] m_data;
  m_data = dataNew;
  m_header.m_glFormat = KTX_GL_RGBA;
  m_header.m_glInternalFormat = KTX_GL_RGBA;
  m_header.m_glBaseInternalFormat = KTX_GL_RGBA;
  return 1;
}

// ******************************************************************
// Get symmetry
// ******************************************************************
int KtxTexture::getBoundingBox(V3d &vMin, V3d &vMax) const
{
  assert(m_header.m_glFormat == KTX_GL_RED);
  int x, y, z;

  const int xDim = getWidth();
  const int yDim = getHeight();
  const int zDim = getDepth();
  const int xDim2 = xDim / 2;
  const int yDim2 = yDim / 2;
  const int zDim2 = zDim / 2;
  const int VAL_BACK_BARRIER = 80;

  int isEdge;

  isEdge = 1;
  for (vMin.x = 1; (vMin.x < xDim2) && isEdge; vMin.x++)
  {
    for (y = 0; (y < yDim) && isEdge; y++)
    {
      for (z = 0; (z < zDim) && isEdge; z++)
      {
        int off = vMin.x + y * xDim + z * xDim * yDim;
        MUint8 val = m_data[off];
        if (val >= VAL_BACK_BARRIER)
          isEdge = 0;
      }
    }
  }
  isEdge = 1;
  for (vMax.x = xDim - 2; (vMax.x > xDim2) && isEdge; vMax.x--)
  {
    for (y = 0; (y < yDim) && isEdge; y++)
    {
      for (z = 0; (z < zDim) && isEdge; z++)
      {
        int off = vMax.x + y * xDim + z * xDim * yDim;
        MUint8 val = m_data[off];
        if (val >= VAL_BACK_BARRIER)
          isEdge = 0;
      }
    }
  }
  // on Y
  isEdge = 1;
  for (vMin.y = 1; (vMin.y < yDim2) && isEdge; vMin.y++)
  {
    for (x = 0; (x < xDim) && isEdge; x++)
    {
      for (z = 0; (z < zDim) && isEdge; z++)
      {
        int off = x + (vMin.y * xDim) + (z * xDim * yDim);
        MUint8 val = m_data[off];
        if (val >= VAL_BACK_BARRIER)
          isEdge = 0;
      }
    }
  }
  isEdge = 1;
  for (vMax.y = yDim - 2; (vMax.y > yDim2) && isEdge; vMax.y--)
  {
    for (x = 0; (x < xDim) && isEdge; x++)
    {
      for (z = 0; (z < zDim) && isEdge; z++)
      {
        int off = x + (vMax.y * xDim) + (z * xDim * yDim);
        MUint8 val = m_data[off];
        if (val >= VAL_BACK_BARRIER)
          isEdge = 0;
      }
    }
  }
  // on Z
  isEdge = 1;
  for (vMin.z = 1; (vMin.z < zDim2) && isEdge; vMin.z++)
  {
    for (x = 0; (x < xDim) && isEdge; x++)
    {
      for (y = 0; (y < yDim) && isEdge; y++)
      {
        int off = x + (y * xDim) + (vMin.z * xDim * yDim);
        MUint8 val = m_data[off];
        if (val >= VAL_BACK_BARRIER)
          isEdge = 0;
      }
    }
  }
  for (vMax.z = zDim - 2; (vMax.z > zDim2) && isEdge; vMax.z--)
  {
    for (x = 0; (x < xDim) && isEdge; x++)
    {
      for (y = 0; (y < yDim) && isEdge; y++)
      {
        int off = x + (y * xDim) + (vMax.z * xDim * yDim);
        MUint8 val = m_data[off];
        if (val >= VAL_BACK_BARRIER)
          isEdge = 0;
      }
    }
  }
  return 1;
}

int KtxTexture::getHorSliceSymmetry(const int z, float &xSym, float &ySym)
{
  const int xDim = getWidth();
  const int yDim = getHeight();
  int offSlice = z * xDim * yDim;

  V3d vMin, vMax;
  getBoundingBox(vMin, vMax);
  V2d vCenter, vRange;
  vCenter.x = (vMin.x + vMax.x) / 2;
  vCenter.y = (vMin.y + vMax.y) / 2;
  vRange.x = (vMax.x - vMin.x) / 2 - 2;
  vRange.y = (vMax.y - vMin.y) / 2 - 2;

  float dif;
  int numPixels;
  int i, x, y;

  dif = 0.0f;
  numPixels = 0;

  for (y = vMin.y; y < vMax.y; y++)
  {
    for (i = 4; i < vRange.x; i++)
    {
      int xL, xR;
      MUint32 valL, valR;
      float   fValL, fValR, d, dBest;
      dBest = +1.0e7f;

      // off 0
      xL = vCenter.x - i;
      xR = vCenter.x + i;
      valL = (MUint32)m_data[offSlice + y * xDim + xL];
      valR = (MUint32)m_data[offSlice + y * xDim + xR];
      fValL = (float)valL;
      fValR = (float)valR;
      d = fValR - fValL;
      d = (d >= 0.0f) ? d: -d;
      dBest = (d < dBest) ? d: dBest;

      // off +1
      xL = vCenter.x + 1 - i;
      xR = vCenter.x + 1 + i;
      valL = (MUint32)m_data[offSlice + y * xDim + xL];
      valR = (MUint32)m_data[offSlice + y * xDim + xR];
      fValL = (float)valL;
      fValR = (float)valR;
      d = fValR - fValL;
      d = (d >= 0.0f) ? d : -d;
      dBest = (d < dBest) ? d : dBest;

      // off -1
      xL = vCenter.x - 1 - i;
      xR = vCenter.x - 1 + i;
      valL = (MUint32)m_data[offSlice + y * xDim + xL];
      valR = (MUint32)m_data[offSlice + y * xDim + xR];
      fValL = (float)valL;
      fValR = (float)valR;
      d = fValR - fValL;
      d = (d >= 0.0f) ? d : -d;
      dBest = (d < dBest) ? d : dBest;
      if (dBest > 20.0f)
      dBest += 0.0000001f;

      dif += dBest;
      numPixels++;
    }   // for (x)
  }     // for (y)
  dif /= (float)numPixels;
  xSym = dif;

  dif = 0.0f;
  numPixels = 0;

  for (x = vMin.x; x < vMax.x; x++)
  {
    for (y = vMin.y; y < vCenter.y; y++)
    for (i = 4; i < vRange.x; i++)
    {
      int yL, yR;
      MUint32 valL, valR;
      float fValL, fValR, d, dBest;
      dBest = +1.0e4f;

      // off 0
      yL = vCenter.y - i;
      yR = vCenter.y + i;
      valL = (MUint32)m_data[offSlice + yL * xDim + x];
      valR = (MUint32)m_data[offSlice + yR * xDim + x];
      fValL = (float)valL;
      fValR = (float)valR;
      d = fValR - fValL;
      d = (d >= 0.0f) ? d : -d;
      dBest = (d < dBest) ? d : dBest;

      // off +1
      yL = vCenter.y + 1 - i;
      yR = vCenter.y + 1 + i;
      valL = (MUint32)m_data[offSlice + yL * xDim + x];
      valR = (MUint32)m_data[offSlice + yR * xDim + x];
      fValL = (float)valL;
      fValR = (float)valR;
      d = fValR - fValL;
      d = (d >= 0.0f) ? d : -d;
      dBest = (d < dBest) ? d : dBest;

      // off -1
      yL = vCenter.y - 1 - i;
      yR = vCenter.y - 1 + i;
      valL = (MUint32)m_data[offSlice + yL * xDim + x];
      valR = (MUint32)m_data[offSlice + yR * xDim + x];
      fValL = (float)valL;
      fValR = (float)valR;
      d = fValR - fValL;
      d = (d >= 0.0f) ? d : -d;
      dBest = (d < dBest) ? d : dBest;


      dif += dBest;
      numPixels++;
    }   // for (x)
  }     // for (y)
  dif /= (float)numPixels;
  ySym = dif;
  return 1;
}

int KtxTexture::scaleUpZ(const int zNew)
{
  const int xDim = getWidth();
  const int yDim = getHeight();
  const int xyDim = xDim * yDim;
  const int zDim = getDepth();
  assert(zNew > (int)zDim);
  const int divRest = zNew % zDim;
  assert(divRest == 0);

  MUint8 *pixelsNew = M_NEW(MUint8[xDim * yDim * zNew]);
  if (!pixelsNew)
    return -1;
  int iDst = 0;
  int zDst;
  for (zDst = 0; zDst < zNew; zDst++)
  {
    int zSrc    = zDim * zDst / zNew;
    int zRatio  = (zDim * zDst) % zNew;
    zRatio      = 1024 * zRatio / zNew;
    int zNext   = zSrc + 1;
    if (zNext >= zDim)
      zNext = zSrc;
    const int zOff = zSrc * xyDim;
    const int zOffNext = zNext * xyDim;
    for (int i = 0; i < xyDim; i++)
    {
      // mix zSrc and zNext layers ith zRatio
      MUint32 valSrc   = (MUint32)m_data[zOff + i];
      MUint32 valNext  = (MUint32)m_data[zOffNext + i];

      MUint32 valNew = (valSrc * (1024 - zRatio) + valNext * zRatio) >> 10;
      pixelsNew[iDst++] = (MUint8)valNew;
    }   // for (i) all in xyDim
  }     // for (z)
  delete [] m_data;
  m_data = pixelsNew;
  setDepth(zNew);
  return 1;
}

int   KtxTexture::rescale(const int xNew, const int yNew, const int zNew)
{
  const int xDim = getWidth();
  const int yDim = getHeight();
  const int xyDim = xDim * yDim;
  const int zDim = getDepth();

  MUint8 *pixelsNew = M_NEW(MUint8[xNew * yNew * zNew]);
  if (!pixelsNew)
    return -1;

  int offDst = 0;
  int xDst, yDst, zDst;
  for (zDst = 0; zDst < zNew; zDst++)
  {
    const int zSrc = zDim * zDst / zNew;
    for (yDst = 0; yDst < yNew; yDst++)
    {
      const int ySrc = yDim * yDst / yNew;
      for (xDst = 0; xDst < xNew; xDst++)
      {
        const int xSrc = xDim * xDst / xNew;
        const int offSrc = xSrc + ySrc * xDim + zSrc * xyDim;
        const MUint8 val = m_data[offSrc];
        pixelsNew[offDst++] = val;
      }   // for (xDst)
    }     // for (yDst)
  }       // for (zDst)
  delete [] m_data;
  m_data = pixelsNew;
  setWidth(xNew);
  setHeight(yNew);
  setDepth(zNew);
  return 1;
}


