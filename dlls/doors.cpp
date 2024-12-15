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
/*

===== doors.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"
#include "game.h"
#include "soundradius.h"

#define noiseMoving noise1
#define noiseArrived noise2

class CBaseDoor : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	virtual void KeyValue( KeyValueData *pkvd );
	float InputByMonster(CBaseMonster* pMonster);
	NODE_LINKENT HandleLinkEnt(int afCapMask, bool nodeQueryStatic);
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Blocked( CBaseEntity *pOther );

	virtual int ObjectCaps( void ) 
	{
		int objectCaps = ( CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION );
		if( pev->spawnflags & SF_ITEM_USE_ONLY ) {
			objectCaps |= FCAP_IMPULSE_USE;
			if (m_iDirectUse == PLAYER_USE_POLICY_DIRECT)
				objectCaps |= FCAP_ONLYDIRECT_USE;
			else if (m_iDirectUse == PLAYER_USE_POLICY_VISIBLE)
				objectCaps |= FCAP_ONLYVISIBLE_USE;
		}
		return objectCaps;
	};
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual void SetToggleState( int state );

	// used to selectivly override defaults
	void EXPORT DoorTouch( CBaseEntity *pOther );

	// local functions
	int DoorActivate();
	void EXPORT DoorGoUp( void );
	void EXPORT DoorGoDown( void );
	void EXPORT DoorHitTop( void );
	void EXPORT DoorHitBottom( void );

	BYTE m_bHealthValue;// some doors are medi-kit doors, they give players health

	BYTE m_bMoveSnd;			// sound a door makes while moving
	BYTE m_bStopSnd;			// sound a door makes when it stops

	locksound_t m_ls;			// door lock sounds

	BYTE m_bLockedSound;		// ordinals from entity selection
	BYTE m_bLockedSentence;	
	BYTE m_bUnlockedSound;	
	BYTE m_bUnlockedSentence;

	short	m_iDirectUse;
	BOOL	m_fIgnoreTargetname;
	short	m_iObeyTriggerMode;

	short m_soundRadius;

	string_t m_fireOnOpening;
	string_t m_fireOnClosing;
	string_t m_fireOnOpened;
	string_t m_fireOnClosed;
	BYTE m_fireOnOpeningState;
	BYTE m_fireOnClosingState;
	BYTE m_fireOnOpenedState;
	BYTE m_fireOnClosedState;

	string_t m_lockedSoundOverride;
	string_t m_unlockedSoundOverride;
	string_t m_lockedSentenceOverride;
	string_t m_unlockedSentenceOverride;

	float m_returnSpeed;

	float SoundAttenuation() const
	{
		return ::SoundAttenuation(m_soundRadius);
	}
	bool IgnoreTargetname() const
	{
		return FBitSet(pev->spawnflags, SF_DOOR_FORCETOUCHABLE) || m_fIgnoreTargetname;
	}
};

TYPEDESCRIPTION	CBaseDoor::m_SaveData[] =
{
	DEFINE_FIELD( CBaseDoor, m_bHealthValue, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bStopSnd, FIELD_CHARACTER ),

	DEFINE_FIELD( CBaseDoor, m_bLockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bLockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSentence, FIELD_CHARACTER ),

	DEFINE_FIELD( CBaseDoor, m_iDirectUse, FIELD_SHORT ),
	DEFINE_FIELD( CBaseDoor, m_fIgnoreTargetname, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseDoor, m_iObeyTriggerMode, FIELD_SHORT ),

	DEFINE_FIELD( CBaseDoor, m_soundRadius, FIELD_SHORT ),

	DEFINE_FIELD( CBaseDoor, m_fireOnOpening, FIELD_STRING ),
	DEFINE_FIELD( CBaseDoor, m_fireOnClosing, FIELD_STRING ),
	DEFINE_FIELD( CBaseDoor, m_fireOnOpened, FIELD_STRING ),
	DEFINE_FIELD( CBaseDoor, m_fireOnClosed, FIELD_STRING ),

	DEFINE_FIELD( CBaseDoor, m_fireOnOpeningState, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_fireOnClosingState, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_fireOnOpenedState, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_fireOnClosedState, FIELD_CHARACTER ),

	DEFINE_FIELD( CBaseDoor, m_lockedSoundOverride, FIELD_STRING ),
	DEFINE_FIELD( CBaseDoor, m_unlockedSoundOverride, FIELD_STRING ),
	DEFINE_FIELD( CBaseDoor, m_lockedSentenceOverride, FIELD_STRING ),
	DEFINE_FIELD( CBaseDoor, m_unlockedSentenceOverride, FIELD_STRING ),

	DEFINE_FIELD( CBaseDoor, m_returnSpeed, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBaseDoor, CBaseToggle )

#define DOOR_SENTENCEWAIT	6.0f
#define DOOR_SOUNDWAIT		3.0f
#define BUTTON_SOUNDWAIT	0.5f

// play door or button locked or unlocked sounds. 
// pass in pointer to valid locksound struct. 
// if flocked is true, play 'door is locked' sound,
// otherwise play 'door is unlocked' sound
// NOTE: this routine is shared by doors and buttons

void PlayLockSounds( entvars_t *pev, locksound_t *pls, int flocked, int fbutton )
{
	// LOCKED SOUND

	// CONSIDER: consolidate the locksound_t struct (all entries are duplicates for lock/unlock)
	// CONSIDER: and condense this code.
	float flsoundwait;

	if( fbutton )
		flsoundwait = BUTTON_SOUNDWAIT;
	else
		flsoundwait = DOOR_SOUNDWAIT;

	if( flocked )
	{
		int fplaysound = ( pls->sLockedSound && gpGlobals->time > pls->flwaitSound );
		int fplaysentence = ( pls->sLockedSentence && !pls->bEOFLocked && gpGlobals->time > pls->flwaitSentence );
		float fvol;

		if( fplaysound && fplaysentence )
			fvol = 0.25f;
		else
			fvol = 1.0f;

		// if there is a locked sound, and we've debounced, play sound
		if( fplaysound )
		{
			// play 'door locked' sound
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, STRING( pls->sLockedSound ), fvol, ATTN_NORM );
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// if there is a sentence, we've not played all in list, and we've debounced, play sound
		if( fplaysentence )
		{
			// play next 'door locked' sentence in group
			int iprev = pls->iLockedSentence;

			pls->iLockedSentence = SENTENCEG_PlaySequentialSz( ENT( pev ), STRING( pls->sLockedSentence ),
					  0.85f, ATTN_NORM, 0, 100, pls->iLockedSentence, FALSE );
			pls->iUnlockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFLocked = ( iprev == pls->iLockedSentence );
	
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
	else
	{
		// UNLOCKED SOUND

		int fplaysound = ( pls->sUnlockedSound && gpGlobals->time > pls->flwaitSound );
		int fplaysentence = ( pls->sUnlockedSentence && !pls->bEOFUnlocked && gpGlobals->time > pls->flwaitSentence );
		float fvol;

		// if playing both sentence and sound, lower sound volume so we hear sentence
		if( fplaysound && fplaysentence )
			fvol = 0.25f;
		else
			fvol = 1.0f;

		// play 'door unlocked' sound if set
		if( fplaysound )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, STRING( pls->sUnlockedSound ), fvol, ATTN_NORM );
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// play next 'door unlocked' sentence in group
		if( fplaysentence )
		{
			int iprev = pls->iUnlockedSentence;

			pls->iUnlockedSentence = SENTENCEG_PlaySequentialSz( ENT( pev ), STRING( pls->sUnlockedSentence ),
					  0.85f, ATTN_NORM, 0, 100, pls->iUnlockedSentence, FALSE );
			pls->iLockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFUnlocked = ( iprev == pls->iUnlockedSentence );
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
}

//
// Cache user-entity-field values until spawn is called.
//
void CBaseDoor::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "skin" ) )//skin is used for content type
	{
		pev->skin = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "movesnd" ) )
	{
		m_bMoveSnd = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "stopsnd" ) )
	{
		m_bStopSnd = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "healthvalue" ) )
	{
		m_bHealthValue = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sound" ) )
	{
		m_bLockedSound = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sentence" ) )
	{
		m_bLockedSentence = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sound" ) )
	{
		m_bUnlockedSound = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sentence" ) )
	{
		m_bUnlockedSentence = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "directuse"))
	{
		m_iDirectUse = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fIgnoreTargetname"))
	{
		m_fIgnoreTargetname = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_iObeyTriggerMode" ) )
	{
		m_iObeyTriggerMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "WaveHeight" ) )
	{
		pev->scale = atof( pkvd->szValue ) * ( 1.0f / 8.0f );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fireonopening"))
	{
		m_fireOnOpening = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fireonopening_triggerstate"))
	{
		m_fireOnOpeningState = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fireonclosing"))
	{
		m_fireOnClosing = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fireonclosing_triggerstate"))
	{
		m_fireOnClosingState = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fireonopened"))
	{
		m_fireOnOpened = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fireonopened_triggerstate"))
	{
		m_fireOnOpenedState = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fireonclosed"))
	{
		m_fireOnClosed = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fireonclosed_triggerstate"))
	{
		m_fireOnClosedState = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sound_override" ) )
	{
		m_lockedSoundOverride = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sound_override" ) )
	{
		m_unlockedSoundOverride = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sentence_override" ) )
	{
		m_lockedSentenceOverride = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "unlocked_sentence_override" ) )
	{
		m_unlockedSentenceOverride = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "return_speed") )
	{
		m_returnSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "soundradius" ) )
	{
		m_soundRadius = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

/*QUAKED func_door (0 .5 .8) ? START_OPEN x DOOR_DONT_LINK TOGGLE
if two doors touch, they are assumed to be connected and operate as a unit.

TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN causes the door to move to its destination when spawned, and operate in reverse.
It is used to temporarily or permanently close off an area when triggered (not usefull for
touch or takedamage doors).

"angle"         determines the opening direction
"targetname"	if set, no touch field will be spawned and a remote button or trigger
				field activates the door.
"health"        if set, door must be shot open
"speed"         movement speed (100 default)
"wait"          wait before returning (3 default, -1 = never return)
"lip"           lip remaining at end of move (8 default)
"dmg"           damage to inflict when blocked (2 default)
"sounds"
0)      no sound
1)      stone
2)      base
3)      stone chain
4)      screechy metal
*/

