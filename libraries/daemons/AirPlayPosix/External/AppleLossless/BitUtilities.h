/*
	Copyright (C) 2004 Apple Inc. All Rights Reserved.
	
	Bit parsing/writing utilities.
*/

#ifndef __BITUTILITIES_H
#define __BITUTILITIES_H

#if PRAGMA_ONCE
#pragma once
#endif

	#include "CommonServices.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct BitBuffer
{
	uint8_t *		cur;
	uint8_t *		end;
	uint32_t		bitIndex;
	uint32_t		byteSize;
	
} BitBuffer;

/*
	BitArray routines
	- these routines operate on a fixed size array of bits
	- bits are numbered starting from the high-order bit of the first byte in the array (same as BitTst/BitSet/BitClr)
	- bounds checking must be done by the client
*/
Boolean		BitArrayTestBit(const uint8_t *bitArray, uint32_t bitIndex);
void		BitArraySetBit(uint8_t *bitArray, uint32_t bitIndex);
void		BitArraySetBits(uint8_t *bitArray, uint32_t bitIndexStart, uint32_t bitIndexEnd);
void		BitArrayClearBit(uint8_t *bitArray, uint32_t bitIndex);
void		BitArrayClearBits(uint8_t *bitArray, uint32_t bitIndexStart, uint32_t bitIndexEnd);

/*
	BitBuffer routines
	- these routines take a fixed size buffer and read/write to it
	- bounds checking must be done by the client
*/
void		BitBufferInit( BitBuffer * bits, uint8_t * buffer, uint32_t byteSize );
uint32_t	BitBufferRead( BitBuffer * bits, uint8_t numBits );						// note: cannot read more than 16 bits at a time
uint8_t		BitBufferReadSmall( BitBuffer * bits, uint8_t numBits );
uint8_t		BitBufferReadOne( BitBuffer * bits );
uint32_t	BitBufferPeek( BitBuffer * bits, uint8_t numBits );						// note: cannot read more than 16 bits at a time
uint32_t	BitBufferPeekOne( BitBuffer * bits );
uint32_t	BitBufferUnpackBERSize( BitBuffer * bits );
uint32_t	BitBufferGetPosition( BitBuffer * bits );
void		BitBufferByteAlign( BitBuffer * bits, Boolean addZeros );
void		BitBufferAdvance( BitBuffer * bits, uint32_t numBits );
void		BitBufferRewind( BitBuffer * bits, uint32_t numBits );
void		BitBufferWrite( BitBuffer * bits, uint32_t value, uint32_t numBits );

/*
	BitStream routines
	- these routines handle arbitrarily-sized bit buffers and allocate memory on the fly as you write along
*/

#ifdef __cplusplus
}
#endif

#endif	/* __BITUTILITIES_H */
