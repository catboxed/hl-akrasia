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
// skill.cpp - code for skill level concerns
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"skill.h"

skilldata_t gSkillData;

//=========================================================
// take the name of a cvar, tack a digit for the skill level
// on, and return the value.of that Cvar 
//=========================================================
static float GetSkillCvar(const char *pName , const char *fallback, bool allowZero, float fallbackValue = 0)
{
	float flValue;
	char szBuffer[64];

	sprintf( szBuffer, "%s%d",pName, gSkillData.iSkillLevel );

	flValue = CVAR_GET_FLOAT( szBuffer );

	if( flValue <= 0 && !allowZero)
	{
		if (fallback)
			flValue = GetSkillCvar(fallback);
		else if (fallbackValue)
			flValue = fallbackValue;
		if (flValue <= 0)
			ALERT( at_console, "\n\n** GetSkillCVar Got a zero for %s **\n\n", szBuffer );
	}

	return flValue;
}

float GetSkillCvar(const char *pName , const char *fallback)
{
	return GetSkillCvar(pName, fallback, false);
}

float GetSkillCvar( const char *pName, float fallback )
{
	return GetSkillCvar(pName, 0, false, fallback);
}

float GetSkillCvarZeroable(const char *pName)
{
	return GetSkillCvar(pName, 0, true);
}
