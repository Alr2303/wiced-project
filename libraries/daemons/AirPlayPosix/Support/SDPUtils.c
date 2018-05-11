/*
	Copyright (C) 2007-2012 Apple Inc. All Rights Reserved.
*/

#include <ctype.h>

#include "CommonServices.h"
#include "DebugServices.h"
#include "StringUtils.h"

#include "SDPUtils.h"

//===========================================================================================================================
//	SDPFindAttribute
//===========================================================================================================================

OSStatus
	SDPFindAttribute( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char *	inAttribute, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		const char **	outNextPtr )
{
	OSStatus			err;
	char				type;
	const char *		valuePtr;
	size_t				valueLen;
	
	while( ( err = SDPGetNext( inSrc, inEnd, &type, &valuePtr, &valueLen, &inSrc ) ) == kNoErr )
	{
		if( type == 'a' )
		{
			const char *		ptr;
			const char *		end;
			
			ptr = valuePtr;
			end = ptr + valueLen;
			while( ( ptr < end ) && ( *ptr != ':' ) ) ++ptr;
			if( ( ptr < end ) && ( strncmpx( valuePtr, (size_t)( ptr - valuePtr ), inAttribute ) == 0 ) )
			{
				ptr += 1;
				*outValuePtr = ptr;
				*outValueLen = (size_t)( end - ptr );
				break;
			}
		}
	}
	if( outNextPtr ) *outNextPtr = inSrc;
	return( err );
}

//===========================================================================================================================
//	SDPFindMediaSection
//===========================================================================================================================

