// **********************************************************
// FILE: memtrack.h
// PURP: trace memory allocation with operators new, delete 
// **********************************************************

#ifndef __memtrack_h
#define __memtrack_h
#pragma once

//  **********************************************************
//  Includes
//  **********************************************************

#include <memory.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>

//  **********************************************************
//  Types
//  **********************************************************

typedef int (*MemTrackCallback)(const void *memPtr, const int memSize, const char *fileNameSrc, const int fileLineNumber);


//  **********************************************************
//  Functions prototypes
//  **********************************************************

#ifdef    __cplusplus
extern "C"
{
#endif    /* __cplusplus */


// Memory overloads
int       MemTrackStart();
int       MemTrackStop();

void     *MemTrackAllocate(size_t memSize, const char *srcFileNam , const int srcFileLine);
void      MemTrackFree(void *pMemory);
int       MemTrackGetSize(int *memSizeAllocatedPeak);

int       MemTrackForAll(MemTrackCallback callback);

#ifdef    __cplusplus
}
#endif  /* __cplusplus */


//  **********************************************************
//  Defines
//  **********************************************************

#if defined(_DEEP_DEBUG)
  #define M_MALLOC(s)     MemTrackAllocate(s, __FILE__, __LINE__)
  #define M_FREE(p)       MemTrackFree(p)
#else
  #define M_MALLOC(s)     malloc(s)
  #define M_FREE(p)       free(p)
#endif


#if defined(_DEEP_DEBUG)
  void *operator new(size_t nSize)      throw();
  void *operator new[](size_t nSize)    throw();

  void * operator new(size_t nSize, char * sFileName, int nStr) throw();
  void * operator new[](size_t nSize, char * sFileName, int nStr) throw();

  #define M_NEW(type)                 new(__FILE__, __LINE__) type

  void operator delete(void * pMem)     throw();
  void operator delete[](void * pMem)   throw();

  void operator delete(void * pMem, char * sFileName, int nStr)     throw();
  void operator delete[](void * pMem, char * sFileName, int nStr)   throw();

#else
  #define M_NEW(type)                 new type
#endif

#endif
