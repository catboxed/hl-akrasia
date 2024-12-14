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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "hornet.h"

#if !CLIENT_DLL
#include "gamerules.h"
#endif

enum firemode_e
{
	FIREMODE_TRACK = 0,
	FIREMODE_FAST
};

LINK_ENTITY_TO_CLASS( weapon_hornetgun, CHgun )

BOOL CHgun::IsUseable( void )
{
	return TRUE;
}

void CHgun::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), MyWModel() );

	int defaultAmmoGive = g_AmmoRegistry.GetMaxAmmo("hornets");
	if (defaultAmmoGive <= 0)
		defaultAmmoGive = HIVEHAND_DEFAULT_GIVE;
	InitDefaultAmmo(defaultAmmoGive);
	m_iFirePhase = 0;

	FallInit();// get ready to fall down.
}

void CHgun::Precache( void )
{
	PRECACHE_MODEL( "models/v_hgun.mdl" );
	PRECACHE_MODEL( MyWModel() );
	PrecachePModel( "models/p_hgun.mdl" );

	m_usHornetFire = PRECACHE_EVENT( 1, "events/firehornet.sc" );

	UTIL_PrecacheOther( "hornet" );

	PRECACHE_SOUND( "agrunt/ag_fire1.wav" );
	PRECACHE_SOUND( "agrunt/ag_fire2.wav" );
	PRECACHE_SOUND( "agrunt/ag_fire3.wav" );
}

int CHgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
#if !CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
		{
			// in multiplayer, all hivehands come full. 
			pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = g_AmmoRegistry.GetMaxAmmo(PrimaryAmmoIndex());
		}
#endif
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( WeaponId() );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CHgun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hornets";
	p->pszAmmo2 = NULL;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = WeaponId();
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;
	p->pszAmmoEntity = NULL;
	p->iDropAmmo = 0;

	return 1;
}

BOOL CHgun::Deploy()
{
	return DefaultDeploy( "models/v_hgun.mdl", "models/p_hgun.mdl", HGUN_UP, "hive" );
}

void CHgun::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( HGUN_DOWN );

	//!!!HACKHACK - can't select hornetgun if it's empty! no way to get ammo for it, either.
	if( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}

void CHgun::PrimaryAttack()
{
	Reload();

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		return;
	}
#if !CLIENT_DLL
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	CBaseEntity *pHornet = CBaseEntity::Create( "hornet", m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -12.0f, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	pHornet->pev->velocity = gpGlobals->v_forward * 300.0f;

	float flRechargeTimePause = 0.5f;

	if( g_pGameRules->IsMultiplayer() )
		flRechargeTimePause = 0.3f;

	m_flRechargeTime = gpGlobals->time + flRechargeTimePause;
#endif
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usHornetFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.25f;

	if( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25f;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CHgun::SecondaryAttack( void )
{
	Reload();

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		return;
	}

	//Wouldn't be a bad idea to completely predict these, since they fly so fast...
#if !CLIENT_DLL
	CBaseEntity *pHornet;
	Vector vecSrc;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 8.0f + gpGlobals->v_up * -12.0f;

	m_iFirePhase++;
	switch( m_iFirePhase )
	{
	case 1:
		vecSrc = vecSrc + gpGlobals->v_up * 8.0f;
		break;
	case 2:
		vecSrc = vecSrc + gpGlobals->v_up * 8.0f;
		vecSrc = vecSrc + gpGlobals->v_right * 8.0f;
		break;
	case 3:
		vecSrc = vecSrc + gpGlobals->v_right * 8.0f;
		break;
	case 4:
		vecSrc = vecSrc + gpGlobals->v_up * -8.0f;
		vecSrc = vecSrc + gpGlobals->v_right * 8.0f;
		break;
	case 5:
		vecSrc = vecSrc + gpGlobals->v_up * -8.0f;
		break;
	case 6:
		vecSrc = vecSrc + gpGlobals->v_up * -8.0f;
		vecSrc = vecSrc + gpGlobals->v_right * -8.0f;
		break;
	case 7:
		vecSrc = vecSrc + gpGlobals->v_right * -8.0f;
		break;
	case 8:
		vecSrc = vecSrc + gpGlobals->v_up * 8.0f;
		vecSrc = vecSrc + gpGlobals->v_right * -8.0f;
		m_iFirePhase = 0;
		break;
	}

	pHornet = CBaseEntity::Create( "hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	pHornet->pev->velocity = gpGlobals->v_forward * 1200.0f;
	pHornet->pev->angles = UTIL_VecToAngles( pHornet->pev->velocity );

	pHornet->SetThink( &CHornet::StartDart );

	float flRechargeTimePause = 0.5f;

	if( g_pGameRules->IsMultiplayer() )
		flRechargeTimePause = 0.3f;

	m_flRechargeTime = gpGlobals->time + flRechargeTimePause;
#endif

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usHornetFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
}

void CHgun::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= g_AmmoRegistry.GetMaxAmmo(m_iPrimaryAmmoType) )
		return;

	while( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < g_AmmoRegistry.GetMaxAmmo(m_iPrimaryAmmoType) && m_flRechargeTime < gpGlobals->time )
	{
		float flRechargeTimePause = 0.5f;
		if( bIsMultiplayer() )
			flRechargeTimePause = 0.3f;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
		m_flRechargeTime += flRechargeTimePause;
	}
}

void CHgun::WeaponIdle( void )
{
	Reload();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
	if( flRand <= 0.75f )
	{
		iAnim = HGUN_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0f / 16.0f * 2.0f;
	}
	else if( flRand <= 0.875f )
	{
		iAnim = HGUN_FIDGETSWAY;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0f / 16.0f;
	}
	else
	{
		iAnim = HGUN_FIDGETSHAKE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 35.0f / 16.0f;
	}
	SendWeaponAnim( iAnim );
}
