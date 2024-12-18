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
#if !defined(PLAYER_H)
#define PLAYER_H

#include "pm_materials.h"
#include "mod_features.h"
#include "basemonster.h"
#include "objecthint_spec.h"
#if FEATURE_ROPE
class CRope;
#endif
#include "com_model.h"
#include <cstdint>

#include <vector>

#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

enum
{
	INVENTORY_ITEM_NO_CHANGE = -2,
	INVENTORY_ITEM_NONE_GIVEN_MAXITEMS = -1,
	INVENTORY_ITEM_NONE_GIVEN_MAXCOUNT = 0,
	INVENTORY_ITEM_GIVEN = 1,
	INVENTORY_ITEM_GIVEN_OVERFLOW = 2,
	INVENTORY_ITEM_COUNT_CHANGED = 3
};

#define STRIP_WEAPONS_ONLY 0
#define STRIP_SUIT 1
#define STRIP_SUITLIGHT 2
#define STRIP_LONGJUMP 4
#define STRIP_DONT_TURNOFF_FLASHLIGHT 8
#define STRIP_ALL_ITEMS (STRIP_SUIT | STRIP_SUITLIGHT | STRIP_LONGJUMP)

#define SF_DISPLACER_TARGET_DISABLED 1

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER		( 1<<0 )
#define PFLAG_ONSWING		( 1<<0 )
#define PFLAG_ONTRAIN		( 1<<1 )
#define PFLAG_ONBARNACLE	( 1<<2 )
#define PFLAG_DUCKING		( 1<<3 )		// In the process of ducking, but totally squatted yet
#define PFLAG_USING		( 1<<4 )		// Using a continuous entity
#define PFLAG_OBSERVER		( 1<<5 )		// player is locked in stationary cam mode. Spectators can move, observers can't.
#define	PFLAG_LATCHING		( 1<<6 )	// Player is latching to a target
#define	PFLAG_ATTACHED		( 1<<7 )	// Player is attached by a barnacle tongue tip


#define PFLAG_ONROPE		( 1<<8 )
//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST	4		// max of 4 suit sentences queued up at any time

#define SUIT_GROUP			TRUE
#define	SUIT_SENTENCE		FALSE

#define	SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN	60
#define SUIT_NEXT_IN_5MIN	300
#define SUIT_NEXT_IN_10MIN	600
#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define CSUITNOREPEAT		32

#define TEAM_NAME_LENGTH	16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_GRAPPLE,
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END
};

#define CHAT_INTERVAL 1.0f

#define ARMOR_RATIO	0.2	// Armor Takes 80% of the damage

// suppress capabilities flags
#define PLAYER_SUPPRESS_ATTACK (1<<0)
#define PLAYER_SUPPRESS_JUMP (1<<1)
#define PLAYER_SUPPRESS_DUCK (1<<2)
#define PLAYER_SUPPRESS_STEP_SOUND (1<<3)
#define PLAYER_SUPPRESS_USE (1<<5)

// trigger_camera related player flags
#define PLAYER_CAMERA_INVULNERABLE (1 << 0)

// this is trigger_camera flag, need to have it here
#define SF_CAMERA_STOP_BY_PLAYER_INPUT_USE (1 << 25)

class CBasePlayer : public CBaseMonster
{
public:
	// Spectator camera
	void	Observer_FindNextPlayer( bool bReverse );
	void	Observer_HandleButtons();
	void	Observer_SetMode( int iMode );
	void	Observer_CheckTarget();
	void	Observer_CheckProperties();
	EHANDLE	m_hObserverTarget;
	float	m_flNextObserverInput;
	int		m_iObserverWeapon;	// weapon of current tracked target
	int		m_iObserverLastMode;// last used observer mode
	int		IsObserver() { return pev->iuser1; };

	int					random_seed;    // See that is shared between client & server for shared weapons code

	int					m_iPlayerSound;// the index of the sound list slot reserved for this player
	int					m_iTargetVolume;// ideal sound volume. 
	int					m_iWeaponVolume;// how loud the player's weapon is right now.
	int					m_iExtraSoundTypes;// additional classification for this weapon's sound
	int					m_iWeaponFlash;// brightness of the weapon flash
	float				m_flStopExtraSoundTime;

	float				m_flFlashLightTime;	// Time until next battery draw/Recharge
	int					m_iFlashBattery;		// Flashlight Battery Draw

	int					m_afButtonLast;
	int					m_afButtonPressed;
	int					m_afButtonReleased;

