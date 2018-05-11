/*
	Copyright (C) 2004 Apple Inc. All Rights Reserved.
	
	DPAG mixing/matrixing routines to/from 32-bit predictor buffers.
*/

#ifndef __MATRIXLIB_H
#define __MATRIXLIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 16-bit routines
void	mix16( int16_t * in, unsigned int stride, int32_t * u, int32_t * v, int numSamples, int mixbits, int mixres );
void	unmix16( int32_t * u, int32_t * v, int16_t * out, unsigned int stride, int numSamples, int mixbits, int mixres );

// 20-bit routines
void	mix20( uint8_t * in, unsigned int stride, int32_t * u, int32_t * v, int numSamples, int mixbits, int mixres );
void	unmix20( int32_t * u, int32_t * v, uint8_t * out, unsigned int stride, int numSamples, int mixbits, int mixres );

// 24-bit routines
// - 24-bit data sometimes compresses better by shifting off the bottom byte so these routines deal with
//	 the specified "unused lower bytes" in the combined "shift" buffer
void	mix24( uint8_t * in, unsigned int stride, int32_t * u, int32_t * v, int numSamples,
				int mixbits, int mixres, uint16_t * shiftUV, int bytesShifted );
void	unmix24( int32_t * u, int32_t * v, uint8_t * out, unsigned int stride, int numSamples,
				 int mixbits, int mixres, uint16_t * shiftUV, int bytesShifted );

// 32-bit routines
// - note that these really expect the internal data width to be < 32-bit but the arrays are 32-bit
// - otherwise, the calculations might overflow into the 33rd bit and be lost
// - therefore, these routines deal with the specified "unused lower" bytes in the combined "shift" buffer
void	mix32( int32_t * in, unsigned int stride, int32_t * u, int32_t * v, int numSamples,
				int mixbits, int mixres, uint16_t * shiftUV, int bytesShifted );
void	unmix32( int32_t * u, int32_t * v, int32_t * out, unsigned int stride, int numSamples,
				 int mixbits, int mixres, uint16_t * shiftUV, int bytesShifted );

// 20/24/32-bit <-> 32-bit helper routines (not really matrixing but convenient to put here)
void	copy20ToPredictor( uint8_t * in, unsigned int stride, int32_t * out, int numSamples );
void	copy24ToPredictor( uint8_t * in, unsigned int stride, int32_t * out, int numSamples );

void	copyPredictorTo24( int32_t * in, uint8_t * out, unsigned int stride, int numSamples );
void	copyPredictorTo24Shift( int32_t * in, uint16_t * shift, uint8_t * out, unsigned int stride, int numSamples, int bytesShifted );
void	copyPredictorTo20( int32_t * in, uint8_t * out, unsigned int stride, int numSamples );

void	copyPredictorTo32( int32_t * in, int32_t * out, unsigned int stride, int numSamples );
void	copyPredictorTo32Shift( int32_t * in, uint16_t * shift, int32_t * out, unsigned int stride, int numSamples, int bytesShifted );

#ifdef __cplusplus
}
#endif

#endif	/* __MATRIXLIB_H */