LINK_ENTITY_TO_CLASS( func_door, CBaseDoor )
//
// func_water - same as a door. 
//
LINK_ENTITY_TO_CLASS( func_water, CBaseDoor )

void CBaseDoor::Spawn()
{
	Precache();
	SetMovedir( pev );

	if( pev->skin == 0 )
	{
		//normal door
		if( FBitSet( pev->spawnflags, SF_DOOR_PASSABLE ) )
			pev->solid = SOLID_NOT;
		else
			pev->solid = SOLID_BSP;
	}
	else
	{
		// special contents
		pev->solid = SOLID_NOT;
		SetBits( pev->spawnflags, SF_DOOR_SILENT );	// water is silent for now
	}

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if( pev->speed == 0.0f )
		pev->speed = 100.0f;

	m_vecPosition1 = pev->origin;

	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + ( pev->movedir * ( fabs( pev->movedir.x * ( pev->size.x - 2.0f ) ) + fabs( pev->movedir.y * ( pev->size.y - 2.0f ) ) + fabs( pev->movedir.z * ( pev->size.z - 2.0f ) ) - m_flLip ) );
	ASSERTSZ( m_vecPosition1 != m_vecPosition2, "door start/end positions are equal" );
	if( FBitSet( pev->spawnflags, SF_DOOR_START_OPEN ) )
	{
		// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( pev, m_vecPosition2 );
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if (m_fIgnoreTargetname) {
		pev->spawnflags |= SF_DOOR_FORCETOUCHABLE;
	}

	// if the door is flagged for USE button activation only, use NULL touch function
	if( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) &&
			!IgnoreTargetname() )
	{
		SetTouch( NULL );
	}
	else // touchable button
		SetTouch( &CBaseDoor::DoorTouch );
}
 