	edict_t				*m_pentSndLast;			// last sound entity to modify player room type
	int					m_SndRoomtype;		// last roomtype set by sound entity
	float				m_flSndRange;			// dist from player to sound entity
	int					m_ClientSndRoomtype;

	float				m_flFallVelocity;

	int					m_rgItems[MAX_ITEMS];
	int					m_fKnownItem;		// True when a new item needs to be added
	int					m_fNewAmmo;			// True when a new item has been added

	unsigned int		m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden
	float				m_fNextSuicideTime; // the time after which the player can next use the suicide command

	// these are time-sensitive things that we keep track of
	float				m_flTimeStepSound;	// when the last stepping sound was made
	float				m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float				m_flSwimTime;		// how long player has been underwater
	float				m_flDuckTime;		// how long we've been ducking
	float				m_flWallJumpTime;	// how long until next walljump

	float				m_flSuitUpdate;					// when to play next suit update
	int					m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int					m_iSuitPlayNext;				// next sentence slot for queue storage;
	int					m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float				m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int					m_lastDamageAmount;		// Last damage taken
	float				m_tbdPrev;				// Time-based damage timer

	float				m_flgeigerRange;		// range to nearest radiation source
	float				m_flgeigerDelay;		// delay per update of range msg to client
	int					m_igeigerRangePrev;
	int					m_iStepLeft;			// alternate left/right foot stepping sound
	char				m_szTextureName[CBTEXTURENAMEMAX];	// current texture name we're standing on
	char				m_chTextureType;		// current texture type

	int					m_idrowndmg;			// track drowning damage taken
	int					m_idrownrestored;		// track drowning damage restored

	int					m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
										// the hude via the DAMAGE message
	BOOL				m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	BOOL				m_fGameHUDInitialized;
	int					m_iTrain;				// Train control position
	BOOL				m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info

