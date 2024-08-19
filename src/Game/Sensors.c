/*=============================================================================
    Name    : Sensors.c
    Purpose : Code for handling and rendering the sensors manager.

    Created 10/8/1997 by lmoloney
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#include "Sensors.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "Alliance.h"
#include "Battle.h"
#include "Blobs.h"
#include "Bounties.h"
#include "BTG.h"
#include "Camera.h"
#include "CameraCommand.h"
#include "CommandDefs.h"
#include "CommandWrap.h"
#include "Debug.h"
#include "devstats.h"
#include "FastMath.h"
#include "font.h"
#include "GravWellGenerator.h"
#include "InfoOverlay.h"
#include "Light.h"
#include "main.h"
#include "mainrgn.h"
#include "Memory.h"
#include "mouse.h"
#include "MultiplayerGame.h"
#include "NIS.h"
#include "PiePlate.h"
#include "Ping.h"
#include "prim2d.h"
#include "prim3d.h"
#include "Probe.h"
#include "ProximitySensor.h"
#include "Randy.h"
#include "Region.h"
#include "render.h"
#include "rResScaling.h"
#include "rglu.h"
#include "SaveGame.h"
#include "Select.h"
#include "Shader.h"
#include "ShipSelect.h"
#include "SinglePlayer.h"
#include "SoundEvent.h"
#include "SoundEventDefs.h"
#include "StatScript.h"
#include "StringSupport.h"
#include "Subtitle.h"
#include "Tactical.h"
#include "TaskBar.h"
#include "Teams.h"
#include "Tutor.h"
#include "Tweak.h"
#include "Universe.h"
#include "UnivUpdate.h"
#include "utility.h"
#include "Vector.h"
#include "rStateCache.h"

void (*smHoldLeft)(void);
void (*smHoldRight)(void);

#if SM_TOGGLE_SENSOR_LEVEL
void smToggleSensorsLevel(void);
#endif

/*=============================================================================
    Data:
=============================================================================*/
//set this to TRUE if you want it to be the Fleet Intel version of the SM
bool smFleetIntel = FALSE;

//toggles for what to display
Camera smCamera;
Camera smTempCamera;

sdword smTacticalOverlay = FALSE;
sdword smResources = TRUE;
sdword smNonResources = TRUE;
bool smFocusOnMothershipOnClose = FALSE;

//arrays of stars for quick star rendering
extern ubyte* stararray;
extern color HorseRaceDropoutColor;

//region handle for the sensors manager viewport
regionhandle smViewportRegion;
regionhandle smBaseRegion;
rectangle smViewRectangle;

sdword smClicking = FALSE;

//table of functions to draw a mission sphere at varying levels of visibility
void smSelectHold(void);

//colors for differing sphere statuses
color spcNormal   = SPC_Normal;
color spcSelected = SPC_Selected;
color spcTeamColor= SPC_TeamColor;

// color for printing the A for alliances
color alliescolor = colRGB(255,255,0);

//list of blobs encompassing all objects in universe
//LinkedList smBlobList;
//BlobProperties smBlobProperties;

//fog-of-war sub-blob stuff
BlobProperties smFOWBlobProperties;
real32 smFOW_AsteroidK1;
real32 smFOW_AsteroidK2;
real32 smFOW_AsteroidK3;
real32 smFOW_AsteroidK4;
real32 smFOW_DustGasK2;
real32 smFOW_DustGasK3;
real32 smFOW_DustGasK4;
real32 smFOWBlobUpdateTime = SM_FOWBlobUpdateTime;

//global flag which says the sensors manager is active
bool smSensorsActive = FALSE;

bool smSensorsDisable = FALSE;

//for updating blobs
sdword smRenderCount;
color smBackgroundColor = colFuscia, smIntBackgroundColor;
color smBlobColor = SM_BlobColor;
color smBlobColorHigh = SM_BlobColorHigh;
color smBlobColorLow = SM_BlobColorLow;
color smIntBlobColor;
color smIntBlobColorHigh;
color smIntBlobColorLow;

//for zooming
real32 smZoomTime;
region **smScrollListLeft, **smScrollListTop, **smScrollListRight, **smScrollListBottom;
sdword smScrollCountLeft, smScrollCountTop, smScrollCountRight, smScrollCountBottom;
sdword smScrollDistLeft, smScrollDistTop, smScrollDistRight, smScrollDistBottom;
sdword smScrollLeft, smScrollTop, smScrollRight, smScrollBottom;
vector smLookStart, smLookEnd;
vector smEyeStart, smEyeEnd;
bool8 smZoomingIn = FALSE, smZoomingOut = FALSE, smFocus = FALSE, smFocusTransition = FALSE;
FocusCommand *smFocusCommand = NULL;

//for selections
rectangle smSelectionRect;
//blob *smLastClosestBlob = NULL;

blob *closestBlob = NULL;
blob *probeBlob = NULL;
//for enabling 'ghost mode' after a player dies
bool smGhostMode = FALSE;
bool ioSaveState;

//option for fuzzy sensors blobs
sdword smFuzzyBlobs = TRUE;

//cursor text font, from mouse.c
extern fonthandle mouseCursorFont;
color smCurrentWorldPlaneColor;
real32 smCurrentCameraDistance;

//switch for 'instant' transitions to sensors manager
bool smInstantTransition = FALSE;
real32 smCurrentZoomLength;
real32 smCurrentMainViewZoomLength;

//for horizon tick marks
smticktext smTickText[SM_MaxTicksOnScreen];
sdword smTickTextIndex;

//for panning the world plane about whilley-nilley
vector smCameraLookatPoint = {0.0f, 0.0f, 0.0f};
vector smCameraLookVelocity = {0.0f, 0.0f, 0.0f};
bool smCentreWorldPlane = FALSE;

//info for sorting the blobs
blob **smBlobSortList = NULL;
sdword smBlobSortListLength = 0;
sdword smNumberBlobsSorted = 0;

//for multiplayer hyperspace
sdword MP_HyperSpaceFlag=FALSE;

//for sensors weirdness
#define NUM_WEIRD_POINTS         600
#define MAX_REPLACEMENTS         5
#define WEIRDUNIVERSESTRETCH     5/4
typedef struct
{
    vector location;
    color  color;
    real32 size;
    bool   ping;
} weirdStruct;

static weirdStruct smWeird[NUM_WEIRD_POINTS];
udword   smSensorWeirdness=0;

//region for hyperspace button
regionhandle smHyperspaceRegion = NULL;

// Tactical overlay list items
typedef struct ShipTOItem {
    ShipClass shipClass;
    real32    x, y;
    real32    radius;
    color     col;
    bool      sphereDraw;
    Ship*     sphereShip;
    real32    sphereRadius;
    color     sphereCol;
} ShipTOItem;

ShipTOItem shipTOList[SM_NumberTOs];
static udword shipTOCount;

//what ships have a TO in the SM
ubyte smShipTypeRenderFlags[TOTAL_NUM_SHIPS] =
{
    SM_TO,                                      //AdvanceSupportFrigate
    0,                                          //AttackBomber
    SM_TO | SM_Mesh,                            //Carrier
    0,                                          //CloakedFighter
    SM_TO,                                      //CloakGenerator
    SM_TO,                                      //DDDFrigate
    0,                                          //DefenseFighter
    SM_TO,                                      //DFGFrigate
    SM_TO,                                      //GravWellGenerator
    0,                                          //HeavyCorvette
    SM_TO | SM_Mesh,                            //HeavyCruiser
    0,                                          //HeavyDefender
    0,                                          //HeavyInterceptor
    SM_TO,                                      //IonCannonFrigate
    0,                                          //LightCorvette
    0,                                          //LightDefender
    0,                                          //LightInterceptor
    0,                                          //MinelayerCorvette
    SM_TO,                                      //MissileDestroyer
    SM_Mesh | SM_Exclude,                       //Mothership
    0,                                          //MultiGunCorvette
    SM_Exclude,                                 //Probe
    SM_Exclude,                                 //ProximitySensor
    0,                                          //RepairCorvette
    SM_TO,                                      //ResearchShip
    SM_TO,                                      //ResourceCollector
    SM_TO,                                      //ResourceController
    0,                                          //SalCapCorvette
    SM_TO,                                      //SensorArray
    SM_TO,                                      //StandardDestroyer
    SM_TO,                                      //StandardFrigate
    SM_Exclude,                                 //Drone
    SM_Exclude,                                 //TargetDrone
    SM_Mesh,                                    //HeadShotAsteroid
    SM_Mesh | SM_Exclude,                       //CryoTray
    0,                                          //P1Fighter
    SM_TO,                                      //P1IonArrayFrigate
    0,                                          //P1MissileCorvette
    SM_TO | SM_Mesh,                            //P1Mothership
    0,                                          //P1StandardCorvette
    0,                                          //P2AdvanceSwarmer
    SM_TO,                                      //P2FuelPod
    SM_TO | SM_Mesh,                            //P2Mothership
    SM_TO,                                      //P2MultiBeamFrigate
    0,                                          //P2Swarmer
    SM_TO,                                      //P3Destroyer
    SM_TO,                                      //P3Frigate
    SM_TO | SM_Mesh,                            //P3Megaship
    SM_TO | SM_Mesh,                            //FloatingCity
    SM_TO,                                      //CargoBarge
    SM_TO | SM_Mesh,                            //MiningBase
    SM_TO | SM_Mesh,                            //ResearchStation
    0,                                          //JunkYardDawg
    SM_TO,                                      //JunkYardHQ
    0,                                          //Ghostship
    0,                                          //Junk_LGun
    0,                                          //Junk_SGun
    0,                                          //ResearchStationBridge
    0                                           //ResearchStationTower
};
ubyte smDerelictTypeMesh[NUM_DERELICTTYPES] =
{
    0,                                          //AngelMoon,
    0,                                          //AngelMoon_clean,
    0,                                          //Crate,
    SM_Mesh,                                    //FragmentPanel0a,
    SM_Mesh,                                    //FragmentPanel0b,
    SM_Mesh,                                    //FragmentPanel0c,
    SM_Mesh,                                    //FragmentPanel1,
    SM_Mesh,                                    //FragmentPanel2,
    SM_Mesh,                                    //FragmentPanel3,
    SM_Mesh,                                    //FragmentStrut,
    0,                                          //Homeworld,
    0,                                          //LifeBoat,
    0,                                          //PlanetOfOrigin,
    0,                                          //PlanetOfOrigin_scarred,
    0,                                          //PrisonShipOld,
    0,                                          //PrisonShipNew,
    SM_Mesh,                                    //Scaffold,
    SM_Mesh,                                    //Scaffold_scarred,
    SM_Mesh,                                    //ScaffoldFingerA_scarred,
    SM_Mesh,                                    //ScaffoldFingerB_scarred,
    0,                                          //Shipwreck,
    //pre-revision ships as junkyard derelicts:
    0,                                          //JunkAdvanceSupportFrigate,
    SM_Mesh,                                    //JunkCarrier,
    0,                                          //JunkDDDFrigate,
    0,                                          //JunkHeavyCorvette,
    SM_Mesh,                                    //JunkHeavyCruiser,
    0,                                          //JunkIonCannonFrigate,
    0,                                          //JunkLightCorvette,
    0,                                          //JunkMinelayerCorvette,
    0,                                          //JunkMultiGunCorvette,
    0,                                          //JunkRepairCorvette,
    0,                                          //JunkResourceController,
    0,                                          //JunkSalCapCorvette,
    0,                                          //JunkStandardFrigate,
    0,                                          //Junk0_antenna
    0,                                          //Junk0_fin1
    0,                                          //Junk0_fin2
    0,                                          //Junk0_GunAmmo
    0,                                          //Junk0_panel
    0,                                          //Junk0_sensors
    0,                                          //Junk1_partA
    0,                                          //Junk1_partB
    0,                                          //Junk1_shell
    0,                                          //Junk1_strut
    0,                                          //Junk2_panelA
    0,                                          //Junk2_panelB
    0,                                          //Junk2_panelC
    0,                                          //Junk2_panelD
    0,                                          //Junk2_shipwreck
    0,                                          //Junk3_Boiler
    0,                                          //Junk3_BoilerCasing
    0,                                          //M13PanelA
    0,                                          //M13PanelB
    0,                                          //M13PanelC
                                                    //hyperspace gate dummy derelict
    0,                                          //HyperspaceGate

};

/*-----------------------------------------------------------------------------
    tweakables for the sensors manager from sensors.script
-----------------------------------------------------------------------------*/
//for blobs
sdword smBlobUpdateRate         = SM_BlobUpdateRate;
real32 smClosestMargin          = SM_ClosestMargin;
real32 smFarthestMargin         = SM_FarthestMargin;
//for selections
real32 smSelectedFlashSpeed     = SM_SelectedFlashSpeed;
real32 smFocusScalar            = SM_FocusScalar;
real32 smFocusRadius            = SM_FocusRadius;
sdword smSelectionWidth         = SM_SelectionWidth;
sdword smSelectionHeight        = SM_SelectionHeight;
//zooming in/out
real32 smZoomLength             = SM_ZoomLength;
real32 smMainViewZoomLength     = SM_MainViewZoomLength;
//for tactical overlay
color smPlayerListTextColor     = SM_PlayerListTextColor;
color smCursorTextColor         = SM_CursorTextColor;
color smTOColor                 = SM_TOColor;
sdword smPlayerListTextMargin   = SM_PlayerListTextMargin;
sdword smPlayerListMarginX      = SM_PlayerListMarginX;
sdword smPlayerListMarginY      = SM_PlayerListMarginY;
sdword smTOBottomCornerX        = SM_TOBottomCornerX;
sdword smTOBottomCornerY        = SM_TOBottomCornerY;
sdword smTOLineSpacing          = SM_TOLineSpacing;
real32 smTORadius               = 1.0f;
//for world plane
color smWorldPlaneColor         = SM_WorldPlaneColor;
color smMovePlaneColor          = SM_MovePlaneColor;
color smHeightCircleColor       = SM_HeightCircleColor;
real32 smWorldPlaneDistanceFactor = SM_WorldPlaneDistanceFactor;
sdword smWorldPlaneSegments     =  SM_WorldPlaneSegments;
real32 smMovementWorldPlaneDim  = SM_MovementWorldPlaneDim;
real32 smRadialTickStart        = SM_RadialTickStart;
real32 smRadialTickInc          = SM_RadialTickInc;
real32 smTickInnerMult          = SM_TickInnerMult;
real32 smTickOuterMult          = SM_TickOuterMult;
real32 smTickExtentFactor       = SM_TickExtentFactor;
real32 smBlobCircleSize         = SM_BlobCircleSize;
real32 smShortFootLength        = SM_ShortFootLength;
real32 smBackgroundDim          = SM_BackgroundDim;
//for horizon ticks
real32 smHorizTickAngle =       SM_HorizTickAngle;
real32 smHorizTickVerticalFactor = SM_HorizTickVerticalFactor;
real32 smHorizonLineDistanceFactor =  SM_HorizonLineDistanceFactor;
real32 smHorizonTickHorizFactor = SM_HorizonTickHorizFactor;
sdword smTickTextSpacing        = SM_TickTextSpacing;
//for movement/selections
real32 smTickLength = SM_TickLength;
real32 smClosestDistance        = SM_ClosestDistance;
//for rendering
real32 smProjectionScale        = SM_ProjectionScale;
//for hot key groups
sdword smHotKeyOffsetX          = 0;
sdword smHotKeyOffsetY          = 0;
//for panning the world plane about whilley-nilley
real32 smPanSpeedMultiplier     = SM_PanSpeedMultiplier;
real32 smPanTrack               = SM_PanTrack;
real32 smPanEvalThreshold       = SM_PanEvalThreshold;
real32 smPanUnivExtentMult      = SM_PanUnivExtentMult;
sdword smCursorPanX             = SM_CursorPanX;
sdword smCursorPanY             = SM_CursorPanY;
sdword smMaxFrameTicks          = SM_MaxFrameTicks;

/*-----------------------------------------------------------------------------
    tweakables for sensors manager from level files
-----------------------------------------------------------------------------*/
//depth cuing
real32 smDepthCueRadius = SM_DepthCueRadius;
real32 smDepthCueStartRadius = SM_DepthCueStartRadius;
real32 smCircleBorder = SM_CircleBorder;
real32 smZoomMax = SM_ZoomMax;
real32 smZoomMin = SM_ZoomMin;
real32 smZoomMinFactor = SM_ZoomMinFactor;
real32 smZoomMaxFactor = SM_ZoomMaxFactor;
real32 smInitialDistance = SM_InitialDistance;
real32 smUniverseSizeX = SM_UniverseSizeX;
real32 smUniverseSizeY = SM_UniverseSizeY;
real32 smUniverseSizeZ = SM_UniverseSizeZ;

/*-----------------------------------------------------------------------------
    Tweakables for the fleet intel version of the SM
-----------------------------------------------------------------------------*/
real32 smSkipFadeoutTime = SM_SkipFadeoutTime;