void CBaseDoor::SetToggleState( int state )
{
	if( state == TS_AT_TOP )
		UTIL_SetOrigin( pev, m_vecPosition2 );
	else
		UTIL_SetOrigin( pev, m_vecPosition1 );
}

void CBaseDoor::Precache( void )
{
	const char *pszSound;
	bool NullSound = false;

	if ( FStringNull( pev->noiseMoving ) )
	{
		// set the door's "in-motion" sound
		switch( m_bMoveSnd )
		{
			case 1:
				pszSound = "doors/doormove1.wav";
				break;
			case 2:
				pszSound = "doors/doormove2.wav";
				break;
			case 3:
				pszSound = "doors/doormove3.wav";
				break;
			case 4:
				pszSound = "doors/doormove4.wav";
				break;
			case 5:
				pszSound = "doors/doormove5.wav";
				break;
			case 6:
				pszSound = "doors/doormove6.wav";
				break;
			case 7:
				pszSound = "doors/doormove7.wav";
				break;
			case 8:
				pszSound = "doors/doormove8.wav";
				break;
			case 9:
				pszSound = "doors/doormove9.wav";
				break;
			case 10:
				pszSound = "doors/doormove10.wav";
				break;
			case 0:
			default:
				pszSound = "common/null.wav";
				NullSound = true;
				break;
		}
		pev->noiseMoving = MAKE_STRING( pszSound );
	}
	else
	{
		pszSound = STRING( pev->noiseMoving );
	}

	if( !NullSound )
		PRECACHE_SOUND( pszSound );
	NullSound = false;

	if ( FStringNull( pev->noiseArrived ) )
	{
		// set the door's 'reached destination' stop sound
		switch( m_bStopSnd )
		{
			case 1:
				pszSound = "doors/doorstop1.wav";
				break;
			case 2:
				pszSound = "doors/doorstop2.wav";
				break;
			case 3:
				pszSound = "doors/doorstop3.wav";
				break;
			case 4:
				pszSound = "doors/doorstop4.wav";
				break;
			case 5:
				pszSound = "doors/doorstop5.wav";
				break;
			case 6:
				pszSound = "doors/doorstop6.wav";
				break;
			case 7:
				pszSound = "doors/doorstop7.wav";
				break;
			case 8:
				pszSound = "doors/doorstop8.wav";
				break;
			case 0:
			default:
				pszSound = "common/null.wav";
				NullSound = true;
				break;
		}
		pev->noiseArrived = MAKE_STRING( pszSound );
	}
	else
	{
		pszSound = STRING( pev->noiseArrived );
	}

	if( !NullSound )
		PRECACHE_SOUND( pszSound );

	// get door button sounds, for doors which are directly 'touched' to open
	if (!FStringNull(m_lockedSoundOverride))
	{
		pszSound = STRING( m_lockedSoundOverride );
		PRECACHE_SOUND( pszSound );
		m_ls.sLockedSound = m_lockedSoundOverride;
	}
	else if( m_bLockedSound )
	{
		pszSound = ButtonSound( (int)m_bLockedSound );
		PRECACHE_SOUND( pszSound );
		m_ls.sLockedSound = MAKE_STRING( pszSound );
	}

	if (!FStringNull(m_unlockedSoundOverride))
	{
		pszSound = STRING( m_unlockedSoundOverride );
		PRECACHE_SOUND( pszSound );
		m_ls.sUnlockedSound = m_unlockedSoundOverride;
	}
	else if( m_bUnlockedSound )
	{
		pszSound = ButtonSound( (int)m_bUnlockedSound );
		PRECACHE_SOUND( pszSound );
		m_ls.sUnlockedSound = MAKE_STRING( pszSound );
	}

	// get sentence group names, for doors which are directly 'touched' to open
	if (!FStringNull(m_lockedSentenceOverride))
	{
		m_ls.sLockedSentence = m_lockedSentenceOverride;
	}
	else
	{
		switch( m_bLockedSentence )
		{
			case 1:
				// access denied
				m_ls.sLockedSentence = MAKE_STRING( "NA" );
				break;
			case 2:
				// security lockout
				m_ls.sLockedSentence = MAKE_STRING( "ND" );
				break;
			case 3:
				// blast door
				m_ls.sLockedSentence = MAKE_STRING( "NF" );
				break;
			case 4:
				// fire door
				m_ls.sLockedSentence = MAKE_STRING( "NFIRE" );
				break;
			case 5:
				// chemical door
				m_ls.sLockedSentence = MAKE_STRING( "NCHEM" );
				break;
			case 6:
				// radiation door
				m_ls.sLockedSentence = MAKE_STRING( "NRAD" );
				break;
			case 7:
				// gen containment
				m_ls.sLockedSentence = MAKE_STRING( "NCON" );
				break;
			case 8:
				// maintenance door
				m_ls.sLockedSentence = MAKE_STRING( "NH" );
				break;
			case 9:
				// broken door
				m_ls.sLockedSentence = MAKE_STRING( "NG" );
				break;
			default:
				m_ls.sLockedSentence = 0;
				break;
		}
	}

	if (!FStringNull(m_unlockedSentenceOverride))
	{
		m_ls.sUnlockedSentence = m_unlockedSentenceOverride;
	}
	else
	{
		switch( m_bUnlockedSentence )
		{
			case 1:
				// access granted
				m_ls.sUnlockedSentence = MAKE_STRING( "EA" );
				break;
			case 2:
				// security door
				m_ls.sUnlockedSentence = MAKE_STRING( "ED" );
				break;
			case 3:
				// blast door
				m_ls.sUnlockedSentence = MAKE_STRING( "EF" );
				break;
			case 4:
				// fire door
				m_ls.sUnlockedSentence = MAKE_STRING( "EFIRE" );
				break;
			case 5:
				// chemical door
				m_ls.sUnlockedSentence = MAKE_STRING( "ECHEM" );
				break;
			case 6:
				// radiation door
				m_ls.sUnlockedSentence = MAKE_STRING( "ERAD" );
				break;
			case 7:
				// gen containment
				m_ls.sUnlockedSentence = MAKE_STRING( "ECON" );
				break;
			case 8:
				// maintenance door
				m_ls.sUnlockedSentence = MAKE_STRING( "EH" );
				break;
			default:
				m_ls.sUnlockedSentence = 0;
				break;
		}
	}
}

