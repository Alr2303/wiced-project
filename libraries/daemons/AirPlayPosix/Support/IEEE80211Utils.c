/*
	Copyright (C) 2008-2013 Apple Inc. All Rights Reserved.
*/

#include "IEEE80211Utils.h"

#include "CommonServices.h"
#include "DebugServices.h"
#include "DataBufferUtils.h"
#include "StringUtils.h"

#define PTR_LEN_LEN( SRC, END )		( SRC ), (size_t)( ( END ) - ( SRC ) ), (size_t)( ( END ) - ( SRC ) )

//===========================================================================================================================
//	IEGet
//===========================================================================================================================

OSStatus
	IEGet( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint8_t				inID, 
		const uint8_t **	outPtr, 
		size_t *			outLen, 
		const uint8_t **	outNext )
{
	OSStatus			err;
	uint8_t				eid;
	const uint8_t *		ptr;
	size_t				len;
	
	while( ( err = IEGetNext( inSrc, inEnd, &eid, &ptr, &len, &inSrc ) ) == kNoErr )
	{
		if( eid == inID )
		{
			if( outPtr )  *outPtr = ptr;
			if( outLen )  *outLen = len;
			break;
		}
	}
	if( outNext ) *outNext = inSrc;
	return( err );
}

//===========================================================================================================================
//	IEGetNext
//===========================================================================================================================

OSStatus
	IEGetNext( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint8_t *			outID, 
		const uint8_t **	outData, 
		size_t *			outLen, 
		const uint8_t **	outNext )
{
	const uint8_t *		ptr;
	size_t				len;
	const uint8_t *		next;
	
	// IE's have the following format (IEEE 802.11-2007 section 7.3.2):
	//
	//		<1:eid> <1:length> <length:data>
	
	check( inSrc <= inEnd );
	len = (size_t)( inEnd - inSrc );
	if( len < 2 )
	{
		if( len != 0 ) dlog( kLogLevelNotice, "### Short IE header:\n%1.1H\n", PTR_LEN_LEN( inSrc, inEnd ) );
		return( kNotFoundErr );
	}
	
	len  = inSrc[ 1 ];
	ptr  = inSrc + 2;
	next = ptr + len;
	if( ( next < inSrc ) || ( next > inEnd ) )
	{
		dlog( kLogLevelNotice, "### Bad IE length:\n%1.1H\n", PTR_LEN_LEN( inSrc, inEnd ) );
		return( kUnderrunErr );
	}
	
	*outID   = inSrc[ 0 ];
	*outData = ptr;
	*outLen  = len;
	if( outNext ) *outNext = next;
	return( kNoErr );
}

//===========================================================================================================================
//	IEGetVendorSpecific
//===========================================================================================================================

OSStatus
	IEGetVendorSpecific( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint32_t			inVID, 
		const uint8_t **	outData, 
		size_t *			outLen, 
		const uint8_t **	outNext )
{
	const uint8_t *		ptr;
	const uint8_t *		next;
	uint8_t				eid;
	size_t				len;
	uint32_t			vid;
	
	// Vendor-specific IE's have the following format (IEEE 802.11-2007 section 7.3.2.26):
	//
	//		<1:eid=221> <1:length> <3:oui> <1:type> <length - 4:data>
	
	for( ptr = inSrc; ( inEnd - ptr ) >= 2; ptr = next )
	{
		eid  = ptr[ 0 ];
		len  = ptr[ 1 ];
		next = ptr + 2 + len;
		if( eid != kIEEE80211_EID_Vendor )
		{
			continue;
		}
		if( ( next < inSrc ) || ( next > inEnd ) )
		{
			dlog( kLogLevelNotice, "### Overlong vendor IE len:\n%1.1H\n", PTR_LEN_LEN( inSrc, inEnd ) );
			return( kSizeErr );
		}
		if( len < 4 )
		{
			dlog( kLogLevelNotice, "### Short vendor IE:\n%1.1H\n", PTR_LEN_LEN( inSrc, inEnd ) );
			continue;
		}
		
		vid = (uint32_t)( ( ptr[ 2 ] << 24 ) | ( ptr[ 3 ] << 16 ) | ( ptr[ 4 ] << 8 ) | ptr[ 5 ] );
		if( vid != inVID )
		{
			continue;
		}
		
		*outData = ptr + 6; // Skip eid, len, oui, and type.
		*outLen  = len - 4; // Skip oui and type (length doesn't include the eid and len).
		if( outNext ) *outNext = next;
		return( kNoErr );
	}
	if( ptr != inEnd )
	{
		dlog( kLogLevelNotice, "### Bad vendor IE len:\n%1.1H\n", PTR_LEN_LEN( inSrc, inEnd ) );
	}
	return( kNotFoundErr );
}