/*-----------------------------------------------------------------------------
    Tweak table for sensors.script
-----------------------------------------------------------------------------*/
scriptEntry smTweaks[] =
{
    //sensors manager tweaks
    makeEntry(smBlobUpdateRate      , scriptSetSdwordCB),
    makeEntry(smSelectedFlashSpeed  , scriptSetReal32CB),
    makeEntry(smFocusScalar         , scriptSetReal32CB),
    makeEntry(smFocusRadius         , scriptSetReal32CB),
    makeEntry(smZoomLength          , scriptSetReal32CB),
    makeEntry(smMainViewZoomLength  , scriptSetReal32CB),
    makeEntry(smPlayerListTextColor , scriptSetRGBCB),
    makeEntry(smCursorTextColor     , scriptSetRGBCB),
    makeEntry(smTOColor             , scriptSetRGBCB),
    makeEntry(smWorldPlaneColor     , scriptSetRGBCB),
    makeEntry(smPlayerListTextMargin, scriptSetSdwordCB),
    makeEntry(smPlayerListMarginX   , scriptSetSdwordCB),
    makeEntry(smPlayerListMarginY   , scriptSetSdwordCB),
    makeEntry(smTOBottomCornerX     , scriptSetSdwordCB),
    makeEntry(smTOBottomCornerY     , scriptSetSdwordCB),
    makeEntry(smTOLineSpacing       , scriptSetSdwordCB),
    makeEntry(smTickLength          , scriptSetReal32CB),
    makeEntry(smClosestDistance     , scriptSetReal32CB),
    makeEntry(smProjectionScale     , scriptSetReal32CB),
    makeEntry(smSelectionWidth      , scriptSetSdwordCB),
    makeEntry(smSelectionHeight     , scriptSetSdwordCB),
    makeEntry(smZoomMaxFactor       , scriptSetReal32CB),
    makeEntry(smZoomMinFactor       , scriptSetReal32CB),
    makeEntry(smHotKeyOffsetX       , scriptSetSdwordCB),
    makeEntry(smHotKeyOffsetY       , scriptSetSdwordCB),
    makeEntry(smClosestMargin       , scriptSetReal32CB),
    makeEntry(smFarthestMargin      , scriptSetReal32CB),
    makeEntry(smBlobCircleSize      , scriptSetReal32CB),
    makeEntry(smShortFootLength     , scriptSetReal32CB),
    makeEntry(smBlobColor           , scriptSetRGBCB),
    makeEntry(smBlobColorHigh       , scriptSetRGBCB),
    makeEntry(smBlobColorLow        , scriptSetRGBCB),
    makeEntry(smTORadius            , scriptSetReal32CB),
    makeEntry(smPanSpeedMultiplier  , scriptSetReal32CB),
    makeEntry(smPanTrack            , scriptSetReal32CB),
    makeEntry(smPanEvalThreshold    , scriptSetReal32CB),
    makeEntry(smCursorPanX          , scriptSetSdwordCB),
    makeEntry(smCursorPanY          , scriptSetSdwordCB),
    makeEntry(smMaxFrameTicks       , scriptSetSdwordCB),
    makeEntry(smSkipFadeoutTime     , scriptSetReal32CB),
    makeEntry(smWorldPlaneDistanceFactor, scriptSetReal32CB),
    makeEntry(smPanUnivExtentMult   , scriptSetReal32CB),
//    { "smBobDensityLow",            scriptSetReal32CB, &smBlobProperties.bobDensityLow },
//    { "smBobDensityHigh",           scriptSetReal32CB, &smBlobProperties.bobDensityHigh },
//    { "smBobStartSphereSize",       scriptSetReal32CB, &smBlobProperties.bobStartSphereSize },
//    { "smBobRadiusCombineMargin",   scriptSetReal32CB, &smBlobProperties.bobRadiusCombineMargin },
//    { "smBobOverlapFactor",         scriptSetReal32CB, &smBlobProperties.bobOverlapFactor },
//    { "smBobSmallestRadius",         scriptSetReal32CB, &smBlobProperties.bobSmallestRadius },
//    { "smBobBiggestRadius",         scriptSetReal32CB, &smBlobProperties.bobBiggestRadius },
//    { "smBobDoingCollisionBobs",    scriptSetBool, &smBlobProperties.bobDoingCollisionBobs },
    { "FOWBobDensityLow",           scriptSetReal32CB, &smFOWBlobProperties.bobDensityLow },
    { "FOWBobDensityHigh",          scriptSetReal32CB, &smFOWBlobProperties.bobDensityHigh },
    { "FOWBobStartSphereSize",      scriptSetReal32CB, &smFOWBlobProperties.bobStartSphereSize },
    { "FOWBobRadiusCombineMargin",  scriptSetReal32CB, &smFOWBlobProperties.bobRadiusCombineMargin },
    { "FOWBobOverlapFactor",        scriptSetBlobPropertyOverlap, &smFOWBlobProperties },
    { "FOWBobSmallestRadius",        scriptSetReal32CB, &smFOWBlobProperties.bobSmallestRadius },
    { "FOWBobBiggestRadius",        scriptSetBlobBiggestRadius, &smFOWBlobProperties },
//    { "FOWBobDoingCollisionBobs",   scriptSetBool, &smFOWBlobProperties.bobDoingCollisionBobs },
    { "FOW_AsteroidK1",             scriptSetReal32CB, &smFOW_AsteroidK1 },
    { "FOW_AsteroidK2",             scriptSetReal32CB, &smFOW_AsteroidK2 },
    { "FOW_AsteroidK3",             scriptSetReal32CB, &smFOW_AsteroidK3 },
    { "FOW_AsteroidK4",             scriptSetReal32CB, &smFOW_AsteroidK4 },
    { "FOW_DustGasK2",              scriptSetReal32CB, &smFOW_DustGasK2 },
    { "FOW_DustGasK3",              scriptSetReal32CB, &smFOW_DustGasK3 },
    { "FOW_DustGasK4",              scriptSetReal32CB, &smFOW_DustGasK4 },
    { "smFOWBlobUpdateTime",        scriptSetReal32CB, &smFOWBlobUpdateTime },
    
    END_SCRIPT_ENTRY
};

/*=============================================================================
    Private Functions:
=============================================================================*/

/*-----------------------------------------------------------------------------
    Name        : smStrchr
    Description : Like strchr but returns NULL when searching for a NULL terminator.
    Inputs      :
    Outputs     :
    Return      : NULL
----------------------------------------------------------------------------*/
void *smStrchr(char *string, char character)
{
    while (*string)
    {
        if (character == *string)
        {
            return(string);
        }
        string++;
    }
    return(NULL);
}

#define SM_NumberTickMarkSpacings       4
#define SM_TickSwitchHysteresis         0.9f
#define SM_SpokeLengthFactor            1.05f
typedef struct
{
    real32 radius;                              //'on' radius of the ticks
    real32 spacing;                             //spacing between ticks, meters
    real32 length;                              //factor of the circle radius
}
ticklod;
real32 smTickSwitchHysteresis = SM_TickSwitchHysteresis;
real32 smSpokeLengthFactor = SM_SpokeLengthFactor;
ticklod smTickMarkInfo[SM_NumberTickMarkSpacings] =
{
    {100000.0f, 20000.0f, 0.025f},
    {50000.0f, 10000.0f, 0.015f},
    {10000.0f, 2000.0f, 0.007f},
};
/*-----------------------------------------------------------------------------
    Name        : smWorldPlaneDraw
    Description : Draw the sensors manager world plane.
    Inputs      : z - height to draw it at so we can use same drawing code for
                    the movement pie plate.
                  c - color to use for drawing the beast
                  bDrawSpokes - true if we are to draw spokes
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static real32 smTickFactorX[4] = {0.0f, 1.0f, 0.0f, -1.0f};
static real32 smTickFactorY[4] = {1.0f, 0.0f, -1.0f, 0.0f};
void smWorldPlaneDraw(vector *centre, bool bDrawSpokes, color c)
{
    real32 radius = smCamera.distance * smWorldPlaneDistanceFactor;
    real32 tickExtent = radius * smTickExtentFactor;
    real32 ringRadius;
    real32 angle, sinTheta, cosTheta;
    sdword spoke, LOD, thisLOD;
    vector spokeRim, tickEnd0, tickEnd1, p0, p1;
    real32 tickFactorX = 0.0f, tickFactorY = 1.0f, tickRadius;

    //draw the circle
    rndTextureEnable(FALSE);
    ringRadius = (smZoomMin + smZoomMax) / 2.0f * smWorldPlaneDistanceFactor;
    primCircleOutlineZ(centre, ringRadius, smWorldPlaneSegments, c);

    //draw the radial ticks
    tickEnd0.z = tickEnd1.z = centre->z;
    for (angle = smRadialTickStart; angle < 2.0f * PI; angle += smRadialTickInc)
    {
        sinTheta = (real32)sin((double)angle);
        cosTheta = (real32)cos((double)angle);
        tickEnd0.x = tickEnd1.x = ringRadius * sinTheta;
        tickEnd0.y = tickEnd1.y = ringRadius * cosTheta;
        tickEnd0.x *= smTickInnerMult;
        tickEnd0.y *= smTickInnerMult;
        tickEnd1.x *= smTickOuterMult;
        tickEnd1.y *= smTickOuterMult;
        vecAdd(p0, *centre, tickEnd0);
        vecAdd(p1, *centre, tickEnd1);
        primLine3(&p0, &p1, c);
    }
    //draw the spokes with gradations, if applicable
    if (bDrawSpokes)
    {
        spokeRim.z = centre->z;

        for (LOD = 0; LOD < SM_NumberTickMarkSpacings; LOD++)
        {
            if (smTickMarkInfo[LOD].radius <= radius)
            {
                break;
            }
        }
        for (spoke = 0; spoke < 4; spoke++)
        {
            tickFactorX = smTickFactorX[spoke];
            tickFactorY = smTickFactorY[spoke];
            spokeRim.y = spokeRim.x = ringRadius * smSpokeLengthFactor;
            spokeRim.x *= tickFactorX;
            spokeRim.y *= tickFactorY;
            vecAdd(p0, *centre, spokeRim);
            primLine3(centre, &p0, c);

            for (thisLOD = 0; thisLOD < LOD; thisLOD++)
            {
                tickRadius = smTickMarkInfo[thisLOD].spacing;
                while (tickRadius <= tickExtent)
                {
                    tickEnd0.x = radius * smTickMarkInfo[thisLOD].length * tickFactorX;
                    tickEnd0.y = radius * smTickMarkInfo[thisLOD].length * tickFactorY;
                    tickEnd1.x = -tickEnd0.x;
                    tickEnd1.y = -tickEnd0.y;
                    tickEnd0.x += tickRadius * tickFactorY;
                    tickEnd0.y += tickRadius * tickFactorX;
                    tickEnd1.x += tickRadius * tickFactorY;
                    tickEnd1.y += tickRadius * tickFactorX;
                    vecAdd(p0, *centre, tickEnd0);
                    vecAdd(p1, *centre, tickEnd1);
                    primLine3(&p0, &p1, c);
                    tickRadius += smTickMarkInfo[thisLOD].spacing;
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : makeShipsNotBeDisabled
    Description : removes disabled ships from selection
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/

void makeShipsNotBeDisabled(SelectCommand *selection)
{
    sdword i;
    for(i=0;i<selection->numShips;i++)
    {
        if(selection->ShipPtr[0]->flags & SOF_Disabled)
        {
            selection->numShips--;
            selection->ShipPtr[i] = selection->ShipPtr[selection->numShips];
            i--;
        }
    }
}

// Helper for scaling down the length/alpha at the edges to avoid visible popping.
// Returns how edgy you are in 0:1 range
static real32 getHorizonEdgeFactor( real32 angle, real32 angleLow, real32 angleHigh ) {
    const real32 threshRange = 0.25f;
    const real32 threshRcp   = 1.0f  / threshRange;
    const real32 threshHigh  = 1.0f - threshRange;
    const real32 threshLow   = threshRange;
    const real32 relAngle    = (angle - angleLow) / (angleHigh - angleLow); // Re-range to 0:1

         if (relAngle <= threshLow)  return 1.0f - (relAngle * threshRcp);
    else if (relAngle >= threshHigh) return (relAngle - threshHigh) * threshRcp;
    else                             return 0.0f;
}

// Helper for queuing up tick text to draw
static void addTickToList( real32 x, real32 y, real32 rads, bool camEyeAboveLookZ ) {
    if (smTickTextIndex >= SM_MaxTicksOnScreen)
        return;

    smticktext* smt = &smTickText[smTickTextIndex];
    smt->x = primGLToScreenX(x);
    smt->y = primGLToScreenY(y);

    real32 degrees = -RAD_TO_DEG(rads - smHorizTickAngle);
    if (degrees < 0.0f)   degrees += 360.0f;
    if (degrees > 360.0f) degrees -= 360.0f;
    
    const char* format;
         if (degrees < 10)  format = "00%.0f";
    else if (degrees < 100) format = "0%.0f";
    else                    format = "%.0f";
    
    memset(smt->text, '\0', sizeof(smt->text));
    snprintf(smt->text, sizeof(smt->text), format, degrees);

    if (camEyeAboveLookZ)
         smt->y -= fontHeight(" ") + smTickTextSpacing;
    else smt->y += smTickTextSpacing;

    smt->x -= fontWidth(smt->text) / 2;

    smTickTextIndex++;
}

/*-----------------------------------------------------------------------------
    Name        : smHorizonLineDraw
    Description : Draw the horizon line with compass tick marks
    Inputs      : cam - context we're rendering from
                  thisBlob - blob we're rendering
                  modelView, projection - current matrices
                  distance - the distance to draw the ring at
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
#define cam     ((Camera *)voidCam)
void smHorizonLineDraw(void *voidCam, hmatrix *modelView, hmatrix *projection, real32 distance)
{
    fonthandle oldFont = fontCurrentGet();
    
    const real32 mw         = (real32) MAIN_WindowWidth;
    const real32 mh         = (real32) MAIN_WindowHeight;
    const real32 maxAspect  = max(mw,mh) / min(mw,mh);
    const real32 startAngle = cam->angle - PI - DEG_TO_RAD(cam->fieldofview) * maxAspect * 0.75f - smHorizTickAngle * 2.0f;
    const real32 endAngle   = cam->angle - PI + DEG_TO_RAD(cam->fieldofview) * maxAspect * 0.75f + smHorizTickAngle * 2.0f;

    const real32 startRefAngle = floorf((startAngle / smHorizTickAngle)) * smHorizTickAngle;
    const real32 endRefAngle   = floorf((endAngle   / smHorizTickAngle)) * smHorizTickAngle;

    vector startPoint, endPoint;
    startPoint.x = cam->lookatpoint.x + cosf(startRefAngle) * distance * smWorldPlaneDistanceFactor;   //position of starting point
    startPoint.y = cam->lookatpoint.y + sinf(startRefAngle) * distance * smWorldPlaneDistanceFactor;
    endPoint.z = startPoint.z = cam->lookatpoint.z;

    fontMakeCurrent(mouseCursorFont);
    glLineWidth( sqrtf(getResDensityRelative()) );

    const bool depthTestEnabled =   glIsEnabled(GL_DEPTH_TEST);
    const bool blendDisabled    = ! glIsEnabled(GL_BLEND);
    if (depthTestEnabled) glDisable(GL_DEPTH_TEST);
    if (blendDisabled)    glEnable(GL_BLEND);

    // Draw the ticks on the horizon separately first
    smTickTextIndex = 0;
    for (real32 angle=startRefAngle; angle<endAngle; angle+=smHorizTickAngle) {
        endPoint.x = cam->lookatpoint.x + cosf(angle + smHorizTickAngle) * distance * smWorldPlaneDistanceFactor; //position of current point
        endPoint.y = cam->lookatpoint.y + sinf(angle + smHorizTickAngle) * distance * smWorldPlaneDistanceFactor;

        vector horizPoint;
        horizPoint.x = endPoint.x;
        horizPoint.y = endPoint.y;
        if (cam->eyeposition.z > cam->lookatpoint.z)
             horizPoint.z = cam->lookatpoint.z + cam->clipPlaneFar * smHorizTickVerticalFactor;
        else horizPoint.z = cam->lookatpoint.z - cam->clipPlaneFar * smHorizTickVerticalFactor;

        //figure out where to draw the tick text
        real32 x, y, radius;
        selCircleComputeGeneral(modelView, projection, &horizPoint, 1.0f, &x, &y, &radius);
        if (radius > 0.0f) { // If in front of the camera
            const bool eyeAboveLookZ = cam->eyeposition.z > cam->lookatpoint.z;
            addTickToList( x, y, angle, eyeAboveLookZ );
        }

        // Vertical tick lower
        const real32 edgeiness       = getHorizonEdgeFactor( angle, startRefAngle, endRefAngle );
        const real32 edgeFactor      = 0.25f;
        const real32 edgeinessInv    = 1.0f - edgeiness; // Good, good
        const real32 edgeHorizFactor = 1.0f + edgeiness * edgeFactor;
        primLine3Fade(&endPoint, &horizPoint, smCurrentWorldPlaneColor, edgeinessInv);

        // Vertical tick upper
        horizPoint.x = (endPoint.x - cam->lookatpoint.x) * smHorizonTickHorizFactor * edgeHorizFactor + cam->lookatpoint.x;
        horizPoint.y = (endPoint.y - cam->lookatpoint.y) * smHorizonTickHorizFactor * edgeHorizFactor + cam->lookatpoint.y;
        horizPoint.z = cam->lookatpoint.z;
        primLine3Fade(&endPoint, &horizPoint, smCurrentWorldPlaneColor, edgeinessInv);

        //draw from this point to next point next time through
        startPoint = endPoint; 
    }


    // Draw the main arc in much finer detail, but using a more efficient method.
    const ubyte  red      = colRed  ( smCurrentWorldPlaneColor );
    const ubyte  green    = colGreen( smCurrentWorldPlaneColor );
    const ubyte  blue     = colBlue ( smCurrentWorldPlaneColor );
    const real32 fineStep = smHorizTickAngle / 32.0f;

    startPoint.x = cam->lookatpoint.x + cosf(startRefAngle) * distance * smWorldPlaneDistanceFactor;   //position of starting point
    startPoint.y = cam->lookatpoint.y + sinf(startRefAngle) * distance * smWorldPlaneDistanceFactor;
    endPoint.z   = startPoint.z = cam->lookatpoint.z;

    glBegin(GL_LINE_STRIP);    
    for (real32 angle=startRefAngle; angle<endAngle; angle+=fineStep) {
        const real32 edgeinessInv = 1.0f - getHorizonEdgeFactor( angle, startRefAngle, endRefAngle );
        const ubyte  alpha        = (ubyte) (255.0f * edgeinessInv);
        endPoint.x = cam->lookatpoint.x + cosf(angle + smHorizTickAngle) * distance * smWorldPlaneDistanceFactor; //position of current point
        endPoint.y = cam->lookatpoint.y + sinf(angle + smHorizTickAngle) * distance * smWorldPlaneDistanceFactor;
        glColor4ub( red, green, blue, alpha );
        glVertex3fv( &startPoint.x );
        glVertex3fv( &endPoint  .x );
        startPoint = endPoint; //draw from this point to next point next time through
    }
    glEnd();

    // Restore state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    if (blendDisabled)    glDisable(GL_BLEND);

    fontMakeCurrent( oldFont );
    glLineWidth( 1.0f );
}
#undef cam

/*-----------------------------------------------------------------------------
    Name        : smTickTextDraw
    Description : Draw the tick text created in smHorizonLineDraw
    Inputs      :
    Outputs     :
    Return      : void
    Note        : must be in 2d mode for this function
----------------------------------------------------------------------------*/
void smTickTextDraw(void)
{
    fonthandle oldFont = fontCurrentGet();

    fontMakeCurrent(mouseCursorFont);

    for (smTickTextIndex--; smTickTextIndex >= 0; smTickTextIndex--)
    {
        smticktext* smt = &smTickText[smTickTextIndex];
        fontPrint(smt->x, smt->y, smCurrentWorldPlaneColor, smt->text);
    }
    fontMakeCurrent(oldFont);
}




// Callback type for mesh drawing
typedef void TypeSpecificDrawFunction( SpaceObjRotImp* obj, lod* level );

// General purpose mesh draw
static void drawMesh( SpaceObjRotImp* obj, bool selectFlash, lod* level, Camera* camera, TypeSpecificDrawFunction draw ) {
    // Flash by not drawing it
    if (selectFlash)
        return;

    // Make total transform matrix including rotation and position
    hmatrix matPosRot;
    hmatMakeHMatFromMat(&matPosRot, &obj->rotinfo.coordsys);
    hmatPutVectIntoHMatrixCol4( obj->posinfo.position, matPosRot );

    // Get in there
    glPushMatrix();
    glMultMatrixf( &matPosRot.m11 );

    rndLightingEnable(TRUE);
    shPushLightMatrix(&matPosRot);

    if (rndShipVisible( (SpaceObj*)obj, camera )) {
        extern bool8 g_WireframeHack; /// TODO gross... 
        bool wireOn = g_WireframeHack;
        g_WireframeHack = FALSE;
        draw( obj, level );
        g_WireframeHack = (ubyte)wireOn;
    }
        
    shPopLightMatrix();
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    rndLightingEnable(FALSE);
    rndTextureEnable(FALSE);
    glPopMatrix();
}



// Normal dot size on sensors for ships
static real32 shipDotSize( Ship* ship ) {
    // Scale sensor dots for modern resolutions, keeping original look where they bunch up a little
    real32 size = max( 1.0f, 1.28f * getResDensityRelative() );

    // Make drones a little smaller so they don't cramp things overly much
    // This wasn't in the original game, but this fine a distinction couldn't be made back then, now it can
    if (ship && ship->shiptype == Drone)
        size *= 0.80f;

    return size;
}



static bool shipBelongsToPlayer( Ship* ship ) {
    return ship->playerowner == universe.curPlayerPtr;
}



static bool shipRenderFlagIsSet( Ship* ship, udword flag ) {
    return bitTest(smShipTypeRenderFlags[ship->shiptype], flag);
}



