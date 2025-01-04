/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#pragma once
#if !defined(UTIL_H)
#define UTIL_H

#include "extdll.h"
#include "hull_types.h"
//
// Misc utility code
//
#include "activity.h"
#include "enginecallback.h"

#include "blood_types.h"
#include "fx_types.h"
#include "hull_sizes.h"
#include "template_property_types.h"
#include "util_shared.h"

#include <cstring>
#include <cctype>
inline void MESSAGE_BEGIN( int msg_dest, int msg_type, const float *pOrigin, entvars_t *ent );  // implementation later in this file

extern globalvars_t				*gpGlobals;

// Use this instead of ALLOC_STRING on constant strings
#define STRING(offset)		(const char *)(gpGlobals->pStringBase + (int)offset)

string_t ALLOC_STRING(const char* str);

#if !XASH_64BIT || CLIENT_DLL
#define MAKE_STRING(str)	((int)(long int)str - (int)(long int)STRING(0))
#else
static inline string_t MAKE_STRING(const char *szValue)
{
	long long ptrdiff = szValue - STRING(0);
	if( ptrdiff > INT_MAX || ptrdiff < INT_MIN )
		return ALLOC_STRING( szValue );
	else
		return (string_t)ptrdiff;
}
#endif

extern void ClearStringPool();
extern void ClearPrecachedModels();
extern void ClearPrecachedSounds();
extern void ReportPrecachedModels();
extern void ReportPrecachedSounds();
extern void AddMapBSPAsPrecachedModel();

extern void WRITE_COLOR(const Color& color);
extern void WRITE_COLOR(const Vector& color);
extern void WRITE_VECTOR(const Vector& vecSrc);
extern void WRITE_CIRCLE(const Vector& vecSrc, float radius);

inline edict_t *FIND_ENTITY_BY_CLASSNAME(edict_t *entStart, const char *pszName) 
{
	return FIND_ENTITY_BY_STRING(entStart, "classname", pszName);
}

inline edict_t *FIND_ENTITY_BY_TARGETNAME(edict_t *entStart, const char *pszName) 
{
	return FIND_ENTITY_BY_STRING(entStart, "targetname", pszName);
}

// for doing a reverse lookup. Say you have a door, and want to find its button.
inline edict_t *FIND_ENTITY_BY_TARGET(edict_t *entStart, const char *pszName) 
{
	return FIND_ENTITY_BY_STRING(entStart, "target", pszName);
}

// Keeps clutter down a bit, when writing key-value pairs
#define WRITEKEY_INT(pf, szKeyName, iKeyValue) ENGINE_FPRINTF(pf, "\"%s\" \"%d\"\n", szKeyName, iKeyValue)
#define WRITEKEY_FLOAT(pf, szKeyName, flKeyValue)								\
		ENGINE_FPRINTF(pf, "\"%s\" \"%f\"\n", szKeyName, flKeyValue)
#define WRITEKEY_STRING(pf, szKeyName, szKeyValue)								\
		ENGINE_FPRINTF(pf, "\"%s\" \"%s\"\n", szKeyName, szKeyValue)
#define WRITEKEY_VECTOR(pf, szKeyName, flX, flY, flZ)							\
		ENGINE_FPRINTF(pf, "\"%s\" \"%f %f %f\"\n", szKeyName, flX, flY, flZ)

// Makes these more explicit, and easier to find
#define FILE_GLOBAL static
#define DLL_GLOBAL

// More explicit than "int"
typedef int EOFFSET;

// Keeps clutter down a bit, when declaring external entity/global method prototypes
#define DECLARE_GLOBAL_METHOD(MethodName)  extern void DLLEXPORT MethodName( void )
#define GLOBAL_METHOD(funcname)					void DLLEXPORT funcname(void)

// This is the glue that hooks .MAP entity class names to our CPP classes
// The _declspec forces them to be exported by name so we can do a lookup with GetProcAddress()
// The function is used to intialize / allocate the object for the entity