//
// Doors not tied to anything (e.g. button, another door) can be touched, to make them activate.
//
void CBaseDoor::DoorTouch( CBaseEntity *pOther )
{
	// Ignore touches by anything but players
	if( !pOther->IsPlayer() )
		return;

	// If door has master, and it's not ready to trigger, 
	// play 'locked' sound
	if( m_sMaster && !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		PlayLockSounds( pev, &m_ls, TRUE, FALSE );

	// If door is somebody's target, then touching does nothing.
	// You have to activate the owner (e.g. button).
	if( !FStringNull( pev->targetname ) && !IgnoreTargetname() )
	{
		// play locked sound
		PlayLockSounds( pev, &m_ls, TRUE, FALSE );
		return; 
	}

	m_hActivator = pOther;// remember who activated the door

	if( DoorActivate() )
		SetTouch( NULL ); // Temporarily disable the touch function, until movement is finished.
}

//
// Used by SUB_UseTargets, when a door is the target of a button.
//
void CBaseDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;

	// if not ready to be used, ignore "use" command.
	bool shouldActivate = false;
	const bool atEndPosition = m_toggle_state == TS_AT_BOTTOM || ( FBitSet( pev->spawnflags, SF_DOOR_NO_AUTO_RETURN ) && m_toggle_state == TS_AT_TOP );

	if (m_iObeyTriggerMode == 0)
	{
		shouldActivate = atEndPosition;
	}
	else if (m_iObeyTriggerMode == 1)
	{
		shouldActivate = atEndPosition && (useType == USE_TOGGLE || (m_toggle_state == TS_AT_BOTTOM && useType == USE_ON) || (m_toggle_state == TS_AT_TOP && useType == USE_OFF));
	}
	else if (m_iObeyTriggerMode == 2)
	{
		if (useType == USE_TOGGLE)
			shouldActivate = true;
		else if (atEndPosition)
			shouldActivate = (m_toggle_state == TS_AT_BOTTOM && useType == USE_ON) || (m_toggle_state == TS_AT_TOP && useType == USE_OFF);
		else
			shouldActivate = (m_toggle_state == TS_GOING_DOWN && useType == USE_ON) || (m_toggle_state == TS_GOING_UP && useType == USE_OFF);
	}

	if( shouldActivate )
		DoorActivate();
}