	EHANDLE				m_pTank;				// the tank which the player is currently controlling,  NULL if no tank
	EHANDLE				m_hViewEntity;			// The view entity being used, or null if the player is using itself as the view entity
	bool				m_bResetViewEntity;		//True if the player's view needs to be set back to the view entity
	float				m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())

	BOOL			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true. 
	BOOL			m_fLongJump; // does this player have the longjump module?

	int			m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int			m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int			m_iClientMaxHealth;
	int			m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new
	int			m_iClientMaxBattery;
	int			m_iHideHUD;		// the players hud weapon info is to be hidden
	int			m_iClientHideHUD;
	int			m_iFOV;			// field of view
	int			m_iClientFOV;	// client's known FOV

	// usable player items 
	CBasePlayerWeapon *m_rgpPlayerWeapons[MAX_WEAPONS];
	CBasePlayerWeapon *m_pActiveItem;
	CBasePlayerWeapon *m_pClientActiveItem;  // client version of the active item
	CBasePlayerWeapon *m_pLastItem;

	std::uint64_t m_WeaponBits;

	//Not saved, used to update client.
	std::uint64_t m_ClientWeaponBits;

	// shared ammo slots
	int	m_rgAmmo[MAX_AMMO_TYPES];
	int	m_rgAmmoLast[MAX_AMMO_TYPES];

	Vector				m_vecAutoAim;
	BOOL				m_fOnTarget;
	int					m_iDeaths;
	float				m_flRespawnTimer;	// used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	virtual void Spawn( void );
	void Pain( void );

	//virtual void Think( void );
	virtual void Jump( void );
	virtual void Duck( void );
	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual Vector GetGunPosition( void );
	virtual int TakeHealth(CBaseEntity *pHealer, float flHealth, int bitsDamageType );
	void SetHealth(int health, bool allowOverheal = false);
	void SetMaxHealth(int maxHealth, bool clampValue = true);
	virtual int TakeArmor(CBaseEntity *pCharger, float flArmor, int flags = 0);
	int MaxArmor();
	void SetMaxArmor(int maxArmor, bool clampValue = true);
	void SetArmor(int armor, bool allowOvercharge = false);
	float ArmorStrength();
	bool IsInvulnerable();
	virtual void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual void	Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib );
	virtual Vector BodyTarget( const Vector &posSrc ) { return Center( ) + pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 ); };		// position to shoot at
	virtual bool IsAlive( void ) override { return IsFullyAlive(); }
	virtual bool ShouldFadeOnDeath( void ) override { return false; }
	virtual	bool IsPlayer( void ) override { return true; }			// Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned

	virtual bool IsNetClient( void ) override { return true; }		// Bots should return FALSE for this, they can't receive NET messages
															// Spectators should return TRUE for this
	virtual const char *TeamID( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	void SetPhysicsKeyValues();
	void RenewItems(void);
	void PackDeadPlayerItems( void );
	void RemoveAllItems( int stripFlags );
	bool SwitchWeapon( CBasePlayerWeapon *pWeapon );
	bool SwitchToBestWeapon();

	void SetWeaponBit(int id) {
		m_WeaponBits |= 1ULL << id;
	}
	void ClearWeaponBit(int id) {
		m_WeaponBits &= ~(1ULL << id);
	}
	bool HasWeaponBit(int id) {
		return (m_WeaponBits & (1ULL << id)) != 0;
	}

	bool HasSuit() const
	{
		return (m_iItemsBits & PLAYER_ITEM_SUIT) != 0;
	}
	bool HasFlashlight() const
	{
		return (m_iItemsBits & PLAYER_ITEM_FLASHLIGHT) != 0;
	}
	bool HasNVG() const
	{
		return FEATURE_NIGHTVISION && (m_iItemsBits & PLAYER_ITEM_NIGHTVISION) != 0;
	}
	bool HasSuitLight() const {
		return HasFlashlight() || HasNVG();
	}
	void RemoveSuitLight();

	void SetJustSuit() {
		m_iItemsBits |= PLAYER_ITEM_SUIT;
	}
	void SetFlashlight() {
		m_iItemsBits |= PLAYER_ITEM_FLASHLIGHT;
	}
	void SetFlashlightOnly();
	void RemoveFlashlight();
	void SetNVG() {
		m_iItemsBits |= PLAYER_ITEM_NIGHTVISION;
	}
	void SetNVGOnly();
	void RemoveNVG();

	void SetSuitAndDefaultLight();
	void SetDefaultLight();
	void SetLongjump(bool enabled);

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData( void );
	void GatherAndSendObjectHints();
	
	static	TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	virtual int		ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void	Precache( void );
	bool			IsOnLadder( void );
	bool FlashlightIsOn() { return FBitSet(pev->effects, EF_DIMLIGHT); }
	bool NVGIsOn() { return m_fNVGisON; }
	bool SuitLightIsOn( void ) { return FlashlightIsOn() || NVGIsOn(); }
	void SuitLightTurnOn( void );
	void SuitLightTurnOff( bool playOffSound = true );
	void UpdateSuitLightBattery( bool on );
	void FlashlightToggle();
	void FlashlightTurnOn();
	void FlashlightTurnOff( bool playOffSound = true );
	void NVGToggle();
	void NVGTurnOn();
	void NVGTurnOff( bool playOffSound = true );

	void UpdatePlayerSound ( void );
	void DeathSound ( void );

	int DefaultClassify();
	int Classify ( void );
	void SetAnimation( PLAYER_ANIM playerAnim );
	void SetWeaponAnimType( const char *szExtention );
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands( void );
	void CheatImpulseCommands( int iImpulse );

	void StartDeathCam( void );
	void StartObserver( Vector vecPosition, Vector vecViewAngle );
	void StopObserver();

	void AddPoints( int score, bool bAllowNegativeScore ) override;
	void AddPointsToTeam( int score, bool bAllowNegativeScore ) override;
	void AddFloatPoints( float score, bool bAllowNegativeScore ) override;
	int AddPlayerItem( CBasePlayerWeapon *pItem ) override;
	bool RemovePlayerItem( CBasePlayerWeapon *pItem, bool bCallHoster );
	void DropPlayerItem ( const char *pszItemName );
	void DropPlayerItemById( int iId );
	void DropAmmo();
	bool HasPlayerItem( CBasePlayerWeapon *pCheckItem );
	bool HasNamedPlayerItem( const char *pszItemName );
	CBasePlayerWeapon* GetWeaponByName( const char *pszItemName );
	bool HasWeapons( void );// do I have ANY weapons?
	void SendCurWeaponClear();
	void SendCurWeaponDead();
	void SelectPrevItem( int iItem );
	void SelectLastItem(void);
	void SelectItem(const char *pstr);
	void ItemPreFrame( void );
	void ItemPostFrame( void );
	void GiveNamedItem( const char *szName, int spawnFlags = 0 );
	void EnableControl(bool fControl);

	int  GiveAmmo( int iAmount, const char *szName );
	void RemoveAmmo( int iAmount, const char *szName );
	void SendAmmoUpdate(void);

	void WaterMove( void );
	void EXPORT PlayerDeathThink( void );
	std::pair<CBaseEntity*, const ObjectHintSpec*> GetInteractiveEntity(std::vector<std::pair<CBaseEntity*, const ObjectHintSpec*>>* hintedEntities = nullptr);
	void PlayerUse( void );

	void CheckSuitUpdate();
	void SetSuitUpdate( const char *name, bool fgroup, int iNoRepeat );
	void UpdateGeigerCounter( void );
	void CheckTimeBasedDamage( void );

	bool FBecomeProne( void ) override;
	void BarnacleVictimBitten ( entvars_t *pevBarnacle );
	void BarnacleVictimReleased ( void );
	static int GetAmmoIndex(const char *psz);
	int AmmoInventory( int iAmmoIndex );
	int Illumination( void );

	void ResetAutoaim( void );
	Vector GetAutoaimVector( float flDelta  );
	Vector GetAutoaimVectorFromPoint( const Vector& vecSrc,float flDelta  );
	Vector AutoaimDeflection( const Vector &vecSrc, float flDist, float flDelta  );

	void ForceClientDllUpdate( void );  // Forces all client .dll specific data to be resent to client.

	void DeathMessage( entvars_t *pevKiller );

	void SetCustomDecalFrames( int nFrames );
	int GetCustomDecalFrames( void );

	void SetMovementMode();
	void RecruitFollowers();
	void DisbandFollowers();

	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn;// while charging, when to absorb another unit of player's ammo?

	// Player ID
	void InitStatusBar( void );
	void UpdateStatusBar( void );

	void InsertWeaponById( CBasePlayerWeapon* pItem );
	CBasePlayerWeapon* WeaponById( int id );

	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];

	int m_lastSeenEntityIndex;
	int m_lastSeenHealth;
	int m_lastSeenArmor;
	float m_lastSeenTime;

	void SetPrefsFromUserinfo( char *infobuffer );

	float m_flNextChatTime;

	int m_iAutoWepSwitch;

	Vector m_vecLastViewAngles;
	float m_flNextRespawnMessageTime;
