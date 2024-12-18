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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"saverestore.h"
#include	"client.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"game.h"
#include	"pm_shared.h"
#include	"ent_templates.h"
#include	"studio.h"
#include	"scriptevent.h"

bool g_fIsXash3D = false;

void EntvarsKeyvalue( entvars_t *pev, KeyValueData *pkvd );

extern DLL_GLOBAL Vector g_vecAttackDir;
extern DLL_GLOBAL int g_iSkillLevel;

static DLL_FUNCTIONS gFunctionTable =
{
	GameDLLInit,				//pfnGameInit
	DispatchSpawn,				//pfnSpawn
	DispatchThink,				//pfnThink
	DispatchUse,				//pfnUse
	DispatchTouch,				//pfnTouch
	DispatchBlocked,			//pfnBlocked
	DispatchKeyValue,			//pfnKeyValue
	DispatchSave,				//pfnSave
	DispatchRestore,			//pfnRestore
	DispatchObjectCollsionBox,	//pfnAbsBox

	SaveWriteFields,			//pfnSaveWriteFields
	SaveReadFields,				//pfnSaveReadFields

	SaveGlobalState,			//pfnSaveGlobalState
	RestoreGlobalState,			//pfnRestoreGlobalState
	ResetGlobalState,			//pfnResetGlobalState

	ClientConnect,				//pfnClientConnect
	ClientDisconnect,			//pfnClientDisconnect
	ClientKill,					//pfnClientKill
	ClientPutInServer,			//pfnClientPutInServer
	ClientCommand,				//pfnClientCommand
	ClientUserInfoChanged,		//pfnClientUserInfoChanged
	ServerActivate,				//pfnServerActivate
	ServerDeactivate,			//pfnServerDeactivate

	PlayerPreThink,				//pfnPlayerPreThink
	PlayerPostThink,			//pfnPlayerPostThink

	StartFrame,					//pfnStartFrame
	ParmsNewLevel,				//pfnParmsNewLevel
	ParmsChangeLevel,			//pfnParmsChangeLevel

	GetGameDescription,         //pfnGetGameDescription    Returns string describing current .dll game.
	PlayerCustomization,        //pfnPlayerCustomization   Notifies .dll of new customization for player.

	SpectatorConnect,			//pfnSpectatorConnect      Called when spectator joins server
	SpectatorDisconnect,        //pfnSpectatorDisconnect   Called when spectator leaves the server
	SpectatorThink,				//pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

	Sys_Error,					//pfnSys_Error				Called when engine has encountered an error

	PM_Move,					//pfnPM_Move
	PM_Init,					//pfnPM_Init				Server version of player movement initialization
	PM_FindTextureType,			//pfnPM_FindTextureType

	SetupVisibility,			//pfnSetupVisibility        Set up PVS and PAS for networking for this client
	UpdateClientData,			//pfnUpdateClientData       Set up data sent only to specific client
	AddToFullPack,				//pfnAddToFullPack
	CreateBaseline,				//pfnCreateBaseline			Tweak entity baseline for network encoding, allows setup of player baselines, too.
	RegisterEncoders,			//pfnRegisterEncoders		Callbacks for network encoding
	GetWeaponData,				//pfnGetWeaponData
	CmdStart,					//pfnCmdStart
	CmdEnd,						//pfnCmdEnd
	ConnectionlessPacket,		//pfnConnectionlessPacket
	GetHullBounds,				//pfnGetHullBounds
	CreateInstancedBaselines,   //pfnCreateInstancedBaselines
	InconsistentFile,			//pfnInconsistentFile
	AllowLagCompensation,		//pfnAllowLagCompensation
};

void OnFreeEntPrivateData(edict_s* pEdict)
{
	entvars_t* pev = VARS(pEdict);
	if (pev && !FStringNull(pev->classname) && FStrEq(STRING(pev->classname), "worldspawn"))
	{
		ClearStringPool();
		ClearPrecachedModels();
		ClearPrecachedSounds();
	}
}

void GameDLLShutdown()
{
}

int ShouldCollide(edict_t *pentTouched, edict_t *pentOther)
{
	//if ((pentTouched->v.deadflag == DEAD_DEAD) && FStrEq(STRING(pentOther->v.classname), "func_pushable"))
	//	return 0;
	return 1;
}

NEW_DLL_FUNCTIONS gNewDLLFunctions =
{
	OnFreeEntPrivateData,
	GameDLLShutdown,
	ShouldCollide,
	0,
	0
};

static void SetObjectCollisionBox( entvars_t *pev );

#if !XASH_WIN32
extern "C" {
#endif
int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
{
	if( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
	{
		return 0;
	}
	
	memcpy( pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS) );
	return 1;
}

int GetEntityAPI2( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
	if( !pFunctionTable || *interfaceVersion != INTERFACE_VERSION )
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return 0;
	}

	memcpy( pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS) );
	return 1;
}

int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		ALERT(at_console, "Couldn't set new functions!\n");
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return 0;
	}

	ALERT(at_console, "Set new functions!\n");
	memcpy(pFunctionTable, &gNewDLLFunctions, sizeof(gNewDLLFunctions));
	return 1;
}

int Server_GetPhysicsInterface( int version, server_physics_api_t *api, physics_interface_t *interface )
{
	g_fIsXash3D = true;
	return 0; // do not tell engine to init physics interface, as we're not using it
}

#if !XASH_WIN32
}
#endif