float CBaseDoor::InputByMonster(CBaseMonster *pMonster)
{
	if (FBitSet(pev->spawnflags, SF_DOOR_NOMONSTERS))
		return 0.0f;

	short originalTriggerMode = m_iObeyTriggerMode;
	m_iObeyTriggerMode = 2;
	Use(pMonster, pMonster, USE_ON, 0.0f);
	m_iObeyTriggerMode = originalTriggerMode;
	return pev->nextthink - pev->ltime;
}

NODE_LINKENT CBaseDoor::HandleLinkEnt(int afCapMask, bool nodeQueryStatic)
{
	if (nodeQueryStatic) {
		return NLE_ALLOW;
	}

	const int toggleState = GetToggleState();

	// monster should try for it if the door is open and looks as if it will stay that way
	if( toggleState == TS_AT_TOP && (( pev->spawnflags & SF_DOOR_NO_AUTO_RETURN ) || (m_flWait == -1.0f)) )
	{
		return NLE_ALLOW;
	}

	if (!UTIL_IsMasterTriggered( m_sMaster, this )) {
		return NLE_PROHIBIT;
	}

	if( ( pev->spawnflags & SF_DOOR_USE_ONLY ) )
	{
		// door is use only.
		if( ( afCapMask & bits_CAP_OPEN_DOORS ) )
		{
			// let monster right through if he can open doors
			if (!( pev->spawnflags & SF_DOOR_NOMONSTERS ))
				return NLE_NEEDS_INPUT;
		}
		return NLE_PROHIBIT;
	}
	else
	{
		// door must be opened with a button or trigger field.
		if( ( afCapMask & bits_CAP_OPEN_DOORS ) )
		{
			if (!FStringNull(pev->targetname) && !FBitSet(pev->spawnflags, SF_DOOR_FORCETOUCHABLE))
			{
				return NLE_PROHIBIT;
			}
			if( !( pev->spawnflags & SF_DOOR_NOMONSTERS ) ) {
				return NLE_NEEDS_INPUT;
			}
		}

		return NLE_PROHIBIT;
	}
}

//
// Causes the door to "do its thing", i.e. start moving, and cascade activation.
//
int CBaseDoor::DoorActivate()
{
	if( !UTIL_IsMasterTriggered( m_sMaster, m_hActivator ) )
		return 0;

	if( FBitSet( pev->spawnflags, SF_DOOR_NO_AUTO_RETURN ) && (m_toggle_state == TS_AT_TOP || (m_iObeyTriggerMode == 2 && m_toggle_state == TS_GOING_UP)) )
	{
		// door should close
		DoorGoDown();
	}
	else
	{
		// door should open
		if( m_hActivator != 0 && m_hActivator->IsPlayer() )
		{
			// give health if player opened the door (medikit)
			//VARS( m_eoActivator )->health += m_bHealthValue;

			m_hActivator->TakeHealth( this, m_bHealthValue, DMG_GENERIC );
		}

		// play door unlock sounds
		PlayLockSounds( pev, &m_ls, FALSE, FALSE );

		DoorGoUp();
	}

	return 1;
}

//
// Starts the door going to its "up" position (simply ToggleData->vecPosition2).
//
void CBaseDoor::DoorGoUp( void )
{
	entvars_t *pevActivator;

	// It could be going-down, if blocked.
	ASSERT( m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN );

	// emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
		if( m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ), 1, SoundAttenuation() );

	m_toggle_state = TS_GOING_UP;

	SetMoveDone( &CBaseDoor::DoorHitTop );
	if( FClassnameIs( pev, "func_door_rotating" ) )		// !!! BUGBUG Triggered doors don't work with this yet
	{
		float sign = 1.0f;

		if( m_hActivator != 0 )
		{
			pevActivator = m_hActivator->pev;

			if( !FBitSet( pev->spawnflags, SF_DOOR_ONEWAY ) && pev->movedir.y ) 		// Y axis rotation, move away from the player
			{
				Vector vec = pevActivator->origin - pev->origin;
				const bool allowOpenInMoveDirection = g_modFeatures.doors_open_in_move_direction;

				Vector vnext;
				if (!allowOpenInMoveDirection || FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
				{
					UTIL_MakeVectors( pevActivator->angles );
					vnext = ( pevActivator->origin + ( gpGlobals->v_forward * 10.f ) ) - pev->origin;
				}
				else
					vnext = ( pevActivator->origin + ( pevActivator->velocity * 10.f ) ) - pev->origin;

				if( ( vec.x * vnext.y - vec.y * vnext.x ) < 0.0f )
					sign = -1.0f;
			}
		}
		AngularMove( m_vecAngle2*sign, pev->speed );
	}
	else
		LinearMove( m_vecPosition2, pev->speed );

	if ( pev->spawnflags & SF_DOOR_START_OPEN )
	{
		if (m_fireOnClosing)
			FireTargets(STRING(m_fireOnClosing), m_hActivator, this, (USE_TYPE)m_fireOnClosingState);
	}
	else
	{
		if (m_fireOnOpening)
			FireTargets(STRING(m_fireOnOpening), m_hActivator, this, (USE_TYPE)m_fireOnOpeningState);
	}
}

