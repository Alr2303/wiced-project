/*
	Copyright (C) 2001-2004 Apple Inc. All Rights Reserved.
	
	Dynamic Predictor routines.
*/

#ifndef __DPLIB_H__
#define __DPLIB_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// defines

#define DENSHIFT_MAX  15
#define DENSHIFT_DEFAULT 9
#define AINIT 38
#define BINIT (-29)
#define CINIT (-2)
#define NUMCOEPAIRS 16

// prototypes

void init_coefs( int16_t * coefs, unsigned int denshift, int numPairs );
void copy_coefs( int16_t * srcCoefs, int16_t * dstCoefs, int numPairs );

// NOTE: these routines read at least "numactive" samples so the i/o buffers must be at least that big

void pc_block( int32_t * in, int32_t * pc, int num, int16_t * coefs, int numactive, unsigned int chanbits, unsigned int denshift );
void unpc_block( int32_t * pc, int32_t * out, int num, int16_t * coefs, int numactive, unsigned int chanbits, unsigned int denshift );

#ifdef __cplusplus
}
#endif

#endif	/* __DPLIB_H__ */