int DispatchSpawn( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pent );

	if( pEntity )
	{
		// Initialize these or entities who don't link to the world won't have anything in here
		pEntity->pev->absmin = pEntity->pev->origin - Vector( 1.0f, 1.0f, 1.0f );
		pEntity->pev->absmax = pEntity->pev->origin + Vector( 1.0f, 1.0f, 1.0f );

		if (!pEntity->IsEnabledInMod())
			return -1;

		pEntity->Spawn();

		// Try to get the pointer again, in case the spawn function deleted the entity.
		// UNDONE: Spawn() should really return a code to ask that the entity be deleted, but
		// that would touch too much code for me to do that right now.
		pEntity = (CBaseEntity *)GET_PRIVATE( pent );

		if( pEntity )
		{
			if( g_pGameRules && !g_pGameRules->IsAllowedToSpawn( pEntity ) )
				return -1;	// return that this entity should be deleted
			if( pEntity->pev->flags & FL_KILLME )
				return -1;
		}

		// Handle global stuff here
		if( pEntity && pEntity->pev->globalname ) 
		{
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( pEntity->pev->globalname );
			if( pGlobal )
			{
				// Already dead? delete
				if( pGlobal->state == GLOBAL_DEAD )
					return -1;
				else if( !FStrEq( STRING( gpGlobals->mapname ), pGlobal->levelName ) )
					pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
				// In this level & not dead, continue on as normal
			}
			else
			{
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd( pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON );
				//ALERT( at_console, "Added global entity %s (%s)\n", STRING( pEntity->pev->classname ), STRING( pEntity->pev->globalname ) );
			}
		}

	}

	return 0;
}

void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
	if( !pkvd || !pentKeyvalue )
		return;

	// Get the actualy entity object
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pentKeyvalue );
	if (pEntity && pkvd->szClassName)
		pEntity->PreEntvarsKeyvalue(pkvd);
	if (pkvd->fHandled)
		return;

	EntvarsKeyvalue( VARS( pentKeyvalue ), pkvd );

	// If the key was an entity variable, or there's no class set yet, don't look for the object, it may
	// not exist yet.
	if ( pkvd->fHandled || pkvd->szClassName == NULL )
		return;

	if( !pEntity )
		return;

	pEntity->KeyValue( pkvd );
}

// HACKHACK -- this is a hack to keep the node graph entity from "touching" things (like triggers)
// while it builds the graph
bool gTouchDisabled = false;

void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
	if( gTouchDisabled )
		return;

	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pentTouched );
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE( pentOther );

	if( pEntity && pOther && ! ( ( pEntity->pev->flags | pOther->pev->flags ) & FL_KILLME ) )
		pEntity->Touch( pOther );
}

void DispatchUse( edict_t *pentUsed, edict_t *pentOther )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pentUsed );
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE( pentOther );

	if( pEntity && !( pEntity->pev->flags & FL_KILLME ) )
		pEntity->Use( pOther, pOther, USE_TOGGLE, 0 );
}

void DispatchThink( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pent );
	if( pEntity )
	{
		if( FBitSet( pEntity->pev->flags, FL_DORMANT ) )
			ALERT( at_error, "Dormant entity %s is thinking!!\n", STRING( pEntity->pev->classname ) );

		pEntity->Think();
	}
}

void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pentBlocked );
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE( pentOther );

	if( pEntity )
		pEntity->Blocked( pOther );
}

void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pent );

	if( pEntity && pSaveData )
	{
		ENTITYTABLE *pTable = &pSaveData->pTable[pSaveData->currentIndex];

		gpGlobals->time = pSaveData->time;

		if( pTable->pent != pent )
			ALERT( at_error, "ENTITY TABLE OR INDEX IS WRONG!!!!\n" );

		if( pEntity->ObjectCaps() & FCAP_DONT_SAVE )
			return;

		// These don't use ltime & nextthink as times really, but we'll fudge around it.
		if( pEntity->pev->movetype == MOVETYPE_PUSH )
		{
			float delta = pEntity->pev->nextthink - pEntity->pev->ltime;
			pEntity->pev->ltime = gpGlobals->time;
			pEntity->pev->nextthink = pEntity->pev->ltime + delta;
		}

		pTable->location = pSaveData->size;		// Remember entity position for file I/O
		pTable->classname = pEntity->pev->classname;	// Remember entity class for respawn

		CSave saveHelper( pSaveData );
		pEntity->Save( saveHelper );

		pTable->size = pSaveData->size - pTable->location;	// Size of entity block is data size written to block
	}
}

// Find the matching global entity.  Spit out an error if the designer made entities of
// different classes with the same global name
CBaseEntity *FindGlobalEntity( string_t classname, string_t globalname )
{
	edict_t *pent = FIND_ENTITY_BY_STRING( NULL, "globalname", STRING( globalname ) );
	CBaseEntity *pReturn = CBaseEntity::Instance( pent );
	if( pReturn )
	{
		if( !FClassnameIs( pReturn->pev, STRING( classname ) ) )
		{
			ALERT( at_console, "Global entity found %s, wrong class %s\n", STRING( globalname ), STRING( pReturn->pev->classname ) );
			pReturn = NULL;
		}
	}

	return pReturn;
}