//
// The door has reached the "up" position.  Either go back down, or wait for another activation.
//
void CBaseDoor::DoorHitTop( void )
{
	if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseArrived ), 1.0f, SoundAttenuation() );
	}

	ASSERT( m_toggle_state == TS_GOING_UP );
	m_toggle_state = TS_AT_TOP;

	// toggle-doors don't come down automatically, they wait for refire.
	if( FBitSet( pev->spawnflags, SF_DOOR_NO_AUTO_RETURN ) )
	{
		// Re-instate touch method, movement is complete
		if( !FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) ||
				IgnoreTargetname() )
			SetTouch( &CBaseDoor::DoorTouch );
	}
	else
	{
		// In flWait seconds, DoorGoDown will fire, unless wait is -1, then door stays open
		pev->nextthink = pev->ltime + m_flWait;
		SetThink( &CBaseDoor::DoorGoDown );

		if( m_flWait == -1.0f )
		{
			pev->nextthink = -1.0f;
		}
	}

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if( pev->netname && ( pev->spawnflags & SF_DOOR_START_OPEN ) )
		FireTargets( STRING( pev->netname ), m_hActivator, this, USE_TOGGLE );

	if ( pev->spawnflags & SF_DOOR_START_OPEN )
	{
		if (m_fireOnClosed)
			FireTargets(STRING(m_fireOnClosed), m_hActivator, this, (USE_TYPE)m_fireOnClosedState);
	}
	else
	{
		if (m_fireOnOpened)
			FireTargets(STRING(m_fireOnOpened), m_hActivator, this, (USE_TYPE)m_fireOnOpenedState);
	}

	SUB_UseTargets( m_hActivator );
}

//
// Starts the door going to its "down" position (simply ToggleData->vecPosition1).
//
void CBaseDoor::DoorGoDown( void )
{
	if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
		if( m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ), 1.0f, SoundAttenuation() );
#if DOOR_ASSERT
	ASSERT( m_toggle_state == TS_AT_TOP );
#endif // DOOR_ASSERT
	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone( &CBaseDoor::DoorHitBottom );
	if( FClassnameIs( pev, "func_door_rotating" ) )//rotating door
		AngularMove( m_vecAngle1, m_returnSpeed <= 0.0f ? pev->speed : m_returnSpeed );
	else
		LinearMove( m_vecPosition1, m_returnSpeed <= 0.0f ? pev->speed : m_returnSpeed );

	if ( pev->spawnflags & SF_DOOR_START_OPEN )
	{
		if (m_fireOnOpening)
			FireTargets(STRING(m_fireOnOpening), m_hActivator, this, (USE_TYPE)m_fireOnOpeningState, 0.0f);
	}
	else
	{
		if (m_fireOnClosing)
			FireTargets(STRING(m_fireOnClosing), m_hActivator, this, (USE_TYPE)m_fireOnClosingState, 0.0f);
	}
}

//
// The door has reached the "down" position.  Back to quiescence.
//
void CBaseDoor::DoorHitBottom( void )
{
	if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseArrived ), 1.0f, SoundAttenuation() );
	}

	ASSERT( m_toggle_state == TS_GOING_DOWN );
	m_toggle_state = TS_AT_BOTTOM;

	// Re-instate touch method, cycle is complete
	if( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) &&
			!IgnoreTargetname() )
	{
		// use only door
		SetTouch( NULL );
	}
	else // touchable door
		SetTouch( &CBaseDoor::DoorTouch );

	SUB_UseTargets( m_hActivator );

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if( pev->netname && !( pev->spawnflags & SF_DOOR_START_OPEN ) )
		FireTargets( STRING( pev->netname ), m_hActivator, this, USE_TOGGLE );

	if ( pev->spawnflags & SF_DOOR_START_OPEN )
	{
		if (m_fireOnOpened)
			FireTargets(STRING(m_fireOnOpened), m_hActivator, this, (USE_TYPE)m_fireOnOpenedState);
	}
	else
	{
		if (m_fireOnClosed)
			FireTargets(STRING(m_fireOnClosed), m_hActivator, this, (USE_TYPE)m_fireOnClosedState);
	}
}

