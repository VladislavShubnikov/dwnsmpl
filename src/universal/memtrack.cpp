// **********************************************************
// FILE: memtrack.cpp
// PURP: trace memory allocation with operators new, delete 
// **********************************************************

//  **********************************************************
//  Includes
//  **********************************************************

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include "mtypes.h"
#include "memtrack.h"


//  **********************************************************
//  Defines
//  **********************************************************

#define MEMORY_BLOCK_MAGIC      9999

//#define TRACE_MEM_DEBUG

//  **********************************************************
//  Types
//  **********************************************************

#pragma pack(push, 2)
typedef struct tagMemRingPart
{
  struct tagMemRingPart *m_next;
  struct tagMemRingPart *m_prev;
} MemRingPart;

typedef struct tagMemRingLock
{
  MemRingPart   m_link;
} MemRingLock;

typedef struct tagMemBlockHeader
{
  int         m_magic;              // magic number
  int         m_size;               // block size
  MemRingPart m_part;               // link to next block
  char        m_srcFileName[32];
  int         m_srcLine;
} MemBlockHeader;

typedef struct tagMemBlock
{
  MemBlockHeader    m_header;       // used for free (and total memory size accounting)
  int               m_data[1];      // some actual data
} MemBlock;
#pragma pack(pop)

//  **********************************************************
//  Vars
//  **********************************************************
static int              s_memorySizeTotal = 0;
static int              s_memorySizePeak = 0;
static MemRingLock      s_memRingLock;
static int              s_memRingLockInit = 0;
static int              s_flagMemTrackActive = 0;

static unsigned int     s_trashX = 0x6537495;
static unsigned int     s_trashY = 0xa56e78d6;
static unsigned int     s_trashZ = 0x645d4e2;

//  **********************************************************
//  Func
//  **********************************************************


#if defined(_DEEP_DEBUG)

void * operator new(size_t nSize, char *fileName, int lineNumber)   throw()
{
  void    *pMem;

  pMem = MemTrackAllocate(nSize, fileName, lineNumber);
  return pMem;
}

void * operator new[](size_t nSize, char *fileName, int lineNumber) throw()
{
  void    *pMem;

  pMem = MemTrackAllocate(nSize, fileName, lineNumber);
  return pMem;
}


void * operator new(size_t nSize)   throw()
{
  void    *pMem;

  pMem = M_MALLOC(nSize);
  return pMem;
}
void * operator new[](size_t nSize) throw()
{
  void    *pMem;

  pMem = M_MALLOC(nSize);
  return pMem;
}

void operator delete(void * pMem)   throw()
{
  M_FREE(pMem);
}
void operator delete[](void * pMem) throw()
{
  M_FREE(pMem);
}

void operator delete(void * pMem, char * sFileName, int nStr)     throw()
{
  USE_PARAM(sFileName);
  USE_PARAM(nStr);
  M_FREE(pMem);
}

void operator delete[](void * pMem, char * sFileName, int nStr)   throw()
{
  USE_PARAM(sFileName);
  USE_PARAM(nStr);
  M_FREE(pMem);
}

#endif


__inline static void _memRingLockInitizlize(MemRingLock *lock)
{
  lock->m_link.m_next = (MemRingPart*)lock;
  lock->m_link.m_prev = (MemRingPart*)lock;
}
__inline static void _memRingPartInitizlize(MemRingPart *part)
{
  part->m_next = (MemRingPart*)NULL;
  part->m_prev = (MemRingPart*)NULL;
}
__inline MemRingPart *_memRingPartGetNext(MemRingPart *part)
{
  return part->m_next;
}
__inline MemRingPart *_memRingPartGetPrev(MemRingPart *part)
{
  return part->m_prev;
}

__inline static int _memRingLockIsEmpty(const MemRingLock *lock)
{
  int empty;
  empty = (lock->m_link.m_next == &lock->m_link);
  return empty;
}
__inline static void _memRingLockAddPart(MemRingLock *lock, MemRingPart *part)
{
  part->m_next = lock->m_link.m_next;
  part->m_prev = &lock->m_link;

  (lock->m_link.m_next)->m_prev = part;
  lock->m_link.m_next = part;
}

#define memOFFSETOF(s,m)   (uintptr_t)&(((s *)0)->m)
#define _memRingPartGetData(part,type,entry)  \
  ((type *)(((char *)(part)) - memOFFSETOF(type, entry)))

__inline static MemRingPart *_memRingLockGetFirst(MemRingLock *lock)
{
  return lock->m_link.m_next;
}
__inline static MemRingPart *_memRingLockGetLast(MemRingLock *lock)
{
  return lock->m_link.m_prev;
}
__inline static MemRingPart *_memRingLockGetTerminator(MemRingLock *lock)
{
  return &lock->m_link;
}
__inline static void _memRingPartRemove(MemRingPart *part)
{
  (part->m_prev)->m_next = part->m_next;
  (part->m_next)->m_prev = part->m_prev;
  part->m_next = (MemRingPart*)NULL;
  part->m_prev = (MemRingPart*)NULL;
}
__inline static int _memRingPartIsInRing(const MemRingPart *part)
{
  return (part->m_next != NULL) && (part->m_prev != NULL);
}

static void _memoryFillTrash(int *pMemory, const int memSizeBytes)
{
  int           seed, memSizeInts, i;
  time_t        timer;

  // init trash keys
  time(&timer);
  seed = (int)timer & 0xffff;

  memSizeInts = memSizeBytes >> 2;
  memSizeInts--;
  if (memSizeInts <= 0)
    return;
  for (i = 0; i < memSizeInts; i++)
  {
    pMemory[i] = (s_trashX << 8) ^ seed;
    s_trashX ^= s_trashX << 16;
    s_trashX ^= s_trashX >> 5;
    s_trashX ^= s_trashX << 1;
    unsigned int t = s_trashX; s_trashX = s_trashY;
    s_trashY = s_trashZ;
    s_trashZ = t ^ s_trashX ^ s_trashY;
  }
}