int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pent );

	if( pEntity && pSaveData )
	{
		entvars_t tmpVars;
		Vector oldOffset;

		CRestore restoreHelper( pSaveData );

		gpGlobals->time = pSaveData->time;

		if( globalEntity )
		{
			CRestore tmpRestore( pSaveData );
			tmpRestore.PrecacheMode( false );
			tmpRestore.ReadEntVars( "ENTVARS", &tmpVars );

			// HACKHACK - reset the save pointers, we're going to restore for real this time
			pSaveData->size = pSaveData->pTable[pSaveData->currentIndex].location;
			pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;
			// -------------------

			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( tmpVars.globalname );

			// Don't overlay any instance of the global that isn't the latest
			// pSaveData->szCurrentMapName is the level this entity is coming from
			// pGlobla->levelName is the last level the global entity was active in.
			// If they aren't the same, then this global update is out of date.
			if( !FStrEq( pSaveData->szCurrentMapName, pGlobal->levelName ) )
				return 0;

			// Compute the new global offset
			oldOffset = pSaveData->vecLandmarkOffset;
			CBaseEntity *pNewEntity = FindGlobalEntity( tmpVars.classname, tmpVars.globalname );
			if( pNewEntity )
			{
				//ALERT( at_console, "Overlay %s with %s\n", STRING( pNewEntity->pev->classname ), STRING( tmpVars.classname ) );
				// Tell the restore code we're overlaying a global entity from another level
				restoreHelper.SetGlobalMode( 1 );	// Don't overwrite global fields
				pSaveData->vecLandmarkOffset = ( pSaveData->vecLandmarkOffset - pNewEntity->pev->mins ) + tmpVars.mins;
				pEntity = pNewEntity;// we're going to restore this data OVER the old entity
				pent = ENT( pEntity->pev );
				// Update the global table to say that the global definition of this entity should come from this level
				gGlobalState.EntityUpdate( pEntity->pev->globalname, gpGlobals->mapname );
			}
			else
			{
				// This entity will be freed automatically by the engine.  If we don't do a restore on a matching entity (below)
				// or call EntityUpdate() to move it to this level, we haven't changed global state at all.
				return 0;
			}
		}

		if( pEntity->ObjectCaps() & FCAP_MUST_SPAWN )
		{
			pEntity->Restore( restoreHelper );
			pEntity->Spawn();
		}
		else
		{
			pEntity->Restore( restoreHelper );
			pEntity->Precache();
		}

		// Again, could be deleted, get the pointer again.
		pEntity = (CBaseEntity *)GET_PRIVATE( pent );
#if 0
		if( pEntity && pEntity->pev->globalname && globalEntity ) 
		{
			ALERT( at_console, "Global %s is %s\n", STRING( pEntity->pev->globalname ), STRING( pEntity->pev->model ) );
		}
#endif
		// Is this an overriding global entity (coming over the transition), or one restoring in a level
		if( globalEntity )
		{
			//ALERT( at_console, "After: %f %f %f %s\n", pEntity->pev->origin.x, pEntity->pev->origin.y, pEntity->pev->origin.z, STRING( pEntity->pev->model ) );
			pSaveData->vecLandmarkOffset = oldOffset;
			if( pEntity )
			{
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );
				pEntity->OverrideReset();
			}
		}
		else if( pEntity && pEntity->pev->globalname ) 
		{
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( pEntity->pev->globalname );
			if( pGlobal )
			{
				// Already dead? delete
				if( pGlobal->state == GLOBAL_DEAD )
					return -1;
				else if( !FStrEq( STRING( gpGlobals->mapname ), pGlobal->levelName ) )
				{
					pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
				}
				// In this level & not dead, continue on as normal
			}
			else
			{
				ALERT( at_error, "Global Entity %s (%s) not in table!!!\n", STRING( pEntity->pev->globalname ), STRING( pEntity->pev->classname ) );
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd( pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON );
			}
		}
	}
	return 0;
}

void DispatchObjectCollsionBox( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pent );
	if( pEntity )
	{
		pEntity->SetObjectCollisionBox();
	}
	else
		SetObjectCollisionBox( &pent->v );
}

void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	CSave saveHelper( pSaveData );
	saveHelper.WriteFields( pname, pBaseData, pFields, fieldCount );
}

void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	CRestore restoreHelper( pSaveData );
	restoreHelper.ReadFields( pname, pBaseData, pFields, fieldCount );
}

edict_t *EHANDLE::Get( void ) 
{ 
	if( m_pent )
	{
		if( m_pent->serialnumber == m_serialnumber )
			return m_pent; 
		else
			return NULL;
	}
	return NULL; 
}

edict_t *EHANDLE::Set( edict_t *pent )
{
	if( pent )
	{
		m_pent = pent;
		m_serialnumber = m_pent->serialnumber;
	}
	else
	{
		m_pent = NULL;
		m_serialnumber = 0;
	}
	return pent; 
}

EHANDLE::operator CBaseEntity *() 
{ 
	return (CBaseEntity *)GET_PRIVATE( Get() ); 
}

CBaseEntity *EHANDLE::operator = ( CBaseEntity *pEntity )
{
	if( pEntity )
	{
		m_pent = ENT( pEntity->pev );
		if( m_pent )
			m_serialnumber = m_pent->serialnumber;
	}
	else
	{
		m_pent = NULL;
		m_serialnumber = 0;
	}
	return pEntity;
}

EHANDLE::operator int ()
{
	return Get() != NULL;
}

CBaseEntity * EHANDLE::operator -> ()
{
	return (CBaseEntity *)GET_PRIVATE( Get() ); 
}

// give health
int CBaseEntity::TakeHealth(CBaseEntity *pHealer, float flHealth, int bitsDamageType )
{
	if( !pev->takedamage )
		return 0;

	const bool overhealAllowed = FBitSet(bitsDamageType, HEAL_ALLOW_OVERFLOW);

	// heal
	if( pev->health >= pev->max_health && !overhealAllowed )
		return 0;

	if (flHealth <= 0)
		return 0;

	pev->health += flHealth;

	if( pev->health > pev->max_health && !overhealAllowed ) {
		flHealth -= (pev->health - pev->max_health);
		pev->health = pev->max_health;
	}

	return (int)flHealth;
}

// inflict damage on this entity.  bitsDamageType indicates type of damage inflicted, ie: DMG_CRUSH

void CBaseEntity::ApplyDamageToHealth(float flDamage)
{
	const float healthBeforeDamage = pev->health;

	// do the damage
	pev->health -= flDamage;

	if (m_healthMinThreshold > 0 && pev->health < m_healthMinThreshold)
	{
		if (IsPlayer())
		{
			pev->health = Q_max((int)m_healthMinThreshold, 1);
		}
		else
		{
			pev->health = Q_max(m_healthMinThreshold, 1.0f);
			pev->health = Q_min(healthBeforeDamage, pev->health);
		}
		m_healthMinThreshold = 0.0f;
	}
}

void CBaseEntity::SetNonLethalHealthThreshold()
{
	if (m_healthMinThreshold <= 0.0f)
		m_healthMinThreshold = 1.0f;
}

