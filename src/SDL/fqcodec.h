// Copyright (c) 1997-99 Relic Entertainment Inc.
// Written by Janik Joire
//
// $History: $
//
// Version 1.6a

#ifndef ___FQCODEC_H
#define ___FQCODEC_H

#include "Types.h"

// General constants
#ifndef OK
#define OK		0
#endif

#ifndef ERR
#define ERR		-1
#endif

// Rate constants
#define FQ_HRATE	11025L	// Hz
#define FQ_RATE		22050L	// Hz
#define FQ_DRATE	44100L	// Hz

// Size constants
#define FQ_QSIZE	64
#define FQ_HSIZE	128
#define FQ_SIZE		256
#define FQ_DSIZE	512

// Mode constants
#define FQ_MINIT	0x0000		// Initialize CODEC
#define FQ_MDOUBLE	0x0001		// Double mode
#define FQ_MNORM	0x0002		// Normal mode
#define FQ_MHALF	0x0004		// Half mode


// Functions
int fqDecOver(float *aFPBlock,float *aFSBlock,float *aTPBlock,float *aTSBlock, float *aCBlock,float *aWBlock,udword nSize);
int fqDecBlock(float *aFPBlock,float *aFSBlock, float *aTPBlock,float *aTSBlock,int nMode,int nFact);
int fqWriteTBlock(float *aLBlock,float *aRBlock,short nChan, void *pBuf1,udword nSize1,void *pBuf2,udword nSize2);

#endif