//===========================================================================================================================
//	IECopyCoalescedVendorSpecific
//===========================================================================================================================

OSStatus
	IECopyCoalescedVendorSpecific( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint32_t			inVID, 
		uint8_t **			outPtr, 
		size_t *			outLen )
{
	OSStatus			err;
	uint8_t *			buf;
	size_t				totalLen;
	uint8_t *			tmp;
	const uint8_t *		ptr;
	size_t				len;
	
	buf = NULL;
	totalLen = 0;
	
	while( ( err = IEGetVendorSpecific( inSrc, inEnd, inVID, &ptr, &len, &inSrc ) ) == kNoErr )
	{
		tmp = (uint8_t *) malloc_compat( totalLen + len + 1 ); // +1 to avoid malloc( 0 ) being undefined.
		require_action( tmp, exit, err = kNoMemoryErr );
		if( buf )
		{
			memcpy( tmp, buf, totalLen );
			free_compat( buf );
		}
		buf = tmp;
		memcpy( buf + totalLen, ptr, len );
		totalLen += len;
	}
	require_quiet( buf, exit );
	
	*outPtr = buf;
	*outLen = totalLen;
	buf = NULL;
	err = kNoErr;
	
exit:
	if( buf ) free_compat( buf );
	return( err );
}

//===========================================================================================================================
//	IEGetAppleGeneral
//===========================================================================================================================

OSStatus	IEGetAppleGeneral( const uint8_t *inSrc, const uint8_t *inEnd, uint8_t *outProductID, uint16_t *outFlags )
{
	OSStatus			err;
	const uint8_t *		ptr;
	size_t				len;
	
	// The Apple General IE has the following format:
	//
	//		<1:productID> <2:big endian flags>
	
	err = IEGetVendorSpecific( inSrc, inEnd, kIEEE80211_VID_AppleGeneral, &ptr, &len, NULL );
	require_noerr_quiet( err, exit );
	if( len < 3 )
	{
		dlog( kLogLevelNotice, "### Bad Apple general IE length (%zu):\n%1.1H\n", len, PTR_LEN_LEN( inSrc, inEnd ) );
		err = kSizeErr;
		goto exit;
	}
	
	*outProductID	= ptr[ 0 ];
	*outFlags		= (uint16_t)( ( ptr[ 1 ] << 8 ) | ptr[ 2 ] );
	
exit:
	return( err );
}

//===========================================================================================================================
//	IEGetDWDS
//===========================================================================================================================

OSStatus	IEGetDWDS( const uint8_t *inSrc, const uint8_t *inEnd, uint8_t *outRole, uint32_t *outFlags )
{
	OSStatus			err;
	const uint8_t *		ptr;
	size_t				len;
	
	// The DWDS IE has the following format:
	//
	//		<1:subtype> <1:version> <1:role> <4:flags>
	
	err = IEGetVendorSpecific( inSrc, inEnd, kIEEE80211_VID_DWDS, &ptr, &len, NULL );
	require_noerr_quiet( err, exit );
	if( len < 7 )
	{
		dlog( kLogLevelNotice, "### Bad DWDS IE length (%zu):\n%1.1H\n", len, PTR_LEN_LEN( inSrc, inEnd ) );
		err = kSizeErr;
		goto exit;
	}
	if( ptr[ 0 ] != 0x00 ) // SubType
	{
		dlog( kLogLevelNotice, "### Unknown DWDS subtype: (%d)\n%1.1H\n", ptr[ 0 ], PTR_LEN_LEN( inSrc, inEnd ) );
		err = kTypeErr;
		goto exit;
	}
	if( ptr[ 1 ] != 0x01 ) // Version
	{
		dlog( kLogLevelNotice, "### Unknown DWDS version: (%d)\n%1.1H\n", ptr[ 1 ], PTR_LEN_LEN( inSrc, inEnd ) );
		err = kVersionErr;
		goto exit;
	}
	
	*outRole	= ptr[ 2 ];
	*outFlags	= (uint32_t)( ( ptr[ 3 ] << 24 ) | ( ptr[ 4 ] << 16 ) | ( ptr[ 5 ] << 8 ) | ptr[ 6 ] );	
	
exit:
	return( err );
}