int CBaseEntity::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Vector vecTemp;

	if( !pev->takedamage )
		return 0;

	// UNDONE: some entity types may be immune or resistant to some bitsDamageType
	
	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
	if( pevAttacker == pevInflictor )	
	{
		vecTemp = pevAttacker->origin - VecBModelOrigin( pev );
	}
	else
	// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - VecBModelOrigin( pev );
	}

	// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();

	// save damage based on the target's armor level
	// figure momentum add (don't let hurt brushes or other triggers move player)
	if( ( !FNullEnt( pevInflictor ) ) && (pev->movetype == MOVETYPE_WALK || pev->movetype == MOVETYPE_STEP ) && ( pevAttacker->solid != SOLID_TRIGGER ) )
	{
		Vector vecDir = pev->origin - ( pevInflictor->absmin + pevInflictor->absmax ) * 0.5f;
		vecDir = vecDir.Normalize();

		float flForce = flDamage * ( ( 32.0f * 32.0f * 72.0f ) / ( pev->size.x * pev->size.y * pev->size.z ) ) * 5.0f;

		if( flForce > 1000.0f )
			flForce = 1000.0f;
		pev->velocity = pev->velocity + vecDir * flForce;
	}

	ApplyDamageToHealth(flDamage);

	if( pev->health <= 0 )
	{
		Killed( pevInflictor, pevAttacker, GIB_NORMAL );
		return 0;
	}

	return 1;
}

void CBaseEntity::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	UTIL_Remove( this );
}

CBaseEntity *CBaseEntity::GetNextTarget( void )
{
	if( FStringNull( pev->target ) )
		return NULL;
	edict_t *pTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );
	if( FNullEnt( pTarget ) )
		return NULL;

	return Instance( pTarget );
}

// Global Savedata for Delay
TYPEDESCRIPTION	CBaseEntity::m_SaveData[] =
{
	DEFINE_FIELD( CBaseEntity, m_pGoalEnt, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBaseEntity, m_EFlags, FIELD_CHARACTER ),

	DEFINE_FIELD( CBaseEntity, m_pfnThink, FIELD_FUNCTION ),		// UNDONE: Build table of these!!!
	DEFINE_FIELD( CBaseEntity, m_pfnTouch, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnUse, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnBlocked, FIELD_FUNCTION ),

	DEFINE_FIELD( CBaseEntity, m_entTemplate, FIELD_STRING ),
	DEFINE_FIELD( CBaseEntity, m_ownerEntTemplate, FIELD_STRING ),
	DEFINE_FIELD( CBaseEntity, m_soundList, FIELD_STRING ),
	DEFINE_FIELD( CBaseEntity, m_objectHint, FIELD_STRING ),
};

void CBaseEntity::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "soundlist")) {
		m_soundList = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	} else if (FStrEq(pkvd->szKeyName, "ent_template")) {
		m_entTemplate = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	} else if (FStrEq(pkvd->szKeyName, "objecthint")) {
		m_objectHint = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	} else {
		pkvd->fHandled = false;
	}
}

int CBaseEntity::PRECACHE_SOUND(const char *soundName)
{
	return PRECACHE_SOUND(soundName, m_soundList, GetMyEntTemplate());
}

int CBaseEntity::PRECACHE_SOUND(const char *soundName, string_t soundList, const EntTemplate *entTemplate)
{
	if (!FStringNull(soundList)) {
		if (g_soundReplacement.EnsureReplacementFile(STRING(soundList))) {
			const auto& replacement = g_soundReplacement.FindReplacement(STRING(soundList), soundName);
			if (!replacement.empty()) {
				return ::PRECACHE_SOUND(replacement.c_str());
			}
		}
	}
	if (entTemplate)
	{
		const char* replacement = entTemplate->GetSoundReplacement(soundName);
		if (replacement)
			return ::PRECACHE_SOUND(replacement);
	}
	return ::PRECACHE_SOUND(soundName);
}

bool CBaseEntity::EmitSoundDyn(int channel, const char *sample, float volume, float attenuation, int flags, int pitch)
{
	const char* soundToPlay = nullptr;
	if (!FStringNull(m_soundList)) {
		const auto& replacement = g_soundReplacement.FindReplacement(STRING(m_soundList), sample);
		if (!replacement.empty()) {
			soundToPlay = replacement.c_str();
		}
	}
	if (!soundToPlay)
	{
		const EntTemplate* entTemplate = GetMyEntTemplate();
		if (entTemplate)
		{
			soundToPlay = entTemplate->GetSoundReplacement(sample);
		}
	}
	if (!soundToPlay)
		soundToPlay = sample;
	return EMIT_SOUND_DYN(edict(), channel, soundToPlay, volume, attenuation, flags, pitch);
}

bool CBaseEntity::EmitSound(int channel, const char *sample, float volume, float attenuation)
{
	return EmitSoundDyn(channel, sample, volume, attenuation, 0, PITCH_NORM);
}

void CBaseEntity::EmitAmbientSound(const Vector &vecOrigin, const char *sample, float vol, float attenuation, int iFlags, int pitch)
{
	if (!FStringNull(m_soundList)) {
		const auto& replacement = g_soundReplacement.FindReplacement(STRING(m_soundList), sample);
		if (!replacement.empty()) {
			UTIL_EmitAmbientSound(edict(), vecOrigin, replacement.c_str(), vol, attenuation, iFlags, pitch);
			return;
		}
	}
	UTIL_EmitAmbientSound(edict(), vecOrigin, sample, vol, attenuation, iFlags, pitch);
}

void CBaseEntity::StopSound(int channel, const char *sample)
{
	if (!FStringNull(m_soundList)) {
		const auto& replacement = g_soundReplacement.FindReplacement(STRING(m_soundList), sample);
		if (!replacement.empty()) {
			STOP_SOUND(edict(), channel, replacement.c_str());
			return;
		}
	}
	STOP_SOUND(edict(), channel, sample);
}

const char* CBaseEntity::GetSoundScriptNameForTemplate(const char *name, const EntTemplate* entTemplate)
{
	return entTemplate ? entTemplate->GetSoundScriptNameOverride(name) : nullptr;
}

