#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/prctl.h>
#include <nuttx/config.h>

#include "CommonServices.h"     // For OSStatus
#include "TickUtils.h"
#include "RandomNumberUtils.h"
#include "NetUtils.h"


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
		sRandomFD = open( "/dev/random", O_RDONLY );
		if( sRandomFD < 0 ) { perror( "open random device error: "); sleep( 1 ); continue; }
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


OSStatus	GetInterfaceMACAddress( const char *inInterfaceName, uint8_t *outMACAddress )
{
	OSStatus			err = kUnknownErr;
	int     			sock;
    int                 ret;
	struct ifreq		ifr;
	
	sock = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sock == -1 ) goto exit;
	
	memset( &ifr, 0, sizeof( ifr ) );
	strncpy( ifr.ifr_name, inInterfaceName, sizeof( ifr.ifr_name ) - 1);
	ret = ioctl( sock, SIOCGIFHWADDR, &ifr );
	if ( ret == -1 ) goto exit;
	
	memcpy( outMACAddress, &ifr.ifr_hwaddr.sa_data, 6 );


    err = kNoErr;
	
exit:
	close( sock );
	return( err );
}

OSStatus    GetPrimaryMACAddressPlatform( uint8_t outMAC[ 6 ] )
{
    OSStatus                    err = kNotFoundErr;
    int                         ret;
    struct ifaddrs *            iaList;
    const struct ifaddrs *      ia;
    uint8_t *out = outMAC;

    iaList = NULL;
    ret = getifaddrs( &iaList );
    if ( ret == -1)     goto exit;

    for( ia = iaList; ia; ia = ia->ifa_next )
    {
        if( !( ia->ifa_flags & IFF_UP ) )           continue; // Skip inactive.
        if( ia->ifa_flags & IFF_LOOPBACK )          continue; // skip loopback interface
        if( !ia->ifa_addr )                         continue; // Skip no addr.
        if( !ia->ifa_name )                         continue; // Skip no name.
        err = GetInterfaceMACAddress(ia->ifa_name, out);
        break;
    }

exit:
    if( iaList ) freeifaddrs( iaList );
    return( err );
}

char *	GetProcessNameByPID( pid_t inPID, char *inNameBuf, size_t inMaxLen )
{
	int ret;

	if (inNameBuf == NULL || inMaxLen < CONFIG_TASK_NAME_SIZE)
	  {
	    return NULL;
	  }

	inNameBuf[0]='\0';
	
	ret = prctl(PR_GET_NAME, inNameBuf, inPID );
	if (ret < 0 )
	  {
	    return NULL;
	  }
	else
	  {
        return inNameBuf;
	  }
}
