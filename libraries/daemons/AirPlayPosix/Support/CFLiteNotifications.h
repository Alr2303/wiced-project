/*
	Copyright (C) 2008-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__CFLiteNotifications_h__
#define	__CFLiteNotifications_h__

#include "CFCompat.h"
#include "CommonServices.h"
#include "DebugServices.h"

#ifdef __cplusplus
extern "C" {
#endif

//===========================================================================================================================
//	Constants
//===========================================================================================================================

typedef enum
{
	CFNotificationSuspensionBehaviorDrop				= 1,
	CFNotificationSuspensionBehaviorCoalesce			= 2,
	CFNotificationSuspensionBehaviorHold				= 3,
	CFNotificationSuspensionBehaviorDeliverImmediately	= 4
	
}	CFNotificationSuspensionBehavior;

enum
{
	kCFNotificationDeliverImmediately	= ( 1 << 0 ),
	kCFNotificationPostToAllSessions	= ( 1 << 1 )
};

//===========================================================================================================================
//	Types
//===========================================================================================================================

typedef struct CFNotificationCenter *		CFNotificationCenterRef;

typedef void
	( *CFNotificationCallback )( 
		CFNotificationCenterRef inCenter, 
		void *					inObserver, 
		CFStringRef				inName, 
		const void *			inObject, 
		CFDictionaryRef			inUserInfo );

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

OSStatus	CFNotificationCenter_Initialize( void );
void		CFNotificationCenter_Finalize( void );

CFNotificationCenterRef	CFNotificationCenterGetLocalCenter( void );
CFNotificationCenterRef	CFNotificationCenterGetDistributedCenter( void );

void
	CFNotificationCenterAddObserver(
		CFNotificationCenterRef 			inCenter, 
		const void *						inObserver, 
		CFNotificationCallback				inCallBack, 
		CFStringRef							inName, 
		const void *						inObject, 
		CFNotificationSuspensionBehavior	inSuspensionBehavior );

void
	CFNotificationCenterRemoveObserver(
		CFNotificationCenterRef	inCenter, 
		const void *			inObserver, 
		CFStringRef				inName, 
		const void *			inObject );

void	CFNotificationCenterRemoveEveryObserver( CFNotificationCenterRef inCenter, const void *inObserver );

void
	CFNotificationCenterPostNotification( 
		CFNotificationCenterRef inCenter, 
		CFStringRef				inName, 
		const void *			inObject, 
		CFDictionaryRef			inUserInfo, 
		Boolean					inDeliverImmediately );

void
	CFNotificationCenterPostNotificationWithOptions(
		CFNotificationCenterRef	inCenter, 
		CFStringRef				inName, 
		const void *			inObject, 
		CFDictionaryRef			inUserInfo, 
		CFOptionFlags			inOptions );

#if 0
#pragma mark == Debugging ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFLiteNotifications_Test
	@abstract	Unit test.
*/

OSStatus	CFLiteNotifications_Test( void );

#ifdef __cplusplus
}
#endif

#endif // __CFLiteNotifications_h__