const char* CBaseEntity::GetSoundScriptNameForMyTemplate(const char *name, string_t* usedTemplate)
{
	const char* nameOverride = nullptr;
	if (usedTemplate)
		*usedTemplate = iStringNull;

	nameOverride = GetSoundScriptNameForTemplate(name, GetMyEntTemplate());
	if (nameOverride)
	{
		if (usedTemplate)
		{
			if (m_entTemplate)
				*usedTemplate = m_entTemplate;
			else
				*usedTemplate = pev->classname;
		}
		return nameOverride;
	}

	nameOverride = GetSoundScriptNameForTemplate(name, GetOwnerEntTemplate());
	if (nameOverride)
	{
		if (usedTemplate)
			*usedTemplate = m_ownerEntTemplate;
		return nameOverride;
	}

	return name;
}

const SoundScript* CBaseEntity::GetSoundScript(const char *name)
{
	name = GetSoundScriptNameForMyTemplate(name);
	return g_SoundScriptSystem.GetSoundScript(name);
}

bool CBaseEntity::EmitSoundScript(const SoundScript *soundScript, const SoundScriptParamOverride paramsOverride, int flags)
{
	if (soundScript)
	{
		const char* sample = soundScript->Wave();
		return EmitSoundScriptSelectedSample(soundScript, sample, paramsOverride, flags);
	}
	return false;
}

bool CBaseEntity::EmitSoundScript(const char *name, const SoundScriptParamOverride paramsOverride, int flags)
{
	const SoundScript* soundScript = GetSoundScript(name);
	if (soundScript)
	{
		return EmitSoundScript(soundScript, paramsOverride, flags);
	}
	return false;
}

bool CBaseEntity::EmitSoundScriptSelectedSample(const SoundScript* soundScript, int sampleIndex, const SoundScriptParamOverride paramsOverride, int flags)
{
	if (soundScript)
	{
		const char* sample = soundScript->Wave(sampleIndex);
		return EmitSoundScriptSelectedSample(soundScript, sample, paramsOverride, flags);
	}
	return false;
}

bool CBaseEntity::EmitSoundScriptSelectedSample(const SoundScript* soundScript, const char* sample, const SoundScriptParamOverride paramsOverride, int flags)
{
	if (soundScript && sample)
	{
		int channel = soundScript->channel;
		FloatRange volume = soundScript->volume;
		float attenuation = soundScript->attenuation;
		IntRange pitch = soundScript->pitch;

		paramsOverride.ApplyOverride(channel, volume, attenuation, pitch);

		return EmitSoundDyn(soundScript->channel, sample, RandomizeNumberFromRange(volume), attenuation, flags, RandomizeNumberFromRange(pitch));
	}
	return false;
}

bool CBaseEntity::EmitSoundScriptSelectedSample(const char* name, int sampleIndex, const SoundScriptParamOverride paramsOverride, int flags)
{
	return EmitSoundScriptSelectedSample(GetSoundScript(name), sampleIndex, paramsOverride, flags);
}

bool CBaseEntity::EmitSoundScriptSelectedSample(const char* name, const char* sample, const SoundScriptParamOverride paramsOverride, int flags)
{
	return EmitSoundScriptSelectedSample(GetSoundScript(name), sample, paramsOverride, flags);
}

void CBaseEntity::StopSoundScript(const SoundScript* soundScript)
{
	if (soundScript)
	{
		const char* sample = soundScript->Wave();
		// TODO: should we loop over all waves and stop each of them? We don't know which one has been playing
		StopSoundScriptSelectedSample(soundScript, sample);
	}
}

void CBaseEntity::StopSoundScript(const char *name)
{
	const SoundScript* soundScript = GetSoundScript(name);
	if (soundScript)
	{
		return StopSoundScript(soundScript);
	}
}

void CBaseEntity::StopSoundScriptSelectedSample(const SoundScript* soundScript, int sampleIndex)
{
	if (soundScript)
	{
		const char* sample = soundScript->Wave(sampleIndex);
		StopSoundScriptSelectedSample(soundScript, sample);
	}
}

void CBaseEntity::StopSoundScriptSelectedSample(const SoundScript* soundScript, const char* sample)
{
	if (soundScript && sample)
	{
		StopSound(soundScript->channel, sample);
	}
}

void CBaseEntity::StopSoundScriptSelectedSample(const char* name, int sampleIndex)
{
	StopSoundScriptSelectedSample(GetSoundScript(name), sampleIndex);
}

void CBaseEntity::StopSoundScriptSelectedSample(const char* name, const char* sample)
{
	StopSoundScriptSelectedSample(GetSoundScript(name), sample);
}

void CBaseEntity::EmitSoundScriptAmbient(const Vector& vecOrigin, const SoundScript* soundScript, const SoundScriptParamOverride paramsOverride, int flags)
{
	if (soundScript)
	{
		const char* sample = soundScript->Wave();
		if (sample)
		{
			int channel = soundScript->channel;
			FloatRange volume = soundScript->volume;
			float attenuation = soundScript->attenuation;
			IntRange pitch = soundScript->pitch;

			paramsOverride.ApplyOverride(channel, volume, attenuation, pitch);

			EmitAmbientSound(vecOrigin, sample, RandomizeNumberFromRange(volume), attenuation, flags, RandomizeNumberFromRange(pitch));
		}
	}
}

void CBaseEntity::EmitSoundScriptAmbient(const Vector& vecOrigin, const char *name, const SoundScriptParamOverride paramsOverride, int flags)
{
	const SoundScript* soundScript = GetSoundScript(name);
	if (soundScript)
	{
		return EmitSoundScriptAmbient(vecOrigin, soundScript, paramsOverride, flags);
	}
}

void CBaseEntity::PrecacheSoundScript(const SoundScript& soundScript)
{
	for (const auto& wave : soundScript.waves)
	{
		PRECACHE_SOUND(wave);
	}
}

void CBaseEntity::RegisterAndPrecacheSoundScriptByName(const char *name, const SoundScript &defaultSoundScript)
{
	const SoundScript* soundScript = g_SoundScriptSystem.ProvideDefaultSoundScript(GetSoundScriptNameForMyTemplate(name), defaultSoundScript);
	if (soundScript)
	{
		PrecacheSoundScript(*soundScript);
	}
}

