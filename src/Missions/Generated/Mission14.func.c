//
//  Mission14.func.c
//
//  Finite state machine function pointers for "Mission14" mission
//
//  This code was autogenerated from Mission14.kp by KAS2C Version 2.05sdl
//


#include "Mission14.h"  // prototypes and #includes for exposed game functions

// FSM init/watch function pointers.
const void* Mission14_FunctionPointers[] =
{
	(void*)Init_Mission14_FleetIntel,
	(void*)Watch_Mission14_FleetIntel,
	(void*)Init_Mission14_FleetIntel_FCIntro,
	(void*)Watch_Mission14_FleetIntel_FCIntro,
	(void*)Init_Mission14_FleetIntel_FIDestroyGennyLBXIn,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGennyLBXIn,
	(void*)Init_Mission14_FleetIntel_FIDestroyGennyOpenSensors,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGennyOpenSensors,
	(void*)Init_Mission14_FleetIntel_FIDestroyGenny,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGenny,
	(void*)Init_Mission14_FleetIntel_FIDestroyGennyPING,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGennyPING,
	(void*)Init_Mission14_FleetIntel_FIDestroyGennyWAITEND,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGennyWAITEND,
	(void*)Init_Mission14_FleetIntel_FIDestroyGennyEND,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGennyEND,
	(void*)Init_Mission14_FleetIntel_FISensorTech,
	(void*)Watch_Mission14_FleetIntel_FISensorTech,
	(void*)Init_Mission14_FleetIntel_FIDestroyGatesLBXIn,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGatesLBXIn,
	(void*)Init_Mission14_FleetIntel_FIDestroyGatesOpenSensors,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGatesOpenSensors,
	(void*)Init_Mission14_FleetIntel_FIDestroyGates,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGates,
	(void*)Init_Mission14_FleetIntel_FIDestroyGatesPING,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGatesPING,
	(void*)Init_Mission14_FleetIntel_FIDestroyGatesWAITEND,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGatesWAITEND,
	(void*)Init_Mission14_FleetIntel_FIDestroyGatesEND,
	(void*)Watch_Mission14_FleetIntel_FIDestroyGatesEND,
	(void*)Init_Mission14_FleetIntel_FIHyperdriveOnline,
	(void*)Watch_Mission14_FleetIntel_FIHyperdriveOnline,
	(void*)Init_Mission14_FleetIntel_FCTakeUsHome,
	(void*)Watch_Mission14_FleetIntel_FCTakeUsHome,
	(void*)Init_Mission14_FleetIntel_FIIdle,
	(void*)Watch_Mission14_FleetIntel_FIIdle,
	(void*)Init_Mission14_MiningBase,
	(void*)Watch_Mission14_MiningBase,
	(void*)Init_Mission14_MiningBase_Rotate,
	(void*)Watch_Mission14_MiningBase_Rotate,
	(void*)Init_Mission14_AIShips,
	(void*)Watch_Mission14_AIShips,
	(void*)Init_Mission14_AIShips_GiveToAI,
	(void*)Watch_Mission14_AIShips_GiveToAI,
	(void*)Init_Mission14_IonSphere,
	(void*)Watch_Mission14_IonSphere,
	(void*)Init_Mission14_IonSphere_WatchForEnemies,
	(void*)Watch_Mission14_IonSphere_WatchForEnemies,
	(void*)Init_Mission14_IonSphere_Loop,
	(void*)Watch_Mission14_IonSphere_Loop,
	(void*)Init_Mission14_IonSphere_BackInFormation,
	(void*)Watch_Mission14_IonSphere_BackInFormation,
	(void*)Init_Mission14_IonSphere_NullState,
	(void*)Watch_Mission14_IonSphere_NullState,
	(void*)Init_Mission14_HiddenDummy,
	(void*)Watch_Mission14_HiddenDummy,
	(void*)Init_Mission14_HiddenDummy_NullState,
	(void*)Watch_Mission14_HiddenDummy_NullState,
	(void*)Init_Mission14_RoamingMG,
	(void*)Watch_Mission14_RoamingMG,
	(void*)Init_Mission14_RoamingMG_Patrol,
	(void*)Watch_Mission14_RoamingMG_Patrol,
	(void*)Init_Mission14_RoamingMG_Dock,
	(void*)Watch_Mission14_RoamingMG_Dock,
	(void*)Init_Mission14_RoamingMG_Launch,
	(void*)Watch_Mission14_RoamingMG_Launch,
	(void*)Init_Mission14_RoamingMG_Decide,
	(void*)Watch_Mission14_RoamingMG_Decide,
	(void*)Init_Mission14_RoamingMG_CheckIonAttackers,
	(void*)Watch_Mission14_RoamingMG_CheckIonAttackers,
	(void*)Init_Mission14_RoamingMG_ProtectIons,
	(void*)Watch_Mission14_RoamingMG_ProtectIons,
	(void*)Init_Mission14_RoamingMG_AttackLayer2Vol,
	(void*)Watch_Mission14_RoamingMG_AttackLayer2Vol,
	(void*)Init_Mission14_RoamingMG_NullState,
	(void*)Watch_Mission14_RoamingMG_NullState,
	(void*)Init_Mission14_ProxDeath,
	(void*)Watch_Mission14_ProxDeath,
	(void*)Init_Mission14_ProxDeath_Decide,
	(void*)Watch_Mission14_ProxDeath_Decide,
	(void*)Init_Mission14_ProxDeath_Dock,
	(void*)Watch_Mission14_ProxDeath_Dock,
	(void*)Init_Mission14_ProxDeath_Launch,
	(void*)Watch_Mission14_ProxDeath_Launch,
	(void*)Init_Mission14_ProxDeath_CheckProx,
	(void*)Watch_Mission14_ProxDeath_CheckProx,
	(void*)Init_Mission14_ProxDeath_DIEProxDIE,
	(void*)Watch_Mission14_ProxDeath_DIEProxDIE,
	(void*)Init_Mission14_ProxDeath_ScoutSpeedBurst,
	(void*)Watch_Mission14_ProxDeath_ScoutSpeedBurst,
	(void*)Init_Mission14_ProxDeath_GuardMiningBase,
	(void*)Watch_Mission14_ProxDeath_GuardMiningBase,
	(void*)Init_Mission14_ProxDeath_NullState,
	(void*)Watch_Mission14_ProxDeath_NullState,
	(void*)Init_Mission14_InnerDefenseTeam,
	(void*)Watch_Mission14_InnerDefenseTeam,
	(void*)Init_Mission14_InnerDefenseTeam_Decide,
	(void*)Watch_Mission14_InnerDefenseTeam_Decide,
	(void*)Init_Mission14_InnerDefenseTeam_Patrol,
	(void*)Watch_Mission14_InnerDefenseTeam_Patrol,
	(void*)Init_Mission14_InnerDefenseTeam_Attack,
	(void*)Watch_Mission14_InnerDefenseTeam_Attack,
	(void*)Init_Mission14_InnerDefenseTeam_ProtectMiningBase,
	(void*)Watch_Mission14_InnerDefenseTeam_ProtectMiningBase,
	(void*)Init_Mission14_InnerDefenseTeam_NullState,
	(void*)Watch_Mission14_InnerDefenseTeam_NullState,
	(void*)Init_Mission14_InnerCloakingTeam,
	(void*)Watch_Mission14_InnerCloakingTeam,
	(void*)Init_Mission14_InnerCloakingTeam_Guard,
	(void*)Watch_Mission14_InnerCloakingTeam_Guard,
	(void*)Init_Mission14_InnerCloakingTeam_GuardCloakityOn,
	(void*)Watch_Mission14_InnerCloakingTeam_GuardCloakityOn,
	(void*)Init_Mission14_InnerCloakingTeam_GuardCloakityOff,
	(void*)Watch_Mission14_InnerCloakingTeam_GuardCloakityOff,
	(void*)Init_Mission14_InnerCloakingTeam_NullState,
	(void*)Watch_Mission14_InnerCloakingTeam_NullState,
	(void*)Init_Mission14_HyperspaceGateTeam,
	(void*)Watch_Mission14_HyperspaceGateTeam,
	(void*)Init_Mission14_HyperspaceGateTeam_HyperspaceOut,
	(void*)Watch_Mission14_HyperspaceGateTeam_HyperspaceOut,
	(void*)Init_Mission14_HyperspaceGateTeam_HyperspaceIn,
	(void*)Watch_Mission14_HyperspaceGateTeam_HyperspaceIn,
	(void*)Init_Mission14_HyperspaceGateTeam_Decide,
	(void*)Watch_Mission14_HyperspaceGateTeam_Decide,
	(void*)Init_Mission14_HyperspaceGateTeam_Patrol,
	(void*)Watch_Mission14_HyperspaceGateTeam_Patrol,
	(void*)Init_Mission14_HyperspaceGateTeam_AttackBox,
	(void*)Watch_Mission14_HyperspaceGateTeam_AttackBox,
	(void*)Init_Mission14_HyperspaceGateTeam_AttackSphere,
	(void*)Watch_Mission14_HyperspaceGateTeam_AttackSphere,
	(void*)Init_Mission14_HyperspaceGateTeam_Dock,
	(void*)Watch_Mission14_HyperspaceGateTeam_Dock,
	(void*)Init_Mission14_HyperspaceGateTeam_Launch,
	(void*)Watch_Mission14_HyperspaceGateTeam_Launch,
	(void*)Init_Mission14_HyperspaceGateTeam_NullState,
	(void*)Watch_Mission14_HyperspaceGateTeam_NullState,
	(void*)Init_Mission14_DummyController,
	(void*)Watch_Mission14_DummyController,
	(void*)Init_Mission14_DummyController_Null,
	(void*)Watch_Mission14_DummyController_Null,
	(void*)Init_Mission14,
	(void*)Watch_Mission14,
};

// Number of FSM init/watch function pointers.
const unsigned int Mission14_FunctionPointerCount = 142;
