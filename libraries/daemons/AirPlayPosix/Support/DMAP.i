/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/

// WARNING: This is a special include-only file that is intended to be included by other files and is not standalone.

#if 0
#pragma mark == Macros ==
#endif

#if( DMAP_DECLARE_CONTENT_CODE_CODES )
	#define DMAP_TABLE_BEGIN()									enum {
	#define DMAP_TABLE_END()									kDMAPContentCodeEnd };
	#define DMAP_ENTRY( CODE_SYMBOL, CODE, NAME, TYPE )			CODE_SYMBOL = CODE, 
#elif( DMAP_DECLARE_CONTENT_CODE_NAMES )
	#define DMAP_TABLE_BEGIN()
	#define DMAP_TABLE_END()
	#define DMAP_ENTRY( CODE_SYMBOL, CODE, NAME, TYPE )			extern const char CODE_SYMBOL ## NameStr[];
#elif( DMAP_DEFINE_CONTENT_CODE_NAMES )
	#define DMAP_TABLE_BEGIN()
	#define DMAP_TABLE_END()
	#define DMAP_ENTRY( CODE_SYMBOL, CODE, NAME, TYPE )			const char CODE_SYMBOL ## NameStr[] = NAME;
#elif( DMAP_DECLARE_CONTENT_CODE_TABLE )
	#define DMAP_TABLE_BEGIN()									extern const DMAPContentCodeEntry	kDMAPContentCodeTable[];
	#define DMAP_TABLE_END()
	#define DMAP_ENTRY( CODE_SYMBOL, CODE, NAME, TYPE )
#elif( DMAP_DEFINE_CONTENT_CODE_TABLE )
	#if( DMAP_EXPORT_CONTENT_CODE_SYMBOLIC_NAMES )
		#define DMAP_TABLE_BEGIN()								const DMAPContentCodeEntry		kDMAPContentCodeTable[] = {
		#define DMAP_TABLE_END()								{ kDMAPUndefinedCode, "", kDMAPCodeValueType_Undefined, "", "" } };
		#define DMAP_ENTRY( CODE_SYMBOL, CODE, NAME, TYPE )		{ CODE, NAME, TYPE, # CODE_SYMBOL, # CODE_SYMBOL "NameStr" }, 
	#else
		#define DMAP_TABLE_BEGIN()								const DMAPContentCodeEntry		kDMAPContentCodeTable[] = {
		#define DMAP_TABLE_END()								{ kDMAPUndefinedCode, "", kDMAPCodeValueType_Undefined } };
		#define DMAP_ENTRY( CODE_SYMBOL, CODE, NAME, TYPE )		{ CODE, NAME, TYPE }, 
	#endif
#endif

DMAP_TABLE_BEGIN()

DMAP_ENTRY( kDAAPSongAlbumCode,						0x6173616C /* 'asal' */, "daap.songalbum",					kDMAPCodeValueType_UTF8Characters )
DMAP_ENTRY( kDAAPSongArtistCode,					0x61736172 /* 'asar' */, "daap.songartist",					kDMAPCodeValueType_UTF8Characters )
DMAP_ENTRY( kDAAPSongComposerCode,					0x61736370 /* 'ascp' */, "daap.songcomposer",				kDMAPCodeValueType_UTF8Characters )
DMAP_ENTRY( kDAAPSongDataKindCode,					0x6173646B /* 'asdk' */, "daap.songdatakind",				kDMAPCodeValueType_UInt8 )
DMAP_ENTRY( kDAAPSongDiscCountCode,					0x61736463 /* 'asdc' */, "daap.songdisccount",				kDMAPCodeValueType_UInt16 )
DMAP_ENTRY( kDAAPSongDiscNumberCode,				0x6173646E /* 'asdn' */, "daap.songdiscnumber",				kDMAPCodeValueType_UInt16 )
DMAP_ENTRY( kDAAPSongGenreCode,						0x6173676E /* 'asgn' */, "daap.songgenre",					kDMAPCodeValueType_UTF8Characters )
DMAP_ENTRY( kDAAPSongTimeCode,						0x6173746D /* 'astm' */, "daap.songtime",					kDMAPCodeValueType_UInt32 )
DMAP_ENTRY( kDAAPSongTrackCountCode,				0x61737463 /* 'astc' */, "daap.songtrackcount",				kDMAPCodeValueType_UInt16 )
DMAP_ENTRY( kDAAPSongTrackNumberCode,				0x6173746E /* 'astn' */, "daap.songtracknumber",			kDMAPCodeValueType_UInt16 )
DMAP_ENTRY( kDACPPlayerStateCode,					0x63617073 /* 'caps' */, "dacp.playerstate",				kDMAPCodeValueType_UInt8 )
DMAP_ENTRY( kDMAPContentCodesNameCode,				0x6D636E61 /* 'mcna' */, "dmap.contentcodesname",			kDMAPCodeValueType_UTF8Characters )
DMAP_ENTRY( kDMAPContentCodesNumberCode,			0x6D636E6D /* 'mcnm' */, "dmap.contentcodesnumber",			kDMAPCodeValueType_UInt32 )
DMAP_ENTRY( kDMAPContentCodesTypeCode,				0x6D637479 /* 'mcty' */, "dmap.contentcodestype",			kDMAPCodeValueType_UInt16 )
DMAP_ENTRY( kDMAPDictionaryCollectionCode,			0x6D64636C /* 'mdcl' */, "dmap.dictionary",					kDMAPCodeValueType_DictionaryHeader )
DMAP_ENTRY( kDMAPItemIDCode, 						0x6D696964 /* 'miid' */, "dmap.itemid",						kDMAPCodeValueType_UInt32 )
DMAP_ENTRY( kDMAPItemNameCode, 						0x6D696E6D /* 'minm' */, "dmap.itemname",					kDMAPCodeValueType_UTF8Characters )
DMAP_ENTRY( kDMAPListingItemCode,					0x6D6C6974 /* 'mlit' */, "dmap.listingitem",				kDMAPCodeValueType_DictionaryHeader )
DMAP_ENTRY( kDMAPPersistentIDCode,					0x6D706572 /* 'mper' */, "dmap.persistentid",				kDMAPCodeValueType_UInt64 )
DMAP_ENTRY( kDMAPServerInfoResponseCode, 			0x6D737276 /* 'msrv' */, "dmap.serverinforesponse",			kDMAPCodeValueType_DictionaryHeader )
DMAP_ENTRY( kDMAPStatusCode, 						0x6D737474 /* 'mstt' */, "dmap.status",						kDMAPCodeValueType_UInt32 )
DMAP_ENTRY( kDMAPStatusStringCode,					0x6D737473 /* 'msts' */, "dmap.statusstring",				kDMAPCodeValueType_UTF8Characters )
DMAP_ENTRY( kExtDAAPITMSSongIDCode,					0x61655349 /* 'aeSI' */, "com.apple.itunes.itms-songid",	kDMAPCodeValueType_UInt32 )

DMAP_TABLE_END()

// #undef macros so this file can be re-included again with different options.

#undef DMAP_TABLE_BEGIN
#undef DMAP_TABLE_END
#undef DMAP_ENTRY

#undef DMAP_DECLARE_CONTENT_CODE_CODES
#undef DMAP_DECLARE_CONTENT_CODE_NAMES
#undef DMAP_DEFINE_CONTENT_CODE_NAMES
#undef DMAP_DECLARE_CONTENT_CODE_TABLE
#undef DMAP_DEFINE_CONTENT_CODE_TABLE