//===========================================================================================================================
//	IEGetTLV16
//===========================================================================================================================

OSStatus
	IEGetTLV16( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint32_t			inVID, 
		uint16_t			inAttrID, 
		void *				inBuf, 
		size_t				inBufLen, 
		size_t *			outLen, 
		const uint8_t **	outNext )
{
	OSStatus			err;
	const uint8_t *		src;
	const uint8_t *		ptr;
	const uint8_t *		end;
	size_t				len;
	uint8_t *			dst;
	uint8_t *			lim;
	Boolean				gotHeader;
	uint16_t			attrID;
	size_t				attrLen;
	
	src = (const uint8_t *) inSrc;
	dst = (uint8_t *) inBuf;
	lim = dst + inBufLen;
	
	// 16-bit TLVs are a little tricky because an attribute may span multiple IE's. It is further complicated by the 
	// attribute header only being in the first IE for the attribute so if we find a 16-bit TLV IE, but it's not the 
	// one we want, we can't just check the next IE blindly because we may misinterpret the next IE as being a header. 
	// So we have to walk to the end of it.
	//
	// TLV16 attributes are a vendor-specific IE with the following format (WPS 1.0 spec sections 7.1 and 7.2):
	//
	// 		<1:eid=221> <1:length> <4:oui=00 50 F2 04> <2:attrID> <2:dataLength> <Min(dataLength, 0xFF - 8):data>
	//
	// TLV16 attributes may span multiple IE's if they won't fit in a single 255 byte IE data section.
	// Subsequent IE's to have their data section concatenated have the following format:
	//
	// 		<1:eid=221> <1:length> <4:oui=00 50 F2 04> <Min(dataLength, 0xFF - 4):data>
	
	gotHeader = false;
	attrID    = 0;
	attrLen   = 0;
	for( ;; )
	{
		err = IEGetVendorSpecific( src, inEnd, inVID, &ptr, &len, &src );
		require_noerr_quiet( err, exit );
		
		if( !gotHeader )
		{
			if( len < 4 ) { dlog( kLogLevelNotice, "### Short TLV16:\n%1.1H\n", PTR_LEN_LEN( inSrc, inEnd ) ); continue; }
			attrID  = (uint16_t)( ( ptr[ 0 ] << 8 ) | ptr[ 1 ] );
			attrLen = (uint16_t)( ( ptr[ 2 ] << 8 ) | ptr[ 3 ] );
			ptr += 4;
			len -= 4;
			gotHeader = true;
		}
		if( attrID == inAttrID )
		{
			if( inBuf )
			{
				end = ptr + len;
				while( ( ptr < end ) && ( dst < lim ) )
				{
					*dst++ = *ptr++;
				}
				require_action_quiet( ptr == end, exit, err = kNoSpaceErr );
			}
			else
			{
				dst += len;
			}
		}
		attrLen -= Min( len, attrLen );
		if( attrLen == 0 )
		{
			if( attrID == inAttrID ) break;
			gotHeader = false;
		}
	}
	err = kNoErr;
	
exit:
	*outLen = (size_t)( dst - ( (uint8_t *) inBuf ) ); // Note: this works even if inBuf is NULL.
	if( outNext ) *outNext = src;
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	IEBufferAppendIE
//===========================================================================================================================

OSStatus	IEBufferAppendIE( IEBuffer *inBuf, uint8_t inEID, const void *inData, size_t inLen )
{
	OSStatus			err;
	const uint8_t *		src;
	const uint8_t *		end;
	
	require_noerr_action_quiet( inBuf->firstErr, exit2, err = inBuf->firstErr );
	
	// IEEE 802.11 IE's are in the following format:
	//
	// 		<1:eid> <1:length> <length:data>
	
	if( inLen == kSizeCString ) inLen = strlen( (const char *) inData );
	require_action( ( inBuf->len + 1 + 1 + inLen ) < sizeof( inBuf->buf ), exit, err = kSizeErr );
	
	inBuf->buf[ inBuf->len++ ] = inEID;
	inBuf->buf[ inBuf->len++ ] = (uint8_t) inLen;
	
	src = (const uint8_t *) inData;
	end = src + inLen;
	while( src < end ) inBuf->buf[ inBuf->len++ ] = *src++;
	err = kNoErr;
	
exit:
	if( !inBuf->firstErr ) inBuf->firstErr = err;
	
exit2:
	return( err );
}

//===========================================================================================================================
//	IEBufferStartVendorIE
//===========================================================================================================================

OSStatus	IEBufferStartVendorIE( IEBuffer *inBuf, uint32_t inVID )
{
	OSStatus			err;
	
	require_noerr_action_quiet( inBuf->firstErr, exit2, err = inBuf->firstErr );
	
	// IEEE 802.11 vendor-specific IE's are in the following format:
	//
	// 		<1:eid=0xDD> <1:length> <3:oui> <1:type> <length - 4:data>
	
	require_action( ( inBuf->len + 1 + 1 + 3 + 1 ) < sizeof( inBuf->buf ), exit, err = kSizeErr );
	
	inBuf->buf[ inBuf->len++ ]	= kIEEE80211_EID_Vendor;
	inBuf->savedOffset			= inBuf->len;
	inBuf->buf[ inBuf->len++ ]	= 0; // Placeholder to update when the IE ends.
	inBuf->buf[ inBuf->len++ ]	= (uint8_t)( ( inVID >> 24 ) & 0xFF );
	inBuf->buf[ inBuf->len++ ]	= (uint8_t)( ( inVID >> 16 ) & 0xFF );
	inBuf->buf[ inBuf->len++ ]	= (uint8_t)( ( inVID >>  8 ) & 0xFF );
	inBuf->buf[ inBuf->len++ ]	= (uint8_t)(   inVID         & 0xFF );
	err = kNoErr;
	
exit:
	if( !inBuf->firstErr ) inBuf->firstErr = err;
	
exit2:
	return( err );
}

//===========================================================================================================================
//	IEBufferEndVendorIE
//===========================================================================================================================

OSStatus	IEBufferEndVendorIE( IEBuffer *inBuf )
{
	OSStatus			err;
	
	require_noerr_action_quiet( inBuf->firstErr, exit2, err = inBuf->firstErr );
	require_action( inBuf->savedOffset > 0, exit, err = kNotPreparedErr );
	
	inBuf->buf[ inBuf->savedOffset ] = (uint8_t)( ( inBuf->len - inBuf->savedOffset ) - 1 );
	inBuf->savedOffset = 0;
	err = kNoErr;
	
exit:
	if( !inBuf->firstErr ) inBuf->firstErr = err;
	
exit2:
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	DataBuffer_AppendIE
//===========================================================================================================================

OSStatus	DataBuffer_AppendIE( DataBuffer *inDB, uint8_t inEID, const void *inData, size_t inLen )
{
	OSStatus			err;
	uint8_t *			dst;
	const uint8_t *		src;
	const uint8_t *		end;
	
	// IEEE 802.11 IE's are in the following format:
	//
	// 		<1:eid> <1:length> <length:data>
	
	if( inLen == kSizeCString ) inLen = strlen( (const char *) inData );
	require_action( inLen <= 0xFF, exit, err = kSizeErr );
	
	err = DataBuffer_Grow( inDB, 1 + 1 + inLen, &dst ); // eid + len + data.
	require_noerr( err, exit );
	
	*dst++ = inEID;
	*dst++ = (uint8_t) inLen;
	
	src = (const uint8_t *) inData;
	end = src + inLen;
	while( src < end ) *dst++ = *src++;
	check( dst == DataBuffer_GetEnd( inDB ) );
	
exit:
	if( !inDB->firstErr ) inDB->firstErr = err;
	return( err );
}

//===========================================================================================================================
//	DataBuffer_AppendAppleGeneralIE
//===========================================================================================================================

OSStatus
	DataBuffer_AppendAppleGeneralIE( 
		DataBuffer *	inDB, 
		uint8_t			inProductID, 
		uint16_t		inFlags, 
		const uint8_t	inRadio1BSSID[ 6 ], uint8_t inRadio1Channel, 
		const uint8_t	inRadio2BSSID[ 6 ], uint8_t inRadio2Channel )
{
	OSStatus		err;
	uint8_t			buf[ 19 ];
	size_t			len;
	
	require_action( inRadio1BSSID || ( inRadio1Channel == 0 ), exit, err = kParamErr );
	require_action( inRadio2BSSID || ( inRadio2Channel == 0 ), exit, err = kParamErr );
	
	// The Apple General IE has the following format:
	//
	//		<1:productID> <2:big endian flags> [sub IEs]
	
	len = 0;
	buf[ len++ ] = inProductID;
	buf[ len++ ] = (uint8_t)( inFlags >> 8 );
	buf[ len++ ] = (uint8_t)( inFlags & 0xFF );
	
	if( ( inRadio1Channel != 0 ) && ( inRadio2Channel != 0 ) )
	{
		buf[ len++ ] = kAppleGeneralIE_SubEID_ChannelInfo;
		buf[ len++ ] = 14; // 2 * <6:BSSID> + <1:channel>
		
		memcpy( &buf[ len ], inRadio1BSSID, 6 );
		len += 6;
		buf[ len++ ] = inRadio1Channel;
		
		memcpy( &buf[ len ], inRadio2BSSID, 6 );
		len += 6;
		buf[ len++ ] = inRadio2Channel;
		
		check( len == 19 );
	}
	
	err = DataBuffer_AppendVendorIE( inDB, kIEEE80211_VID_AppleGeneral, buf, len );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DataBuffer_AppendVendorIE
//===========================================================================================================================

OSStatus	DataBuffer_AppendVendorIE( DataBuffer *inDB, uint32_t inVID, const void *inData, size_t inLen )
{
	OSStatus			err;
	size_t				originalLen;
	uint8_t *			dst;
	const uint8_t *		src;
	size_t				len;
	
	originalLen = DataBuffer_GetLen( inDB );
	
	// IEEE 802.11 vendor-specific IE's are in the following format:
	//
	// 		<1:eid> <1:length> <3:oui> <1:type> <length - 4:data>
	//
	// Vendor IEs larger than the max of 255 bytes (249 of payload) may be split across multiple IEs.
	// When split, each IE contains <1:eid> <1:length> <3:oui> <1:type> followed by up to 249 bytes
	// of payload. When read back, the payloads are concatenated to reconstruct the original IE.
	
	if( inLen == kSizeCString ) inLen = strlen( (const char *) inData );
	src = (const uint8_t *) inData;
	do
	{
		len = Min( inLen, 249 ); // Max of 255 - 1 (eid) - 1 (len) - 4 (vid) = 249.
		
		err = DataBuffer_Grow( inDB, 1 + 1 + 4 + len, &dst ); // eid + len + vid + data.
		require_noerr( err, exit );
		
		*dst++ = kIEEE80211_EID_Vendor;
		*dst++ = (uint8_t)( 4 + len );
		*dst++ = (uint8_t)( ( inVID >> 24 ) & 0xFF );
		*dst++ = (uint8_t)( ( inVID >> 16 ) & 0xFF );
		*dst++ = (uint8_t)( ( inVID >>  8 ) & 0xFF );
		*dst++ = (uint8_t)(   inVID         & 0xFF );
		memcpy( dst, src, len );
		src   += len;
		inLen -= len;
		dst   += len;
		check( dst == DataBuffer_GetEnd( inDB ) );
		
	}	while( inLen > 0 );
	
exit:
	if( err )				inDB->bufferLen = originalLen; // Restore on errors.
	if( !inDB->firstErr )	inDB->firstErr = err;
	return( err );
}

//===========================================================================================================================
//	DataBuffer_AppendTLV16
//===========================================================================================================================

OSStatus	DataBuffer_AppendTLV16( DataBuffer *inDB, uint16_t inType, const void *inData, size_t inLen )
{
	OSStatus			err;
	uint8_t *			dst;
	const uint8_t *		src;
	const uint8_t *		end;
	
	// 16-bit TLV's have the following format (big endian):
	//
	//		<2:type> <2:length> <length:data>
	
	if( inLen == kSizeCString ) inLen = strlen( (const char *) inData );
	require_action( inLen <= 0xFFFF, exit, err = kSizeErr );
	
	err = DataBuffer_Grow( inDB, 2 + 2 + inLen, &dst ); // Append <2:type> <2:len> <len:value>
	require_noerr( err, exit );
	
	*dst++ = (uint8_t)( ( inType >> 8 ) & 0xFF );
	*dst++ = (uint8_t)(   inType        & 0xFF );
	*dst++ = (uint8_t)( ( inLen  >> 8 ) & 0xFF );
	*dst++ = (uint8_t)(   inLen         & 0xFF );
	
	src = (const uint8_t *) inData;
	end = src + inLen;
	while( src < end ) *dst++ = *src++;
	check( dst == DataBuffer_GetEnd( inDB ) );
	
exit:
	if( !inDB->firstErr ) inDB->firstErr = err;
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	TLV16Get
//===========================================================================================================================

OSStatus
	TLV16Get( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint16_t			inType, 
		const uint8_t **	outData, 
		size_t *			outLen, 
		const uint8_t **	outNext )
{
	OSStatus			err;
	uint16_t			type;
	const uint8_t *		dataPtr;
	size_t				dataLen;
	
	while( ( err = TLV16GetNext( inSrc, inEnd, &type, &dataPtr, &dataLen, &inSrc ) ) == kNoErr )
	{
		if( type == inType )
		{
			*outData = dataPtr;
			*outLen  = dataLen;
			break;
		}
	}
	if( outNext ) *outNext = inSrc;
	return( err );
}

//===========================================================================================================================
//	TLV16GetNext
//===========================================================================================================================

OSStatus
	TLV16GetNext( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint16_t *			outType, 
		const uint8_t **	outData, 
		size_t *			outLen, 
		const uint8_t **	outNext )
{
	OSStatus			err;
	const uint8_t *		ptr;
	uint16_t			type;
	size_t				len;
	const uint8_t *		next;
	
	// 16-bit TLV's have the following format:
	//
	//		<2:type> <2:length> <length:data>
	
	check( inSrc <= inEnd );
	len = (size_t)( inEnd - inSrc );
	if( len < 4 )
	{
		if( len != 0 ) dlog( kLogLevelNotice, "### Short TLV header:\n%1.1H\n", PTR_LEN_LEN( inSrc, inEnd ) );
		err = kNotFoundErr;
		goto exit;
	}
	
	type = (uint16_t)( ( inSrc[ 0 ] << 8 ) | inSrc[ 1 ] );
	len  = (size_t)(   ( inSrc[ 2 ] << 8 ) | inSrc[ 3 ] );
	ptr  = inSrc + 4;
	next = ptr + len;
	if( ( next < inSrc ) || ( next > inEnd ) )
	{
		dlog( kLogLevelNotice, "### Bad TLV length:\n%1.1H\n", PTR_LEN_LEN( inSrc, inEnd ) );
		err = kUnderrunErr;
		goto exit;
	}
	
	*outType = type;
	*outData = ptr;
	*outLen  = len;
	if( outNext ) *outNext = next;
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#pragma mark == Debugging ==
#endif

