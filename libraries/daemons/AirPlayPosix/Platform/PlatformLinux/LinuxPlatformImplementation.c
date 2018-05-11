#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include "CommonServices.h"     // For OSStatus
#include "TickUtils.h"
#include "RandomNumberUtils.h"
#include "NetUtils.h"


char *	    GetProcessNameByPID( pid_t inPID, char *inNameBuf, size_t inMaxLen );


uint64_t    UpTicks( void )
{
	uint64_t			nanos;
	struct timespec		ts;
	
	ts.tv_sec  = 0;
	ts.tv_nsec = 0;
	clock_gettime( CLOCK_MONOTONIC, &ts );
	nanos = ts.tv_sec;
	nanos *= 1000000000;
	nanos += ts.tv_nsec;
	return( nanos );
}

uint64_t    UpTicksPerSecond( void )
{
    return(1000000000ULL);
}


//===========================================================================================================================
//	RandomBytes
//===========================================================================================================================

OSStatus	RandomBytes( void *inBuffer, size_t inByteCount )
{
	static int		sRandomFD = -1;
	uint8_t *		dst;
	ssize_t			n;
	
	while( sRandomFD < 0 )
	{
		sRandomFD = open( "/dev/urandom", O_RDONLY );
		if( sRandomFD < 0 ) { perror( "open urandom error: "); sleep( 1 ); continue; }
		break;
	}
	dst = (uint8_t *) inBuffer;
	while( inByteCount > 0 )
	{
		n = read( sRandomFD, dst, inByteCount );
		if( n < 0 ) { perror( "read urandom error: "); sleep( 1 ); continue; }
		dst += n;
		inByteCount -= n;
	}
	return( kNoErr );
}

OSStatus	GetPrimaryMACAddressPlatform( uint8_t outMAC[ 6 ] )
{
	OSStatus					err = kNotFoundErr;
    int                         ret;
	struct ifaddrs *			iaList;
	const struct ifaddrs *		ia;
	
	iaList = NULL;
	ret = getifaddrs( &iaList );
	if ( ret == -1)     goto exit;
	
	for( ia = iaList; ia; ia = ia->ifa_next )
	{
		const struct sockaddr_ll *		sll;
		
		if( !( ia->ifa_flags & IFF_UP ) )			continue; // Skip inactive.
		if( ia->ifa_flags & IFF_LOOPBACK )			continue; // Skip loopback.
		if( !ia->ifa_addr )							continue; // Skip no addr.
		if( ia->ifa_addr->sa_family != AF_PACKET )	continue; // Skip non-AF_PACKET.
		sll = (const struct sockaddr_ll *) ia->ifa_addr;
		if( sll->sll_halen != 6 )					continue; // Skip wrong length.
		
		memcpy( outMAC, sll->sll_addr, 6 );
		break;
	}

	if ( ia == NULL )   goto exit;

    err = kNoErr;
	
exit:
	if( iaList ) freeifaddrs( iaList );
	return( err );
}

OSStatus	GetInterfaceMACAddress( const char *inInterfaceName, uint8_t *outMACAddress )
{
	OSStatus			err = kUnknownErr;
	int     			sock;
    int                 ret;
	struct ifreq		ifr;
	
	sock = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sock == -1 ) goto exit;
	
	memset( &ifr, 0, sizeof( ifr ) );
	strncpy( ifr.ifr_name, inInterfaceName, sizeof( ifr.ifr_name ) );
	ret = ioctl( sock, SIOCGIFHWADDR, &ifr );
	if ( ret == -1 ) goto exit;
	
	memcpy( outMACAddress, &ifr.ifr_hwaddr.sa_data, 6 );


    err = kNoErr;
	
exit:
	close( sock );
	return( err );
}

char *	GetProcessNameByPID( pid_t inPID, char *inNameBuf, size_t inMaxLen )
{
	char		path[ PATH_MAX ];
	FILE *		file;
	char *		ptr;
	size_t		len;
	
	if( inMaxLen < 1 ) return( "" );
	
	snprintf( path, sizeof( path ), "/proc/%lld/cmdline", (long long) inPID );
	file = fopen( path, "r" );
	*path = '\0';
	if( file )
	{
		ptr = fgets( path, sizeof( path ), file );
		if( !ptr ) *path = '\0';
		fclose( file );
	}
	ptr = strrchr( path, '/' );
	ptr = ptr ? ( ptr + 1 ) : path;
	len = strlen( ptr );
	if( len >= inMaxLen ) len = inMaxLen - 1;
	memcpy( inNameBuf, ptr, len );
	inNameBuf[ len ] = '\0';
	return( inNameBuf );
}
