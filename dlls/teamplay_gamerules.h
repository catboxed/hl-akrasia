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
//
// teamplay_gamerules.h
//
#pragma once
#if !defined(TEAMPLAY_GAMERULES_H)
#define TEAMPLAY_GAMERULES_H

#include "gamerules.h"

#define MAX_TEAMNAME_LENGTH		16
#define MAX_TEAMS			32

#define TEAMPLAY_TEAMLISTLENGTH		MAX_TEAMS*MAX_TEAMNAME_LENGTH

class CHalfLifeTeamplay : public CHalfLifeMultiplay
{
public:
	CHalfLifeTeamplay();

	virtual bool ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) override;
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	virtual bool IsTeamplay( void ) override;
	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual const char *GetTeamID( CBaseEntity *pEntity );
	virtual bool ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) override;
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void InitHUD( CBasePlayer *pl );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor );
	virtual const char *GetGameDescription( void ) { return "HL Teamplay"; }  // this is the game name that gets seen in the server browser
	virtual void UpdateGameMode( CBasePlayer *pPlayer );  // the client needs to be informed of the current game mode
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void Think( void );
	virtual int GetTeamIndex( const char *pTeamName );
	virtual const char *GetIndexedTeamName( int teamIndex );
	virtual bool IsValidTeam( const char *pTeamName ) override;
	const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer );
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib ) override;

private:
	void RecountTeams( bool bResendInfo = false );
	const char *TeamWithFewestPlayers( void );

	bool m_DisableDeathMessages;
	bool m_DisableDeathPenalty;
	bool m_teamLimit;				// This means the server set only some teams as valid
	char m_szTeamList[TEAMPLAY_TEAMLISTLENGTH];
};
#endif // TEAMPLAY_GAMERULES_H
