/*
 * DACP Client Button Monitor
 *
 * $Copyright (C) 2016 Cypress Semiconductors All Rights Reserved.$
 *
 * $Id: DACPClientButtonMonitor.c $
 */

#include "AirPlayReceiverServer.h"
#include "DACPCommon.h"

//===========================================================================================================================
//	Internal
//===========================================================================================================================

//===========================================================================================================================
//	Logging
//===========================================================================================================================

ulog_define( PlatformDACPClient, kLogLevelNotice, kLogFlags_Default, "PlatformDACPClient", NULL );
#define dacp_ulog( LEVEL, ... )	ulog( &log_category_from_name( PlatformDACPClient ), (LEVEL), __VA_ARGS__ )
#define dacp_nlog( fmt, ... )	dacp_ulog( kLogLevelNotice, fmt "\n", ##__VA_ARGS__ )
#define dacp_tlog( fmt, ... )	dacp_ulog( kLogLevelTrace, fmt "\n", ##__VA_ARGS__ )
#define dacp_elog( fmt, ... )	dacp_ulog( kLogLevelError, fmt "\n", ##__VA_ARGS__ )

//===========================================================================================================================
//	External Functions
//===========================================================================================================================

//===========================================================================================================================
//	PlatformDACPClientSendCommand
//==========================================================================================================================

int	PlatformDACPClientSendCommand( const char *commandStr )
{
	int			err;
	Boolean			b;

	if( gAirPlayReceiverServer == NULL )
	{
		dacp_tlog( "Airplay server not started- ignoring DACP command '%s'", commandStr );
		return -1;
	}

	b = AirPlayReceiverServerGetBoolean( gAirPlayReceiverServer, CFSTR( kAirPlayProperty_Playing ), NULL, &err );
	if ( err != kNoErr || !b )
	{
		dacp_tlog( "Not playing- ignoring DACP command '%s'", commandStr );
		return -1;
	}

	dacp_nlog( "Sending DACP command '%s'", commandStr );
	AirPlayReceiverServerSendDACPCommand( gAirPlayReceiverServer, commandStr );

	return 0;
}