static void shipDrawMesh( SpaceObjRotImp* obj, lod* level ) {
    Ship* ship = (Ship*) obj;

    if (ship->bindings)
         meshRenderShipHierarchy( ship->bindings, ship->currentLOD, level->pData, ship->colorScheme );
    else meshRender( level->pData, ship->colorScheme );
}



static color shipDotColor( Ship* ship ) {
    // Special colour overrides
    bool forceAlly     = (ship->attributes & ATTRIBUTES_SMColorField) == ATTRIBUTES_SMColorYellow;
    bool forceFriendly = (ship->attributes & ATTRIBUTES_SMColorField) == ATTRIBUTES_SMColorGreen;

    if (forceFriendly || shipBelongsToPlayer(ship))
        return teFriendlyColor;

    if (forceAlly || allianceIsShipAlly(ship, universe.curPlayerPtr))
        return teAlliedColor;
    
    // By process of elimination, a bad egg! >:[
    return teHostileColor;
}



static void shipDrawAsDot( Ship* ship, bool selectFlash, color background ) {
    color baseCol  = shipDotColor( ship );
    color pointCol = selectFlash ? background : baseCol;
    primPointSize3( &ship->posinfo.position, shipDotSize(ship), pointCol );
}



static void shipDrawAsMesh( Ship* ship, bool selectFlash, lod* level, Camera* camera ) {
    drawMesh( (SpaceObjRotImp*) ship, selectFlash, level, camera, shipDrawMesh );
}



// Special TO sphere stuff. I may have been slightly overdescriptive with the function name...
static void shipSetGravWellGenTacticalOverlayDrawListSphereInfo( Ship* ship, ShipTOItem* item ) {
    if (ship->shiptype == GravWellGenerator)
    if (((GravWellGeneratorSpec *) ship->ShipSpecifics)->GravFieldOn) {
        item->sphereDraw   = TRUE;
        item->sphereShip   = ship;
        item->sphereRadius = ((GravWellGeneratorStatics*) ship->staticinfo->custstatinfo)->GravWellRadius;
        item->sphereCol    = TW_GRAVWELL_SPHERE_COLOUR;
    }
}



static void shipAddToTacticalOverlayDrawList( Ship* ship, bool selectFlash, color background ) {
    if (shipTOCount > SM_NumberTOs)
        return;

    color colBase  = selectFlash ? background : shipDotColor(ship);
    color colFaded = colRGB( colRed(colBase)  / TO_IconColorFade,
                             colGreen(colBase)/ TO_IconColorFade,
                             colBlue(colBase) / TO_IconColorFade );

    ShipTOItem* item = &shipTOList[ shipTOCount ];
    memset( item, 0x00, sizeof(*item) );
    item->shipClass    = ship->staticinfo->shipclass;
    item->radius       = ship->collInfo.selCircleRadius;
    item->x            = ship->collInfo.selCircleX;
    item->y            = ship->collInfo.selCircleY;
    item->col          = colFaded;
    
    shipSetGravWellGenTacticalOverlayDrawListSphereInfo( ship, item );

    shipTOCount++;
}



// Don't draw cloaked ships unless they belong to the player, and don't draw invisible things or things marked as do-not-render
static bool shipIsHidden( Ship* ship, bool isPlayer ) {
    bool isInvisible = (ship->attributes & ATTRIBUTES_SMColorField) == ATTRIBUTES_SMColorInvisible;
    bool isCloaked   = bitTest( ship->flags, SOF_Cloaked );
    bool isHidden    = bitTest( ship->flags, SOF_Hide );

    return isInvisible || isHidden || (isCloaked && !isPlayer);
}



static void shipDraw( Ship* ship, Camera *camera, bool bFlashOn, color background ) {
    if ( ! smNonResources) // not drawing ships right now
        return;

    // Flash when ships are selected and it lines up with the flash timing
    bool selectFlash = bFlashOn && (ship->flags & SOF_Selected);
    bool isPlayer    = shipBelongsToPlayer( ship );
    bool toFlagSet   = shipRenderFlagIsSet( ship, SM_TO );

    // Tactical overlay stuff. Drawn all at once later in 2D phase
    if (smTacticalOverlay && isPlayer && toFlagSet)
        shipAddToTacticalOverlayDrawList( ship, selectFlash, background );

    // To draw something you have to be able to see it, believe it or not
    if (shipIsHidden( ship, isPlayer ))
        return;

    // If it's a ship type to render as a mesh (e.g cruiser, mothership, big stuff) then do that
    // Otherwise just draw as a point
    // @todo The meshes could use something 'extra' for visibility at very long ranges, like colourisation via fog, grow into a dot, something
    if (shipRenderFlagIsSet(ship, SM_Mesh)) { 
        lod* level     = lodLevelGet( ship, &camera->eyeposition, &ship->posinfo.position );
        bool isLodMesh = (level->flags & LM_LODType) == LT_Mesh;

        if (isLodMesh)
             shipDrawAsMesh( ship, selectFlash, level, camera );
        else shipDrawAsDot ( ship, selectFlash, background );
    } else {
        shipDrawAsDot( ship, selectFlash, background );
    }
}



static bool derelictRenderFlagIsSet( Derelict* zoolander, udword flag ) {
    return bitTest( smDerelictTypeMesh[zoolander->derelicttype], flag );
}



static lod* derelictTryGetMeshLod( Derelict* derek, Camera* camera ) {
    if (derek->derelicttype == HyperspaceGate)
        return NULL; // Has no LODs, would crash

    lod* level  = lodLevelGet( derek, &camera->eyeposition, &derek->posinfo.position );
    bool isMesh = (level->flags & LM_LODType) == LT_Mesh;

    if (isMesh)
         return level;
    else return NULL;
}



static bool derelictIsFriendly( Derelict* derek ) {
    return derek != NULL
        && singlePlayerGame
        && derek->derelicttype   == PrisonShipOld
        && spGetCurrentMission() == MISSION_8_THE_CATHEDRAL_OF_KADESH;
}



static color derelictDotColour( Derelict* derek ) {
    if (derek->derelicttype == Crate)
        return teCrateColor;

    if (derelictIsFriendly( derek ))
        return teFriendlyColor;

    return teNeutralColor;
}



static real32 derelictDotSize( Derelict* derek ) {
    real32 refSize = shipDotSize( NULL );

    // Treat this one more like a ship, given its nature
    if (derelictIsFriendly( derek ))
        return refSize;

    // In general though, don't crowd the sensor view too badly, or it gets ugly and fits poorly with asteroids
    return max( 1.0f, refSize * 0.5f );
}



static void derelictDrawMesh( SpaceObjRotImp* obj, lod* level ) {
    Derelict* der = (Derelict*) obj;
    meshRender( level->pData, der->colorScheme );
}



// Derelict meshes don't get drawn in cloudy blobs.
static void derelictDraw( Derelict* derek, Camera* camera, bool meshAllowed ) {
    bool wantMesh = derelictRenderFlagIsSet( derek, SM_Mesh );
    lod* level    = derelictTryGetMeshLod( derek, camera );
    bool hasMesh  = level != NULL;

    if (hasMesh && wantMesh && meshAllowed) {
        drawMesh( (SpaceObjRotImp*)derek, FALSE, level, camera, derelictDrawMesh );
    } else {
        color  col  = derelictDotColour( derek );
        real32 size = derelictDotSize( derek );
        primPointSize3( &derek->posinfo.position, size, col );
    }
}



static void nebGasDustDraw( SpaceObj* obj, Camera* camera ) {
    if (smResources == FALSE)
        return;

    // This is a bit hacky, but it lines up well with 1999 Homeworld.
    // The method of drawing used originally can't scale easily (2D array of alphas drawn with GL_POINT in screenspace).
    // Because of this, at high resolutions it was much more difficult to see them than intended.
    real32 distCur    = sqrtf(vecDistanceSquared( camera->eyeposition, obj->posinfo.position ));
    real32 distThresh = 175'000.0f;
    real32 distCloser = max( 0.0f, distThresh - distCur );
    real32 distScale  = 1.0f - (distCloser / distThresh);
    real32 baseSize   = 500.0f * 1.15f * sqrtf( getResDensityRelative() );
    real32 size       = baseSize * distScale;
    color  col        = obj->staticinfo->staticheader.LOD->pointColor;

    primBlurryPoint3Fade( &obj->posinfo.position, size, col, 1.0f );
}



static void asteroidDraw( Asteroid* ast ) {
    if (smResources == FALSE)
        return;
    
    real32 radius = ast->staticinfo->staticheader.staticCollInfo.collspheresize;
    color  col    = ast->staticinfo->staticheader.LOD->pointColor;
    real32 size   = 1.5f * sqrtf( getResDensityRelative() );

    if (radius > SM_LargeResourceSize)
        size *= 1.5f;
    
    primPointSize3( &ast->posinfo.position, size, col );
}



static void drawNebulaTendrils( void ) {
    //display the nebulae tendrils as strings
    rndTextureEnable(FALSE);
    rndLightingEnable(FALSE);
    rndAdditiveBlends(FALSE);

    glLineWidth( sqrtf(getResDensityRelative()) );
    glEnable(GL_BLEND);
    glShadeModel(GL_FLAT); // 2024: Guess they're not supposed to be interpolated. Or is for ye olde speed? Might look cool to lerp the colour.

    // @todo It would be cool to render them as fuzzy lines
    glBegin(GL_LINES);
    for (udword i=0; i<numNebulae; i++) {
        nebulae_t* neb = &nebNebulae[i];

        for (udword t=0; t<neb->numTendrils; t++) {
            color c = neb->tendrilTable[t].colour;
            glColor4ub(colRed(c), colGreen(c), colBlue(c), 192);
            glVertex3fv((GLfloat*)&neb->tendrilTable[t].a->position);
            glVertex3fv((GLfloat*)&neb->tendrilTable[t].b->position);
        }
    }
    glEnd();

    glDisable(GL_BLEND);
    glLineWidth(1.0f);
}



static void drawShipTacticalOverlays( void ) {
    if (shipTOCount == 0)
        return;

    primModeSet2();

    for (udword ti=0; ti<shipTOCount; ti++) {
        ShipTOItem* to   = &shipTOList[ ti ];
        toicon*     icon = toClassIcon[ to->shipClass ];

        // Mark usage for tactical overlay legend
        toClassUsed[to->shipClass][0] = TRUE; // @todo Player index hardcoded, check

        if (to->radius > 0.0f) {
            color  col       = to->col;
            real32 radius    = max( to->radius, smTORadius ); // @todo Maybe can allow it be a little smaller on modern displays?
            real32 thickness = max( 0.75f, 0.65f * sqrtf(getResDensityRelative()) );

            primLineLoopStart2( thickness, col );

            for (sdword i=icon->nPoints-1; i>= 0; i--)
                primLineLoopPoint3F(to->x + icon->loc[i].x * radius,
                                    to->y + icon->loc[i].y * radius);

            primLineLoopEnd2();

            if (to->sphereDraw)
                toFieldSphereDraw( to->sphereShip, to->sphereRadius, to->sphereCol );
        }
    }

    primModeClear2();
    rndLightingEnable(FALSE);
    rndTextureEnable(FALSE);
}



/*-----------------------------------------------------------------------------
    Name        : smBlobDrawClear
    Description : Render all the ships inside a blob.  This would be for blobs
                    with player ships or when player has full sensors researched.
    Inputs      : camera - POV to render from
                  thisBlob - blob we're rendering
                  modelView, projection - current matrices
                  background - color of the blob being drawn to
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smBlobDrawClear(Camera *camera, blob *thisBlob, hmatrix *modelView, hmatrix *projection, color background)
{
    // Clear some stuff per-frame
    shipTOCount = 0;

    // Selection flashing
    real32 flashRate  = smSelectedFlashSpeed;
    bool   flashState = fmodf(universe.totaltimeelapsed, flashRate) > flashRate/2.0f;

    // Draw all objects in the sphere
    udword     count   = thisBlob->blobObjects->numSpaceObjs;
    SpaceObj** objIter = thisBlob->blobObjects->SpaceObjPtr;

    for (udword i=0; i<count; i++) {
        // We might even render it
        SpaceObj* obj = *objIter++;

        // Compute selection circle
        if (obj->flags & SOF_Targetable)
            selCircleCompute( modelView, projection, (SpaceObjRotImpTarg*)obj ); 

        // Clear any leftover state
        rndTextureEnable(FALSE);
        rndLightingEnable(FALSE);
        rndAdditiveBlends(FALSE);
        
        // Draw
        switch (obj->objtype) {
            case OBJ_ShipType:
                shipDraw( (Ship*)obj, camera, flashState, background );
                break;

            case OBJ_AsteroidType:
                asteroidDraw( (Asteroid*)obj );
                break;

            case OBJ_NebulaType:
            case OBJ_GasType:
            case OBJ_DustType:
                nebGasDustDraw( obj, camera );
                break;

            case OBJ_DerelictType:
                derelictDraw( (Derelict*)obj, camera, TRUE );
        }
    }

    // Draw non-object stuff
    drawNebulaTendrils();
    drawShipTacticalOverlays();
}

/*-----------------------------------------------------------------------------
    Name        : SensorsWeirdnessUtilities
    Description : Utility functions for the sensors weirdness in the single player game
    Inputs      : various
    Outputs     : various
    Return      : various
----------------------------------------------------------------------------*/
//generates a random location within (and slightly outside) the bounds of the universe
vector smwGenerateRandomPoint(void)
{
    vector vec;

    vec.x = (real32)frandyrandombetween(RANDOM_AI_PLAYER,-smUniverseSizeX*WEIRDUNIVERSESTRETCH,smUniverseSizeX*WEIRDUNIVERSESTRETCH);
    vec.y = (real32)frandyrandombetween(RANDOM_AI_PLAYER,-smUniverseSizeY*WEIRDUNIVERSESTRETCH,smUniverseSizeY*WEIRDUNIVERSESTRETCH);
    vec.z = (real32)frandyrandombetween(RANDOM_AI_PLAYER,-smUniverseSizeZ*WEIRDUNIVERSESTRETCH,smUniverseSizeZ*WEIRDUNIVERSESTRETCH);

    return vec;
}

real32 smwGenerateRandomSize(void)
{
    udword sizenum = (udword)randyrandom(RANDOM_AI_PLAYER, 7);

    switch (sizenum)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            return derelictDotSize( NULL );
        case 4:
        case 5:
        case 6:
        default:
            return shipDotSize( NULL );
    }
}


//generates a random color for the point
color smwGenerateRandomColor(void)
{
    udword colornum;
    color  col;

    colornum = (udword)randyrandom(RANDOM_AI_PLAYER, 7);

    switch (colornum)
    {
        case 0:
//            col = teFriendlyColor;
//            break;
        case 1:
        case 2:
        case 3:
            col = teHostileColor;
            break;
        case 4:
        case 5:
        case 6:
        default:
            col = teResourceColor;
            break;
    }
    return col;
}

void smwGeneratePing(vector location)
{
    sdword pingcnt;
    char name[20] = "0";

    //very small chance of a new ping
    if (((udword)randyrandom(RANDOM_AI_PLAYER, 200)) <= 1)
    {
        pingcnt = randyrandom(RANDOM_AI_PLAYER, 3);

        sprintf(name, "Weirdness%i", pingcnt);
        pingAnomalyPingRemove(name);
        pingAnomalyPositionPingAdd(name, &location);
    }
}



/*-----------------------------------------------------------------------------
    Name        : smSensorWeirdnessDraw
    Description : Draws the sensor manager malfunction in Missions 7 and 8
    Inputs      :
    Outputs     : Draws a bunch of weirdness on the screen
    Return      : void
----------------------------------------------------------------------------*/
void smSensorWeirdnessDraw(hmatrix *modelView, hmatrix *projection)
{
    static udword redraw = 0;
    static udword num_points;

    if (!redraw)
    {
        num_points  = (udword)randyrandom(RANDOM_AI_PLAYER, smSensorWeirdness/3);
        num_points += (smSensorWeirdness*2/3);
        redraw      = (udword)(randyrandombetween(RANDOM_AI_PLAYER, 20, 30) * getResFrequencyRelative());
    }
    else
    {
        redraw--;
    }

    const udword num_replaces = (udword)randyrandom(RANDOM_AI_PLAYER, MAX_REPLACEMENTS);
    for (udword i=0;i<num_replaces;i++)
    {
        //chose a random point
        udword j = (udword)randyrandom(RANDOM_AI_PLAYER, smSensorWeirdness);

        //generate a random location for it
        smWeird[j].location = smwGenerateRandomPoint();
        smWeird[j].color    = smwGenerateRandomColor();
        smWeird[j].size     = smwGenerateRandomSize();
        smwGeneratePing(smWeird[j].location);
    }

    for (udword i=0; i<num_points; i++)
        primPointSize3( &smWeird[i].location, smWeird[i].size, smWeird[i].color );
}

/*-----------------------------------------------------------------------------
    Name        : smSensorsWeirdnessInit
    Description : Initializes the sensors weirdness stuff
    Inputs      :
    Outputs     : Fills the smWeirdCol and smWeirdLoc arrays
    Return      : void
----------------------------------------------------------------------------*/
void smSensorWeirdnessInit(void)
{
    udword i;

    for (i=0;i<NUM_WEIRD_POINTS;i++)
    {
        smWeird[i].location    = smwGenerateRandomPoint();
        smWeird[i].color       = smwGenerateRandomColor();
        smWeird[i].ping        = FALSE;
    }
}



/*-----------------------------------------------------------------------------
    Name        : smEnemyFogOfWarBlobCallback
    Description : Sub-blob creation callback for creating fog-of-war sub-blobs
    Inputs      : superBlob - the blob we're looking in
                  obj - object to evaluate for inclusion
    Outputs     :
    Return      : TRUE if it's an enemy ship or a foggy resource
----------------------------------------------------------------------------*/
bool smEnemyFogOfWarBlobCallback(blob *superBlob, SpaceObj *obj)
{
    switch (obj->objtype)
    {
        case OBJ_ShipType:                                  //make sure it's an enemy ship
            if (allianceIsShipAlly((Ship *)obj,universe.curPlayerPtr))
            {                                               //if this is an allied ship (alliance formed while in SM)
                return(FALSE);
            }
            if (((Ship *)obj)->flags & (SOF_Hide | SOF_Cloaked))
            {                                               //don't show hidden or cloaked ships
                return(FALSE);
            }
            if ((obj->attributes & ATTRIBUTES_SMColorField) == ATTRIBUTES_SMColorInvisible)
            {
                return(FALSE);
            }
            return(TRUE);
        case OBJ_BulletType:
            break;
        case OBJ_AsteroidType:
            return(TRUE);
        case OBJ_NebulaType:
            break;
        case OBJ_GasType:
        case OBJ_DustType:
            return(TRUE);
        case OBJ_DerelictType:
        case OBJ_EffectType:
        case OBJ_MissileType:
            break;
    }
    return(FALSE);
}



#define SM_VISUALIZE_SUBBLOBS   0
#if SM_VISUALIZE_SUBBLOBS == 1
    #define smVisSubBlob primCircleOutlineZ
#else 
    #define smVisSubBlob
#endif 