#if CLIENT_DLL
#define LINK_ENTITY_TO_CLASS(mapClassName,DLLClassName)
#else // CLIENT_DLL
#define LINK_ENTITY_TO_CLASS(mapClassName,DLLClassName) extern "C" EXPORT void mapClassName( entvars_t *pev ); void mapClassName( entvars_t *pev ) { GetClassPtr( (DLLClassName *)pev ); }
#endif // CLIENT_DLL

//
// Conversion among the three types of "entity", including identity-conversions.
//
#if DEBUG
	extern edict_t *DBG_EntOfVars(const entvars_t *pev);
	inline edict_t *ENT(const entvars_t *pev)	{ return DBG_EntOfVars(pev); }
#else
	inline edict_t *ENT(const entvars_t *pev)	{ return pev->pContainingEntity; }
#endif
inline edict_t *ENT(edict_t *pent)		{ return pent; }
inline edict_t *ENT(EOFFSET eoffset)			{ return (*g_engfuncs.pfnPEntityOfEntOffset)(eoffset); }
inline EOFFSET OFFSET(EOFFSET eoffset)			{ return eoffset; }
inline EOFFSET OFFSET(const edict_t *pent)	
{ 
#if _DEBUG
	if ( !pent )
		ALERT( at_error, "Bad ent in OFFSET()\n" );
#endif
	return (*g_engfuncs.pfnEntOffsetOfPEntity)(pent); 
}
inline EOFFSET OFFSET(entvars_t *pev)				
{ 
#if _DEBUG
	if ( !pev )
		ALERT( at_error, "Bad pev in OFFSET()\n" );
#endif
	return OFFSET(ENT(pev)); 
}
inline entvars_t *VARS(entvars_t *pev)
{
	return pev;
}

inline entvars_t *VARS(edict_t *pent)			
{ 
	if ( !pent )
		return NULL;

	return &pent->v; 
}

inline entvars_t* VARS(EOFFSET eoffset)				{ return VARS(ENT(eoffset)); }
inline int	  ENTINDEX(edict_t *pEdict)			{ return (*g_engfuncs.pfnIndexOfEdict)(pEdict); }
inline edict_t* INDEXENT( int iEdictNum )		{ return (*g_engfuncs.pfnPEntityOfEntIndex)(iEdictNum); }
inline void MESSAGE_BEGIN( int msg_dest, int msg_type, const float *pOrigin, entvars_t *ent ) {
	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ENT(ent));
}

// Testing the three types of "entity" for nullity
#define eoNullEntity 0
inline bool FNullEnt(EOFFSET eoffset)			{ return eoffset == 0; }
inline bool FNullEnt(const edict_t* pent)	{ return pent == NULL || FNullEnt(OFFSET(pent)); }
inline bool FNullEnt(entvars_t* pev)				{ return pev == NULL || FNullEnt(OFFSET(pev)); }

// Testing strings for nullity
#define iStringNull 0
inline bool FStringNull(string_t iString)			{ return iString == iStringNull; }

#define cchMapNameMost 32

// Dot products for view cone checking
#define VIEW_FIELD_FULL		-1.0f // +-180 degrees
#define	VIEW_FIELD_WIDE		-0.7f // +-135 degrees 0.1 // +-85 degrees, used for full FOV checks
#define	VIEW_FIELD_NARROW	0.7f // +-45 degrees, more narrow check used to set up ranged attacks
#define	VIEW_FIELD_ULTRA_NARROW	0.9f // +-25 degrees, more narrow check used to set up ranged attacks

typedef enum 
{
	MONSTERSTATE_NONE = 0,
	MONSTERSTATE_IDLE,
	MONSTERSTATE_COMBAT,
	MONSTERSTATE_ALERT,
	MONSTERSTATE_HUNT,
	MONSTERSTATE_PRONE,
	MONSTERSTATE_SCRIPT,
	MONSTERSTATE_PLAYDEAD,
	MONSTERSTATE_DEAD
} MONSTERSTATE;



// Things that toggle (buttons/triggers/doors) need this
typedef enum
{
	TS_AT_TOP,
	TS_AT_BOTTOM,
	TS_GOING_UP,
	TS_GOING_DOWN
} TOGGLE_STATE;

