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
==========================
This file contains "stubs" of class member implementations so that we can predict certain
 weapons client side.  From time to time you might find that you need to implement part of the
 these functions.  If so, cut it from here, paste it in hl_weapons.cpp or somewhere else and
 add in the functionality you need.
==========================
*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"nodes.h"
#include	"skill.h"

// Globals used by game logic
const Vector g_vecZero = Vector( 0, 0, 0 );
int gmsgWeapPickup = 0;
enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;

ItemInfo CBasePlayerWeapon::ItemInfoArray[MAX_WEAPONS];

bool EMIT_SOUND_DYN( edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch ) { return true; }
int PRECACHE_MODEL(const char* name) {return 0;}
int PRECACHE_SOUND(const char* name) {return 0;}
void SET_MODEL(edict_t *e, const char *m) {}

// CBaseEntity Stubs
int CBaseEntity::TakeHealth( CBaseEntity* pHealer, float flHealth, int bitsDamageType ) { return 1; }
int CBaseEntity::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) { return 1; }
CBaseEntity *CBaseEntity::GetNextTarget( void ) { return NULL; }
void CBaseEntity::KeyValue( KeyValueData* pkvd ) { pkvd->fHandled = FALSE; }
int CBaseEntity::Save( CSave &save ) { return 1; }
int CBaseEntity::Restore( CRestore &restore ) { return 1; }
void CBaseEntity::Activate() {}
void CBaseEntity::SetObjectCollisionBox( void ) { }
BOOL CBaseEntity::IsInWorld( void ) { return TRUE; }
int CBaseEntity::DamageDecal( int bitsDamageType ) { return -1; }
void CBaseEntity::UpdateOnRemove( void ) { }
int CBaseEntity::PRECACHE_SOUND(const char *soundName) { return 0; }

// CBaseDelay Stubs
void CBaseDelay::KeyValue( struct KeyValueData_s * ) { }
int CBaseDelay::Restore( class CRestore & ) { return 1; }
int CBaseDelay::Save( class CSave & ) { return 1; }

// CBaseAnimating Stubs
int CBaseAnimating::Restore( class CRestore & ) { return 1; }
int CBaseAnimating::Save( class CSave & ) { return 1; }

// DEBUG Stubs
edict_t *DBG_EntOfVars( const entvars_t *pev ) { return NULL; }
void DBG_AssertFunction( BOOL fExpr, const char *szExpr, const char *szFile, int szLine, const char *szMessage) { }

// UTIL_* Stubs
void UTIL_PrecacheOther( const char *szClassname, EntityOverrides entityOverrides ) { }
void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount ) { }
void UTIL_DecalTrace( TraceResult *pTrace, int decalNumber ) { }
void UTIL_GunshotDecalTrace( TraceResult *pTrace, int decalNumber ) { }
void UTIL_MakeVectors( const Vector &vecAngles ) { }
BOOL UTIL_IsValidEntity( edict_t *pent ) { return TRUE; }
void UTIL_SetOrigin( entvars_t *, const Vector &org ) { }
void UTIL_LogPrintf(char *,...) { }
void UTIL_ClientPrintAll( int,char const *,char const *,char const *,char const *,char const *) { }
void ClientPrint( entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 ) { }

// CBaseToggle Stubs
int CBaseToggle::Restore( class CRestore & ) { return 1; }
int CBaseToggle::Save( class CSave & ) { return 1; }
void CBaseToggle::KeyValue( struct KeyValueData_s * ) { }

void UTIL_Remove( CBaseEntity *pEntity ){ }
struct skilldata_t gSkillData;
void UTIL_SetSize( entvars_t *pev, const Vector &vecMin, const Vector &vecMax ){ }
CBaseEntity *UTIL_FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius ){ return 0;}