/*-----------------------------------------------------------------------------
    Name        : smBlobDrawCloudy
    Description : Draw a cloudy sensors blob, such as one in the shroud.
    Inputs      : same as for the clear draw
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smBlobDrawCloudy(Camera *camera, blob *thisBlob, hmatrix *modelView, hmatrix *projection, color background)
{
    //compute a list of sub-blobs for the enemies.  It will be deleted with the parent blob.
    if (thisBlob->subBlobs.num == BIT31)
    {                                                       //if list not yet created
        bobSubBlobListCreate(&smFOWBlobProperties, &thisBlob->subBlobs, thisBlob, smEnemyFogOfWarBlobCallback);
        thisBlob->subBlobTime = universe.totaltimeelapsed;
    }
    else
    {
        if (universe.totaltimeelapsed - thisBlob->subBlobTime >= smFOWBlobUpdateTime)
        {
            bobListDelete(&thisBlob->subBlobs);
            bobSubBlobListCreate(&smFOWBlobProperties, &thisBlob->subBlobs, thisBlob, smEnemyFogOfWarBlobCallback);
            thisBlob->subBlobTime = universe.totaltimeelapsed;
        }
    }

    //go through all sub-blobs and remove any that are hidden due to too many resources
    thisBlob->nHiddenShips = 0;
    for (Node* subBlobNode = thisBlob->subBlobs.head; subBlobNode != NULL; subBlobNode = subBlobNode->next)
    {
        blob* subBlob = (blob *)listGetStructOfNode(subBlobNode);
        if (subBlob->shipMass == 0.0f)
        {                                                   //if no ships in blob
            continue;                                       //skip it
        }

        // if ((k1 * #rocks + k2 * #rockRUs) / (k3 * #ships + k4 * shipMass) > 1.0)
        sdword nShips = subBlob->nCapitalShips + subBlob->nAttackShips + subBlob->nNonCombat;
        if ((smFOW_AsteroidK1 * (real32)subBlob->nRocks + smFOW_AsteroidK2 * subBlob->nRockRUs) /
            (smFOW_AsteroidK3 * (nShips) + smFOW_AsteroidK4 * subBlob->shipMass) > 1.0f)
        {                                                   //if there are too many asteroids to see these ships
            smVisSubBlob(&subBlob->centre, subBlob->radius, 16, colRGB(0, 255, 0));
            thisBlob->nHiddenShips += nShips;
            continue;                                       //don't draw them
        }

        // if (k2 * #dustRUs / (k3 * #ships + k4 * shipMass) > 1.0)
        if ((smFOW_DustGasK2 * subBlob->nDustRUs) /
            (smFOW_DustGasK3 * nShips + smFOW_DustGasK4 * subBlob->shipMass) > 1.0f)
        {                                                   //if there's too much dust
            smVisSubBlob(&subBlob->centre, subBlob->radius, 16, colRGB(255, 255, 0));
            thisBlob->nHiddenShips += nShips;
            continue;                                       //don't draw the ships
        }

        //there's not too many resources, draw them as normal
        smVisSubBlob(&subBlob->centre, subBlob->radius, 16, colWhite);
        real32 radius, screenX, screenY;
        selCircleComputeGeneral(modelView, projection, &subBlob->centre, 1.0f, &screenX, &screenY, &radius);
        for (sdword index = subBlob->blobObjects->numSpaceObjs - 1; index >= 0 ; index--)
        {                                                   //all ships in sub-blob get selection circle of sub-blob
            SpaceObj* obj = subBlob->blobObjects->SpaceObjPtr[index];
            if (obj->objtype == OBJ_ShipType)
            {
                ((Ship *)obj)->collInfo.selCircleX = screenX;
                ((Ship *)obj)->collInfo.selCircleY = screenY;
                ((Ship *)obj)->collInfo.selCircleRadius = radius;
            }
        }
        
        // The nasty is visible
        primPointSize3( &subBlob->centre, shipDotSize(NULL), teHostileColor );
    }

    // Draw all objects in the sphere
    udword     count   = thisBlob->blobObjects->numSpaceObjs;
    SpaceObj** objIter = thisBlob->blobObjects->SpaceObjPtr;

    for (udword i=0; i<count; i++) {
        SpaceObj* obj = *objIter++;

        // Compute selection circle
        if (obj->flags & SOF_Targetable)
            selCircleCompute( modelView, projection, (SpaceObjRotImpTarg*)obj ); 

        // Clear any leftover state
        rndTextureEnable(FALSE);
        rndLightingEnable(FALSE);
        rndAdditiveBlends(FALSE);

        switch (obj->objtype) {
            case OBJ_ShipType: //ships were drawn from sub-blobs already, just count up hidden ones
                thisBlob->nHiddenShips += (udword) shipIsHidden( (Ship*)obj, FALSE );
                break;                        

            case OBJ_GasType:
            case OBJ_DustType: 
                nebGasDustDraw( obj, camera ); // Nebulae omitted intentionally here
                break;

            case OBJ_AsteroidType:
                asteroidDraw( (Asteroid*)obj );
                break;

            case OBJ_DerelictType:
                derelictDraw( (Derelict*)obj, camera, FALSE );
                break;
        }
    }

    rndLightingEnable(FALSE);
    rndTextureEnable(FALSE);
}



/*-----------------------------------------------------------------------------
    Name        : smBlobListSort
    Description : qsort callback for sorting a list of blob pointers by sortDistance
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
int smBlobListSort(const void *p0, const void *p1)
{
    blob *b0 = *((blob **)p0);
    blob *b1 = *((blob **)p1);

    if (b0->cameraSortDistance < b1->cameraSortDistance)
    {
        return(-1);
    }
    else
    {
        return(1);
    }
}

/*-----------------------------------------------------------------------------
    Name        : smBlobsSortToCamera
    Description : list - linked list of blobs
                  camera - camera to sort to
    Inputs      :
    Outputs     : (Re)allocates and fills in smBlobSortList with pointers to
                    the blobs in list, sorted by distance from camera->eyeposition
    Return      :
----------------------------------------------------------------------------*/
void smBlobsSortToCamera(LinkedList *list, Camera *camera)
{
    blob *thisBlob;
    Node *node;
    vector distance;

    //grow the blob sorting list if needed
    if (list->num > smBlobSortListLength)
    {
        smBlobSortListLength = list->num;
        smBlobSortList = memRealloc(smBlobSortList, sizeof(blob **) * smBlobSortListLength, "smBlobSortList", NonVolatile);
    }
    node = list->head;
    //do a pass through the blobs to figure their sorting distances and count the blobs
    for (node = list->head, smNumberBlobsSorted = 0; node != NULL; node = node->next, smNumberBlobsSorted++)
    {
        thisBlob = (blob *)listGetStructOfNode(node);

        vecSub(distance, camera->eyeposition, thisBlob->centre);
        thisBlob->cameraSortDistance = vecMagnitudeSquared(distance);//figure out a distance for sorting
        smBlobSortList[smNumberBlobsSorted] = thisBlob;     //store reference to blob
    }
    dbgAssertOrIgnore(smNumberBlobsSorted == list->num);
    qsort(smBlobSortList, smNumberBlobsSorted, sizeof(blob **), smBlobListSort);
}

/*-----------------------------------------------------------------------------
    Name        : smBlobDropDistance
    Description : Compute the distance from the movement cursor to a drop-down
                    line for a blob when the move mechanism is up.
    Inputs      : thisBlob - blob to use for the drop-down line
    Outputs     :
    Return      : distance from center of blob's plane point to the centre
                    point of the movement plane point.
----------------------------------------------------------------------------*/
real32 smBlobDropLineDistance(blob *thisBlob)
{
    vector planePoint = thisBlob->centre;

    planePoint.z = selCentrePoint.z;
    return(ABS(piePlanePoint.x - planePoint.x) +            //manhattan distance
            ABS(piePlanePoint.y - planePoint.y));
}

/*-----------------------------------------------------------------------------
    Name        : smBlobsDraw
    Description : Draws all blobs for the current sensors screen
    Inputs      : list - linked list of objects
    Outputs     :
    Return      : Pointer to blob most directly under the mouse cursor or NULL if none.
----------------------------------------------------------------------------*/
blob *smBlobsDraw(Camera *camera, LinkedList *list, hmatrix *modelView, hmatrix *projection, sdword sensorLevel)
{
    glEnable( GL_MULTISAMPLE );
    glLineWidth( sqrtf(getResDensityRelative()) );

    oval o;
    blob *thisBlob;
//    Node *node;
    blob *closestMoveBlob = NULL;
    real32 closestMoveDistance = REALlyBig, moveDistance;
    sdword index, blobIndex, nSegments, distance, closestDistance = SDWORD_Max;
    real32 distanceToOrigin, distanceToOriginSquared, factor;
    real32 closestSortDistance = REALlyBig, farthestSortDistance = REALlyNegative, highestBlob = 0.0f;
    color c;
    sdword radius;
    vector p0, p1;
    real32 x, y, screenRadius;
#if BOB_VERBOSE_LEVEL >= 2
    sdword nBlobs = 0;
#endif
    bool bClosestMove = FALSE;      // used for playing the tick sound when a blob is highligted
    static bool bPlayedSound;       // ditto
    sdword carrierHalfWidth = 0, mothershipHalfWidth = 0;
    char *carrier = NULL, *mothership = NULL;
    fonthandle oldFont = 0;

    mouseCursorObjPtr  = NULL;               //Falko: got an obscure crash where mouseCursorObjPtr was mangled, will this work?

    memset(toClassUsed, 0, sizeof(toClassUsed));
    closestBlob = NULL;

    distanceToOriginSquared = vecMagnitudeSquared(camera->eyeposition);
    distanceToOrigin = sqrtf(distanceToOriginSquared);

    smBlobsSortToCamera(list, camera);
    primModeSet2();

    //go through the list to find the farthest and closest blobs, plus the blob closest
    //to movement cursor
//    node = list->head;
//    while (node != NULL)
//        thisBlob = (blob *)listGetStructOfNode(node);
//        node = node->next;
    for (blobIndex = 0; blobIndex < smNumberBlobsSorted; blobIndex++)
    {
        thisBlob = smBlobSortList[blobIndex];
        closestSortDistance = min(closestSortDistance, thisBlob->cameraSortDistance);
        farthestSortDistance = max(farthestSortDistance, thisBlob->cameraSortDistance);
        highestBlob = max(highestBlob, ABS(thisBlob->centre.z));
        moveDistance = smBlobDropLineDistance(thisBlob);
        if (moveDistance < closestMoveDistance)
        {                                                   //closest blob to movement mechanism
            closestMoveDistance = moveDistance;
            closestMoveBlob = thisBlob;
        }
    }
    closestSortDistance = sqrtf(closestSortDistance);
    farthestSortDistance = sqrtf(farthestSortDistance);
    if (closestSortDistance >= farthestSortDistance)
    {
        closestSortDistance = farthestSortDistance - 1.0f;
    }
    closestSortDistance -= smClosestMargin;
    farthestSortDistance += smFarthestMargin;

    //do 1 pass (backwards) through all the blobs to render the blobs themselves
    for (blobIndex = smNumberBlobsSorted - 1; blobIndex >= 0; blobIndex--)
    {
        thisBlob = smBlobSortList[blobIndex];

        //compute on-screen location of the blob
        //!!! tune this circle a bit better
        selCircleComputeGeneral(modelView, projection, &thisBlob->centre, thisBlob->radius, &thisBlob->screenX, &thisBlob->screenY, &thisBlob->screenRadius);

        if (thisBlob->screenRadius <= 0.0f)
        {
            continue;
        }

        o.centreX = primGLToScreenX(thisBlob->screenX);
        o.centreY = primGLToScreenY(thisBlob->screenY);
        o.radiusX = o.radiusY = primGLToScreenScaleX(thisBlob->screenRadius);

        if ((thisBlob->flags & (BTF_Explored | BTF_ProbeDroid)) || //if player has ships in this blob
            (sensorLevel == 2 && bitTest(thisBlob->flags, BTF_UncloakedEnemies)))//or you have a sensors array and there are uncloaked enemies
        {
            //v-grad the blob color
            if (thisBlob->centre.z > 0.0f)
            {                                               //if blob is above world plane
                factor = thisBlob->centre.z / highestBlob;
                c = colBlend(smIntBlobColorHigh, smIntBlobColor, factor);
            }
            else
            {                                               //blob below the world plane
                factor = (-thisBlob->centre.z) / highestBlob;
                c = colBlend(smIntBlobColorLow, smIntBlobColor, factor);
            }
            //depth-grad the blob brightness
            factor = sqrtf(thisBlob->cameraSortDistance);
            factor = (factor - closestSortDistance) /
                (farthestSortDistance - closestSortDistance);
            if (factor < 0.0f)
            {
                factor = 0.0f;
            }
            if (factor > 1.0f)
            {
                factor = 1.0f;
            }
            c = colBlend(smIntBackgroundColor, c, factor);
            thisBlob->lastColor = c;
            //radius = o.radiusX * smCircleBorder / 65536;
            if (thisBlob->screenRadius <= SM_BlobRadiusMax)
            {
                radius = primGLToScreenScaleX(thisBlob->screenRadius * smCircleBorder);
                /*
                nSegments = radius * SEL_SegmentsMax / MAIN_WindowHeight + SEL_SegmentsMin;
                nSegments = min(nSegments, SEL_SegmentsMax);
                */
                nSegments = pieCircleSegmentsCompute(thisBlob->screenRadius * smCircleBorder);
                if (smFuzzyBlobs)
                {
                    primCircleBorder(o.centreX, o.centreY, o.radiusX, radius, nSegments, c);
                } 
                else
                {
                    primCircleSolid2(o.centreX, o.centreY, radius, nSegments, c);
                }
            }
        }
#if BOB_VERBOSE_LEVEL >= 2
        nBlobs++;
#endif
        if (mouseInRectGeneral(o.centreX - o.radiusX, o.centreY - o.radiusY, o.centreX + o.radiusX, o.centreY + o.radiusY))
        {                                                   //if inside bounding box of circle
            distance = ABS(o.centreX - mouseCursorX()) + ABS(o.centreY - mouseCursorY());
            if (distance < closestDistance)
            {                                               //if closest yet
                closestDistance = distance;
                closestBlob = thisBlob;
            }
        }
    }

#if SM_VERBOSE_LEVEL >= 3
    fontPrintf(20, MAIN_WindowHeight / 2, colWhite, "%d Blobs", nBlobs);
#endif
    //set the selected bit on all selected ships.  This is only a temporary flag.
    for (index = 0; index < selSelected.numShips; index++)
    {
        bitSet(selSelected.ShipPtr[index]->flags, SOF_Selected);
    }

    lightPositionSet();
    primModeClear2();
    rndLightingEnable(FALSE);
    rndTextureEnable(FALSE);
    //draw the world plane
    //!!! always where the camera is pointing !!!
    if (smCentreWorldPlane)
    {
        p0.x = p0.y = 0.0f;
    }
    else
    {
        p0 = smCamera.lookatpoint;
    }
    p0.z = 0.0f;
    smWorldPlaneDraw(&p0, TRUE, smCurrentWorldPlaneColor);

    //draw the horizon plane
    smHorizonLineDraw(&smCamera, modelView, projection, smCurrentCameraDistance);

    primModeSet2();
    smTickTextDraw();
    primModeClear2();
    rndLightingEnable(FALSE);
    rndTextureEnable(FALSE);

    //another pass through all the blobs to render the objects in the blobs
//    node = list->head;
//    while (node != NULL)
//    {
//        thisBlob = (blob *)listGetStructOfNode(node);
//        node = node->next;
    if (smTacticalOverlay)
    {
        oldFont = fontMakeCurrent(selGroupFont3);
        carrier = ShipTypeToNiceStr(Carrier);
        mothership = ShipTypeToNiceStr(Mothership);
        carrierHalfWidth = fontWidth(carrier) / 2;
        mothershipHalfWidth = fontWidth(mothership) / 2;
    }

    for (blobIndex = 0; blobIndex < smNumberBlobsSorted; blobIndex++)
    {
        thisBlob = smBlobSortList[blobIndex];
        if ((thisBlob->flags & (BTF_Explored | BTF_ProbeDroid)) || //if players in this blob
            (sensorLevel == 2 && bitTest(thisBlob->flags, BTF_UncloakedEnemies)))//or you have a sensors array and there are uncloaked enemies
        {
            smBlobDrawClear(camera, thisBlob, modelView, projection, thisBlob->lastColor);
            if (thisBlob->screenRadius > 0.0f)
            {
                if (smTacticalOverlay)
                {
                    if (bitTest(thisBlob->flags, BTF_Mothership))
                    {
                        primModeSet2();
                        fontPrint(primGLToScreenX(thisBlob->screenX) - mothershipHalfWidth, primGLToScreenY(thisBlob->screenY) + primGLToScreenScaleX(thisBlob->screenRadius), smTOColor, mothership);
                        primModeClear2();
                        rndLightingEnable(FALSE);           //mouse is self-illuminated
                        rndTextureEnable(FALSE);
                    }
                    else if (bitTest(thisBlob->flags, BTF_Carrier))
                    {
                        primModeSet2();
                        fontPrint(primGLToScreenX(thisBlob->screenX) - carrierHalfWidth, primGLToScreenY(thisBlob->screenY) + primGLToScreenScaleX(thisBlob->screenRadius), smTOColor, carrier);
                        primModeClear2();
                        rndLightingEnable(FALSE);           //mouse is self-illuminated
                        rndTextureEnable(FALSE);
                    }
                }
            }
        }
        else
        {
            smBlobDrawCloudy(camera, thisBlob, modelView, projection, thisBlob->lastColor);
        }
    }
    if (smTacticalOverlay)
    {
        fontMakeCurrent(oldFont);
    }

    //single player Sensor Manager malfunction
    //threshold is 10 to avoid divide by zero problems
    if (smSensorWeirdness > 10)
    {
        smSensorWeirdnessDraw(modelView, projection);
    }

    rndTextureEnable(FALSE);
    rndLightingEnable(FALSE);

    {
#if TO_STANDARD_COLORS
            c = teResourceColor;
#else
            c = thisAsteroid->staticinfo->staticheader.LOD->pointColor;
#endif


        rndGLStateLog("Minor asteroids");

        const real32 ast0Size = max( 0.5f, 0.91f * sqrtf(getResDensityRelative()) );
        glPointSize( ast0Size );
        glBegin( GL_POINTS );
        glColor3ub(colRed(c), colGreen(c), colBlue(c));

        for (Node* node = universe.MinorSpaceObjList.head;  node != NULL;  node = node->next) {
            Asteroid* thisAsteroid = (Asteroid *)listGetStructOfNode(node);
            dbgAssertOrIgnore(thisAsteroid->objtype == OBJ_AsteroidType && thisAsteroid->asteroidtype == Asteroid0);
            glVertex3fv( &thisAsteroid->posinfo.position.x );
        }

        glEnd();
        glPointSize( 1.0f );
    }

    //do a pass through the blobs to draw the drop-down lines and blob circles
    if (piePointSpecMode != PSM_Idle)
    {                                                   //if we're in movement mode
        for (blobIndex = 0; blobIndex < smNumberBlobsSorted; blobIndex++)
        {
            thisBlob = smBlobSortList[blobIndex];

            //draw the blob cirle on the world plane
            p0 = thisBlob->centre;                              //position of the yellow circle
            c = colBlack;
            if ( (!(thisBlob->flags & (BTF_Explored | BTF_ProbeDroid))) && (sensorLevel != 2))
            {                                               //if it's all enemies
                if (thisBlob->nCapitalShips + thisBlob->nAttackShips + thisBlob->nNonCombat > thisBlob->nHiddenShips)
                {                                           //and there are visible ships here
                    if (closestMoveBlob == thisBlob && closestMoveDistance < closestMoveBlob->radius / SM_BlobClosenessFactor)
                    {
                        c = TW_SHIP_LINE_CLOSEST_COLOR;     //draw in closest move color
                        bClosestMove = TRUE;
                    }
                    else
                    {
                        c = TW_MOVE_ENEMY_COLOR;            //in enemy move color
                    }
                }
            }
            else if (thisBlob->nCapitalShips + thisBlob->nAttackShips + thisBlob->nNonCombat + thisBlob->nFriendlyShips > 0)
            {                                               //else player has ships here
                if (thisBlob->nFriendlyShips == 0)
                {
                    c = TW_MOVE_ENEMY_COLOR;                //in enemy move color
                }
                else if (closestMoveBlob == thisBlob && closestMoveDistance < closestMoveBlob->radius / SM_BlobClosenessFactor)
                {
                    c = TW_SHIP_LINE_CLOSEST_COLOR;         //draw in closest move color
                    bClosestMove = TRUE;
                }
                else
                {
                    c = TW_MOVE_PIZZA_COLOR;                //in move color
                }
            }
            if (c != colBlack)
            {                                               //if a color was specified, whatever it may have been
                p1.x = thisBlob->centre.x;                  //draw from blob to move plane
                p1.y = thisBlob->centre.y;
                p1.z = piePlanePoint.z;
                if (ABS(p1.z - p0.z) >= smShortFootLength)
                {
                    primLine3(&p0, &p1, c);                     //draw line to plane
                    selCircleComputeGeneral(modelView, projection, &p1, smBlobCircleSize, &x, &y, &screenRadius);
                    nSegments = pieCircleSegmentsCompute(screenRadius);
                    primCircleOutlineZ(&p1, smBlobCircleSize, nSegments, c);//draw the circle on plane
                    primPointSize3( &p1, 2.0f*sqrtf(getResDensityRelative()), c ); // cap the line, using smaller cap than non-sensor view (uses 3x root)
                }
            }
        }
    }

    //now remove the selected bit from all selected ships.
    for (index = 0; index < selSelected.numShips; index++)
    {
        bitClear(selSelected.ShipPtr[index]->flags, SOF_Selected);
    }

    if (bClosestMove && !bPlayedSound)
    {
        bPlayedSound = TRUE;
        soundEvent(NULL, UI_MoveChimes);
    }
    else if (!bClosestMove)
    {
        bPlayedSound = FALSE;
    }

    primModeSet2();
    pingListDraw(&smCamera, modelView, projection, &smViewRectangle);
    primModeClear2();

    glDisable( GL_MULTISAMPLE );
    glLineWidth( 1.0f );

    return(closestBlob);
}