OSStatus
	SDPFindMediaSection( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outMediaSectionPtr, 
		const char **	outMediaSectionEnd, 
		const char **	outMediaValuePtr, 
		size_t *		outMediaValueLen, 
		const char **	outNextPtr )
{
	OSStatus			err;
	char				type;
	const char *		valuePtr;
	size_t				valueLen;
	const char *		mediaSectionPtr;
	const char *		mediaValuePtr;
	size_t				mediaValueLen;
	
	// Find a media section.
	
	mediaSectionPtr = NULL;
	mediaValuePtr   = NULL;
	mediaValueLen   = 0;
	while( SDPGetNext( inSrc, inEnd, &type, &valuePtr, &valueLen, &inSrc ) == kNoErr )
	{
		if( type == 'm' )
		{
			mediaSectionPtr	= valuePtr - 2; // Skip back to "m=".
			mediaValuePtr	= valuePtr;
			mediaValueLen	= valueLen;
			break;
		}
	}
	require_action_quiet( mediaSectionPtr, exit, err = kNotFoundErr );
	
	// Find the end of the media section.
	
	while( SDPGetNext( inSrc, inEnd, &type, &valuePtr, &valueLen, &inSrc ) == kNoErr )
	{
		if( type == 'm' )
		{
			inSrc = valuePtr - 2; // Skip back to "m=" of the next section.
			break;
		}
	}
	
	if( outMediaSectionPtr )	*outMediaSectionPtr	= mediaSectionPtr;
	if( outMediaSectionEnd )	*outMediaSectionEnd	= inSrc;
	if( outMediaValuePtr )		*outMediaValuePtr	= mediaValuePtr;
	if( outMediaValueLen )		*outMediaValueLen	= mediaValueLen;
	if( outNextPtr )			*outNextPtr			= inSrc;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	SDPFindMediaSectionEx
//===========================================================================================================================

OSStatus
	SDPFindMediaSectionEx( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outMediaTypePtr, 
		size_t *		outMediaTypeLen, 
		const char **	outPortPtr, 
		size_t *		outPortLen, 
		const char **	outProtocolPtr, 
		size_t *		outProtocolLen, 
		const char **	outFormatPtr, 
		size_t *		outFormatLen, 
		const char **	outSubSectionsPtr, 
		size_t *		outSubSectionsLen, 
		const char **	outSrc )
{
	OSStatus			err;
	char				type;
	const char *		valuePtr;
	size_t				valueLen;
	const char *		mediaValuePtr;
	size_t				mediaValueLen;
	const char *		subSectionPtr;
	const char *		ptr;
	const char *		end;
	const char *		token;
	
	// Find a media section. Media sections start with "m=" and end with another media section or the end of data.
	// The following is an example of two media sections, "audio" and "video".
	//
	//		m=audio 21010 RTP/AVP 5\r\n
	//		c=IN IP4 224.0.1.11/127\r\n
	//		a=ptime:40\r\n
	//		m=video 61010 RTP/AVP 31\r\n
	//		c=IN IP4 224.0.1.12/127\r\n
	
	while( ( err = SDPGetNext( inSrc, inEnd, &type, &mediaValuePtr, &mediaValueLen, &inSrc ) ) == kNoErr )
	{
		if( type == 'm' )
		{
			break;
		}
	}
	require_noerr_quiet( err, exit );
	subSectionPtr = inSrc;
	
	// Find the end of the media section. Media sections end with another media section or the end of data.
	
	while( SDPGetNext( inSrc, inEnd, &type, &valuePtr, &valueLen, &inSrc ) == kNoErr )
	{
		if( type == 'm' )
		{
			inSrc = valuePtr - 2; // Skip back to "m=" of the next section.
			break;
		}
	}
	
	if( outSubSectionsPtr ) *outSubSectionsPtr	= subSectionPtr;
	if( outSubSectionsLen )	*outSubSectionsLen	= (size_t)( inSrc - subSectionPtr );
	if( outSrc )			*outSrc				= inSrc;
	
	// Parse details out of the media line (after the m= prefix). It has the following format:
	//
	//		<media> <port> <proto> <fmt> ...
	//
	//		"video 49170/2 RTP/AVP 31"
	
	ptr = mediaValuePtr;
	end = ptr + mediaValueLen;
	
	// Media Type
	
	while( ( ptr < end ) && isspace_safe( *ptr ) ) ++ptr;
	token = ptr;
	while( ( ptr < end ) && !isspace_safe( *ptr ) ) ++ptr;
	if( outMediaTypePtr ) *outMediaTypePtr = token;
	if( outMediaTypeLen ) *outMediaTypeLen = (size_t)( ptr - token );
	
	// Port
	
	while( ( ptr < end ) && isspace_safe( *ptr ) ) ++ptr;
	token = ptr;
	while( ( ptr < end ) && !isspace_safe( *ptr ) ) ++ptr;
	if( outPortPtr ) *outPortPtr = token;
	if( outPortLen ) *outPortLen = (size_t)( ptr - token );
	
	// Protocol
	
	while( ( ptr < end ) && isspace_safe( *ptr ) ) ++ptr;
	token = ptr;
	while( ( ptr < end ) && !isspace_safe( *ptr ) ) ++ptr;
	if( outProtocolPtr ) *outProtocolPtr = token;
	if( outProtocolLen ) *outProtocolLen = (size_t)( ptr - token );
	
	// Format
	
	while( ( ptr < end ) && isspace_safe( *ptr ) ) ++ptr;
	while( ( ptr < end ) && isspace_safe( end[ -1 ] ) ) --end;
	if( outFormatPtr ) *outFormatPtr = ptr;
	if( outFormatLen ) *outFormatLen = (size_t)( end - ptr );
	
exit:
	return( err );
}

//===========================================================================================================================
//	SDPFindParameter
//===========================================================================================================================

OSStatus
	SDPFindParameter( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char *	inName, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		const char **	outNextPtr )
{
	const char *		namePtr;
	size_t				nameLen;
	const char *		valuePtr;
	size_t				valueLen;
	
	while( SDPGetNextParameter( inSrc, inEnd, &namePtr, &nameLen, &valuePtr, &valueLen, &inSrc ) == kNoErr )
	{
		if( strncmpx( namePtr, nameLen, inName ) == 0 )
		{
			if( outValuePtr ) *outValuePtr = valuePtr;
			if( outValueLen ) *outValueLen = valueLen;
			if( outNextPtr )  *outNextPtr  = inSrc;
			return( kNoErr );
		}
	}
	if( outNextPtr ) *outNextPtr = inSrc;
	return( kNotFoundErr );
}

//===========================================================================================================================
//	SDPFindSessionSection
//===========================================================================================================================

OSStatus
	SDPFindSessionSection( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outSectionPtr, 
		const char **	outSectionEnd, 
		const char **	outNextPtr )
{
	const char *		sessionPtr;
	char				type;
	const char *		valuePtr;
	size_t				valueLen;
	
	// SDP session sections start at the beginning of the text and go until the next section (or the end of text).
	
	sessionPtr = inSrc;
	while( SDPGetNext( inSrc, inEnd, &type, &valuePtr, &valueLen, &inSrc ) == kNoErr )
	{
		if( type == 'm' )
		{
			inSrc = valuePtr - 2; // Skip to just before "m=" of the next section.
			break;
		}
	}
	if( outSectionPtr ) *outSectionPtr	= sessionPtr;
	if( outSectionEnd ) *outSectionEnd	= inSrc;
	if( outNextPtr )	*outNextPtr		= inSrc;
	return( kNoErr );
}

//===========================================================================================================================
//	SDPFindAttribute
//===========================================================================================================================

OSStatus
	SDPFindType( 
		const char *	inSrc, 
		const char *	inEnd, 
		char			inType, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		const char **	outNextPtr )
{
	OSStatus			err;
	char				type;
	const char *		valuePtr;
	size_t				valueLen;
	
	while( ( err = SDPGetNext( inSrc, inEnd, &type, &valuePtr, &valueLen, &inSrc ) ) == kNoErr )
	{
		if( type == inType )
		{
			if( outValuePtr ) *outValuePtr = valuePtr;
			if( outValueLen ) *outValueLen = valueLen;
			break;
		}
	}
	if( outNextPtr ) *outNextPtr = inSrc;
	return( err );
}

//===========================================================================================================================
//	SDPGetNext
//===========================================================================================================================

OSStatus
	SDPGetNext( 
		const char *	inSrc, 
		const char *	inEnd, 
		char *			outType, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		const char **	outNextPtr )
{
	OSStatus			err;
	char				type;
	const char *		val;
	size_t				len;
	char				c;
	
	require_action_quiet( inSrc < inEnd, exit, err = kNotFoundErr );
	
	// Parse a <type>=<value> line (e.g. "v=0\r\n").
	
	require_action( ( inEnd - inSrc ) >= 2, exit, err = kSizeErr );
	type = inSrc[ 0 ];
	require_action( inSrc[ 1 ] == '=', exit, err = kMalformedErr );
	inSrc += 2;
	
	for( val = inSrc; ( inSrc < inEnd ) && ( ( c = *inSrc ) != '\r' ) && ( c != '\n' ); ++inSrc ) {}
	len = (size_t)( inSrc - val );
		
	while( ( inSrc < inEnd ) && ( ( ( c = *inSrc ) == '\r' ) || ( c == '\n' ) ) ) ++inSrc;
	
	// Return results.
	
	*outType		= type;
	*outValuePtr	= val;
	*outValueLen	= len;
	*outNextPtr		= inSrc;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	SDPGetNextAttribute
//===========================================================================================================================

OSStatus
	SDPGetNextAttribute( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outNamePtr, 
		size_t *		outNameLen, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		const char **	outNextPtr )
{
	OSStatus			err;
	char				type;
	const char *		src;
	const char *		ptr;
	const char *		end;
	size_t				len;
	
	while( ( err = SDPGetNext( inSrc, inEnd, &type, &src, &len, &inSrc ) ) == kNoErr )
	{
		if( type == 'a' )
		{
			break;
		}
	}
	require_noerr_quiet( err, exit );
	
	ptr = src;
	end = src + len;
	while( ( src < end ) && ( *src != ':' ) ) ++src;
	
	if( outNamePtr )  *outNamePtr  = ptr;
	if( outNameLen )  *outNameLen  = (size_t)( src - ptr );
	
	if( src < end ) ++src;
	if( outValuePtr ) *outValuePtr = src;
	if( outValueLen ) *outValueLen = (size_t)( end - src );
	
exit:
	if( outNextPtr ) *outNextPtr = inSrc;
	return( err );
}

//===========================================================================================================================
//	SDPGetNextParameter
//===========================================================================================================================

OSStatus
	SDPGetNextParameter( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outNamePtr, 
		size_t *		outNameLen, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		const char **	outNextPtr )
{
	const char *		ptr;
	
	while( ( inSrc < inEnd ) && isspace_safe( *inSrc ) ) ++inSrc;
	if( inSrc == inEnd ) return( kNotFoundErr );
	
	for( ptr = inSrc; ( inSrc < inEnd ) && ( *inSrc != '=' ); ++inSrc ) {}
	if( outNamePtr ) *outNamePtr = ptr;
	if( outNameLen ) *outNameLen = (size_t)( inSrc - ptr );
	if( inSrc < inEnd ) ++inSrc;
	
	for( ptr = inSrc; ( inSrc < inEnd ) && ( *inSrc != ';' ); ++inSrc ) {}
	if( outValuePtr ) *outValuePtr = ptr;
	if( outValueLen ) *outValueLen = (size_t)( inSrc - ptr );
	
	if( outNextPtr ) *outNextPtr = ( inSrc < inEnd ) ? ( inSrc + 1 ) : inSrc;
	return( kNoErr );
}

//===========================================================================================================================
//	SDPScanFAttribute
//===========================================================================================================================

int
	SDPScanFAttribute( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outSrc, 
		const char *	inAttribute, 
		const char *	inFormat, 
		... )
{
	int			n;
	va_list		args;
	
	va_start( args, inFormat );
	n = SDPScanFAttributeV( inSrc, inEnd, outSrc, inAttribute, inFormat, args );
	va_end( args );
	return( n );
}

//===========================================================================================================================
//	SDPScanFAttributeV
//===========================================================================================================================

int
	SDPScanFAttributeV( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outSrc, 
		const char *	inAttribute, 
		const char *	inFormat, 
		va_list			inArgs )
{
	int					n;
	OSStatus			err;
	const char *		valuePtr;
	size_t				valueLen;
	
	n = 0;
	err = SDPFindAttribute( inSrc, inEnd, inAttribute, &valuePtr, &valueLen, outSrc );
	require_noerr_quiet( err, exit );
	
	n = VSNScanF( valuePtr, valueLen, inFormat, inArgs );
	
exit:
	return( n );
}

#if 0
#pragma mark -
#pragma mark == Debugging ==
#endif

