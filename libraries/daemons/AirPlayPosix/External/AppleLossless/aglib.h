/*
	Copyright (C) 2001-2005 Apple Inc. All Rights Reserved.
*/

#ifndef AGLIB_H
#define AGLIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QBSHIFT 9
#define QB (1<<QBSHIFT)
#define PB0 40
#define MB0 10
#define KB0 14
#define MAX_RUN_DEFAULT 255

#define MMULSHIFT 2
#define MDENSHIFT (QBSHIFT - MMULSHIFT - 1)
#define MOFF ((1<<(MDENSHIFT-2)))

#define BITOFF 24

/* Max. prefix of 1's. */
#define MAX_PREFIX_16			9
#define MAX_PREFIX_TOLONG_16	15
#define MAX_PREFIX_32			9

/* Max. bits in 16-bit data type */
#define MAX_DATATYPE_BITS_16	16

typedef struct AGParamRec
{
    unsigned int mb, mb0, pb, kb, wb, qb;
    unsigned int fw, sw;

    unsigned int maxrun;

    // fw = 1, sw = 1;

} AGParamRec, *AGParamRecPtr;

struct BitBuffer;

void	set_standard_ag_params(AGParamRecPtr params, unsigned int fullwidth, unsigned int sectorwidth);
void	set_ag_params(AGParamRecPtr params, unsigned int m, unsigned int p, unsigned int k, unsigned int f, unsigned int s, unsigned int maxrun);

int		dyn_comp(AGParamRecPtr params, int32_t * pc, struct BitBuffer * bitstream, int numSamples, int bitSize, unsigned int * outNumBits);
int		dyn_decomp(AGParamRecPtr params, struct BitBuffer * bitstream, int32_t * pc, int numSamples, int maximumSize, unsigned int * outNumBits);

// IBM XLC support
#if __IBMC__ || __IBMCPP__
	#define __cntlzw	__cntlz4
#endif

#ifdef __cplusplus
}
#endif

#endif //#ifndef AGLIB_H