/*-----------------------------------------------------------------------------
    Name        : smXXXFactorGet
    Description : Gets the current zoom factors based on the current zoom time.
    Inputs      : current - current point in function, in  frames
                  range - extent of function, in frames
    Outputs     :
    Return      : current zoom factor
    Notes       : currently a linear interpolation.
----------------------------------------------------------------------------*/
real32 smScrollFactorGet(real32 current, real32 range)
{
    real32 val;

    val = current / range;
    if (val < 0.0f)
    {
        val = 0.0f;
    }
    if (val > 1.0f)
    {
        val = 1.0f;
    }
    return(val);
}
/*
real32 smEyeFactorGet(real32 current, real32 range)
{
    return(current / range);
}
real32 smLookFactorGet(real32 current, real32 range)
{
    return(current / range);
}
*/
/*-----------------------------------------------------------------------------
    Name        : smZoomUpdate
    Description : Updates the zoom function while transitioning TO sensors manager.
    Inputs      : current - current point in function, in time
                  range - extent of function, in time
                  bUpdateCamera - update camera if true
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smZoomUpdate(real32 current, real32 range, bool bUpdateCamera)
{
    real32 scrollFactor;
    real32 eyeFactor;
    real32 lookFactor;
    sdword index, delta, val;
    vector distvec;

    scrollFactor = smScrollFactorGet(current, range);
    eyeFactor = lookFactor = scrollFactor;
//    eyeFactor = smEyeFactorGet(current, range);
//    lookFactor = smLookFactorGet(current, range);

    //animate the sides of the sensors manager screen
    val = (sdword)((real32)smScrollDistLeft * scrollFactor);
    delta = val - smScrollLeft;
    smScrollLeft = val;
    smViewRectangle.x0 += delta;
    for (index = 0; index < smScrollCountLeft; index++)
    {
        regRegionScroll(smScrollListLeft[index], delta, 0);
    }
    val = (sdword)((real32)smScrollDistTop * scrollFactor);
    delta = val - smScrollTop;
    smScrollTop = val;
    smViewRectangle.y0 += delta;
    for (index = 0; index < smScrollCountTop; index++)
    {
        regRegionScroll(smScrollListTop[index], 0, delta);
    }
    val = (sdword)((real32)smScrollDistRight * scrollFactor);
    delta = val - smScrollRight;
    smScrollRight = val;
    smViewRectangle.x1 -= delta;
    for (index = 0; index < smScrollCountRight; index++)
    {
        regRegionScroll(smScrollListRight[index], -delta, 0);
    }
    val = (sdword)((real32)smScrollDistBottom * scrollFactor);
    delta = val - smScrollBottom;
    smScrollBottom = val;
    smViewRectangle.y1 -= delta;
    for (index = 0; index < smScrollCountBottom; index++)
    {
        regRegionScroll(smScrollListBottom[index], 0, -delta);
    }

    smIntBackgroundColor = colMultiplyClamped(smBackgroundColor, scrollFactor);
    smIntBlobColor = colMultiplyClamped(smBlobColor, scrollFactor);
    smIntBlobColorHigh = colMultiplyClamped(smBlobColorHigh, scrollFactor);
    smIntBlobColorLow = colMultiplyClamped(smBlobColorLow, scrollFactor);
    rndSetClearColor(colRGBA(colRed(smIntBackgroundColor),
                             colGreen(smIntBackgroundColor),
                             colBlue(smIntBackgroundColor),
                             255));
    btgSetColourMultiplier((1.0f - smBackgroundDim) * (1.0f - scrollFactor) + smBackgroundDim);
    if (piePointSpecMode != PSM_Idle)
    {
        smCurrentWorldPlaneColor = smWorldPlaneColor;
    }
    else
    {
        smCurrentWorldPlaneColor = colMultiplyClamped(smWorldPlaneColor, scrollFactor);
    }
    //blend between the far clip distances of the 2 cameras
    smCurrentCameraDistance = (smCamera.clipPlaneFar - mrCamera->clipPlaneFar) * scrollFactor + mrCamera->clipPlaneFar;

    //animate the camera position based upon the current zoom factors
    if (bUpdateCamera)
    {
        vecVectorsBlend(&smCamera.lookatpoint, &smLookStart, &smLookEnd, lookFactor);
        vecVectorsBlend(&smCamera.eyeposition, &smEyeStart, &smEyeEnd, eyeFactor);

        vecSub(distvec, smCamera.eyeposition, smCamera.lookatpoint);
//        distvec.z *= -1.0f;
        GetDistanceAngleDeclination(&smCamera, &distvec);
    }
    //mouseClipToRect(NULL);
}

/*-----------------------------------------------------------------------------
    Name        : smSensorsCloseForGood
    Description : Closes sensors manager after zooming in.
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void smSensorsCloseForGood(void)
{
#if SM_VERBOSE_LEVEL >= 1
    dbgMessage("smSensorsCloseForGood");
#endif
    if (tutorial)
    {
        tutGameMessage("Game_SensorsClose");
    }
    mrRenderMainScreen = TRUE;
    feScreenDelete(smBaseRegion);
//    bobListDelete(&smBlobList);
//    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    rndSetClearColor(colRGBA(colRed(universe.backgroundColor),
                             colGreen(universe.backgroundColor),
                             colBlue(universe.backgroundColor),
                             255));
    smSensorsActive = FALSE;
    universe.dontUpdateRenderList = FALSE;
    if (smScrollListLeft) memFree(smScrollListLeft);
    if (smScrollListRight) memFree(smScrollListRight);
    if (smScrollListTop) memFree(smScrollListTop);
    if (smScrollListBottom) memFree(smScrollListBottom);
    universe.mainCameraCommand.actualcamera.ignoreZoom = FALSE;
    mouseCursorShow();
    mrHoldLeft = mrHoldRight = mrNULL;
    if (ioSaveState)
    {
        ioEnable();
    }

    if(smFocusOnMothershipOnClose)
    {
//      CameraStackEntry *entry;
//      entry = currentCameraStackEntry(&universe.mainCameraCommand);

        ccFocusOnMyMothership(&universe.mainCameraCommand);
        smFocusOnMothershipOnClose = FALSE;

//      if(entry != currentCameraStackEntry(&universe.mainCameraCommand))
//          listDeleteNode(&entry->stacklink);
    }

    smZoomingIn = FALSE;
    smZoomingOut = FALSE;
    mouseClipToRect(NULL);                                  //make sure mouse does not stay locked to the SM viewport region
    MP_HyperSpaceFlag = FALSE;
    bitClear(tbDisable,TBDISABLE_SENSORS_USE);
    btgSetColourMultiplier(1.0f);                           //make sure we always come out of the SM with the BTG in good order
}


/*-----------------------------------------------------------------------------
    Name        : smClickedOnPlayer
    Description : returns the a pointer to the player you right clicked on.  NULL otherwise/
    Inputs      : none
    Outputs     : player clicked on or NULL
    Return      : void
----------------------------------------------------------------------------*/
sdword smClickedOnPlayer(rectangle *viewportRect)
{
    fonthandle oldfont;
    rectangle  playerColorRect;
    sdword     index;

    oldfont = fontMakeCurrent(selGroupFont2);

    playerColorRect.y1 = viewportRect->y1 - smPlayerListMarginY;
    playerColorRect.y0 = playerColorRect.y1 - fontHeight(" ");
    playerColorRect.x0 = viewportRect->x0 + smPlayerListMarginX;
    playerColorRect.x1 = playerColorRect.x0 + fontHeight(" ");

    //draw the list of player names/colors
    for (index = universe.numPlayers - 1; index >= 0; index--)
    {
        if (universe.players[index].playerState==PLAYER_ALIVE)
        {
            playerColorRect.x1 = fontWidth(playerNames[index]) + fontHeight(" ")*2;
            if ((mouseInRect(&playerColorRect)) && (index!=sigsPlayerIndex))
            {
                return (index);
            }
            playerColorRect.y0 -= smPlayerListMarginY + fontHeight(" ");//update the position
            playerColorRect.y1 -= smPlayerListMarginY + fontHeight(" ");
        }
    }

    fontMakeCurrent(oldfont);

    return (-1);
}


/*-----------------------------------------------------------------------------
    Name        : smPlayerNamesDraw
    Description : Draw the names of the players in a multi-player game.
    Inputs      : viewportRect - boundaries of the current viewport
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smPlayerNamesDraw(rectangle *viewportRect)
{
    fonthandle oldfont;
    sdword index;
    rectangle playerColorRect;
    color c;
    char buffer[50];
    sdword bounty;

    oldfont = fontMakeCurrent(selGroupFont2);

    playerColorRect.y1 = viewportRect->y1 - smPlayerListMarginY;
    playerColorRect.y0 = playerColorRect.y1 - fontHeight(" ");
    playerColorRect.x0 = viewportRect->x0 + smPlayerListMarginX;
    playerColorRect.x1 = playerColorRect.x0 + fontHeight(" ");

    //draw the list of player names/colors
    for (index = universe.numPlayers - 1; index >= 0; index--)
    {
        if (universe.players[index].playerState==PLAYER_ALIVE)
        {
            sdword x;
            bool playerhasdroppedOutOrQuit = playerHasDroppedOutOrQuit(index);

            c = teColorSchemes[index].textureColor.base;
            if (playerhasdroppedOutOrQuit)
            {
                c = HorseRaceDropoutColor;
            }

            primRectSolid2(&playerColorRect, c);                 //draw colored rectangle
            x = playerColorRect.x1 + smPlayerListTextMargin;
            fontPrint(x, //print the player name in a stock color
                      playerColorRect.y0, playerhasdroppedOutOrQuit ? HorseRaceDropoutColor : smPlayerListTextColor, playerNames[index]);
            x += fontWidth(playerNames[index])+fontWidth(" ");
            if (index != sigsPlayerIndex)
            {
                if ((allianceArePlayersAllied(&universe.players[sigsPlayerIndex],&universe.players[index])) ||
                    ((universe.players[sigsPlayerIndex].Allies!=0)&&(index == sigsPlayerIndex)))
                {
                    fontPrint(x, playerColorRect.y0, playerhasdroppedOutOrQuit ? HorseRaceDropoutColor : alliescolor, strGetString(strPlayersAllied));
                }
            }

            x += fontWidth(strGetString(strPlayersAllied));
            if((!singlePlayerGame) && (tpGameCreated.bountySize != MG_BountiesOff))
            {
                //draw bounty values
                fontPrint(x,
                          playerColorRect.y0, playerhasdroppedOutOrQuit ? HorseRaceDropoutColor : colWhite, "B: ");
                bounty = getPlayerBountyRender(&universe.players[index]);
                /*itoa(bounty,buffer,10);*/
                sprintf(buffer, "%d", bounty);
                x += fontWidth("B: ");

                fontPrint(x, playerColorRect.y0, playerhasdroppedOutOrQuit ? HorseRaceDropoutColor : colWhite, buffer);

                x += fontWidth(buffer) + fontWidth(" ");
            }

            if (playerhasdroppedOutOrQuit)
            {
                fontPrint(x,playerColorRect.y0, c, (playersReadyToGo[index] == PLAYER_QUIT) ? strGetString(strQuit) : strGetString(strDroppedOut));
            }

            playerColorRect.y0 -= smPlayerListMarginY + fontHeight(" ");//update the position
            playerColorRect.y1 -= smPlayerListMarginY + fontHeight(" ");
        }
    }

    fontMakeCurrent(oldfont);
}
/*-----------------------------------------------------------------------------
    Name        : smCursorTextDraw
    Description : Draw sensors manager tactical overlay.  This includes the
                    player names/colors and a breakdown of what's in a mission sphere.
    Inputs      : viewportRect - boundaries of the current viewport
                  selectedBlob - blob under mouse cursor, or NULL if none.
                  sensorLevel - current sensors level
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smCursorTextDraw(rectangle *viewportRect, blob *selectedBlob, sdword sensorLevel)
{
    char *cursorText = NULL;
    SpaceObj *object;
    fonthandle fhSave;

    //now draw the cursor text to reflect what is under the mouse cursor
    if (selectedBlob != NULL)
    {                                                       //if the mouse is over a blob
        if ((selectedBlob->flags & (BTF_Explored | BTF_ProbeDroid)))
        {                                                   //if this blob has been explored
            if ((object = selClickFromArray(selectedBlob->blobObjects->SpaceObjPtr,
                selectedBlob->blobObjects->numSpaceObjs, mouseCursorX(),mouseCursorY())) != NULL)
            {
                switch (object->objtype)
                {
                    case OBJ_ShipType:
                        if (((Ship *)object)->playerowner == universe.curPlayerPtr)
                        {
//                            cursorText = strGetString(strPlayerUnits);
                            cursorText = ShipTypeToNiceStr(((Ship *)object)->shiptype);
                        }
                        else if (allianceIsShipAlly((Ship *)object, universe.curPlayerPtr))
                        {
                            cursorText = strGetString(strAlliedUnits);
                        }
                        else
                        {
                            cursorText = strGetString(strEnemyUnits);
                        }
                        break;
                    case OBJ_AsteroidType:
                    case OBJ_NebulaType:
                    case OBJ_GasType:
                    case OBJ_DustType:
                        cursorText = strGetString(strResources);
                        break;
                    case OBJ_DerelictType:
                        switch (((Derelict *)object)->derelicttype)
                        {
                            case PrisonShipOld:
                            case PrisonShipNew:
                            case Shipwreck:
                            case JunkAdvanceSupportFrigate:
                            case JunkCarrier:
                            case JunkDDDFrigate:
                            case JunkHeavyCorvette:
                            case JunkHeavyCruiser:
                            case JunkIonCannonFrigate:
                            case JunkLightCorvette:
                            case JunkMinelayerCorvette:
                            case JunkMultiGunCorvette:
                            case JunkRepairCorvette:
                            case JunkResourceController:
                            case JunkSalCapCorvette:
                            case JunkStandardFrigate:
                                cursorText = strGetString(strDerelictShip);
                                break;
                            default:
                                cursorText = NULL;
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        else
        {                                                   //else unexplored blob
            if (sensorLevel == 0)
            {                                               //don't know anything on plain blobs
                cursorText = strGetString(strUnexplored);
            }
            else
            {                                               //else we've got some sensors available
                if ((object = selClickFromArray(selectedBlob->blobObjects->SpaceObjPtr,
                    selectedBlob->blobObjects->numSpaceObjs, mouseCursorX(),mouseCursorY())) != NULL)
                {
                    switch (object->objtype)
                    {
                        case OBJ_ShipType:
                            cursorText = strGetString(strEnemyUnits);
                            break;
                        case OBJ_AsteroidType:
                            cursorText = strGetString(strAsteroids);
                            break;
                        case OBJ_NebulaType:
                            cursorText = strGetString(strNebula);
                            break;
                        case OBJ_GasType:
                            cursorText = strGetString(strGasClouds);
                            break;
                        case OBJ_DustType:
                            cursorText = strGetString(strDustClouds);
                            break;
                        case OBJ_DerelictType:
                            cursorText = strGetString(strDerelictShip);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
    //cursor text chosen, print the cursor text if applicable
    if (cursorText != NULL)
    {
        fhSave = fontMakeCurrent(mouseCursorFont);
        fontPrint(viewportRect->x0 + smPlayerListMarginX,
                  viewportRect->y1 - fontHeight(cursorText) - smPlayerListMarginY,
                  TW_CURSORTEXT_COLOR, cursorText);
        fontMakeCurrent(fhSave);
    }
}

/*-----------------------------------------------------------------------------
    Name        : smTacticalOverlayDraw
    Description : Draw the tactical overlay for sensors manager, including
                  resource types and such
    Inputs      : thisBlob - blob to draw
                  viewportRect - rectangle encompassing the sensors manager
                  sensorLevel - current sensors level
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
/*
void smTacticalOverlayDraw(blob *thisBlob, rectangle *viewportRect, sdword sensorLevel)
{
    sdword x, y, yStart, xSpacing;

    dbgAssertOrIgnore(thisBlob != NULL);
    x = mouseCursorX() + smTOBottomCornerX;
    yStart = y = mouseCursorY() - fontHeight(" ") + smTOBottomCornerY;
    if (thisBlob->RUs != 0)
    {                                                   //if there's resources in the blob
        if ((sensorLevel == 2 && bitTest(thisBlob->flags, BTF_Explored)) || bitTest(thisBlob->flags, BTF_ProbeDroid))
        {                                               //if max sensor level, print breakdown of resources in the blob
            xSpacing = fontWidth(strGetString(strResourcesRes))+fontWidth(" ");
            if (thisBlob->nGasRUs != 0)
            {
                fontPrintf(x + xSpacing, y, smTOColor, "%d %s", thisBlob->nGasRUs, strGetString(strGas));
                y -= fontHeight(" ") + smTOLineSpacing;
            }
            if (thisBlob->nDustRUs != 0)
            {
                fontPrintf(x + xSpacing, y, smTOColor, "%d %s", thisBlob->nDustRUs,strGetString(strDust));
                y -= fontHeight(" ") + smTOLineSpacing;
            }
            if (thisBlob->nRockRUs != 0)
            {
                fontPrintf(x + xSpacing, y, smTOColor, "%d %s", thisBlob->nRockRUs, strGetString(strRock));
                y -= fontHeight(" ") + smTOLineSpacing;
            }
            y += fontHeight(" ") + smTOLineSpacing;
            fontPrintf(x, y, smTOColor, "%s ",strGetString(strResourcesRes));
            y -= fontHeight(" ") + smTOLineSpacing;
        }
        else if (sensorLevel >= 1 || bitTest(thisBlob->flags, BTF_Explored))
        {                                               //else crappy sensors; just print the types
            fontPrintf(x, y, smTOColor, "%s %s %s %s", strGetString(strResourcesRes),
                (thisBlob->flags & (BTF_Asteroid)) ? strGetString(strRock) : "",
                (thisBlob->flags & (BTF_DustCloud)) ? strGetString(strDust) : "",
                (thisBlob->flags & (BTF_GasCloud | BTF_Nebula)) ? strGetString(strGas) : "");
            y -= fontHeight(" ") + smTOLineSpacing;
        }
    }
    //print types and numbers of ships
    if (thisBlob->flags & (BTF_Explored | BTF_ProbeDroid))
    {
        if (sensorLevel == 2 || bitTest(thisBlob->flags, BTF_ProbeDroid))
        {                                                   //level 3 explored
            if (thisBlob->nNonCombat > 0)
            {
                fontPrintf(x, y, smTOColor, "%d %s", thisBlob->nNonCombat, strGetString(strNonCombatShips));
                y -= fontHeight(" ") + smTOLineSpacing;
            }
            if (thisBlob->nAttackShips > 0)
            {
                fontPrintf(x, y, smTOColor, "%d %s", thisBlob->nAttackShips, strGetString(strStrikeCraft));
                y -= fontHeight(" ") + smTOLineSpacing;
            }
            if (thisBlob->nCapitalShips > 0)
            {
                fontPrintf(x, y, smTOColor, "%d %s", thisBlob->nCapitalShips, strGetString(strCapitalShips));
                y -= fontHeight(" ") + smTOLineSpacing;
            }
        }
        else if (sensorLevel == 0)
        {                                                   //level 0 explored
            xSpacing = thisBlob->nCapitalShips + thisBlob->nAttackShips + thisBlob->nNonCombat;
            if (xSpacing > 0)
            {
                fontPrintf(x, y, smTOColor, "%d %s", xSpacing, strGetString(strEnemyUnits));
                y -= fontHeight(" ") + smTOLineSpacing;
            }
        }
        else if (sensorLevel == 1)
        {                                                   //level 1 explored
            xSpacing = thisBlob->nCapitalShips + thisBlob->nAttackShips + thisBlob->nNonCombat;
            if (xSpacing > 0)
            {
                fontPrint(x, y, smTOColor, strGetString(strEnemyUnits));
                y -= fontHeight(" ") + smTOLineSpacing;
            }
        }
    }
    else if (sensorLevel >= 1)
    {                                                       //level 0 unexplored
        xSpacing = thisBlob->nCapitalShips + thisBlob->nAttackShips + thisBlob->nNonCombat;
        if (xSpacing > 0)
        {
            fontPrint(x, y, smTOColor, strGetString(strEnemyUnits));
            y -= fontHeight(" ") + smTOLineSpacing;
        }
    }
    //if nothing was printed, just say it's unexplored
    if (y == yStart)
    {
        fontPrint(x, y, smTOColor, strGetString(strUnexplored));
    }
}
*/