CBaseEntity* CBaseMonster::CheckTraceHullAttack( float flDist, int iDamage, int iDmgType ) { return NULL; }
void CBaseMonster::BarnacleVictimBitten( entvars_t *pevBarnacle ) { }
void CBaseMonster::BarnacleVictimReleased( void ) { }
BOOL CBaseMonster::FValidateHintType( short sHint ) { return FALSE; }
void CBaseMonster::Look( int iDistance ) { }
int CBaseMonster::DefaultISoundMask( void ) { return 0; }
CSound *CBaseMonster::PBestSound( void ) { return NULL; }
CSound *CBaseMonster::PBestScent( void ) { return NULL; } 
float CBaseAnimating::StudioFrameAdvance( float flInterval ) { return 0.0; }
void CBaseMonster::MonsterThink( void ) { }
int CBaseMonster::IgnoreConditions( void ) { return 0; }
BOOL CBaseMonster::FBecomeProne( void ) { return TRUE; }
BOOL CBaseMonster::CheckRangeAttack1( float flDot, float flDist ) { return FALSE; }
BOOL CBaseMonster::CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
BOOL CBaseMonster::CheckMeleeAttack1( float flDot, float flDist ) { return FALSE; }
BOOL CBaseMonster::CheckMeleeAttack2( float flDot, float flDist ) { return FALSE; }
BOOL CBaseMonster::FCanCheckAttacks( void ) { return FALSE; }
int CBaseMonster::CheckEnemy( CBaseEntity *pEnemy ) { return 0; }
void CBaseMonster::SetActivity( Activity NewActivity ) { }
int CBaseMonster::CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist ) { return 0; }
void CBaseMonster::Move( float flInterval ) { }
BOOL CBaseMonster::ShouldAdvanceRoute( float flWaypointDist ) { return FALSE; }
void CBaseMonster::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval ) { }
void CBaseMonster::MonsterInit( void ) { }
void CBaseMonster::MonsterInitThink( void ) { }
void CBaseMonster::StartMonster( void ) { }
int CBaseMonster::IRelationship( CBaseEntity *pTarget ) { return 0; }
BOOL CBaseMonster::BuildNearestRoute( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist ) { return FALSE; }
CBaseEntity *CBaseMonster::BestVisibleEnemy( void ) { return NULL; }
BOOL CBaseMonster::FInViewCone( CBaseEntity *pEntity ) { return FALSE; }
BOOL CBaseMonster::FInViewCone( Vector *pOrigin ) { return FALSE; }
BOOL CBaseEntity::FVisible( CBaseEntity *pEntity ) { return FALSE; }
BOOL CBaseEntity::FVisible( const Vector &vecOrigin ) { return FALSE; }
float CBaseMonster::ChangeYaw( int yawSpeed ) { return 0; }
int CBaseAnimating::LookupActivity( int activity ) { return 0; }
void CBaseMonster::HandleAnimEvent( MonsterEvent_t *pEvent ) { }
Vector CBaseMonster::GetGunPosition( void ) { return g_vecZero; }
void CBaseEntity::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType ) { }
void CBaseEntity::FireBullets( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker ) { }
void CBaseEntity::TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType ) { }
void CBaseMonster::ReportAIState( ALERT_TYPE ) { }
void CBaseMonster::KeyValue( KeyValueData *pkvd ) { }
void CBaseMonster::Activate() {}
int CBaseMonster::CanPlaySequence( int interruptFlags ) { return FALSE; }
BOOL CBaseMonster::FCanActiveIdle( void ) { return FALSE; }
bool CBaseToggle::PlaySentence( const char *pszSentence, float duration, float volume, float attenuation, bool subtitle ) { return true; }
void CBaseToggle::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener ) { }
void CBaseToggle::SentenceStop( void ) { }
bool CBaseToggle::IsLockedByMaster() { return false; }
void CBaseMonster::MonsterInitDead( void ) { }
float CBaseMonster::HeadHitGroupDamageMultiplier() { return 3.0f; }
void CBaseMonster::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) { }
bool CBaseMonster::IsFullyAlive( void ) { return CBaseToggle::IsFullyAlive(); }
BOOL CBaseMonster::ShouldFadeOnDeath( void ) { return FALSE; }
void CBaseMonster::RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType ) { }
void CBaseMonster::RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType ) { }
void CBaseMonster::FadeMonster( void ) { }
void CBaseMonster::GibMonster( void ) { }
BOOL CBaseMonster::HasHumanGibs( void ) { return FALSE; }
BOOL CBaseMonster::HasAlienGibs( void ) { return FALSE; }
Activity CBaseMonster::GetDeathActivity( void ) { return ACT_DIE_HEADSHOT; }
MONSTERSTATE CBaseMonster::GetIdealState( void ) { return MONSTERSTATE_ALERT; }
Schedule_t* CBaseMonster::GetScheduleOfType( int Type ) { return NULL; }
Schedule_t *CBaseMonster::GetSchedule( void ) { return NULL; }
void CBaseMonster::RunTask( Task_t *pTask ) { }
void CBaseMonster::StartTask( Task_t *pTask ) { }
Schedule_t *CBaseMonster::ScheduleFromName( const char *pName ) { return NULL;}
void CBaseMonster::BecomeDead( void ) {}
void CBaseMonster::RunAI( void ) {}
void CBaseMonster::Killed( entvars_t * pevInflictor, entvars_t *pevAttacker, int iGib ) {}
void CBaseMonster::OnDying() {}
void CBaseMonster::UpdateOnRemove() {}
int CBaseMonster::TakeHealth(CBaseEntity* pHealer, float flHealth, int bitsDamageType) { return 0; }
int CBaseMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) { return 0; }
int CBaseMonster::Restore( class CRestore & ) { return 1; }
int CBaseMonster::Save( class CSave & ) { return 1; }
int CBaseMonster::DefaultClassify() { return 0; }
int CBaseMonster::Classify() { return 0; }
const char* CBaseMonster::DefaultGibModel() {return 0;}
int CBaseMonster::DefaultGibCount() {return 0;}
bool CBaseMonster::IsAlienMonster() {return false;}
Vector CBaseMonster::DefaultMinHullSize() {return Vector(0,0,0); }
Vector CBaseMonster::DefaultMaxHullSize() {return Vector(0,0,0); }
int CBaseMonster::SizeForGrapple() { return GRAPPLE_NOT_A_TARGET; }
bool CBaseMonster::HandleDoorBlockage(CBaseEntity* pDoor) { return false; }