#if FEATURE_DISPLACER
	BOOL	m_fInXen;
	Vector m_DisplacerReturn;
	int m_DisplacerSndRoomtype;
#endif
	BOOL	m_fNVGisON;
	friend class CDisplacer;
	friend class CTriggerXenReturn;

private:
	enum {
		NoAmmoDrop,
		DropAllAmmo,
		DropAmmoFair
	};

	void DropPlayerItemImpl(CBasePlayerWeapon* pWeapon, int dropType = DropAmmoFair, float speed = 400);

public:
	short m_movementState; // no need to save

	bool m_bSentMessages;

#if FEATURE_ROPE
	bool m_bIsClimbing;
	float m_flLastClimbTime;
	CRope *m_pRope;
	bool IsOnRope()
	{
		return ( m_afPhysicsFlags & PFLAG_ONROPE ) != 0;
	}

	void SetRope( CBaseEntity *pRope )
	{
		m_pRope = (CRope*)pRope;
	}
	void SetOnRopeState( bool onRope )
	{
	  if( onRope )
		m_afPhysicsFlags |= PFLAG_ONROPE;
	  else
		m_afPhysicsFlags &= ~PFLAG_ONROPE;

	}
	CRope* GetRope() { return m_pRope; }

	void LetGoRope(float delay = 2.0f);
	bool SetClosestOriginOnRope(const Vector& vecPos);
#endif
	int m_iItemsBits;
	int m_iClientItemsBits;

	BYTE m_timeBasedDmgModifiers[CDMG_TIMEBASED];

	BOOL m_settingsLoaded;
	BOOL m_buddha;
	short m_iSatchelControl;
	short m_iPreferNewGrenadePhysics;

	int m_suppressedCapabilities;
	float m_maxSpeedFraction;

	float m_armorStrength;

	void SetLoopedMp3(string_t loopedMp3);
	string_t m_loopedMp3;

	string_t m_inventoryItems[MAX_INVENTORY_ITEMS];
	short m_inventoryItemCounts[MAX_INVENTORY_ITEMS];
	int FindSlotForItem(string_t item, bool allowOverflow = false, int* result = nullptr);
	bool CanHaveIntenvoryItem(string_t item, bool allowOverflow = false);
	int GiveInventoryItem(string_t item, int count, bool allowOverflow = false);
	int SetInventoryItem(string_t item, int count, bool allowOverflow = false);
	bool RemoveInventoryItem(string_t item, int count);
	void RemoveAllInventoryItems();
	bool HasInventoryItem(string_t item);
	int InventoryItemIndex(string_t item);

	EHANDLE m_camera;
	int m_cameraFlags;

	float m_spriteHintTimeCheck;
};

#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

extern int gmsgHudText;
extern bool gInitHUD;

extern bool g_PlayerFullyInitialized[MAX_CLIENTS];

#endif // PLAYER_H