typedef enum {
	NLE_PROHIBIT = 0,
	NLE_ALLOW,
	NLE_NEEDS_INPUT
} NODE_LINKENT;

// Misc useful
inline bool FStrEq(const char*sz1, const char*sz2)
{
	return (strcmp(sz1, sz2) == 0);
}

inline bool FClassnameIs(edict_t* pent, const char* szClassname)
{
	return FStrEq(STRING(VARS(pent)->classname), szClassname);
}

inline bool FClassnameIs(entvars_t* pev, const char* szClassname)
{
	return FStrEq(STRING(pev->classname), szClassname);
}

class CBaseEntity;

// Misc. Prototypes
extern void			UTIL_SetSize			(entvars_t* pev, const Vector &vecMin, const Vector &vecMax);
extern float		UTIL_VecToYaw			(const Vector &vec);
extern Vector		UTIL_VecToAngles		(const Vector &vec);

extern CBaseEntity	*UTIL_FindEntityInSphere(CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius);
extern CBaseEntity	*UTIL_FindEntityByString(CBaseEntity *pStartEntity, const char *szKeyword, const char *szValue );
extern CBaseEntity	*UTIL_FindEntityByClassname(CBaseEntity *pStartEntity, const char *szName );
extern CBaseEntity	*UTIL_FindEntityByTargetname(CBaseEntity *pStartEntity, const char *szName );
extern CBaseEntity	*UTIL_FindEntityByTargetname(CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pActivator );
extern CBaseEntity	*UTIL_FindEntityGeneric(const char *szName, Vector &vecSrc, float flRadius );
extern bool UTIL_HasClassnameOrTargetname(entvars_t *pevToucher, const char* name);

// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
extern CBaseEntity	*UTIL_PlayerByIndex( int playerIndex );

#define UTIL_EntitiesInPVS(pent)			(*g_engfuncs.pfnEntitiesInPVS)(pent)
extern void			UTIL_MakeVectors		(const Vector &vecAngles);

// Pass in an array of pointers and an array size, it fills the array and returns the number inserted
extern int			UTIL_MonstersInSphere( CBaseEntity **pList, int listMax, const Vector &center, float radius );
extern int			UTIL_EntitiesInBox( CBaseEntity **pList, int listMax, const Vector &mins, const Vector &maxs, int flagMask );

inline void UTIL_MakeVectorsPrivate( const Vector &vecAngles, float *p_vForward, float *p_vRight, float *p_vUp )
{
	g_engfuncs.pfnAngleVectors( vecAngles, p_vForward, p_vRight, p_vUp );
}

extern void			UTIL_MakeAimVectors		( const Vector &vecAngles ); // like MakeVectors, but assumes pitch isn't inverted
extern void			UTIL_MakeInvVectors		( const Vector &vec, globalvars_t *pgv );

extern void			UTIL_SetOrigin			( entvars_t* pev, const Vector &vecOrigin );
extern void			UTIL_EmitAmbientSound	( edict_t *entity, const Vector &vecOrigin, const char *samp, float vol, float attenuation, int fFlags, int pitch );
extern void			UTIL_ParticleEffect		( const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount );
extern void			UTIL_ScreenShake		( const Vector &center, float amplitude, float frequency, float duration, float radius );
extern void			UTIL_ScreenShakeAll		( const Vector &center, float amplitude, float frequency, float duration );
extern void			UTIL_ShowMessage		( const char *pString, CBaseEntity *pPlayer );
extern void			UTIL_ShowMessageAll		( const char *pString );
extern void			UTIL_ScreenFadeAll		( const Vector &color, float fadeTime, float holdTime, int alpha, int flags );
extern void			UTIL_ScreenFadeAll		( const Vector& fadeSource, const Vector &color, float fadeTime, float holdTime, int alpha, int flags );
extern void			UTIL_ScreenFade			( CBaseEntity *pEntity, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags );
extern void			UTIL_ScreenFade			( const Vector& fadeSource, CBaseEntity *pEntity, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags );

typedef enum
{
	ignore_monsters = 1,
	dont_ignore_monsters = 0,
	missile=2
} IGNORE_MONSTERS;

typedef enum
{
	ignore_glass = 1,
	dont_ignore_glass = 0
} IGNORE_GLASS;

extern void			UTIL_TraceLine			(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr);
extern void			UTIL_TraceLine			(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr);

extern void			UTIL_TraceHull			(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr);
extern TraceResult	UTIL_GetGlobalTrace		(void);
extern void			UTIL_TraceModel			(const Vector &vecStart, const Vector &vecEnd, int hullNumber, edict_t *pentModel, TraceResult *ptr);
extern Vector		UTIL_GetAimVector		(edict_t* pent, float flSpeed);
extern int			UTIL_PointContents		(const Vector &vec);

extern int			UTIL_IsMasterTriggered	(string_t sMaster, CBaseEntity *pActivator);
extern bool			UTIL_IsPlayerReference(const char* name);
extern bool			UTIL_TargetnameIsActivator( const char* targetName );
extern bool			UTIL_TargetnameIsActivator( string_t targetName );
extern void			UTIL_BloodStream( const Vector &origin, const Vector &direction, int color, int amount );
extern void			UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount );
extern Vector		UTIL_RandomBloodVector( void );
extern bool			UTIL_ShouldShowBlood( int bloodColor );
extern void			UTIL_BloodDecalTrace( TraceResult *pTrace, int bloodColor );
extern void			UTIL_DecalTrace( TraceResult *pTrace, int decalNumber );
extern void			UTIL_PlayerDecalTrace( TraceResult *pTrace, int playernum, int decalNumber, bool bIsCustom );
extern void			UTIL_GunshotDecalTrace( TraceResult *pTrace, int decalNumber );
extern void			UTIL_Sparks( const Vector &position );
extern void			UTIL_SparkShower( const Vector &position, const SparkEffectParams& params );
extern void			UTIL_Ricochet( const Vector &position, float scale );
extern Vector		UTIL_StringToVector( const char *str );
extern void			UTIL_StringToRandomVector( float *pVector, const char *pString );
extern void			UTIL_StringToIntArray( int *pVector, int count, const char *pString );
extern void			UTIL_StringToCharArray( char *pVector, int count, const char *pString );
extern Vector		UTIL_ClampVectorToBox( const Vector &input, const Vector &clampSize );

extern char			*UTIL_VarArgs( const char *format, ... );
extern void			UTIL_Remove( CBaseEntity *pEntity );
extern bool			UTIL_IsValidEntity( edict_t *pent );
extern bool			UTIL_TeamsMatch( const char *pTeamName1, const char *pTeamName2 );

// Use for ease-in, ease-out style interpolation (accel/decel)
extern float		UTIL_SplineFraction( float value, float scale );

// Search for water transition along a vertical line
extern float		UTIL_WaterLevel( const Vector &position, float minz, float maxz );
extern void			UTIL_Bubbles( Vector mins, Vector maxs, int count );
extern void			UTIL_BubbleTrail( Vector from, Vector to, int count );

struct EntityOverrides
{
	string_t model = iStringNull;
	string_t entTemplate = iStringNull;
	string_t ownerEntTemplate = iStringNull;
	string_t soundList = iStringNull;
};

// allows precacheing of other entities
extern void			UTIL_PrecacheOther(const char *szClassname, EntityOverrides entityOverrides= EntityOverrides());
extern bool			UTIL_PrecacheMonster(const char *szClassname, bool reverseRelationship , Vector *vecMin = NULL, Vector *vecMax = NULL, EntityOverrides entityOverrides= EntityOverrides());

// prints a message to each client
extern void			UTIL_ClientPrintAll( int msg_dest, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );

inline void			UTIL_CenterPrintAll( const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL ) 
{
	UTIL_ClientPrintAll( HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

class CBasePlayerWeapon;
class CBasePlayer;

// prints messages through the HUD
extern void ClientPrint( entvars_t *client, int msg_dest, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );

// prints a message to the HUD say (chat)
extern void			UTIL_SayText( const char *pText, CBaseEntity *pEntity );
extern void			UTIL_SayTextAll( const char *pText, CBaseEntity *pEntity );

typedef struct hudtextparms_s
{
	float		x;
	float		y;
	int			effect;
	byte		r1, g1, b1, a1;
	byte		r2, g2, b2, a2;
	float		fadeinTime;
	float		fadeoutTime;
	float		holdTime;
	float		fxTime;
	int			channel;
} hudtextparms_t;

// prints as transparent 'title' to the HUD
extern void			UTIL_HudMessageAll( const hudtextparms_t &textparms, const char *pMessage );
extern void			UTIL_HudMessage( CBaseEntity *pEntity, const hudtextparms_t &textparms, const char *pMessage );

// show captions
extern void UTIL_ShowCaption(const char* messageId, int holdTime, bool radio);

// for handy use with ClientPrint params
extern char *UTIL_dtos1( int d );
extern char *UTIL_dtos2( int d );
extern char *UTIL_dtos3( int d );
extern char *UTIL_dtos4( int d );

// Writes message to console with timestamp and FragLog header.
extern void			UTIL_LogPrintf( const char *fmt, ... );

// Sorta like FInViewCone, but for nonmonsters. 
extern float UTIL_DotPoints ( const Vector &vecSrc, const Vector &vecCheck, const Vector &vecDir );

extern void UTIL_StripToken( const char *pKey, char *pDest, int nLen );// for redundant keynames

extern void EntvarsKeyvalue( entvars_t *pev, KeyValueData *pkvd );
extern int ReadEntvarKeyvalue(entvars_t* pev, const char* keyName, int* offset, float* outFloat, int* outInteger, Vector* outVector, string_t* outString);

// Misc functions
extern void SetMovedir(entvars_t* pev);
extern Vector VecBModelOrigin( entvars_t* pevBModel );
extern int BuildChangeList( LEVELLIST *pLevelList, int maxList );

//
// How did I ever live without ASSERT?
//
#if	DEBUG
void DBG_AssertFunction(bool fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage);
#define ASSERT(f)		DBG_AssertFunction(f, #f, __FILE__, __LINE__, NULL)
#define ASSERTSZ(f, sz)	DBG_AssertFunction(f, #f, __FILE__, __LINE__, sz)
#else	// !DEBUG
#define ASSERT(f)
#define ASSERTSZ(f, sz)
#endif	// !DEBUG

extern DLL_GLOBAL const Vector g_vecZero;

#define AMBIENT_SOUND_STATIC			0	// medium radius attenuation
#define AMBIENT_SOUND_EVERYWHERE		1
#define AMBIENT_SOUND_SMALLRADIUS		2
#define AMBIENT_SOUND_MEDIUMRADIUS		4
#define AMBIENT_SOUND_LARGERADIUS		8
#define AMBIENT_SOUND_START_SILENT		16
#define AMBIENT_SOUND_NOT_LOOPING		32

#define SPEAKER_START_SILENT			1	// wait for trigger 'on' to start announcements

#define SND_SPAWNING		(1<<8)		// duplicated in protocol.h we're spawing, used in some cases for ambients 
#define SND_STOP			(1<<5)		// duplicated in protocol.h stop sound
#define SND_CHANGE_VOL		(1<<6)		// duplicated in protocol.h change sound vol
#define SND_CHANGE_PITCH	(1<<7)		// duplicated in protocol.h change sound pitch

#define SND_DONT_REPORT_MISSING (1 << 16) // game library level flag, not part of a protocol

#define	LFO_SQUARE			1
#define LFO_TRIANGLE		2
#define LFO_RANDOM			3

// func_rotating
#define SF_BRUSH_ROTATE_Y_AXIS		0
#define SF_BRUSH_ROTATE_INSTANT		1
#define SF_BRUSH_ROTATE_BACKWARDS	2
#define SF_BRUSH_ROTATE_Z_AXIS		4
#define SF_BRUSH_ROTATE_X_AXIS		8
#define SF_PENDULUM_AUTO_RETURN		16
#define	SF_PENDULUM_PASSABLE		32

#define SF_BRUSH_ROTATE_SMALLRADIUS	128
#define SF_BRUSH_ROTATE_MEDIUMRADIUS 256
#define SF_BRUSH_ROTATE_LARGERADIUS 512

#define PUSH_BLOCK_ONLY_X	1
#define PUSH_BLOCK_ONLY_Y	2

#define VEC_VIEW			Vector( 0, 0, 28 )

#define SVC_TEMPENTITY		23
#define SVC_INTERMISSION	30
#define SVC_CDTRACK			32
#define SVC_WEAPONANIM		35
#define SVC_ROOMTYPE		37
#define	SVC_DIRECTOR		51

// triggers
#define	SF_TRIGGER_ALLOWMONSTERS	1// monsters allowed to fire this trigger
#define	SF_TRIGGER_NOCLIENTS		2// players not allowed to fire this trigger
#define SF_TRIGGER_PUSHABLES		4// only pushables can fire this trigger
#define SF_TRIGGER_EVERYTHING		8// can be triggered by anything else

#define SF_LIGHT_START_OFF		1

#define SPAWNFLAG_NOMESSAGE	1
#define SPAWNFLAG_NOTOUCH	1
#define SPAWNFLAG_DROIDONLY	4

#define SPAWNFLAG_USEONLY	1		// can't be touched, must be used (buttons)

#define TELE_PLAYER_ONLY	1
#define TELE_SILENT			2

// Sound Utilities

// sentence groups
#define CBSENTENCENAME_MAX 16

#define CVOXFILESENTENCEMAX_GOLDSOURCE_LEGACY 1536
#define CVOXFILESENTENCEMAX_GOLDSOURCE_ANNIVERSARY_25 2048
#define CVOXFILESENTENCEMAX_XASH3D 4096
#define CVOXFILESENTENCEMAX CVOXFILESENTENCEMAX_XASH3D // max number of sentences in game. NOTE: this must match
							// CVOXFILESENTENCEMAX in engine\sound.h!!!

extern char gszallsentencenames[CVOXFILESENTENCEMAX][CBSENTENCENAME_MAX];
extern int gcallsentences;

int USENTENCEG_Pick(int isentenceg, char *szfound);
int USENTENCEG_PickSequential(int isentenceg, char *szfound, int ipick, bool freset);
void USENTENCEG_InitLRU(unsigned char *plru, int count);

void SENTENCEG_Init();
void SENTENCEG_Stop(edict_t *entity, int isentenceg, int ipick);
int SENTENCEG_PlayRndI(edict_t *entity, int isentenceg, float volume, float attenuation, int flags, int pitch);
int SENTENCEG_PlayRndSz(edict_t *entity, const char *szrootname, float volume, float attenuation, int flags, int pitch);
int SENTENCEG_PlayRndSzSub(edict_t *entity, const char *szrootname, float volume, float attenuation, int flags, int pitch, int holdTime);
int SENTENCEG_PlaySequentialSz(edict_t *entity, const char *szrootname, float volume, float attenuation, int flags, int pitch, int ipick, bool freset);
int SENTENCEG_GetIndex(const char *szrootname);
int SENTENCEG_Lookup(const char *sample, char *sentencenum);

void TEXTURETYPE_Init();
char TEXTURETYPE_Find(char *name);
float TEXTURETYPE_PlaySound(TraceResult *ptr,  Vector vecSrc, Vector vecEnd, int iBulletType);

// NOTE: use EMIT_SOUND_DYN to set the pitch of a sound. Pitch of 100
// is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
// down to 1 is a lower pitch.   150 to 70 is the realistic range.
// EMIT_SOUND_DYN with pitch != 100 should be used sparingly, as it's not quite as
// fast as EMIT_SOUND (the pitchshift mixer is not native coded).

bool EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation,
						   int flags, int pitch);

bool EMIT_SOUND_DYN_SUB(edict_t *entity, int channel, const char *sample, float volume, float attenuation,
						   int flags, int pitch, int holdTime);

inline void EMIT_SOUND(edict_t *entity, int channel, const char *sample, float volume, float attenuation)
{
	EMIT_SOUND_DYN(entity, channel, sample, volume, attenuation, 0, PITCH_NORM);
}

inline void STOP_SOUND(edict_t *entity, int channel, const char *sample)
{
	EMIT_SOUND_DYN(entity, channel, sample, 0, 0, SND_STOP, PITCH_NORM);
}

void EMIT_SOUND_SUIT(edict_t *entity, const char *sample);
void EMIT_GROUPID_SUIT(edict_t *entity, int isentenceg);
void EMIT_GROUPNAME_SUIT(edict_t *entity, const char *groupname);

#define PRECACHE_SOUND_ARRAY( a ) \
	{ for (int i = 0; i < (int)ARRAYSIZE( a ); i++ ) PRECACHE_SOUND( a[i] ); }

#define EMIT_SOUND_ARRAY_DYN( chan, array ) \
	EmitSoundDyn ( chan , array [ RANDOM_LONG(0,ARRAYSIZE( array )-1) ], 1.0, ATTN_NORM, 0, RANDOM_LONG(95,105) );

#define RANDOM_SOUND_ARRAY( array ) (array) [ RANDOM_LONG(0,ARRAYSIZE( (array) )-1) ]

#define PLAYBACK_EVENT( flags, who, index ) PLAYBACK_EVENT_FULL( flags, who, index, 0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );
#define PLAYBACK_EVENT_DELAY( flags, who, index, delay ) PLAYBACK_EVENT_FULL( flags, who, index, delay, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

#define GROUP_OP_AND	0
#define GROUP_OP_NAND	1

extern int g_groupmask;
extern int g_groupop;

class UTIL_GroupTrace
{
public:
	UTIL_GroupTrace( int groupmask, int op );
	~UTIL_GroupTrace( void );

private:
	int m_oldgroupmask, m_oldgroupop;
};

void UTIL_SetGroupTrace( int groupmask, int op );
void UTIL_UnsetGroupTrace( void );

float UTIL_WeaponTimeBase( void );
void UTIL_CleanSpawnPoint( Vector origin, float dist );
void UTIL_DynamicLight( const Vector& vecSrc, float flRadius, byte r, byte g, byte b, float flTime, float flDecay );
void UTIL_MuzzleLight( const Vector& vecSrc );

char *memfgets( byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize );

float RandomizeNumberFromRange(const FloatRange& r);
float RandomizeNumberFromRange(float minF, float maxF);
int RandomizeNumberFromRange(const IntRange& r);
int RandomizeNumberFromRange(int minI, int maxI);

inline bool LineOfSightSeparatedByWaterSurface(int lookerWaterlevel, int targetWaterlevel)
{
	return (lookerWaterlevel != WL_Eyes && targetWaterlevel == WL_Eyes) || (lookerWaterlevel == WL_Eyes && targetWaterlevel == WL_NotInWater);
}

void ReportAIStateByClassname(const char* name);

inline Vector VectorFromColor(const Color& color) {
	return Vector(color.r, color.g, color.b);
}

//TODO: move this to movewith.h later
extern void			UTIL_AssignOrigin		( CBaseEntity* pEntity, const Vector vecOrigin );
extern void			UTIL_AssignOrigin		( CBaseEntity* pEntity, const Vector vecOrigin, bool bInitiator );
extern void			UTIL_SetVelocity		( CBaseEntity *pEnt,	const Vector vecSet );
extern void			UTIL_SetAngles			( CBaseEntity* pEntity, const Vector vecAngles );
extern void			UTIL_SetAngles			( CBaseEntity* pEntity, const Vector vecAngles, bool bInitiator );
extern void			UTIL_SetAvelocity		( CBaseEntity *pEnt, const Vector vecSet );

#endif // UTIL_H
