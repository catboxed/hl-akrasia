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
//=========================================================
// Soundent.h - the entity that spawns when the world 
// spawns, and handles the world's active and free sound
// lists.
//=========================================================
#pragma once
#if !defined(SOUNDENT_H)
#define SOUNDENT_H

#include "soundent_bits.h"

#define	MAX_WORLD_SOUNDS	64 // maximum number of sounds handled by the world at one time.

#define SOUNDLIST_EMPTY	-1

#define SOUNDLISTTYPE_FREE	1// identifiers passed to functions that can operate on either list, to indicate which list to operate on.
#define SOUNDLISTTYPE_ACTIVE 2

#define	SOUND_NEVER_EXPIRE	-1 // with this set as a sound's ExpireTime, the sound will never expire.

//=========================================================
// CSound - an instance of a sound in the world.
//=========================================================
class CSound
{
public:
	void	Clear ( void );
	void	Reset ( void );

	Vector	m_vecOrigin;	// sound's location in space
	int		m_iType;		// what type of sound this is
	int		m_iVolume;		// how loud the sound is
	float	m_flExpireTime;	// when the sound should be purged from the list
	int		m_iNext;		// index of next sound in this list ( Active or Free )
	int		m_iNextAudible;	// temporary link that monsters use to build a list of audible sounds

	bool FIsSound( void );
	bool FIsScent( void );
};

//=========================================================
// CSoundEnt - a single instance of this entity spawns when
// the world spawns. The SoundEnt's job is to update the 
// world's Free and Active sound lists.
//=========================================================
class CSoundEnt : public CBaseEntity 
{
public:
	void Precache ( void );
	void Spawn( void );
	void Think( void );
	void Initialize ( void );

	static void		InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration );
	static void		FreeSound ( int iSound, int iPrevious );
	static int		ActiveList( void );// return the head of the active list
	static int		FreeList( void );// return the head of the free list
	static CSound*	SoundPointerForIndex( int iIndex );// return a pointer for this index in the sound list
	static int		ClientSoundIndex ( edict_t *pClient );

	BOOL	IsEmpty( void ) { return m_iActiveSound == SOUNDLIST_EMPTY; }
	int		ISoundsInList ( int iListType );
	int		IAllocSound ( void );
	virtual int		ObjectCaps( void ) { return FCAP_DONT_SAVE; }

	int		m_iFreeSound;	// index of the first sound in the free sound list
	int		m_iActiveSound; // indes of the first sound in the active sound list
	int		m_cLastActiveSounds; // keeps track of the number of active sounds at the last update. (for diagnostic work)
	BOOL	m_fShowReport; // if true, dump information about free/active sounds.

private:
	CSound		m_SoundPool[ MAX_WORLD_SOUNDS ];
};
#endif // SOUNDENT_H