void CBaseDoor::Blocked( CBaseEntity *pOther )
{
	edict_t	*pentTarget = NULL;
	CBaseDoor *pDoor = NULL;

	// Hurt the blocker a little.
	bool shouldProceed = false;
	if( pev->dmg ) {
		pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );
		if (g_modFeatures.doors_blocked_recheck)
		{
			// Entity became unsolid or killed
			if (pOther->pev->solid == SOLID_NOT || FBitSet(pev->flags, FL_KILLME))
				shouldProceed = true;
		}
	}

	if (!shouldProceed)
		shouldProceed = pOther->HandleDoorBlockage(this);

	if (shouldProceed)
		return;

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if( m_flWait >= 0.0f )
	{
		// BMod Start - Door sound fix.
		if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
			STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );
		// BMod End

		if( m_toggle_state == TS_GOING_DOWN )
		{
			DoorGoUp();
		}
		else
		{
			DoorGoDown();
		}
	}

	// Block all door pieces with the same targetname here.
	if( !FStringNull( pev->targetname ) )
	{
		for( ; ; )
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( pev->targetname ) );

			if( VARS( pentTarget ) != pev )
			{
				if( FNullEnt( pentTarget ) )
					break;

				if( FClassnameIs( pentTarget, "func_door" ) || FClassnameIs( pentTarget, "func_door_rotating" ) )
				{
					pDoor = GetClassPtr( (CBaseDoor *)VARS( pentTarget ) );

					if( pDoor->m_flWait >= 0.0f )
					{
						if( pDoor->pev->velocity == pev->velocity && pDoor->pev->avelocity == pev->velocity )
						{
							// this is the most hacked, evil, bastardized thing I've ever seen. kjb
							if( FClassnameIs( pentTarget, "func_door" ) )
							{
								// set origin to realign normal doors
								pDoor->pev->origin = pev->origin;
								pDoor->pev->velocity = g_vecZero;// stop!
							}
							else
							{
								// set angles to realign rotating doors
								pDoor->pev->angles = pev->angles;
								pDoor->pev->avelocity = g_vecZero;
							}
						}
						if( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
							STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );

						if( pDoor->m_toggle_state == TS_GOING_DOWN )
							pDoor->DoorGoUp();
						else
							pDoor->DoorGoDown();
					}
				}
			}
		}
	}
}

/*QUAKED FuncRotDoorSpawn (0 .5 .8) ? START_OPEN REVERSE  
DOOR_DONT_LINK TOGGLE X_AXIS Y_AXIS
if two doors touch, they are assumed to be connected and operate as  
a unit.

TOGGLE causes the door to wait in both the start and end states for  
a trigger event.

START_OPEN causes the door to move to its destination when spawned,  
and operate in reverse.  It is used to temporarily or permanently  
close off an area when triggered (not usefull for touch or  
takedamage doors).

You need to have an origin brush as part of this entity.  The  
center of that brush will be
the point around which it is rotated. It will rotate around the Z  
axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"distance" is how many degrees the door will be rotated.
"speed" determines how fast the door moves; default value is 100.

REVERSE will cause the door to rotate in the opposite direction.

"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote  
button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
0)	no sound
1)	stone
2)	base
3)	stone chain
4)	screechy metal
*/

class CRotDoor : public CBaseDoor
{
public:
	void Spawn( void );
	virtual void SetToggleState( int state );
};

LINK_ENTITY_TO_CLASS( func_door_rotating, CRotDoor )

void CRotDoor::Spawn( void )
{
	Precache();
	// set the axis of rotation
	CBaseToggle::AxisDir( pev );

	// check for clockwise rotation
	if( FBitSet( pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS ) )
		pev->movedir = pev->movedir * -1.0f;

	//m_flWait = 2; who the hell did this? (sjb)
	m_vecAngle1 = pev->angles;
	m_vecAngle2 = pev->angles + pev->movedir * m_flMoveDistance;

	ASSERTSZ( m_vecAngle1 != m_vecAngle2, "rotating door start/end positions are equal" );

	if( FBitSet( pev->spawnflags, SF_DOOR_PASSABLE ) )
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if( pev->speed == 0.0f )
		pev->speed = 100.0f;

	// DOOR_START_OPEN is to allow an entity to be lighted in the closed position
	// but spawn in the open position
	if( FBitSet( pev->spawnflags, SF_DOOR_START_OPEN ) )
	{	
		// swap pos1 and pos2, put door at pos2, invert movement direction
		pev->angles = m_vecAngle2;
		Vector vecSav = g_modFeatures.door_rotating_starts_open_fix ? m_vecAngle2 : m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		pev->movedir = pev->movedir * -1.0f;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) && !IgnoreTargetname() )
	{
		SetTouch( NULL );
	}
	else // touchable button
		SetTouch( &CBaseDoor::DoorTouch );
}

void CRotDoor::SetToggleState( int state )
{
	if( state == TS_AT_TOP )
		pev->angles = m_vecAngle2;
	else
		pev->angles = m_vecAngle1;

	UTIL_SetOrigin( pev, pev->origin );
}

class CMomentaryDoor : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT MomentaryMoveDone( void );
	void EXPORT StopMoveSound( void );

	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BYTE m_bMoveSnd;			// sound a door makes while moving	
	BYTE m_bStopSnd;			// sound a door makes when it stops

	float m_fLastPos;

	bool CalcRatio(CBaseEntity *pLocus, float *outResult) {
		*outResult = m_fLastPos;
		return true;
	}
};

