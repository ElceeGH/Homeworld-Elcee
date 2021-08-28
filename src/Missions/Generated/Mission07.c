//
//  Mission07.c
//
//  Finite state machine routines for "Mission07" mission
//
//  Copyright (C) 1998 Relic Entertainment Inc.
//  All rights reserved
//
//  This code was autogenerated from Mission07.kp by KAS2C Version 2.05sdl
//


#include "Mission07.h"  // prototypes and #includes for exposed game functions

extern AITeam *CurrentTeamP;
#define kasThisTeamPtr CurrentTeamP

//  for run-time scoping of symbols (variables, timers, etc.)
extern sdword CurrentMissionScope;
extern char CurrentMissionScopeName[];

//  for displaying localized strings
extern udword strCurLanguage;

//
//  "initialize" code for Mission07 mission
//
void Init_Mission07(void)
{
	CurrentMissionScope = KAS_SCOPE_MISSION;
	strcpy(CurrentMissionScopeName, "Mission07");
	
}


//
//  "watch" code for Mission07 mission
//
void Watch_Mission07(void)
{
	CurrentMissionScope = KAS_SCOPE_MISSION;
	strcpy(CurrentMissionScopeName, "Mission07");
	
}


