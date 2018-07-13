// ****************************************************************************
// File: ktxtexture.h
// Purpose: 3d Volume texture loader
// ****************************************************************************

#ifndef  __ktxtexture_h
#define  __ktxtexture_h

// ****************************************************************************
// Includes
// ****************************************************************************

#include <stdlib.h>
#include <stdio.h>

#include "mtypes.h"

// ****************************************************************************
// Defines
// ****************************************************************************

//! Pixel format identifiers,
// The same as in openGL ES
#define   KTX_GL_RED            0x1903
#define   KTX_GL_RGB            0x1907
#define   KTX_GL_RGBA           0x1908

#define   KTX_GL_R8_EXT         0x8229
#define   KTX_GL_RGB8_OES       0x8051
#define   KTX_GL_RGBA8_OES      0x8058

#define   KTX_GL_UNSIGNED_BYTE  0x1401

#define   KTX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define   KTX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

// ****************************************************************************
// Class
// ****************************************************************************

#pragma warning(disable: 4201)
#pragma pack(push, 2)


/** \struct KtxHeader
 *  \brief header
 */
struct KtxHeader
{
  //! magic string to identify data
  char      m_id[12];
  //! should be 0x04030201
  MUint32   m_endianness;
  //! typically is KTX_GL_UNSIGNED_BYTE
  MUint32   m_glType;
  //! set to 1
  MUint32   m_glTypeSize;
  //! KTX_GL_RED, ...
  MUint32   m_glFormat;
  //! The same as format (previous field)
  MUint32   m_glInternalFormat;
  MUint32   m_glBaseInternalFormat;
  //! X dimension
  MUint32   m_pixelWidth;
  //! Y dimension
  MUint32   m_pixelHeight;
  //! Z dimension
  MUint32   m_pixelDepth;
  //! Number of non texture arrays (usually is 0)
  MUint32   m_numberOfArrayElements;
  //! some magic. Equal to 1.
  MUint32   m_numberOfFaces;
  //! typcially 1.
  MUint32   m_numberOfMipmapLevels;
  //! extra key values, usually 0
  MUint32   m_bytesOfKeyValueData;
};

/** \enum KtxError
 *  \brief result codes
 */
enum KtxError
{
  //! Not assigned value
  KTX_ERROR_NA              = -1,
  //! Everything is fine
  KTX_ERROR_OK              = 0,
  //! memory allocation opertaion is failed
  KTX_ERROR_NO_MEMORY       = 1,
  //! Something wrong with file during read
  KTX_ERROR_WRONG_FORMAT    = 2,
  //! Invalid data size
  KTX_ERROR_WRONG_SIZE      = 3,
  //! Some broked data found
  KTX_ERROR_BROKEN_CONTENT  = 4,
  //! Error write data
  KTX_ERROR_WRITE           = 5,
  //! Error open file in folder
  KTX_ERROR_CANT_OPEN_FILE  = 6,

  KTX_ERROR_COUNT
};


enum KtxKeyDataType
{
  KTX_KEY_DATA_NAN        = -1,
  KTX_KEY_DATA_NA         = 0,
  KTX_KEY_DATA_BBOX       = 1,
  KTX_KEY_DATA_MIN_SIZE   = 2,
};

#define KEY_DATA_BUFFER_SIZE    ((4 * 3) * 8)

/** \class KtxKeyData
 *  \brief Optional key data
 */
class KtxKeyData
{
public:
  KtxKeyDataType  m_dataType;
  MUint8          m_buffer[KEY_DATA_BUFFER_SIZE];

public:
  KtxKeyData(): m_dataType(KTX_KEY_DATA_NA)
  {
    for (int i = 0; i < KEY_DATA_BUFFER_SIZE; i++)
      m_buffer[i] = 0;
  }
};


/** \class KtxTexture
 *  \brief Texture in KTX format in memnory (volumetric texture)
 */
class KtxTexture
{
public:
  KtxTexture();
 ~KtxTexture();

  // I/O operations


  /*!
   * \brief Save volumetric texture to KTX file.
   *   More details about KTX format can be found here:
   *   https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
   * \param file Opened file to write texture content
   * \return OK if everythiong fine, or error code, please, see KtxError
   */
  KtxError       saveToFileContent(FILE *file);
  /*!
   * \brief Load volumetric texture from KTX file.
   * \param file Opened file for reading  with texture content
   * \return OK if everythiong fine, or error code, please, see KtxError
   */
  KtxError       loadFromFileContent(FILE *file);