void CBaseEntity::RegisterAndPrecacheSoundScript(const NamedSoundScript &defaultSoundScript)
{
	const SoundScript& soundScript = defaultSoundScript;
	RegisterAndPrecacheSoundScriptByName(defaultSoundScript.name, soundScript);
}

void CBaseEntity::RegisterAndPrecacheSoundScript(const char* derivative, const char* base, const SoundScript& defaultSoundScript, const SoundScriptParamOverride paramsOverride)
{
	string_t baseTemplate = iStringNull;
	const char* baseName = GetSoundScriptNameForMyTemplate(base, &baseTemplate);
	if (baseTemplate)
		g_EntTemplateSystem.EnsureSoundScriptReplacementForTemplate(STRING(baseTemplate), derivative);

	const SoundScript* soundScript = g_SoundScriptSystem.ProvideDefaultSoundScript(GetSoundScriptNameForMyTemplate(derivative), baseName, defaultSoundScript, paramsOverride);
	if (soundScript)
	{
		PrecacheSoundScript(*soundScript);
	}
}

void  CBaseEntity::RegisterAndPrecacheSoundScript(const char* derivative, const NamedSoundScript& defaultSoundScript, const SoundScriptParamOverride paramsOverride)
{
	RegisterAndPrecacheSoundScript(derivative, defaultSoundScript.name, defaultSoundScript, paramsOverride);
}

const char* CBaseEntity::GetVisualNameForTemplate(const char *name, const EntTemplate* entTemplate)
{
	return entTemplate ? entTemplate->GetVisualNameOverride(name) : nullptr;
}

const char* CBaseEntity::GetVisualNameForMyTemplate(const char *name, string_t* usedTemplate)
{
	const char* nameOverride = nullptr;
	if (usedTemplate)
		*usedTemplate = iStringNull;

	nameOverride = GetVisualNameForTemplate(name, GetMyEntTemplate());
	if (nameOverride)
	{
		if (usedTemplate)
		{
			if (m_entTemplate)
				*usedTemplate = m_entTemplate;
			else
				*usedTemplate = pev->classname;
		}
		return nameOverride;
	}

	nameOverride = GetVisualNameForTemplate(name, GetOwnerEntTemplate());
	if (nameOverride)
	{
		if (usedTemplate)
			*usedTemplate = m_ownerEntTemplate;
		return nameOverride;
	}

	return name;
}

const Visual* CBaseEntity::GetVisual(const char *name)
{
	name = GetVisualNameForMyTemplate(name);
	return g_VisualSystem.GetVisual(name);
}

const Visual* CBaseEntity::RegisterVisual(const NamedVisual &defaultVisual, bool precache, string_t* usedTemplate)
{
	if (defaultVisual.mixin)
	{
		string_t mixinTemplate = iStringNull;
		const Visual* visual = RegisterVisual(*defaultVisual.mixin, false, &mixinTemplate);
		Visual changedVisual = defaultVisual;
		changedVisual.CompleteFrom(*visual);
		if (mixinTemplate)
			g_EntTemplateSystem.EnsureVisualReplacementForTemplate(STRING(mixinTemplate), defaultVisual.name);
		return g_VisualSystem.ProvideDefaultVisual(GetVisualNameForMyTemplate(defaultVisual.name, usedTemplate), changedVisual, precache);
	}
	else
		return g_VisualSystem.ProvideDefaultVisual(GetVisualNameForMyTemplate(defaultVisual.name, usedTemplate), defaultVisual, precache);
}

void CBaseEntity::AssignEntityOverrides(EntityOverrides entityOverrides)
{
	if (entityOverrides.model)
		pev->model = entityOverrides.model;
	m_entTemplate = entityOverrides.entTemplate;
	m_ownerEntTemplate = entityOverrides.ownerEntTemplate;
	m_soundList = entityOverrides.soundList;
}

EntityOverrides CBaseEntity::GetProjectileOverrides()
{
	EntityOverrides entityOverrides;
	entityOverrides.ownerEntTemplate = GetMyTemplateName();
	entityOverrides.soundList = m_soundList;
	return entityOverrides;
}

int CBaseEntity::OverridenRenderProps()
{
	int defined = 0;
	if (pev->model)
		defined |= Visual::MODEL_DEFINED;
	if (pev->renderamt)
		defined |= Visual::ALPHA_DEFINED;
	if (pev->rendermode)
		defined |= Visual::RENDERMODE_DEFINED;
	if (pev->rendercolor != g_vecZero)
		defined |= Visual::COLOR_DEFINED;
	if (pev->renderfx)
		defined |= Visual::RENDERFX_DEFINED;
	if (pev->scale)
		defined |= Visual::SCALE_DEFINED;
	return defined;
}

static bool CheckVisualDefine(const Visual* visual, int param, int ignored)
{
	return visual->HasDefined(param) && (ignored & param) == 0;
}

void CBaseEntity::ApplyVisual(const Visual *visual, const char* modelOverride)
{
	const int alreadyOverriden = OverridenRenderProps();
	ApplyDefaultRenderProps(alreadyOverriden);
	const char* model = modelOverride;
	if (!model && pev->model)
		model = STRING(pev->model);

	if (!visual) {
		if (model)
			SET_MODEL(edict(), model);
		return;
	}

	if (!model && CheckVisualDefine(visual, Visual::MODEL_DEFINED, alreadyOverriden) && visual->model && *visual->model)
		model = visual->model;

	if (model)
		SET_MODEL(edict(), model);

	if (CheckVisualDefine(visual, Visual::RENDERMODE_DEFINED, alreadyOverriden))
		pev->rendermode = visual->rendermode;
	if (CheckVisualDefine(visual, Visual::COLOR_DEFINED, alreadyOverriden))
		pev->rendercolor = VectorFromColor(visual->rendercolor);
	if (CheckVisualDefine(visual, Visual::ALPHA_DEFINED, alreadyOverriden))
		pev->renderamt = visual->renderamt;
	if (CheckVisualDefine(visual, Visual::RENDERFX_DEFINED, alreadyOverriden))
		pev->renderfx = visual->renderfx;
	if (CheckVisualDefine(visual, Visual::SCALE_DEFINED, alreadyOverriden))
		pev->scale = RandomizeNumberFromRange(visual->scale);
	if (CheckVisualDefine(visual, Visual::FRAMERATE_DEFINED, alreadyOverriden))
		pev->framerate = RandomizeNumberFromRange(visual->framerate);
}