/*-----------------------------------------------------------------------------
    Name        : smHotkeyGroupsDraw
    Description : Draw the location of hotkey groups.
    Inputs      : void
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smHotkeyGroupsDraw(void)
{
    sdword index;
    real32 averageSize, x, y, radius;
    vector centre;
    fonthandle currentFont = fontCurrentGet();
    MaxSelection selection;

    fontMakeCurrent(selGroupFont3);
    for (index = 0; index < 10; index++)
    {
        MakeShipsNotHidden((SelectCommand *)&selection, (SelectCommand *)&selHotKeyGroup[index]);
        if (selection.numShips > 0)
        {
            centre = selCentrePointComputeGeneral(&selection, &averageSize);
            selCircleComputeGeneral(&rndCameraMatrix, &rndProjectionMatrix, &centre, 1.0f, &x, &y, &radius);
            if (radius > 0.0f)
            {
                fontPrint(primGLToScreenX(x) + smHotKeyOffsetX,
                           primGLToScreenY(y) + smHotKeyOffsetY,
                           selHotKeyNumberColor, selHotKeyString[index]);
            }
        }
    }
    fontMakeCurrent(currentFont);
}

/*-----------------------------------------------------------------------------
    Name        : smPan
    Description : Special-function button that handles both clicks and pan operations.
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void smPan(char *name, featom *atom)
{
    if (uicDragX == 0 && uicDragY == 0)
    {                                                       //if it's just a click
        smCameraLookatPoint.x = smCameraLookatPoint.y = smCameraLookatPoint.z = 0.0f;//recentre the camera
    }
    else
    {                                                       //else it's a drag operation
        vector addVector, lookVector;
        real32 mult, temp;

        vecSub(lookVector, smCamera.lookatpoint, smCamera.eyeposition);
        addVector.x = lookVector.x;
        addVector.y = lookVector.y;
        addVector.z = 0.0f;
        vecNormalize(&addVector);
        mult = smCamera.distance * smPanSpeedMultiplier;
        vecScalarMultiply(addVector, addVector, mult);      //into/out of screen vector
        vecScalarMultiply(lookVector, addVector, (real32)uicDragY);
        vecSubFrom(smCameraLookatPoint, lookVector);          //pan into/out of screen

        temp = addVector.x;                                 //negate-swap x,y
        addVector.x = addVector.y;
        addVector.y = -temp;                                //left/right vector

        vecScalarMultiply(lookVector, addVector, (real32)uicDragX);
        vecAddTo(smCameraLookatPoint, lookVector);          //pan left/right

        lookVector.x = lookVector.y = lookVector.z = 0.0f;
        pieMovePointClipToLimits(smUniverseSizeX * smPanUnivExtentMult,
                                 smUniverseSizeY * smPanUnivExtentMult,
                                 smUniverseSizeZ, &lookVector, &smCameraLookatPoint);
    }
//    smCamera.lookatpoint = smCameraLookatPoint;
}

/*-----------------------------------------------------------------------------
    Name        : smViewportRender
    Description : Callback which draws the main sensors manager viewport.
    Inputs      : standard FE callback
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void smViewportRender(featom *atom, regionhandle region)
{
    blob *selectedBlob;
    CameraStackEntry *curentry = currentCameraStackEntry(&universe.mainCameraCommand);
    sdword frameTicks, sensorLevel;
    real32 frameStep;
    vector rememberVec, diffVec;
    sdword index;

    frameTicks = utyNFrameTicks;
    if (frameTicks > smMaxFrameTicks)
        frameTicks = smMaxFrameTicks;

    frameStep = (real32)frameTicks / (real32)taskFrequency;
    cameraControl(&smCamera, FALSE);                         //update the camera
    curentry->remembercam.angle = smCamera.angle;
    curentry->remembercam.declination = smCamera.declination;

    if (smZoomingOut)
    {
        smZoomTime += frameStep;
        smZoomUpdate(smZoomTime, smCurrentZoomLength, TRUE);
        if (smZoomTime >= smCurrentZoomLength)
        {                                                   //if done zooming out
            soundEvent(NULL, UI_SensorsManager);
            smZoomingOut = FALSE;                           //stop zooming
        }
        if (smZoomTime < smCurrentMainViewZoomLength)
        {                                                   //if still in the regular renderer part of the zoom
            cameraCopyPositionInfo(&universe.mainCameraCommand.actualcamera, &smCamera);
            universe.mainCameraCommand.actualcamera.ignoreZoom = TRUE;
            return;                                         //let mainrgn.c handle the rendering
        }
        else
        {                                                   //else we're in the SM rendering part
            mrRenderMainScreen = FALSE;

            dbgAssertOrIgnore(smZoomTime >= smCurrentMainViewZoomLength);
            universe.mainCameraCommand.actualcamera.ignoreZoom = FALSE;
        }
    }
    else if (smZoomingIn)
    {
        dbgAssertOrIgnore(smEyeStart.x < (real32)1e19 && smEyeStart.x > (real32)-1e19);
        smZoomTime -= frameStep;
        smZoomUpdate(smZoomTime, smCurrentZoomLength, TRUE);
        if (smZoomTime <= 0.0f)
        {                                                   //fully zoomed in
            smZoomingIn = FALSE;
            //...actually close the sensors manager
            smSensorsCloseForGood();                        //stop zooming in already!
        }

        if (smZoomTime >= smCurrentMainViewZoomLength)
        {                                                   //if in sm part of zoom
            cameraCopyPositionInfo(&universe.mainCameraCommand.actualcamera, &smCamera);
            universe.mainCameraCommand.actualcamera.ignoreZoom = TRUE;
        }
        else
        {                                                   //else rendering main game screen
            if (!smFocusTransition)
            {                                               //if at transition point
                smFocusTransition = TRUE;                   //make sure we only do this once a zoom
                //... execute the ccfocus thingus
                if (smFocus)
                {
                    universe.mainCameraCommand.actualcamera.ignoreZoom = FALSE;
                    ccFocusFar(&universe.mainCameraCommand, smFocusCommand, &universe.mainCameraCommand.actualcamera);
                    memFree(smFocusCommand);
                    smFocusCommand = NULL;
                }
                mrRenderMainScreen = TRUE;

                soundEvent(NULL, UI_SoundOfSpace);
            }
            else
            {                                               //else the transition has already been made
                if (!smFocus && smZoomTime > 0.0f)
                {                                           //if not at end of zoom in with no focus
                    cameraCopyPositionInfo(&universe.mainCameraCommand.actualcamera, &smCamera);
                    universe.mainCameraCommand.actualcamera.ignoreZoom = TRUE;
                }
                return;
            }
        }
    }
    else
    {
        smCurrentWorldPlaneColor = smWorldPlaneColor;
        smCurrentCameraDistance = smCamera.clipPlaneFar;
    }
    smViewportRegion->rect = smViewRectangle;
    if (piePointSpecMode != PSM_Idle)
    {
        smCurrentWorldPlaneColor = colMultiplyClamped(smCurrentWorldPlaneColor, smMovementWorldPlaneDim);
    }

    //perform some render-time monkey work of the camera position
    if (!smZoomingIn && !smZoomingOut)
    {                                               //don't move the eye point if zooming
        uicDragX = uicDragY = 0;
        if (keyIsHit(ARRLEFT))
        {                                                       //pan the camera with the cursor keys
            uicDragX -= smCursorPanX * frameTicks / (udword)taskFrequency;
        }
        if (keyIsHit(ARRRIGHT))
        {
            uicDragX += smCursorPanX * frameTicks / (udword)taskFrequency;
        }
        if (keyIsHit(ARRUP))
        {
            uicDragY -= smCursorPanY * frameTicks / (udword)taskFrequency;
        }
        if (keyIsHit(ARRDOWN))
        {
            uicDragY += smCursorPanY * frameTicks / (udword)taskFrequency;
        }
        if (uicDragX != 0 || uicDragY != 0)
        {
            smPan(NULL, NULL);
        }
        //perform cubic spline interpolation of the panned camera point
        vecSub(rememberVec, smCamera.lookatpoint, smCameraLookatPoint);
        if (ABS(rememberVec.x) + ABS(rememberVec.y) + ABS(rememberVec.z) > smPanEvalThreshold)
        {                                                   //if there is any distance between pan point and camera point
            while (frameTicks)
            {
                rememberVec = smCamera.lookatpoint;
                EvalCubic(&smCamera.lookatpoint.x, &smCameraLookVelocity.x, smCameraLookatPoint.x, smPanTrack);
                EvalCubic(&smCamera.lookatpoint.y, &smCameraLookVelocity.y, smCameraLookatPoint.y, smPanTrack);
                EvalCubic(&smCamera.lookatpoint.z, &smCameraLookVelocity.z, smCameraLookatPoint.z, smPanTrack);

                vecSub(diffVec, smCamera.lookatpoint, rememberVec);
                vecAddTo(smCamera.eyeposition, diffVec);    //pan the eyeposition with the lookatpoint
                frameTicks--;
            }
        }
    }
    //now on to the actual rendering code:
    cameraSetEyePosition(&smCamera);

    primModeClear2();
    rndLightingEnable(FALSE);
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    rgluPerspective(smCamera.fieldofview, rndAspectRatio,    //set projection matrix
                   smCamera.clipPlaneNear, smCamera.clipPlaneFar);
    glScalef(smProjectionScale, smProjectionScale, smProjectionScale);
    glMatrixMode( GL_MODELVIEW );

    glLoadIdentity();
#if RND_CAMERA_OFFSET
    vector lookEye = smCamera.eyeposition;
    vector lookAt  = smCamera.lookatpoint;
    lookEye.x += RND_CameraOffsetX;
    lookAt .x += RND_CameraOffsetX;
    rgluLookAt(lookEye, lookAt, smCamera.upvector);
#else
    rgluLookAt(smCamera.eyeposition, smCamera.lookatpoint, smCamera.upvector);
#endif
    glGetFloatv(GL_MODELVIEW_MATRIX,  (GLfloat *)(&rndCameraMatrix));
    glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)(&rndProjectionMatrix));

    //Draw the BTG background
    glPushMatrix();
    glTranslatef(smCamera.eyeposition.x, smCamera.eyeposition.y, smCamera.eyeposition.z);
    rndBackgroundRender(BG_RADIUS, &smCamera, FALSE);  //render the background
    glPopMatrix();

    lightSetLighting();
    rndLightingEnable(TRUE);

    //render all worlds in the universe
    for (index = 0; index < UNIV_NUMBER_WORLDS; index++)
    {
        if (universe.world[index] != NULL)
        {
            if (universe.world[index]->objtype == OBJ_DerelictType)
            {
                rndRenderAHomeworld(&smCamera, universe.world[index]);
            }
        }
        else
        {
            break;
        }
    }

    //update and draw all the sensors manager blobs
    smRenderCount++;
    if (smRenderCount >= smBlobUpdateRate)
    {
        smRenderCount = 0;
    }
    if (smGhostMode)
    {
        sensorLevel = 2;
    }
    else
    {
        sensorLevel = universe.curPlayerPtr->sensorLevel;
    }
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)(&rndCameraMatrix));
    glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)(&rndProjectionMatrix));

    selectedBlob = smBlobsDraw(&smCamera, &universe.collBlobList, &rndCameraMatrix, &rndProjectionMatrix, universe.curPlayerPtr->sensorLevel);

    if (piePointSpecMode != PSM_Idle)
    {                                                       //draw point spec
        mrCamera = &smCamera;                               //point spec draw uses mrCamera
        pieLineDrawCallback = NULL;
        piePlaneDrawCallback = piePlaneDraw;
        pieMovementCursorDrawCallback = pieMovementCursorDraw;
        piePointSpecDraw();                                 //draw the movement mechanism
    }

    primModeSet2();

    //draw the tactical overlay, if applicable
    if (smTacticalOverlay)
    {
#if !TO_STANDARD_COLORS
        if (!singlePlayerGame)
        {                                                   //only print player names if multi-player game is underway
            smPlayerNamesDraw(&smViewRectangle);            //draw the list of player names
        }
#endif
        smHotkeyGroupsDraw();
    }
    if (smHoldRight == smNULL && smHoldLeft == smNULL)
    {
        smCursorTextDraw(&smViewRectangle, selectedBlob, universe.curPlayerPtr->sensorLevel);
    }

    if (smHoldLeft == smSelectHold)                         //and then draw the current selection progress
    {
        primRectOutline2(&smSelectionRect, 1, TW_SELECT_BOX_COLOR);
    }
    if (nisStatic[NIS_SMStaticIndex] != NULL)
    {
        nisStaticDraw(nisStatic[NIS_SMStaticIndex]);
    }
}

/*-----------------------------------------------------------------------------
    Name        : smNULL
    Description : NULL logic handler for when the user's not doing anything with
                    the sensors manager.
    Inputs      : void
    Outputs     : void
    Return      : void
----------------------------------------------------------------------------*/
void smNULL(void)
{
    ;
}

/*-----------------------------------------------------------------------------
    Name        : mrSelectHold
    Description : Function for dragging a selection box
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void smSelectHold(void)
{
    if (ABS(mouseCursorX() - mrOldMouseX) >= selClickBoxWidth ||
        ABS(mouseCursorY() - mrOldMouseY) >= selClickBoxHeight)
    {                                                       //if mouse has moved from anchor point
        mrSelectRectBuild(&smSelectionRect, mrOldMouseX, mrOldMouseY);//create a selection rect
        selRectNone();
    }
    else
    {                                                       //else mouse hasn't moved far enough
        smSelectionRect.x0 = smSelectionRect.x1 =           //select nothing
            smSelectionRect.y0 = smSelectionRect.y1 = 0;
        selRectNone();
    }
}

/*-----------------------------------------------------------------------------
    Name        : smCullAndFocusSelecting
    Description : Culls the current selection list to a reasonable size and
                    focuses on it.
    Inputs      : void
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smCullAndFocusSelecting(void)
{
    sdword index, nToFocusOn;
    vector centre = {0.0f, 0.0f, 0.0f};

    if ((tutorial==TUTORIAL_ONLY) && !tutEnable.bSensorsClose)
    {
        return;
    }

    nToFocusOn = ccFocusCullRadiusMean((FocusCommand *)&selSelecting, smFocusRadius * smFocusRadius, &centre);
    if (nToFocusOn == 0)
    {                                                       //nothing to focus on
        return;
    }
    //now we have a full listing of what we can focus on.  Let's focus on it.
    smFocus = TRUE;
    smFocusCommand = memAlloc(ccFocusSize(selSelecting.numTargets), "SensorsMultiFocus", 0);
    for (index = 0; index < selSelecting.numTargets; index++)
    {
        smFocusCommand->ShipPtr[index] = (Ship *)selSelecting.TargetPtr[index];
    }
    smFocusCommand->numShips = selSelecting.numTargets;
    dbgAssertOrIgnore(!smZoomingOut);
    smZoomingIn = TRUE;

    /* call the sound event for closing the Sensors manager */
    soundEvent(NULL, UI_SensorsExit);

    smFocusTransition = FALSE;
    smEyeEnd = smCamera.eyeposition;
    smLookEnd = smCamera.lookatpoint;
    smLookStart = centre;
    vecSub(smEyeStart, smLookEnd, smEyeEnd);
    vecNormalize(&smEyeStart);
    dbgAssertOrIgnore(smEyeStart.x < (real32)1e19 && smEyeStart.x > (real32)-1e19);
    vecMultiplyByScalar(smEyeStart, CAMERA_FOCUSFAR_DISTANCE);
    vecSub(smEyeStart, smLookStart, smEyeStart);
    smInitialDistance = smCamera.distance;//remember zoom-back distance for next time
    selRectNone();
    //now create a render list as it will be when fully zoomed in
    smTempCamera.lookatpoint = smLookStart;
    smTempCamera.eyeposition = smEyeStart;
    mrCamera = &smTempCamera;
    univUpdateRenderList();
    dbgAssertOrIgnore(smEyeStart.x < (real32)1e19 && smEyeStart.x > (real32)-1e19);

    //the player will get kicked into the sensors manager after several seconds waiting
    //with the camera nowhere near their ships.  This is a security "anti-spy" measure.
    //If when kicked into the SM the player focuses on their own ships explicitly, we
    //don't want them to focus on their mothership, rather on the ships they specified.
    smFocusOnMothershipOnClose = FALSE;
}