  // create / destroy in memory
  void           destroy();
  KtxError       create1D(
                          const int xDim,
                          const int bytesPerPixel
                         );
  KtxError       create2D(
                          const int xDim,
                          const int yDim,
                          const int bytesPerPixel
                        );
  KtxError       create3D(
                          const int xDim,
                          const int yDim,
                          const int zDim,
                          const int bytesPerPixel
                        );
  KtxError        createAs1ByteCopy(const KtxTexture *tex);
  KtxError        createAs1ByteCopyScaledDown(
                                              const KtxTexture *tex,
                                              const int scaleDownTimes
                                             );
  KtxError        createAs1ByteCopyPowerOfTwo(const KtxTexture *tex);
  KtxError        createAs1ByteLessSize(
                                        const KtxTexture *tex,
                                        const int xDimDst,
                                        const int yDimDst,
                                        const int zDimDst
                                       );
  KtxError        createAs1BytePowerByMaxSide(
                                        const KtxTexture *tex,
                                        const int maxDim,
                                        const int isDstCubeTexture = 0,
                                        const int useSmoothInterpolation = 1
                                             );
  void            binarizeByBarrier(
                                    const MUint8 valBarrier,
                                    const MUint8 valLess,
                                    const MUint8 valGreat
                                   );
  KtxError        createMask(
                              const KtxTexture *tex,
                              const int valBarrier,
                              const int numAddVoxels
                            );
  KtxError        createMinSizeTexture(
                                        const KtxTexture *tex,
                                        const int indexChannel
                                      );

  void            clearBorder();

  int             scaleUpZ(const int zNew);
  int             rescale(const int xNew, const int yNew, const int zNew);

  KtxError        createAsCopy(const KtxTexture *tex);
  KtxError        createAsSingleSphere(const int dim);

  //! symmetry check
  int             getBoundingBox(V3d &vMin, V3d &vMax) const;
  int             getHorSliceSymmetry(const int z, float &xSym, float &ySym);

  //! fill with zeros
  void            fillZeroYGreater(const int yClipMax);
  void            fillValZGreater(const int z, const MUint8 valToFill);

  //! gauss smooth
  void            gaussSmooth(const int gaussNeigh, const float gaussSigma);

  //! scale down to size
  int             scaleDownToSize(
                                  const int xDimDst,
                                  const int yDimDst,
                                  const int zDimDst
                                 );
  //! convert format from 1bpp to 4bpp
  int             convertTo4bpp();

  //! get texture width (x size)
  int            getWidth() const    { return m_header.m_pixelWidth;     }
  //! get texture height (y size)
  int            getHeight() const   { return m_header.m_pixelHeight;    }
  //! get texture depth (z size)
  int            getDepth() const    { return m_header.m_pixelDepth;     }
  //! get texture pixels
  MUint8        *getData() const     { return m_data;                    }
  MUint32        getGlFormat() const { return m_header.m_glFormat;       }
  int            getDataSize() const { return m_dataSize;                }

  void           setData(MUint8 *dataMemNew) { m_data = dataMemNew; }

  void           setKeyDataBbox(const V3f &vMin, const V3f &vMax);
  void           setKeyDataMinSize(const V3d &vMin, const V3d &vSize);

  KtxKeyData    *getKeyData() const
  {
    return (KtxKeyData*)&m_keyData;
  }

  void            boxFilter3d(const int radius);

  V3f      *getBoxSize() const
  {
    return (V3f*)&m_boxSize;
  }
  void          setWidth(const int v)
  {
    m_header.m_pixelWidth = v;
  }
  void          setHeight(const int v)
  {
    m_header.m_pixelHeight = v;
  }
  void          setDepth(const int v)
  {
    m_header.m_pixelDepth = v;
  }

 private:
  //! Information about volume
  KtxHeader     m_header;
  //! pixels data
  MUint8       *m_data;
  //! Size in memory, bytes. Can be != to linear size, for compressed formats.
  int           m_dataSize;
  //! Flag for compressed texture (default is 0)
  int           m_isCompressed;
  //! Optional key data
  KtxKeyData    m_keyData;
  //! Optional volume size in mm
  V3f      m_boxSize;

protected:
private:
  KtxError    enlargeByMinSize();
};

const char *KtxTextureGetErrorString(const KtxError err);

#pragma pack(pop)
#pragma warning(default: 4201)

#endif