const EntTemplate* CBaseEntity::GetCacheableEntTemplate(entvars_t* pev, string_t templateName, const EntTemplate*& entTemplate, bool& templateChecked, bool checkByClassname)
{
	if (templateChecked)
	{
		return entTemplate;
	}
	if (!FStringNull(templateName))
	{
		entTemplate = g_EntTemplateSystem.GetTemplate(STRING(templateName));
		templateChecked = true;
		return entTemplate;
	}
	else if (checkByClassname)
	{
		entTemplate = g_EntTemplateSystem.GetTemplate(STRING(pev->classname));
		templateChecked = true;
		return entTemplate;
	}
	else
	{
		entTemplate = nullptr;
		templateChecked = true;
		return entTemplate;
	}
}

const EntTemplate* CBaseEntity::GetMyEntTemplate()
{
	return GetCacheableEntTemplate(pev, m_entTemplate, m_cachedEntTemplate, m_entTemplateChecked, true);
}

string_t CBaseEntity::GetMyTemplateName()
{
	if (FStringNull(m_entTemplate))
	{
		if (GetMyEntTemplate())
			return pev->classname;
	}
	return m_entTemplate;
}

const EntTemplate* CBaseEntity::GetOwnerEntTemplate()
{
	return GetCacheableEntTemplate(pev, m_ownerEntTemplate, m_cachedOwnerEntTemplate, m_ownerEntTemplateChecked, false);
}

bool CBaseEntity::ShouldAutoPrecacheSounds()
{
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate)
		return entTemplate->AutoPrecacheSounds();
	return false;
}

void CBaseEntity::SetMyHealth(const float defaultHealth)
{
	if (!pev->health) {
		const EntTemplate* entTemplate = GetMyEntTemplate();
		if (entTemplate && entTemplate->IsHealthDefined())
			pev->health = entTemplate->Health();
		else
			pev->health = defaultHealth;
	}
}

const Visual* CBaseEntity::MyOwnVisual()
{
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate)
		return g_VisualSystem.GetVisual(entTemplate->OwnVisualName());
	return nullptr;
}

const char* CBaseEntity::MyOwnModel(const char *defaultModel)
{
	if (!FStringNull(pev->model))
		return STRING(pev->model);

	const Visual* ownVisual = MyOwnVisual();
	if (ownVisual && ownVisual->model)
		return ownVisual->model;

#if FEATURE_REVERSE_RELATIONSHIP_MODELS
	if (m_reverseRelationship)
	{
		const char* reverseModel = ReverseRelationshipModel();
		if (reverseModel)
			return reverseModel;
	}
#endif
	return defaultModel;
}

void CBaseEntity::SetMyModel(const char *defaultModel)
{
	ApplyVisual(MyOwnVisual());

	if (FStringNull(pev->model))
	{
		if (defaultModel)
			SET_MODEL(ENT(pev), defaultModel);
	}
}

void CBaseEntity::PrecacheMyModel(const char *defaultModel)
{
	const char* myModel = MyOwnModel(defaultModel);
	if (myModel)
		PRECACHE_MODEL(myModel);
}

int CBaseEntity::Save( CSave &save )
{
	if( save.WriteEntVars( "ENTVARS", pev ) )
		return save.WriteFields( "BASE", this, m_SaveData, ARRAYSIZE( m_SaveData ) );

	return 0;
}

int CBaseEntity::Restore( CRestore &restore )
{
	int status;

	status = restore.ReadEntVars( "ENTVARS", pev );
	if( status )
		status = restore.ReadFields( "BASE", this, m_SaveData, ARRAYSIZE( m_SaveData ) );

	if( pev->modelindex != 0 && !FStringNull( pev->model ) )
	{
		Vector mins, maxs;
		mins = pev->mins;	// Set model is about to destroy these
		maxs = pev->maxs;

		PRECACHE_MODEL( STRING( pev->model ) );
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
		UTIL_SetSize( pev, mins, maxs );	// Reset them
	}

	return status;
}

static void PrecacheSequenceSounds(CBaseEntity* pEntity, bool precacheSounds, bool precacheSoundScripts)
{
	if (!precacheSounds && !precacheSoundScripts)
		return;

	void *pmodel = GET_MODEL_PTR( pEntity->edict() );
	if (!pmodel)
		return;

	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	for( int i = 0; i < pstudiohdr->numseq; i++ )
	{
		mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)( (byte *)pstudiohdr + pstudiohdr->seqindex ) + i;
		mstudioevent_t *pevent = (mstudioevent_t *)( (byte *)pstudiohdr + pseqdesc->eventindex );

		for( int j = 0; j < pseqdesc->numevents; j++ )
		{
			switch (pevent[j].event) {
			case SCRIPT_EVENT_SOUND:
			case SCRIPT_EVENT_SOUND_VOICE:
			case SCRIPT_EVENT_SOUND_VOICE_BODY:
			case SCRIPT_EVENT_SOUND_VOICE_VOICE:
			case SCRIPT_EVENT_SOUND_VOICE_WEAPON:
			{
				if (precacheSounds && *pevent[j].options != '\0')
				{
					string_t s = ALLOC_STRING(pevent[j].options);
					PRECACHE_SOUND(STRING(s));
				}
			}
				break;
			case SCRIPT_EVENT_SOUNDSCRIPT:
			{
				if (precacheSoundScripts && *pevent[j].options != '\0')
				{
					const SoundScript* soundScript = pEntity->GetSoundScript(pevent[j].options);
					if (soundScript)
						pEntity->PrecacheSoundScript(*soundScript);
				}
			}
				break;
			default:
				break;
			}
		}
	}
}