void  *MemTrackAllocate(
                          size_t memSize,
                          const char *srcFileName,
                          const int srcFileLine
                       )
{
  void        *p;
  MemBlock    *block;
  size_t      sizeAlloc;
  const char  *src, *srcEnd;
  char        *dst;

  if (!s_flagMemTrackActive)
  {
    return malloc(memSize);
  }
  if (!s_memRingLockInit)
  {
    s_memRingLockInit = 1;
    _memRingLockInitizlize(&s_memRingLock);
  }

  sizeAlloc = memSize + sizeof(MemBlockHeader) + sizeof(int);
  block = (MemBlock*)malloc(sizeAlloc);
  if (block == NULL)
    return NULL;

#ifdef TRACE_MEM_DEBUG
  if (memSize > 1 * 1024 * 1024)
    src = NULL;
#endif

  s_memorySizeTotal += (int)memSize;
  if (s_memorySizeTotal > s_memorySizePeak)
    s_memorySizePeak = s_memorySizeTotal;

  block->m_header.m_magic = MEMORY_BLOCK_MAGIC;
  block->m_header.m_size  = (int)memSize;
  _memRingLockAddPart(&s_memRingLock, &block->m_header.m_part);

  // copy src file name: find end
  for (src = srcFileName; src[0] != 0; src++)
  {
  }
  srcEnd = src;
  src--;
  while ( (src[-1] != '\\') && (src[-1] != '/') && (srcEnd - src < 32) )
    src--;
  for (dst = block->m_header.m_srcFileName; src[0] != 0; src++, dst++)
  {
    *dst = *src;
  }
  *dst = 0;
  assert(dst - block->m_header.m_srcFileName < 32);


  // copy src file line
  block->m_header.m_srcLine = srcFileLine;

  p = &block->m_data[0];
  _memoryFillTrash((int*)p, (int)memSize);

  // mark end of block
  {
    int *pEnd = (int*)( (char*)p + memSize );
    *pEnd = MEMORY_BLOCK_MAGIC;
  }

  return p;
}


#ifdef TRACE_MEM_DEBUG
static int s_sumLarge = 0;
static int _detectLargeMemoryBlocksCallback(const void *memPtr,
                                            const int memSize,
                                            const char *fileNameSrc,
                                            const int fileLineNumber)
{
  USE_PARAM(memPtr);
  USE_PARAM(fileNameSrc);
  USE_PARAM(fileLineNumber);
  if (memSize < 1 * 1024 * 1024)
    return 1;

  s_sumLarge += memSize;

  return 1;
}
#endif

void  MemTrackFree(void *pMemory)
{
  MemBlock    *block;

  if (!s_flagMemTrackActive)
  {
    free(pMemory);
    return;
  }

#ifdef TRACE_MEM_DEBUG
  int deepDebugKey = 0;
  if (deepDebugKey)
    MemTrackForAll( _detectLargeMemoryBlocksCallback );
#endif
  if (pMemory == NULL)
  {
    assert(pMemory != NULL);
    return;
  }


  block = (MemBlock*)((char*)pMemory - sizeof(MemBlockHeader));
  if (block->m_header.m_magic == MEMORY_BLOCK_MAGIC)
  {
    char *p = (char*)&block->m_data[0];
    int   *pEnd = (int*)( (char*)p + block->m_header.m_size );
    assert(pEnd[0] == MEMORY_BLOCK_MAGIC);
    if (pEnd[0] != MEMORY_BLOCK_MAGIC)
    {
      // simulate exceptions
      *((int*)0) = 0xDEAD;
      raise(SIGSEGV);
      abort();
    }

    //if (block->m_header.m_size > 6 * 1024 * 1024)
    //  p = NULL;


    _memRingPartRemove(&block->m_header.m_part);
    _memoryFillTrash((int*)pMemory, block->m_header.m_size);
    s_memorySizeTotal -= block->m_header.m_size;
    free(block);
  }
  else
  {
    free(pMemory);
  }
}

int       MemTrackGetSize(int *memSizeAllocatedPeak)
{
  if (memSizeAllocatedPeak != NULL)
    *memSizeAllocatedPeak = s_memorySizePeak;
  return s_memorySizeTotal;
}

int       MemTrackStart()
{
  s_flagMemTrackActive = 1;
  return 1;
}
int       MemTrackStop()
{
  s_flagMemTrackActive = 0;
  s_memRingLockInit = 0;
  if (s_memorySizeTotal != 0)
    return 0;
  return 1;
}

int MemTrackForAll(MemTrackCallback callback)
{
  int               ok;
  MemRingPart       *part;
  MemRingPart       *term;
  MemBlockHeader    *blockHead;
  MemBlock          *block;

  ok = 1;
  term = _memRingLockGetTerminator(&s_memRingLock);
  for (part = _memRingLockGetFirst(&s_memRingLock); part != term; part = _memRingPartGetNext(part) )
  {
    blockHead = _memRingPartGetData(part, MemBlockHeader, m_part);
    block = (MemBlock*)blockHead;
    int memSize = blockHead->m_size;
    void *p = &block->m_data[0];
    (*callback)(p, memSize, blockHead->m_srcFileName, blockHead->m_srcLine);
  }
  
  return ok;
}
