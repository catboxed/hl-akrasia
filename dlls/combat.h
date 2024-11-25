#pragma once
#ifndef COMBAT_H
#define COMBAT_H

#include "cbase.h"
#include "bullet_types.h"
#include "cone_degrees.h"

#define DEFAULT_EXPLOSION_RADIUS_MULTIPLIER 2.5f

#define BIG_EXPLOSION_VOLUME	2048
#define NORMAL_EXPLOSION_VOLUME	1024
#define SMALL_EXPLOSION_VOLUME	512

typedef struct
{
	CBaseEntity		*pEntity;
	float			amount;
	int				type;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;

extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker );
extern void AddMultiDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, CBaseEntity *pEntity, float flDamage, int bitsDamageType);

extern void DecalGunshot( TraceResult *pTrace, int iBulletType );
extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern int DamageDecal( CBaseEntity *pEntity, int bitsDamageType );
extern void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );

enum
{
	RADIUSDAMAGE_FIX_GRENADE_POS = (1<<0),
	RADIUSDAMAGE_DONT_TRAVEL_THROUGH_WATER = (1<<1),
	RADIUSDAMAGE_CHECK_VISIBLE = (1<<2),
	RADIUSDAMAGE_SPOT_IS_TARGET_CENTER = (1<<3),
	RADIUSDAMAGE_APPLY_FALLOFF = (1<<4)
};

template<typename Filter>
void RadiusDamage(CBaseEntity* pLooker, Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int bitsDamageType, int flags, Filter filter)
{
	CBaseEntity *pEntity = nullptr;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0f;

	int bInWater = 0;
	if (flags & RADIUSDAMAGE_DONT_TRAVEL_THROUGH_WATER)
		bInWater = UTIL_PointContents( vecSrc ) == CONTENTS_WATER;

	if (flags & RADIUSDAMAGE_FIX_GRENADE_POS)
		vecSrc.z += 1.0f;// in case grenade is lying on the ground

	if( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius ) ) != NULL )
	{
		if( pEntity->pev->takedamage != DAMAGE_NO )
		{
			if(!filter(pEntity))
			{
				continue;
			}

			// blast's don't tavel into or out of water
			if (flags & RADIUSDAMAGE_DONT_TRAVEL_THROUGH_WATER)
			{
				if( bInWater && pEntity->pev->waterlevel == WL_NotInWater )
					continue;
				if( !bInWater && pEntity->pev->waterlevel == WL_Eyes )
					continue;
			}

			if (flags & RADIUSDAMAGE_SPOT_IS_TARGET_CENTER)
				vecSpot = pEntity->Center();
			else
				vecSpot = pEntity->BodyTarget( vecSrc );

			if (pLooker != nullptr)
			{
				flAdjustedDamage = flDamage;

				if (flags & RADIUSDAMAGE_APPLY_FALLOFF)
					flAdjustedDamage -= (vecSpot - vecSrc).Length() * falloff;

				if( !pLooker->FVisible( pEntity ) )
				{
					if( pEntity->IsPlayer() )
					{
						// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still
						// take the residual damage if they don't totally leave the houndeye's effective radius. We restrict it to clients
						// so that monsters in other parts of the level don't take the damage and get pissed.
						flAdjustedDamage *= 0.5f;
					}
					else if( !FClassnameIs( pEntity->pev, "func_breakable" ) && !FClassnameIs( pEntity->pev, "func_pushable" ) )
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flAdjustedDamage = 0.0f;
					}
				}

				if( flAdjustedDamage > 0.0f )
				{
					pEntity->TakeDamage( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
			else
			{
				UTIL_TraceLine( vecSrc, vecSpot, dont_ignore_monsters, ENT( pevInflictor ), &tr );

				if( tr.flFraction == 1.0f || tr.pHit == pEntity->edict() )
				{
					// the explosion can 'see' this entity, so hurt them!
					if( tr.fStartSolid )
					{
						// if we're stuck inside them, fixup the position and distance
						tr.vecEndPos = vecSrc;
						tr.flFraction = 0.0f;
					}

					// decrease damage for an ent that's farther from the bomb.
					flAdjustedDamage = flDamage;
					if (flags & RADIUSDAMAGE_APPLY_FALLOFF)
						flAdjustedDamage -= ( vecSrc - tr.vecEndPos ).Length() * falloff;

					if (flAdjustedDamage > 0.0f)
					{
						if( tr.flFraction != 1.0f )
						{
							pEntity->ApplyTraceAttack( pevInflictor, pevAttacker, flAdjustedDamage, ( tr.vecEndPos - vecSrc ).Normalize(), &tr, bitsDamageType );
						}
						else
						{
							pEntity->TakeDamage( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
						}
					}
				}
			}
		}
	}
}

#endif
