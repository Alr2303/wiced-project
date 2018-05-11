/*
	Copyright (C) 2010 Apple Inc. All Rights Reserved.
*/

#ifndef	__DACPCommonDotH__
#define	__DACPCommonDotH__

//===========================================================================================================================
//	Constants
//===========================================================================================================================

#define kDACPBonjourServiceNamePrefix			"iTunes_Ctrl_"
#define kDACPBonjourServiceType					"_dacp._tcp"
#define kDACPBonjourServiceDomain				"local."

#define kDACPCommandStr_BeginFastFwd			"beginff"
#define kDACPCommandStr_BeginRewind				"beginrew"
#define kDACPCommandStr_DeviceVolumeLegacy		"devicevolume=" // OBSOLETE: Remove after Denon updates their firmware.
#define kDACPCommandStr_GetProperty				"getproperty"
#define kDACPCommandStr_MuteToggle				"mutetoggle"
#define kDACPCommandStr_NextChapter				"nextchapter"
#define kDACPCommandStr_NextContainer			"nextcontainer"
#define kDACPCommandStr_NextGroup				"nextgroup"
#define kDACPCommandStr_NextItem				"nextitem"
#define kDACPCommandStr_Pause					"pause"
#define kDACPCommandStr_Play					"play"
#define kDACPCommandStr_PlayPause				"playpause"
#define kDACPCommandStr_PlayResume				"playresume"
#define kDACPCommandStr_PlaySpecified			"playspec"
#define kDACPCommandStr_PrevChapter				"prevchapter"
#define kDACPCommandStr_PrevContainer			"prevcontainer"
#define kDACPCommandStr_PrevGroup				"prevgroup"
#define kDACPCommandStr_PrevItem				"previtem"
#define kDACPCommandStr_RepeatAdvance			"repeatadv"
#define kDACPCommandStr_RestartItem				"restartitem"
#define kDACPCommandStr_SetProperty				"setproperty?"
#define kDACPCommandStr_ShuffleSongs			"shufflesongs"
#define kDACPCommandStr_ShuffleToggle			"shuffletoggle"
#define kDACPCommandStr_Stop					"stop"
#define kDACPCommandStr_VolumeDown				"volumedown"
#define kDACPCommandStr_VolumeUp				"volumeup"

// Properties

#define kDACPProperty_DevicePreventPlayback		"dmcp.device-prevent-playback"
#define kDACPProperty_DeviceVolume				"dmcp.device-volume"

#endif // __DACPCommonDotH__