void CBasePlayer::DeathSound( void ) { }
int CBasePlayer::TakeHealth( CBaseEntity* pHealer, float flHealth, int bitsDamageType ) { return 0; }
int CBasePlayer::TakeArmor(CBaseEntity *pCharger, float flArmor, int flags) { return 0; }
void CBasePlayer::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) { }
int CBasePlayer::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) { return 0; }
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim ) { }
void CBasePlayer::WaterMove() { }
BOOL CBasePlayer::IsOnLadder( void ) { return FALSE; }
void CBasePlayer::PlayerDeathThink(void) { }
void CBasePlayer::StartDeathCam( void ) { }
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle ) { }
void CBasePlayer::PlayerUse( void ) { }
void CBasePlayer::Jump() { }
void CBasePlayer::Duck() { }
int  CBasePlayer::DefaultClassify( void ) { return 0; }
int  CBasePlayer::Classify( void ) { return 0; }
void CBasePlayer::PreThink(void) { }
void CBasePlayer::CheckTimeBasedDamage()  { }
void CBasePlayer::CheckSuitUpdate() { }
void CBasePlayer::SetSuitUpdate( const char *name, int fgroup, int iNoRepeatTime ) { }
void CBasePlayer::PostThink() { }
void CBasePlayer::Precache( void ) { }
int CBasePlayer::Save( CSave &save ) { return 0; }
int CBasePlayer::Restore( CRestore &restore ) { return 0; }
void CBasePlayer::ImpulseCommands() { }
int CBasePlayer::AddPlayerItem( CBasePlayerWeapon *pItem ) { return FALSE; }
int CBasePlayer::GetAmmoIndex( const char *psz ) { return -1; }
void CBasePlayer::UpdateClientData( void ) { }
BOOL CBasePlayer::FBecomeProne( void ) { return TRUE; }
void CBasePlayer::BarnacleVictimBitten( entvars_t *pevBarnacle ) { }
void CBasePlayer::BarnacleVictimReleased( void ) { }
int CBasePlayer::Illumination( void ) { return 0; }
Vector CBasePlayer::GetAutoaimVector( float flDelta ) { return g_vecZero; }
Vector CBasePlayer::GetAutoaimVectorFromPoint( const Vector& vecSrc, float flDelta ) { return g_vecZero; }
Vector CBasePlayer::AutoaimDeflection( const Vector &vecSrc, float flDist, float flDelta  ) { return g_vecZero; }
void CBasePlayer::ResetAutoaim() { }
Vector CBasePlayer::GetGunPosition( void ) { return g_vecZero; }
const char *CBasePlayer::TeamID( void ) { return ""; }
int CBasePlayer::GiveAmmo( int iCount, const char *szName ) { return 0; }
void CBasePlayer::AddPoints( int score, bool bAllowNegativeScore ) { }
void CBasePlayer::AddFloatPoints(float score, bool bAllowNegativeScore) {}
void CBasePlayer::AddPointsToTeam( int score, bool bAllowNegativeScore ) { }

float CBasePlayerWeapon::GetNextAttackDelay( float flTime ) { return flTime; }
void CBasePlayerWeapon::SetObjectCollisionBox( void ) { }
void CBasePlayerWeapon::KeyValue( KeyValueData *pkvd ) {}
void CBasePlayerWeapon::FallInit( void ) { }
CBaseEntity *CBasePlayerWeapon::Respawn( void ) { return NULL; }
void CBasePlayerWeapon::DefaultTouch( CBaseEntity *pOther ) { }
void CBasePlayerWeapon::DestroyItem( void ) { }
bool CBasePlayerWeapon::IsEnabledInMod() { return true; }
int CBasePlayerWeapon::AddToPlayer( CBasePlayer *pPlayer ) { return TRUE; }
void CBasePlayerWeapon::Drop( void ) { }
void CBasePlayerWeapon::Kill( void ) { }
void CBasePlayerWeapon::AttachToPlayer ( CBasePlayer *pPlayer ) { }
void CBasePlayerWeapon::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {}
int CBasePlayerWeapon::ObjectCaps() {return 0;}
int CBasePlayerWeapon::AddDuplicate( CBasePlayerWeapon *pOriginal ) { return 0; }
int CBasePlayerWeapon::AddToPlayerDefault( CBasePlayer *pPlayer ) { return FALSE; }
int CBasePlayerWeapon::UpdateClientData( CBasePlayer *pPlayer ) { return 0; }
BOOL CBasePlayerWeapon::IsUseable( void ) { return TRUE; }
int CBasePlayerWeapon::PrimaryAmmoIndex( void ) { return -1; }
int CBasePlayerWeapon::SecondaryAmmoIndex( void ) { return -1; }
int CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon *pWeapon ) { return 0; }
int CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon *pWeapon ) { return 0; }	
void CBasePlayerWeapon::RetireWeapon( void ) { }
void CBasePlayerWeapon::InitDefaultAmmo(int defaultGive) { m_iDefaultAmmo = defaultGive; }