/*-----------------------------------------------------------------------------
    Name        : smViewportProcess
    Description : process messages for the main viewport of the sensors manager.
    Inputs      : standard region callback parameters
    Outputs     : ..
    Return      : ..
----------------------------------------------------------------------------*/
udword smViewportProcess(regionhandle region, sdword ID, udword event, udword data)
{
//    Ship *ship;
//    void *a, *b;
    if ((smZoomingIn)||(smZoomingOut))
    {
        return 0;
    }

    switch (event)
    {
        case RPE_HoldLeft:
            smHoldLeft();                                   //call current hold function
            break;
        case RPE_HoldRight:
            smHoldRight();                                  //call current hold function
            break;
        case RPE_PressLeft:
            mouseClipToRect(&smViewRectangle);
            if (smHoldRight == smNULL && (!smFleetIntel))
            {
                smClicking = TRUE;
            }
            if (smHoldRight == smNULL && (!smFleetIntel))
            {
                mrOldMouseX = mouseCursorX();               //store anchor point
                mrOldMouseY = mouseCursorY();
                smHoldLeft = smSelectHold;                  //set to select mode
                smSelectionRect.x0 = smSelectionRect.x1 =   //select nothing
                    smSelectionRect.y0 = smSelectionRect.y1 = 0;
            }
            break;
        case RPE_ReleaseLeft:
            if ((piePointSpecMode & (PSM_XY | PSM_Z)) != 0)
            {                                               //if specified a point
                vector destination;

                dbgAssertOrIgnore(!smFleetIntel);
                MakeShipsMobile((SelectCommand *)&selSelected);
                if (selSelected.numShips > 0)
                {
                    if (piePointSpecZ != 0.0f)
                    {                                       //if a height was specified
                        destination = pieHeightPoint;
                    }
                    else
                    {
                        destination = piePlanePoint;         //else move to point on plane
                    }

             /** !!!!  We should put the speech event AFTER incase the movement destination is
                        invalid...in the probes case this could be often...very often   **/
                    /**  Done :)  **/
                    /** Hooray !! **/
                    if (!(isnan((double)destination.x) || isnan((double)destination.x) || isnan((double)destination.x)))
                    {
                        if(mrNeedProbeHack())
                        {   //looks like we need a little probe hacksy poo here too
                            clWrapMove(&universe.mainCommandLayer,(SelectCommand *)&selSelected,selCentrePoint,destination);
                            speechEventFleetSpec(selSelected.ShipPtr[0], COMM_F_Probe_Deployed, 0, universe.curPlayerIndex);
                        }
                        else
                        {   //move normally
                            if(MP_HyperSpaceFlag)
                            {
                                vector distancevec;
                                real32 cost;
                                //we're in hyperspace pieplate mode!
                                vecSub(distancevec,selCentrePoint,destination);
                                cost = hyperspaceCost(sqrtf(vecMagnitudeSquared(distancevec)),(SelectCommand *)&selSelected);
                                if(universe.curPlayerPtr->resourceUnits < ((sdword)cost))
                                {
                                    //can't afford it!
                                    //good speech event?
                                    if (battleCanChatterAtThisTime(BCE_CannotComply, selSelected.ShipPtr[0]))
                                    {
                                        battleChatterAttempt(SOUND_EVENT_DEFAULT, BCE_CannotComply, selSelected.ShipPtr[0], SOUND_EVENT_DEFAULT);
                                    }
                                }
                                else
                                {
                                    //make unselectable in wrap function
                                    clWrapMpHyperspace(&universe.mainCommandLayer,(SelectCommand *)&selSelected,selCentrePoint,destination);
                                }
                            }
                            else
                            {
                                //normal pieplate movement
                                clWrapMove(&universe.mainCommandLayer,(SelectCommand *)&selSelected,selCentrePoint,destination);
                                if (selSelected.ShipPtr[0]->shiptype == Mothership)
                                {
                                    speechEventFleetSpec(selSelected.ShipPtr[0], COMM_F_MoShip_Move, 0, universe.curPlayerIndex);
                                }
                                else
                                {
                                    speechEvent(selSelected.ShipPtr[0], COMM_Move, 0);
                                }
                            }
                        }

                        //untoggle correct button!
                        if(MP_HyperSpaceFlag)
                        {
                            feToggleButtonSet(SM_Hyperspace, FALSE);
                        }
                        else
                        {
                            feToggleButtonSet(SM_Dispatch, FALSE);
                        }
                    }
                }
//                piePointSpecMode = PSM_Idle;                 //no more selecting
                piePointModeOnOff();
                MP_HyperSpaceFlag = FALSE;      //for now..
            }
            else if (smClicking && !smFleetIntel)
            {
                Ship *clickedOn = NULL;

                if (ABS(mouseCursorX() - mrOldMouseX) < selClickBoxWidth &&
                    ABS(mouseCursorY() - mrOldMouseY) < selClickBoxHeight)
                {                                           //if mouse has moved much
                    smSelectionRect.x0 = mouseCursorX() - smSelectionWidth / 2;
                    smSelectionRect.x1 = mouseCursorX() + smSelectionWidth / 2;
                    smSelectionRect.y0 = mouseCursorY() - smSelectionHeight / 2;
                    smSelectionRect.y1 = mouseCursorY() + smSelectionHeight / 2;
                    clickedOn = selSelectionClick(universe.SpaceObjList.head, &smCamera, mouseCursorX(), mouseCursorY(), FALSE, FALSE);
                    if (clickedOn != NULL)
                    {
                        if (clickedOn->playerowner != universe.curPlayerPtr && (!smGhostMode))
                        {
                            clickedOn = FALSE;
                        }
                    }
                }
                else
                {
                    mrSelectRectBuild(&smSelectionRect, mrOldMouseX, mrOldMouseY);//create a selection rect
                }
                if (smHoldRight == smNULL)
                {
                    if (smGhostMode)
                    {
                        selRectDragAnybodyAnywhere(&smCamera, &smSelectionRect);//and select anything
                    }
                    else
                    {
//                        selRectDragAnywhere(&smCamera, &smSelectionRect);//and select anything
                        selRectDragAnybodyAnywhere(&smCamera, &smSelectionRect);//and select anything
                        MakeShipsFriendlyAndAlliesShips((SelectCommand *)&selSelecting, universe.curPlayerPtr);
                        makeShipsNotBeDisabled((SelectCommand *)&selSelecting);
                    }
                    if (clickedOn != NULL)
                    {
                        if(!(clickedOn->flags & SOF_Disabled))
                        {
                            selSelectionAddSingleShip((MaxSelection *)(&selSelecting), clickedOn);
                        }
                    }
                    smCullAndFocusSelecting();
                }
            }
            mouseClipToRect(NULL);
            smHoldLeft = smNULL;
            smClicking = FALSE;
            break;
        case RPE_PressRight:
            mouseClipToRect(&smViewRectangle);

            smHoldLeft  = smNULL;
            smHoldRight = mrCameraMotion;
            mrOldMouseX = mouseCursorX();                   //save current mouse location for later restoration
            mrOldMouseY = mouseCursorY();
            mouseCursorHide();                              //hide cursor
            mouseCaptureStart();                            //capture it
            mrMouseHasMoved = 0;                            //mouse hasn't moved yet
            region->rect.x0 = region->rect.y0 = 0;          //make it's rectangle full-screen
            region->rect.x1 = MAIN_WindowWidth;
            region->rect.y1 = MAIN_WindowHeight;
            smClicking = FALSE;
            piePointModePause(TRUE);                         //pause the point spec mode
            break;
        case RPE_ReleaseRight:
            mouseClipToRect(NULL);
            if (smHoldRight == mrCameraMotion)
            {                                               //if in camera movement mode
                mousePositionSet(mrOldMouseX, mrOldMouseY); //restore mouse position
                mouseCursorShow();                          //show mouse cursor
                mouseCaptureStop();                         //release back into the wild
                smHoldRight = smNULL;                       //idle mode
            }
            //region->rect = smViewRectangle;                 //restore origional rectangle
            piePointModePause(FALSE);
            break;
        case RPE_WheelUp:
            wheel_up = TRUE;
            break;
        case RPE_WheelDown:
            wheel_down = TRUE;
            break;
        case RPE_KeyDown:
            if (smZoomingOut)
            {                                               //if sensors manager is just starting
                break;                                      //don't process key clicks
            }
            switch (ID)
            {
                case SHIFTKEY:
                    if (smHoldRight != mrCameraMotion)
                    {
                        piePointModeToggle(TRUE);
                    }
                    break;
                case MMOUSE_BUTTON:
                case FKEY:
                    if (!smFleetIntel)
                    {
                        selSelectionCopy((MaxAnySelection *)&selSelecting,(MaxAnySelection *)&selSelected);
                        smCullAndFocusSelecting();
                    }
                    break;

#if SM_TOGGLE_SENSOR_LEVEL
                case LKEY:
                    smToggleSensorsLevel();
                    break;
#endif

                case ZEROKEY:
                case ONEKEY:
                case TWOKEY:
                case THREEKEY:
                case FOURKEY:
                case FIVEKEY:
                case SIXKEY:
                case SEVENKEY:
                case EIGHTKEY:
                case NINEKEY:
                    if (!smFleetIntel)
                    {
                        if (keyIsHit(ALTKEY))
                        {                                   //alt-# select and focus on a hot key group
altCase:
                            if (selHotKeyGroup[NUMKEYNUM(ID)].numShips != 0)
                            {
                                selSelectionCopy((MaxAnySelection *)&selSelected,(MaxAnySelection *)&selHotKeyGroup[NUMKEYNUM(ID)]);
                                selSelectionCopy((MaxAnySelection *)&selSelecting,(MaxAnySelection *)&selSelected);
                                if(MP_HyperSpaceFlag)
                                {
                                    makeSelectionHyperspaceCapable((SelectCommand *)&selSelected);
                                }
                                smCullAndFocusSelecting();
#if SM_VERBOSE_LEVEL >= 2
                                dbgMessagef("Hot key group %d selected and focused upon.", NUMKEYNUM(ID));
#endif
                            }
                        }
                        else if (ID == mrLastKeyPressed && universe.totaltimeelapsed <= mrLastKeyTime + mrNumberDoublePressTime)
                        {                                   //double-#: focus on hot-key group
                            tutGameMessage("KB_GroupSelectFocus");
                            goto altCase;                   //same as alt-#
                        }
                        else
                        {                                   //plain# select a hot key group
                            if (selHotKeyGroup[NUMKEYNUM(ID)].numShips != 0)
                            {
                                tutGameMessage("KB_GroupSelect");

                                selSelectHotKeyGroup(&selHotKeyGroup[NUMKEYNUM(ID)]);
                                selHotKeyNumbersSet(NUMKEYNUM(ID));
#if SEL_ERROR_CHECKING
                                selHotKeyGroupsVerify();
#endif
                                if(MP_HyperSpaceFlag)
                                {
                                    makeSelectionHyperspaceCapable((SelectCommand *)&selSelected);
                                }

                                ioUpdateShipTotals();
                                if (selSelected.numShips > 0)
                                {
                                    soundEvent(NULL, UI_Click);
                                    speechEvent(selHotKeyGroup[NUMKEYNUM(ID)].ShipPtr[0], COMM_AssGrp_Select, NUMKEYNUM(ID));
                                }
                            }
                        }
                    }
                    break;
            }
            mrLastKeyPressed = ID;
            mrLastKeyTime = universe.totaltimeelapsed;
            break;
        case RPE_KeyUp:
            if (smZoomingOut)
            {                                               //if sensors manager is just starting
                break;                                      //don't process key clicks
            }
            switch (ID)
            {
                case SHIFTKEY:
                    if (smHoldRight != mrCameraMotion)
                    {
                        piePointModeToggle(FALSE);
                    }
                    break;
            }
            break;
#if SM_VERBOSE_LEVEL >= 1
        default:
            dbgMessagef("smViewportProcess: unimplemented or unsupported event 0x%x. ID 0x%x, data 0x%x", event, ID, data);
#endif
    }
    return(0);
}

/*-----------------------------------------------------------------------------
    Name        : various
    Description : Processor callbacks for FE buttons
    Inputs      : standard FE callback parameters
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smDispatch(char *name, featom *atom)
{
#if SM_VERBOSE_LEVEL >= 1
    dbgMessage("smDispatch");
#endif
    if (smHoldRight == smNULL)
    {                                                       //cannot bring up MM with RMB held down
        if (!smFleetIntel && ((!(tutorial==TUTORIAL_ONLY)) || tutEnable.bMove))
        {
            if(mrNeedProbeHack())
            {
                mrProbeHack();
            }
            else
            {
                mrRemoveAllProbesFromSelection();
                if(MP_HyperSpaceFlag)
                {
                    MP_HyperSpaceFlag = FALSE;
                }
                else
                {
                    if (piePointSpecMode == PSM_Idle)
                    {                                       //if bringing up the MM
                        makeShipsControllable((SelectCommand *)&selSelected,COMMAND_MP_HYPERSPACING);
                        makeShipsNotIncludeSinglePlayerMotherships((SelectCommand *)&selSelected);
                        if(selSelected.numShips > 0)
                        {                                   //if there are ships we can control
                            piePointModeOnOff();
                        }
                    }
                    else
                    {                                       //always allow the MM to be turned off
                        piePointModeOnOff();
                    }
                }
            }
        }
    }
    feToggleButtonSet(name, piePointSpecMode != PSM_Idle);
}

void smUpdateHyperspaceStatus(bool goForLaunch)
{
    mrUpdateHyperspaceStatus(goForLaunch);

    if (smHyperspaceRegion == NULL)
    {
        return;
    }

    if (goForLaunch)
    {
        bitClear(smHyperspaceRegion->status, RSF_RegionDisabled);
    }
    else
    {
        bitSet(smHyperspaceRegion->status, RSF_RegionDisabled);
    }
}

void smHyperspace(char *name, featom *atom)
{
    regionhandle region = (regionhandle)atom->region;

#if SM_VERBOSE_LEVEL >= 1
    dbgMessage("smHyperspace");
#endif

    if (FEFIRSTCALL(atom))
    {
        smHyperspaceRegion = region;

        if (singlePlayerGame)
        {
            if (singlePlayerGameInfo.playerCanHyperspace)
            {
                bitClear(region->status, RSF_RegionDisabled);
            }
            else
            {
                bitSet(region->status, RSF_RegionDisabled);
            }
        }
        else
        {
            //check research capabilities...
            //if not researched...black out button!

            if(!bitTest(tpGameCreated.flag,MG_Hyperspace))
            {
                bitSet(region->status, RSF_RegionDisabled);
            }
            //multiplayer Hyperspace initial Call!

        }
    }
    else if (FELASTCALL(atom))
    {
        smHyperspaceRegion = NULL;
        //check if movement plate for HYPERSPACE is up...
        //close it...
        if(!singlePlayerGame)
        {
            if(!bitTest(tpGameCreated.flag,MG_Hyperspace))
            {
                return;
            }
        }
        if (piePointSpecMode!=PSM_Idle
            && MP_HyperSpaceFlag)
        {
            soundEvent(NULL, UI_MovementGUIoff);
            MP_HyperSpaceFlag = FALSE;
            piePointModeOnOff();
        }
    }
    else
    {
        if(!singlePlayerGame)
        {
            if(!bitTest(tpGameCreated.flag,MG_Hyperspace))
            {
                return;
            }
        }
        if (singlePlayerGame && singlePlayerGameInfo.playerCanHyperspace)
        {
            spHyperspaceButtonPushed();
        }
        else
        {
            if (!smFleetIntel)
            {
                if(MP_HyperSpaceFlag)
                {
                    MP_HyperSpaceFlag = FALSE;
                    piePointModeOnOff();
                    soundEvent(NULL, UI_MovementGUIoff);
                }
                else
                {
                    makeShipsControllable((SelectCommand *)&selSelected,COMMAND_MP_HYPERSPACING);
                    makeSelectionHyperspaceCapable((SelectCommand *)&selSelected);
                    if(selSelected.numShips == 0)
                    {
                        //removed all ships!
                        //if normal gui up..bring it down!
                        if (piePointSpecMode != PSM_Idle)
                        {
                            //turn it off!
                            soundEvent(NULL, UI_MovementGUIoff);
                            MP_HyperSpaceFlag = FALSE;
                            piePointModeOnOff();
                        }
                    }
                    else
                    {
                        MP_HyperSpaceFlag = TRUE;
                        if (piePointSpecMode == PSM_Idle)
                        {
                            //plate isn't up...turn it on!
                            piePointModeOnOff();
                        }
                    }
                }
            }
        }
    }
}

void smTacticalOverlayToggle(char *name, featom *atom)
{
    smTacticalOverlay ^= TRUE;
#if SM_VERBOSE_LEVEL >= 1
    dbgMessagef("smTacticalOverlayToggle: %s", smTacticalOverlay ? "ON" : "OFF");
#endif
}

void smNonResourceToggle(char *name, featom *atom)
{
    smNonResources ^= TRUE;
#if SM_VERBOSE_LEVEL >= 1
    dbgMessagef("smNonResourceToggle: %s", smNonResources ? "ON" : "OFF");
#endif
}

void smResourceToggle(char *name, featom *atom)
{
    smResources ^= TRUE;
#if SM_VERBOSE_LEVEL >= 1
    dbgMessagef("smResourceToggle: %s", smResources ? "ON" : "OFF");
#endif
}

void smSensorsClose(char *name, featom *atom)
{
    vector direction,dist;
    real32 dStart;

    if ((tutorial==TUTORIAL_ONLY) && !tutEnable.bSensorsClose)
    {
        return;
    }

//    dbgAssertOrIgnore(!smFleetIntel);
    if (smZoomingIn || smZoomingOut)
    {                                                       //if hit while already zooming
        return;
    }
#if SM_VERBOSE_LEVEL >= 1
    dbgMessage("smSensorsClose");
#endif


    if(smFocusOnMothershipOnClose)
    {
        ccFocusOnMyMothership(&universe.mainCameraCommand);
        smFocusOnMothershipOnClose = FALSE;
    }

    //!!! Jason: set the value of smLookStart to the centre of the focussed ships.
    ccLockOnTargetNow(&universe.mainCameraCommand);

    smLookStart = universe.mainCameraCommand.actualcamera.lookatpoint;
    smEyeStart = universe.mainCameraCommand.actualcamera.eyeposition;

    vecSub(direction, smEyeStart, smLookStart);
    dStart = sqrtf(vecMagnitudeSquared(direction));
    smEyeEnd = smCamera.eyeposition;
    smLookEnd = smCamera.lookatpoint;
    vecSub(direction, smLookEnd, smEyeEnd);
    vecNormalize(&direction);
    vecMultiplyByScalar(direction, dStart);
    vecSub(smEyeStart, smLookStart, direction);
    dbgAssertOrIgnore(!smZoomingOut);
    smZoomingIn = TRUE;

    /* call the sound event for closing the Sensors manager */
    soundEvent(NULL, UI_SensorsExit);

    smFocusTransition = FALSE;
    smInitialDistance = smCamera.distance;
    smFocus = FALSE;

    //now create a render list as it will be when fully zoomed in
    smTempCamera.lookatpoint = smLookStart;
    smTempCamera.eyeposition = smEyeStart;
    mrCamera = &smTempCamera;
    univUpdateRenderList();
    dbgAssertOrIgnore(smEyeStart.x < (real32)1e19 && smEyeStart.x > (real32)-1e19);

//Probe Hack Start   ******************
    if(mrNeedProbeHack())
    {
        if(piePointSpecMode != PSM_Idle)
        {   //turn off pie plate movement mech.
            MP_HyperSpaceFlag = FALSE;
            piePointModeOnOff();
        }
    }
    // check to see if pieplate active if it is and out of range turn it off.
    if (piePointSpecMode!=PSM_Idle)
    {
        selCentrePointCompute();
        vecSub(dist,selCentrePoint,universe.mainCameraCommand.actualcamera.lookatpoint);
        if (vecMagnitudeSquared(dist) >= MAX_MOVE_DISTANCE)
        {
            MP_HyperSpaceFlag = FALSE;
            piePointModeOnOff();
        }
    }