void CBaseEntity::Activate()
{
	const EntTemplate* entTemplate = GetMyEntTemplate();
	if (entTemplate)
	{
		for (auto soundIt = entTemplate->PrecachedSoundsBegin(); soundIt != entTemplate->PrecachedSoundsEnd(); ++soundIt)
		{
			PRECACHE_SOUND(soundIt->c_str());
		}
		for (auto soundIt = entTemplate->PrecachedSoundScriptsBegin(); soundIt != entTemplate->PrecachedSoundScriptsEnd(); ++soundIt)
		{
			const SoundScript* soundScript = GetSoundScript(soundIt->c_str());
			if (soundScript)
				PrecacheSoundScript(*soundScript);
		}

		PrecacheSequenceSounds(this, entTemplate->AutoPrecacheSounds(), entTemplate->AutoPrecacheSoundScripts());
	}
}

// Initialize absmin & absmax to the appropriate box
void SetObjectCollisionBox( entvars_t *pev )
{
	if( ( pev->solid == SOLID_BSP ) && 
		 ( pev->angles.x || pev->angles.y || pev->angles.z ) )
	{
		// expand for rotation
		float max, v;
		int i;

		max = 0;
		for( i = 0; i < 3; i++ )
		{
			v = fabs( ( (float *)pev->mins )[i] );
			if( v > max )
				max = v;
			v = fabs( ( (float *)pev->maxs )[i] );
			if( v > max )
				max = v;
		}
		for( i = 0; i < 3; i++ )
		{
			( (float *)pev->absmin )[i] = ( (float *)pev->origin )[i] - max;
			( (float *)pev->absmax )[i] = ( (float *)pev->origin )[i] + max;
		}
	}
	else
	{
		pev->absmin = pev->origin + pev->mins;
		pev->absmax = pev->origin + pev->maxs;
	}

	pev->absmin.x -= 1;
	pev->absmin.y -= 1;
	pev->absmin.z -= 1;
	pev->absmax.x += 1;
	pev->absmax.y += 1;
	pev->absmax.z += 1;
}

void CBaseEntity::SetObjectCollisionBox( void )
{
	::SetObjectCollisionBox( pev );
}

int CBaseEntity::Intersects( CBaseEntity *pOther )
{
	if( pOther->pev->absmin.x > pev->absmax.x ||
		pOther->pev->absmin.y > pev->absmax.y ||
		pOther->pev->absmin.z > pev->absmax.z ||
		pOther->pev->absmax.x < pev->absmin.x ||
		pOther->pev->absmax.y < pev->absmin.y ||
		pOther->pev->absmax.z < pev->absmin.z )
		return 0;
	return 1;
}

void CBaseEntity::MakeDormant( void )
{
	SetBits( pev->flags, FL_DORMANT );
	
	// Don't touch
	pev->solid = SOLID_NOT;
	// Don't move
	pev->movetype = MOVETYPE_NONE;
	// Don't draw
	SetBits( pev->effects, EF_NODRAW );
	// Don't think
	pev->nextthink = 0.0f;
	// Relink
	UTIL_SetOrigin( pev, pev->origin );
}

int CBaseEntity::IsDormant( void )
{
	return FBitSet( pev->flags, FL_DORMANT );
}

bool CBaseEntity::IsInWorld( void )
{
	// position 
	if( pev->origin.x >= 4096.0f )
		return false;
	if( pev->origin.y >= 4096.0f )
		return false;
	if( pev->origin.z >= 4096.0f )
		return false;
	if( pev->origin.x <= -4096.0f )
		return false;
	if( pev->origin.y <= -4096.0f )
		return false;
	if( pev->origin.z <= -4096.0f )
		return false;
	// speed
	if( pev->velocity.x >= 2000.0f )
		return false;
	if( pev->velocity.y >= 2000.0f )
		return false;
	if( pev->velocity.z >= 2000.0f )
		return false;
	if( pev->velocity.x <= -2000.0f )
		return false;
	if( pev->velocity.y <= -2000.0f )
		return false;
	if( pev->velocity.z <= -2000.0f )
		return false;

	return true;
}

bool CBaseEntity::ShouldToggle( USE_TYPE useType, bool currentState )
{
	if( useType != USE_TOGGLE && useType != USE_SET )
	{
		if( ( currentState && useType == USE_ON ) || ( !currentState && useType == USE_OFF ) )
			return false;
	}
	return true;
}

int CBaseEntity::DamageDecal( int bitsDamageType )
{
	if( pev->rendermode == kRenderTransAlpha )
		return -1;

	if( pev->rendermode != kRenderNormal )
		return DECAL_BPROOF1;

	return DECAL_GUNSHOT1 + RANDOM_LONG( 0, 4 );
}

// NOTE: szName must be a pointer to constant memory, e.g. "monster_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity *CBaseEntity::Create( const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner, EntityOverrides entityOverrides )
{
	CBaseEntity *pEntity = CreateNoSpawn(szName, vecOrigin, vecAngles, pentOwner, entityOverrides);
	if (pEntity)
	{
		if (DispatchSpawn( pEntity->edict() ) == -1 )
		{
			REMOVE_ENTITY(pEntity->edict());
			return 0;
		}
	}
	return pEntity;
}

/*
 * Same as Create, but does not call DispatchSpawn. This allows to change some parameters before call to Spawn()
 */
CBaseEntity *CBaseEntity::CreateNoSpawn( const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner, EntityOverrides entityOverrides )
{
	edict_t	*pent;
	CBaseEntity *pEntity;

	if( !szName )
	{
		ALERT( at_console, "Create() - No item name!\n" );
		return NULL;
	}

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szName ) );
	if( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in Create! (%s)\n", szName );
		return NULL;
	}
	pEntity = Instance( pent );
	pEntity->pev->owner = pentOwner;
	pEntity->pev->origin = vecOrigin;
	pEntity->pev->angles = vecAngles;
	pEntity->AssignEntityOverrides(entityOverrides);
	return pEntity;
}
