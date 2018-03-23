/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#if FEATURE_M249

enum m249_e
{
	M249_SLOWIDLE = 0,
	M249_IDLE2,
	M249_LAUNCH,
	M249_RELOAD1,
	M249_HOLSTER,
	M249_DEPLOY,
	M249_SHOOT1,
	M249_SHOOT2,
	M249_SHOOT3,
};


LINK_ENTITY_TO_CLASS(weapon_m249, CM249)

//=========================================================
//=========================================================

void CM249::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_saw.mdl");
	m_iId = WEAPON_M249;

	m_iDefaultAmmo = M249_DEFAULT_GIVE;

	m_iReloadState = RELOAD_STATE_NONE;

	FallInit();// get ready to fall down.
}


void CM249::Precache(void)
{
	PRECACHE_MODEL("models/v_saw.mdl");
	PRECACHE_MODEL("models/w_saw.mdl");
	PRECACHE_MODEL("models/p_saw.mdl");

	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_saw_clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/saw_fire1.wav");
	PRECACHE_SOUND("weapons/saw_fire2.wav");
	PRECACHE_SOUND("weapons/saw_fire3.wav");

	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usM249 = PRECACHE_EVENT(1, "events/m249.sc");
}

int CM249::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = _556_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M249_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M249;
	p->iWeight = M249_WEIGHT;

	return 1;
}

int CM249::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
			WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CM249::Deploy()
{
	m_iReloadState = 0;
	UpdateTape();
	return DefaultDeploy("models/v_saw.mdl", "models/p_saw.mdl", M249_DEPLOY, "mp5", UseDecrement(), pev->body);
}

void CM249::Holster()
{
	m_iReloadState = 0;
	CBasePlayerWeapon::Holster();
}

void CM249::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -1.0f, 1.0f );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -2.0f, 2.0f );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;
	UpdateTape();
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

#ifdef CLIENT_DLL
	if (bIsMultiplayer())
#else
	if (g_pGameRules->IsMultiplayer())
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usM249, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, pev->body, 0, 0);


#ifndef CLIENT_DLL
        Vector vecVelocity = m_pPlayer->pev->velocity;
        Vector vecInvPushDir = gpGlobals->v_forward * 35.0;

        float flNewZVel = CVAR_GET_FLOAT( "sv_maxspeed" );

        if( vecInvPushDir.z >= 10.0 )
            flNewZVel = vecInvPushDir.z;

        Vector vecNewVel;

        if( g_pGameRules->IsDeathmatch() )
        {
            vecNewVel = vecVelocity - vecInvPushDir;
        }
        else
        {
            vecNewVel = vecVelocity;

            float flZTreshold = -( flNewZVel + 100.0 );

            if( vecVelocity.x > flZTreshold )
            {
                vecNewVel.x -= vecInvPushDir.x;
            }

            if( vecVelocity.y > flZTreshold )
            {
                vecNewVel.y -= vecInvPushDir.y;
            }
        }
        m_pPlayer->pev->velocity = vecNewVel;
#endif

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.067);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CM249::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == M249_MAX_CLIP)
		return;

	if (DefaultReload(M249_MAX_CLIP, M249_LAUNCH, 1, pev->body)) {
		m_iReloadState = 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3.78;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.78;
		m_flReloadStart = gpGlobals->time;
	}
}


void CM249::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if ( m_iReloadState && gpGlobals->time >= m_flReloadStart + 1.33 )
	{
		UpdateTape();
		m_iReloadState = 0;
		SendWeaponAnim( M249_RELOAD1, UseDecrement(), pev->body );
		return;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
	int iAnim;
	if (flRand <= 0.8) {
		iAnim = M249_SLOWIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
	} else {
		iAnim = M249_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 155.0/25.0;
	}

	SendWeaponAnim(iAnim, UseDecrement(), pev->body);
}

void CM249::UpdateTape()
{
	if (m_iClip == 0) {
		pev->body = 8;
	} else if (m_iClip > 0 && m_iClip < 8) {
		pev->body = 9 - m_iClip;
	} else {
		pev->body = 0;
	}
}

class CM249AmmoClip : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_saw_clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_saw_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_556CLIP_GIVE, "556", _556_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, CM249AmmoClip)

#endif