//Probe Hack END     ******************

    MP_HyperSpaceFlag = FALSE;
    bitClear(tbDisable,TBDISABLE_SENSORS_USE);
}

void smSensorsSkip(char *name, featom *atom)
{
    subMessageEnded = 2;                                 //the fleet intel thing was skipped
    speechEventActorStop(ACTOR_ALL_ACTORS, smSkipFadeoutTime);
    subTitlesFadeOut(&subRegion[STR_LetterboxBar], smSkipFadeoutTime);
}

void smCancelDispatch(char *name, featom *atom)
{
#if SM_VERBOSE_LEVEL >= 1
    dbgMessage("smCancelDispatch");
#endif
    feToggleButtonSet(SM_TacticalOverlay, FALSE);
    piePointSpecMode = SPM_Idle;
}


#if SM_TOGGLE_SENSOR_LEVEL
void smToggleSensorsLevel(void)
{
    universe.curPlayerPtr->sensorLevel++;
    if (universe.curPlayerPtr->sensorLevel > 2)
    {
        universe.curPlayerPtr->sensorLevel = 0;
    }
#if SM_VERBOSE_LEVEL >= 1
    dbgMessagef("smToggleSensorsLevel: level set to %d", universe.curPlayerPtr->sensorLevel);
#endif
}
#endif //SM_TOGGLE_SENSOR_LEVEL


void smCancelMoveOrClose(char *name, featom *atom)
{
    MP_HyperSpaceFlag = FALSE;          //no matter what, set to FALSE
    if (piePointSpecMode != PSM_Idle)
    {
        piePointModeOnOff();
        soundEvent(NULL, UI_MovementGUIoff);
    }
    else
    {
        smSensorsClose(SM_Close, atom);
    }
}
/*=============================================================================
    Functions:
=============================================================================*/

/*-----------------------------------------------------------------------------
    Name        : smStartup
    Description : Start the sensors manager module.
    Inputs      : void
    Outputs     : adds some callbacks, loads some stuff etc, etc...
    Return      : void
----------------------------------------------------------------------------*/
fecallback smCallbacks[] =
{
    {smDispatch, SM_Dispatch},
    {smHyperspace, SM_Hyperspace},
    {smTacticalOverlayToggle, SM_TacticalOverlay},
    {smNonResourceToggle, SM_NonResource},
    {smResourceToggle, SM_Resource},
    {smSensorsClose, SM_Close},
    {smSensorsSkip, SM_Skip},
    {smCancelDispatch, SM_CancelDispatch},
    {smPan, SM_Pan},
    {smCancelMoveOrClose, SM_CancelMoveOrClose},
    {NULL, NULL},
};

void smStartup(void)
{
    scriptSet(NULL, "sensors.script", smTweaks);
    feDrawCallbackAdd(SM_ViewportName, smViewportRender);   //add render callback
    feCallbackAddMultiple(smCallbacks);

    cameraInit(&smCamera, SM_InitialCameraDist);
    smUpdateParameters();

    smHoldRight = smNULL;
    smHoldLeft = smNULL;
    MP_HyperSpaceFlag = FALSE;      //set to FALSE...should manage itself from here on...

//    listInit(&smBlobList);
}

/*-----------------------------------------------------------------------------
    Name        : smUpdateParameters
    Description : Update the sensors manager parameters after a level has
                    loaded some sensors parameters.
    Inputs      :
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smUpdateParameters(void)
{
    smCamera.clipPlaneNear = smZoomMin / smZoomMinFactor;
    smCamera.clipPlaneFar = smZoomMax * smZoomMaxFactor;
    smCamera.closestZoom = smZoomMin;
    smCamera.farthestZoom = smZoomMax;
    smCamera.distance = smInitialDistance;
    smCircleBorder = SM_CircleBorder;                       //ignore the value set in the level file
}

/*-----------------------------------------------------------------------------
    Name        : smShutdown
    Description : Shut down the sensors manager module.
    Inputs      : void
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smShutdown(void)
{
}

/*-----------------------------------------------------------------------------
    Name        : smSensorsBegin
    Description : Starts the sensors manager
    Inputs      : name, atom - ignored
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smSensorsBegin(char *name, featom *atom)
{
    regionhandle baseRegion, reg;
    sdword index, diff;
    fescreen *screen;
    featom *pAtom;
    sdword smViewExpandLeft;
    sdword smViewExpandRight;
    sdword smViewExpandTop;
    sdword smViewExpandBottom;
    char *screenName;

    bitClear(ghMainRegion->status, RSF_MouseInside);
    mrHoldLeft = mrHoldRight = mrNULL;                      //prevent a wierd bug

    if(smSensorsDisable || ((tutorial==TUTORIAL_ONLY) && !tutEnable.bSensorsManager))
    {
        return;
    }

    ioSaveState = ioDisable();

    tbSensorsHook();

    screenName = smFleetIntel ? SM_FleetIntelScreenName : SM_ScreenName;
    screen = feScreenFind(screenName);
    smBaseRegion = baseRegion = feScreenStart(ghMainRegion, screenName);
    //now find the user region the sensors manager is rendered in
    reg = baseRegion->child;
    smViewportRegion = NULL;
    for (index = 1; index < screen->nAtoms; index++)
    {
//        reg = (regionhandle)screen->atoms[index].region;
        regVerify(reg);
        if (((featom *)reg->userID)->pData == (ubyte *)smViewportRender) //if this is a user region
        {
            smBackgroundColor = ((featom *)reg->userID)->contentColor;
//            glClearColor(colUbyteToReal(colRed(smBackgroundColor)),
//                         colUbyteToReal(colGreen(smBackgroundColor)),
//                         colUbyteToReal(colBlue(smBackgroundColor)), 1.0f);

            smViewportRegion = reg;                         //this must be the region
            break;
        }
        reg = reg->next;
    }
    dbgAssertOrIgnore(smViewportRegion != NULL);
    regFunctionSet(smViewportRegion, (regionfunction) smViewportProcess);
    regFilterSet(smViewportRegion, SM_ViewportFilter);
    smViewRectangle = smViewportRegion->rect;

    /*
    index = 640 - (smViewRectangle.x1 - smViewRectangle.x0);
    index >>= 1;
    index++;
    smViewRectangle.x0 = index;
    smViewRectangle.x1 = MAIN_WindowWidth - index;
    */
    //now let's go through all the regions and build lists of regions outside the
    //main viewport.  These regions are to scroll onto the screen during the zoom.
    reg = baseRegion->child;
    smScrollListTop = smScrollListLeft = smScrollListRight = smScrollListBottom = NULL;
    smScrollCountLeft = smScrollCountTop = smScrollCountRight = smScrollCountBottom = 0;
    smScrollDistLeft = smScrollDistTop = smScrollDistRight = smScrollDistBottom = 0;
    smScrollLeft = smScrollTop = smScrollRight = smScrollBottom = 0;
    for (index = 1; index < screen->nAtoms; index++, reg = reg->next)
    {
//        reg = (regionhandle)screen->atoms[index].region;
        regVerify(reg);
        pAtom = (featom *)reg->userID;
        if (reg->rect.x1 <= smViewportRegion->rect.x0)
        {                                                   //if off left
            smScrollCountLeft++;
            smScrollListLeft = memRealloc(smScrollListLeft, smScrollCountLeft * sizeof(region**), "ScrollListLeft", 0);
            smScrollListLeft[smScrollCountLeft - 1] = reg;
            continue;
        }
        if (reg->rect.y1 <= smViewportRegion->rect.y0)
        {                                                   //if off top
            smScrollCountTop++;
            smScrollListTop = memRealloc(smScrollListTop, smScrollCountTop * sizeof(region**), "ScrollListTop", 0);
            smScrollListTop[smScrollCountTop - 1] = reg;
            continue;
        }
        if (reg->rect.x0 >= smViewportRegion->rect.x1)
        {                                                   //if off right
            smScrollCountRight++;
            smScrollListRight = memRealloc(smScrollListRight, smScrollCountRight * sizeof(region**), "ScrollListRight", 0);
            smScrollListRight[smScrollCountRight - 1] = reg;
            continue;
        }
        if (reg->rect.y0 >= smViewportRegion->rect.y1)
        {                                                   //if off bottom
            smScrollCountBottom++;
            smScrollListBottom = memRealloc(smScrollListBottom, smScrollCountBottom * sizeof(region**), "ScrollListBottom", 0);
            smScrollListBottom[smScrollCountBottom - 1] = reg;
            continue;
        }
    }
    //compute pan distance to get these rectangles to work in high-rez
    smViewExpandLeft    = (MAIN_WindowWidth - 640) / 2;
    smViewExpandRight   = (MAIN_WindowWidth - 640) - smViewExpandLeft;
    smViewExpandTop     = (MAIN_WindowHeight - 480) / 2;
    smViewExpandBottom  = (MAIN_WindowHeight - 480) - smViewExpandTop;
    //let's figure out how far we'll have to scroll each of these lists
    for (index = 0; index < smScrollCountLeft; index++)
    {
        smScrollDistLeft = max(smScrollDistLeft, smScrollListLeft[index]->rect.x1 - smScrollListLeft[index]->rect.x0);
    }
    for (index = 0; index < smScrollCountTop; index++)
    {
        if (smFleetIntel && index == smScrollCountTop - 1)
        {                                                   //consider the last region as a letterbox bar
            dbgAssertOrIgnore(smScrollListTop[index]->drawFunction == feStaticRectangleDraw);
            diff = smScrollListTop[index]->rect.y1 - smScrollListTop[index]->rect.y0;
            diff = (diff) * MAIN_WindowHeight / 480 - diff;
            smScrollListTop[index]->rect.y1 += diff;
            smViewportRegion->rect.y0 += diff;
            smViewRectangle.y0 += diff;
        }
        smScrollDistTop = max(smScrollDistTop, smScrollListTop[index]->rect.y1 - smScrollListTop[index]->rect.y0);
    }
    for (index = 0; index < smScrollCountRight; index++)
    {
        smScrollDistRight = max(smScrollDistRight, smScrollListRight[index]->rect.x1 - smScrollListRight[index]->rect.x0);
    }
    for (index = 0; index < smScrollCountBottom; index++)
    {
        if (smFleetIntel && index == smScrollCountBottom - 1)
        {                                                   //consider the last region as a letterbox bar
            dbgAssertOrIgnore(smScrollListBottom[index]->drawFunction == feStaticRectangleDraw);
            diff = smScrollListBottom[index]->rect.y1 - smScrollListBottom[index]->rect.y0;
            diff = (diff) * MAIN_WindowHeight / 480 - diff;
            smScrollListBottom[index]->rect.y0 -= diff;
            smViewportRegion->rect.y1 -= diff;
            smViewRectangle.y1 -= diff;
        }
        smScrollDistBottom = max(smScrollDistBottom, smScrollListBottom[index]->rect.y1 - smScrollListBottom[index]->rect.y0);
    }
    //unt finally, let's scroll them far enough to be off-screen
    for (index = 0; index < smScrollCountLeft; index++)
    {
        if (smScrollListLeft[index]->rect.y0 <= smViewRectangle.y0)
        {
            smScrollListLeft[index]->rect.y0 -= smViewExpandTop;
        }
        if (smScrollListLeft[index]->rect.y1 >= smViewRectangle.y1)
        {
            smScrollListLeft[index]->rect.y1 += smViewExpandBottom;
        }
        regRegionScroll(smScrollListLeft[index], -smScrollDistLeft - smViewExpandLeft, 0);
    }
    for (index = 0; index < smScrollCountTop; index++)
    {
        if (smScrollListTop[index]->rect.x0 <= smViewRectangle.x0)
        {
            smScrollListTop[index]->rect.x0 -= smViewExpandLeft;
        }
        if (smScrollListTop[index]->rect.x1 >= smViewRectangle.x1)
        {
            smScrollListTop[index]->rect.x1 += smViewExpandRight;
        }
        regRegionScroll(smScrollListTop[index], 0, -smScrollDistTop - smViewExpandTop);
    }
    for (index = 0; index < smScrollCountRight; index++)
    {
        if (smScrollListRight[index]->rect.y0 <= smViewRectangle.y0)
        {
            smScrollListRight[index]->rect.y0 -= smViewExpandTop;
        }
        if (smScrollListRight[index]->rect.y1 >= smViewRectangle.y1)
        {
            smScrollListRight[index]->rect.y1 += smViewExpandBottom;
        }
        regRegionScroll(smScrollListRight[index], smScrollDistRight + smViewExpandRight, 0);
    }
    for (index = 0; index < smScrollCountBottom; index++)
    {
        if (smScrollListBottom[index]->rect.x0 <= smViewRectangle.x0)
        {
            smScrollListBottom[index]->rect.x0 -= smViewExpandLeft;
        }
        if (smScrollListBottom[index]->rect.x1 >= smViewRectangle.x1)
        {
            smScrollListBottom[index]->rect.x1 += smViewExpandRight;
        }
        regRegionScroll(smScrollListBottom[index], 0, smScrollDistBottom + smViewExpandBottom);
    }
    //set the view rectangle to full screen size, to be scaled as we go zoom back
    smViewRectangle.x0 -= smScrollDistLeft + smViewExpandLeft;
    smViewRectangle.y0 -= smScrollDistTop + smViewExpandTop;
    smViewRectangle.x1 += smScrollDistRight + smViewExpandRight;
    smViewRectangle.y1 += smScrollDistBottom + smViewExpandBottom;
    smViewportRegion->rect = smViewRectangle;
    smZoomTime = 0.0f;
    if (smInstantTransition && !smFleetIntel)
    {
        smCurrentZoomLength = 0.1f;
        smCurrentMainViewZoomLength = 0.05f;
    }
    else
    {
        smCurrentZoomLength = smZoomLength;
        smCurrentMainViewZoomLength = smMainViewZoomLength;
    }

    //figure out the start/end points for the camera animation
    smLookStart = universe.mainCameraCommand.actualcamera.lookatpoint;
    smEyeStart = universe.mainCameraCommand.actualcamera.eyeposition;
    //smLookEnd.x = smLookEnd.y = smLookEnd.z = 0.0f;
    smLookEnd = universe.mainCameraCommand.actualcamera.lookatpoint;
    smCameraLookatPoint = smLookEnd;
    smCameraLookVelocity.x = smCameraLookVelocity.y = smCameraLookVelocity.z = 0.0f;
    vecSub(smEyeEnd, smEyeStart, smLookStart);
    vecNormalize(&smEyeEnd);
    vecMultiplyByScalar(smEyeEnd, smInitialDistance);
    vecAddTo(smEyeEnd, smLookEnd);
    smZoomingIn = FALSE;
    smZoomingOut = TRUE;

//    mrRenderMainScreen = FALSE;
    feToggleButtonSet(SM_TacticalOverlay, smTacticalOverlay);
    feToggleButtonSet(SM_Resource       , smResources);
    feToggleButtonSet(SM_NonResource    , smNonResources);
    feToggleButtonSet(SM_Dispatch       , piePointSpecMode != PSM_Idle);

    mouseClipToRect(NULL);
    smHoldLeft = smNULL;
    smHoldRight = smNULL;
//    bobListCreate(&smBlobProperties, &smBlobList, universe.curPlayerIndex);

    smSensorsActive = TRUE;

    soundEventStopSFX(0.5f);

    /* call the sound event for opening the Sensors manager */
    soundEvent(NULL, UI_SensorsIntro);

    universe.dontUpdateRenderList = TRUE;
    smRenderCount = 0;

    //add any additional key messages the sensors manager will need
    regKeyChildAlloc(smViewportRegion, SHIFTKEY,      RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, SHIFTKEY);
    regKeyChildAlloc(smViewportRegion, MKEY,          RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, MKEY);
    regKeyChildAlloc(smViewportRegion, FKEY,          RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, FKEY);
    regKeyChildAlloc(smViewportRegion, MMOUSE_BUTTON, RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, MMOUSE_BUTTON);

    regKeyChildAlloc(smViewportRegion, ARRLEFT,  RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, ARRLEFT );
    regKeyChildAlloc(smViewportRegion, ARRRIGHT, RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, ARRRIGHT);
    regKeyChildAlloc(smViewportRegion, ARRUP,    RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, ARRUP   );
    regKeyChildAlloc(smViewportRegion, ARRDOWN,  RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, ARRDOWN );

#if SM_TOGGLE_SENSOR_LEVEL
    regKeyChildAlloc(smViewportRegion, LKEY, RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, LKEY);
#endif


    for (index = FIRSTNUMKEY; index < FIRSTNUMKEY+10; index++)
    {
        regKeyChildAlloc(smViewportRegion, index, RPE_KeyUp | RPE_KeyDown, (regionfunction) smViewportProcess, 1, index);
    }
    mouseCursorShow();
    cameraCopyPositionInfo(&smCamera, mrCamera);
    if (smFleetIntel)
    {                                                       //if we are in the fleet intel screen, no movement mechanism!
        piePointSpecMode = SPM_Idle;
    }

    bitSet(tbDisable,TBDISABLE_SENSORS_USE);
}

/*-----------------------------------------------------------------------------
    Name        : smObjectDied
    Description : Called when an object dies, this function removes it from the
                    sensors manager blob in which it exists.
    Inputs      : object - object to delete
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void smObjectDied(void *object)
{
    sdword index;

//    bobObjectDied(object,&smBlobList);

    //remove it from the focus command, if there is one.
    if (smFocusCommand)
    {
        for (index = 0; index < smFocusCommand->numShips; index++)
        {
            if ((SpaceObj *)smFocusCommand->ShipPtr[index] == object)
            {
                for (; index < smFocusCommand->numShips - 1; index++)
                {
                    smFocusCommand->ShipPtr[index] = smFocusCommand->ShipPtr[index + 1];
                }
                smFocusCommand->numShips--;
                if (smFocusCommand->numShips <= 0)
                {
                    memFree(smFocusCommand);
                    smFocusCommand = NULL;
                    smFocus = FALSE;
                }
                return;
            }
        }
    }
}

/*=============================================================================
    Save Game Stuff:
=============================================================================*/

void smSave(void)
{
    SaveInfoNumber(Real32ToSdword(smDepthCueRadius));
    SaveInfoNumber(Real32ToSdword(smDepthCueStartRadius));
    SaveInfoNumber(Real32ToSdword(smCircleBorder));
    SaveInfoNumber(Real32ToSdword(smZoomMax));
    SaveInfoNumber(Real32ToSdword(smZoomMin));
    SaveInfoNumber(Real32ToSdword(smZoomMinFactor));
    SaveInfoNumber(Real32ToSdword(smZoomMaxFactor));
    SaveInfoNumber(Real32ToSdword(smInitialDistance));
    SaveInfoNumber(Real32ToSdword(smUniverseSizeX));
    SaveInfoNumber(Real32ToSdword(smUniverseSizeY));
    SaveInfoNumber(Real32ToSdword(smUniverseSizeZ));

    SaveInfoNumber(smSensorWeirdness);
}

void smLoad(void)
{
    sdword loadnum;

    loadnum = LoadInfoNumber(); smDepthCueRadius = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smDepthCueStartRadius = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smCircleBorder = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smZoomMax = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smZoomMin = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smZoomMinFactor = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smZoomMaxFactor = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smInitialDistance = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smUniverseSizeX = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smUniverseSizeY = SdwordToReal32(loadnum);
    loadnum = LoadInfoNumber(); smUniverseSizeZ = SdwordToReal32(loadnum);

    smSensorWeirdness = LoadInfoNumber();

    smUpdateParameters();
}