LINK_ENTITY_TO_CLASS( momentary_door, CMomentaryDoor )

TYPEDESCRIPTION	CMomentaryDoor::m_SaveData[] =
{
	DEFINE_FIELD( CMomentaryDoor, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CMomentaryDoor, m_bStopSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CMomentaryDoor, m_fLastPos, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CMomentaryDoor, CBaseToggle )

void CMomentaryDoor::Spawn( void )
{
	SetMovedir( pev );

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if( pev->speed == 0.0f )
		pev->speed = 100.0f;
	if( pev->dmg == 0.0f )
		pev->dmg = 2.0f;

	m_vecPosition1 = pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + ( pev->movedir * ( fabs( pev->movedir.x * ( pev->size.x - 2.0f ) ) + fabs( pev->movedir.y * ( pev->size.y - 2.0f ) ) + fabs( pev->movedir.z * ( pev->size.z - 2.0f ) ) - m_flLip ) );
	ASSERTSZ( m_vecPosition1 != m_vecPosition2, "door start/end positions are equal" );

	if( FBitSet( pev->spawnflags, SF_DOOR_START_OPEN ) )
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( pev, m_vecPosition2 );
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}
	SetTouch( NULL );

	Precache();
}

void CMomentaryDoor::Precache( void )
{
	const char *pszSound;
	bool NullSound = false;

	// set the door's "in-motion" sound
	switch( m_bMoveSnd )
	{
	case 1:
		pszSound = "doors/doormove1.wav";
		break;
	case 2:
		pszSound = "doors/doormove2.wav";
		break;
	case 3:
		pszSound = "doors/doormove3.wav";
		break;
	case 4:
		pszSound = "doors/doormove4.wav";
		break;
	case 5:
		pszSound = "doors/doormove5.wav";
		break;
	case 6:
		pszSound = "doors/doormove6.wav";
		break;
	case 7:
		pszSound = "doors/doormove7.wav";
		break;
	case 8:
		pszSound = "doors/doormove8.wav";
		break;
	case 0:
	default:
		pszSound = "common/null.wav";
		NullSound = true;
		break;
	}

	if( !NullSound )
		PRECACHE_SOUND( pszSound );
	pev->noiseMoving = MAKE_STRING( pszSound );
	NullSound = false;

	// set the door's 'reached destination' stop sound
	switch( m_bStopSnd )
	{
	case 1:
		pszSound = "doors/doorstop1.wav";
		break;
	case 2:
		pszSound = "doors/doorstop2.wav";
		break;
	case 3:
		pszSound = "doors/doorstop3.wav";
		break;
	case 4:
		pszSound = "doors/doorstop4.wav";
		break;
	case 5:
		pszSound = "doors/doorstop5.wav";
		break;
	case 6:
		pszSound = "doors/doorstop6.wav";
		break;
	case 7:
		pszSound = "doors/doorstop7.wav";
		break;
	case 8:
		pszSound = "doors/doorstop8.wav";
		break;
	case 0:
	default:
		pszSound = "common/null.wav";
		NullSound = true;
		break;
	}

	if( !NullSound )
		PRECACHE_SOUND( pszSound );
	pev->noiseArrived = MAKE_STRING( pszSound );
}

void CMomentaryDoor::KeyValue( KeyValueData *pkvd )
{

	if( FStrEq( pkvd->szKeyName, "movesnd" ) )
	{
		m_bMoveSnd = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "stopsnd" ) )
	{
		m_bStopSnd = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "healthvalue" ) )
	{
		//m_bHealthValue = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CMomentaryDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( useType != USE_SET )		// Momentary buttons will pass down a float in here
		return;

	if( value > 1.0f )
		value = 1.0f;
	if( value < 0.0f )
		value = 0.0f;

	Vector move = m_vecPosition1 + ( value * ( m_vecPosition2 - m_vecPosition1 ) );
	
	Vector delta = move - pev->origin;
	//float speed = delta.Length() * 10.0f;
	float speed = delta.Length() / 0.1f; // move there in 0.1 sec

	if( speed != 0 )
	{
		// This entity only thinks when it moves, so if it's thinking, it's in the process of moving
		// play the sound when it starts moving(not yet thinking)
		if( pev->nextthink < pev->ltime || pev->nextthink == 0.0f )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ), 1.0f, ATTN_NORM );
		// If we already moving to designated point, return
		else if( move == m_vecFinalDest )
			return;

		m_fLastPos = value;
		LinearMove( move, speed );
		SetMoveDone( &CMomentaryDoor::MomentaryMoveDone );
	}
}

void CMomentaryDoor::MomentaryMoveDone( void )
{
	SetThink(&CMomentaryDoor::StopMoveSound);
	pev->nextthink = pev->ltime + 0.1f;
}

void CMomentaryDoor::StopMoveSound()
{
	STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseMoving ) );
	EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noiseArrived ), 1.0f, ATTN_NORM );
	pev->nextthink = -1.0f;
	ResetThink();
}
