/*
    Tutor.c - Functions for the Tutorial System
*/

#include "Tutor.h"
#include "../Missions/Generated/Tutorial1.h"  // why does this need a relative path?

#include <math.h>

#include "AIPlayer.h"
#include "CameraCommand.h"
#include "Collision.h"
#include "ConsMgr.h"
#include "FastMath.h"
#include "File.h"
#include "FontReg.h"
#include "GamePick.h"
#include "KAS.h"
#include "LevelLoad.h"
#include "main.h"
#include "mainrgn.h"
#include "Memory.h"
#include "mouse.h"
#include "PiePlate.h"
#include "Region.h"
#include "render.h"
#include "SaveGame.h"
#include "Select.h"
#include "SinglePlayer.h"
#include "SoundEvent.h"
#include "SpeechEvent.h"
#include "StringSupport.h"
#include "Subtitle.h"
#include "TaskBar.h"
#include "texreg.h"
#include "UIControls.h"
#include "utility.h"
#include "rResScaling.h"
#include "rStateCache.h"

#ifdef _MSC_VER
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif


//hack to get the speech to work when game is paused
bool FalkosFuckedUpTutorialFlag = FALSE;

// Function declarations
void utySinglePlayerGameStart(char *name, featom *atom);
char *tutGetNextTextLine(char *pDest, char *pString, sdword Width, udword pDestSize);
udword tutProcessNextButton(struct tagRegion *reg, smemsize ID, udword event, udword data);
udword tutProcessBackButton(struct tagRegion *reg, smemsize ID, udword event, udword data);
udword uicButtonProcess(regionhandle region, smemsize ID, udword event, udword data);

// flags for controlling game elements from the tutorial
tutGameEnableFlags tutEnable;
char tutBuildRestrict[TOTAL_STD_SHIPS];
char tutGameMessageList[16][256];
long tutGameMessageIndex = 0;
ShipType tutFEContextMenuShipType;

static char tutLastLessonName[256] = "";
static char tutCurrLessonName[256] = "";

/*
sdword  tutTextPointerType = 0;
sdword  tutPointerX, tutPointerY;
rectangle tutPointerRegion;
hvector tutPointerAIPoint;
*/
ShipPtr tutPointerShip;
rectanglei *tutPointerShipHealthRect;
rectanglei *tutPointerShipGroupRect;
//list of named tutorial pointers
static tutpointer tutPointer[TUT_NumberPointers];
bool tutPointersDrawnThisFrame = FALSE;


static sdword  tutTextPosX, tutTextPosY;
static sdword  tutTextSizeX, tutTextSizeY;

static rectanglei tutTextRect;
static fonthandle tutTextFont = 0;

static regionhandle    tutRootRegion;
static sdword tutRootRegionCount = 0;

// visibility flag & region handle for the displayed text
sdword  tutTextVisible = FALSE;
regionhandle tutTextRegion = NULL;
void tutDrawTextFunction(regionhandle reg);

// visibility flag & region handle for the displayed images
sdword  tutImageVisible = FALSE;
regionhandle tutImageRegion = NULL;
void tutDrawImageFunction(regionhandle reg);

// visibility flag & region handle for the "next" button
sdword tutNextVisible = FALSE;
buttonhandle tutNextRegion = NULL;
sdword  tutNextButtonState = 0;
void tutDrawNextButtonFunction(regionhandle reg);

// visibility flag & region handle for the "back" button
sdword tutBackVisible = FALSE;
sdword tutPrevVisible = FALSE;
buttonhandle tutBackRegion = NULL;
void tutDrawBackButtonFunction(regionhandle reg);

// List of image indices for the displayed images
static long szImageIndexList[16];


// Construction manager exports
extern shipavailable *cmShipsAvailable;
extern LinkedList listofShipsInProgress;

featom tutRootAtom =
{
    "TutorialWrapper",      //  char  *name;                                //optional name of control
    FAF_Function,           //  udword flags;                               //flags to control behavior
    0,                      //  udword status;                              //status flags for this atom, checked etc.
    FA_UserRegion,          //  ubyte  type;                                //type of control (button, scroll bar, etc.)
    0,                      //  ubyte  borderWidth;                         //width, in pixels, of the border
    0,                      //  uword  tabstop;                             //denotes the tab ordering of UI controls
    150,                    //  color  borderColor;                         //optional color of border
    4,                      //  color  contentColor;                        //optional color of content
    0,0,                      //  sdword x;                                   //-+
    0,0,                      //  sdword y;                                   // |>rectangle of region
    640,0,                    //  sdword width;                               // |
    480,0,                    //  sdword height;                              //-+
    NULL,                   //  ubyte *pData;                               //pointer to type-specific data
    NULL,                   //  ubyte *attribs;                             //sound(button atom) or font(static text atom) reference
    0,                      //  char   hotKeyModifiers;
    {0},                    //  char   hotKey[FE_NumberLanguages];
    {0},                    //  char   pad2[2]
    {0},                    //  udword drawstyle[2];
    0,                      //  void*  region;
    {0}                     //  udword pad[2];
};


featom tutTextAtom =
{
    "TutorialText",         //  char  *name;                                //optional name of control
    FAF_Function,           //  udword flags;                               //flags to control behavior
    0,                      //  udword status;                              //status flags for this atom, checked etc.
    FA_UserRegion,          //  ubyte  type;                                //type of control (button, scroll bar, etc.)
    0,                      //  ubyte  borderWidth;                         //width, in pixels, of the border
    0,                      //  uword  tabstop;                             //denotes the tab ordering of UI controls
    150,                    //  color  borderColor;                         //optional color of border
    4,                      //  color  contentColor;                        //optional color of content
    0,0,                      //  sdword x;                                   //-+
    0,0,                      //  sdword y;                                   // |>rectangle of region
    0,0,                      //  sdword width;                               // |
    0,0,                      //  sdword height;                              //-+
    NULL,                   //  ubyte *pData;                               //pointer to type-specific data
    NULL,                   //  ubyte *attribs;                             //sound(button atom) or font(static text atom) reference
    0,                      //  char   hotKeyModifiers;
    {0},                    //  char   hotKey[FE_NumberLanguages];
    {0},                    //  char   pad2[2]
    {0},                    //  udword drawstyle[2];
    0,                      //  void*  region;
    {0}                     //  udword pad[2];
};

featom tutImageAtom =
{
    "TutorialImage",        //  char  *name;                                //optional name of control
    FAF_Function,           //  udword flags;                               //flags to control behavior
    0,                      //  udword status;                              //status flags for this atom, checked etc.
    FA_UserRegion,          //  ubyte  type;                                //type of control (button, scroll bar, etc.)
    0,                      //  ubyte  borderWidth;                         //width, in pixels, of the border
    0,                      //  uword  tabstop;                             //denotes the tab ordering of UI controls
    150,                    //  color  borderColor;                         //optional color of border
    4,                      //  color  contentColor;                        //optional color of content
    0,0,                      //  sdword x;                                   //-+
    0,0,                      //  sdword y;                                   // |>rectangle of region
    0,0,                      //  sdword width;                               // |
    0,0,                      //  sdword height;                              //-+
    NULL,                   //  ubyte *pData;                               //pointer to type-specific data
    NULL,                   //  ubyte *attribs;                             //sound(button atom) or font(static text atom) reference
    0,                      //  char   hotKeyModifiers;
    {0},                    //  char   hotKey[FE_NumberLanguages];
    {0},                    //  char   pad2[2]
    {0},                    //  udword drawstyle[2];
    0,                      //  void*  region;
    {0}                     //  udword pad[2];
};

featom tutNextAtom =
{
    "TutorialNext",         //  char  *name;                                //optional name of control
    FAF_Function,           //  udword flags;                               //flags to control behavior
    0,                      //  udword status;                              //status flags for this atom, checked etc.
    FA_Button,              //  ubyte  type;                                //type of control (button, scroll bar, etc.)
    0,                      //  ubyte  borderWidth;                         //width, in pixels, of the border
    0,                      //  uword  tabstop;                             //denotes the tab ordering of UI controls
    150,                    //  color  borderColor;                         //optional color of border
    4,                      //  color  contentColor;                        //optional color of content
    0,0,                      //  sdword x;                                   //-+
    0,0,                      //  sdword y;                                   // |>rectangle of region
    0,0,                      //  sdword width;                               // |
    0,0,                      //  sdword height;                              //-+
    NULL,                   //  ubyte *pData;                               //pointer to type-specific data
    NULL,                   //  ubyte *attribs;                             //sound(button atom) or font(static text atom) reference
    0,                      //  char   hotKeyModifiers;
    {0},                    //  char   hotKey[FE_NumberLanguages];
    {0},                    //  char   pad2[2]
    {0},                    //  udword drawstyle[2];
    0,                      //  void*  region;
    {0}                     //  udword pad[2];
};

featom tutBackAtom =
{
    "TutorialBack",         //  char  *name;                                //optional name of control
    FAF_Function,           //  udword flags;                               //flags to control behavior
    0,                      //  udword status;                              //status flags for this atom, checked etc.
    FA_Button,              //  ubyte  type;                                //type of control (button, scroll bar, etc.)
    0,                      //  ubyte  borderWidth;                         //width, in pixels, of the border
    0,                      //  uword  tabstop;                             //denotes the tab ordering of UI controls
    150,                    //  color  borderColor;                         //optional color of border
    4,                      //  color  contentColor;                        //optional color of content
    0,0,                      //  sdword x;                                   //-+
    0,0,                      //  sdword y;                                   // |>rectangle of region
    0,0,                      //  sdword width;                               // |
    0,0,                      //  sdword height;                              //-+
    NULL,                   //  ubyte *pData;                               //pointer to type-specific data
    NULL,                   //  ubyte *attribs;                             //sound(button atom) or font(static text atom) reference
    0,                      //  char   hotKeyModifiers;
    {0},                    //  char   hotKey[FE_NumberLanguages];
    {0},                    //  char   pad2[2]
    {0},                    //  udword drawstyle[2];
    0,                      //  void*  region;
    {0}                     //  udword pad[2];
};


#define TUT_NUM_IMAGES 33

#define TUT_NEXT_ON     0
#define TUT_NEXT_OFF    1
#define TUT_NEXT_MOUSE  2
#define TUT_PREV_ON     3
#define TUT_PREV_OFF    4
#define TUT_PREV_MOUSE  5
#define TUT_REST_ON     6
#define TUT_REST_OFF    7
#define TUT_REST_MOUSE  8

char *tutImageList[TUT_NUM_IMAGES] =
{
    "Tut_Next_On",
    "Tut_Next_Off",
    "Tut_Next_Mouse",
    "Tut_previous_on",
    "Tut_previous_off",
    "Tut_previous_mouse",
    "Tut_restart_on",
    "Tut_restart_off",
    "Tut_restart_mouse",
    "Tut_Rightclick",
    "Tut_Key_F",
    "Tut_Key_M",
    "Tut_Key_Shift",
    "Tut_Key_Ctrl",
    "Tut_Key_Alt",
    "Tut_Key_Space",
    "Tut_Key_Tab",
    "Tut_Mouse_ButtonR",
    "Tut_Mouse_ButtonR_MoveLRUD",
    "Tut_Mouse_ButtonR_MoveLR",
    "Tut_Mouse_ButtonR_MoveU",
    "Tut_Mouse_ButtonR_MoveD",
    "Tut_Mouse_ButtonLR_MoveUD",
    "Tut_Mouse_ButtonLR_MoveU",
    "Tut_Mouse_ButtonLR_MoveD",
    "Tut_Mouse_ButtonL",
    "Tut_Mouse_ButtonL_MoveLRUD",
    "Tut_Mouse_ButtonM",
    "Tut_Mouse_Middle_RollD",
    "Tut_Mouse_Middle_RollU",
    "Tut_Mouse_Middle_RollUD",
    "Tut_Mouse_Lbandbox",
    ""
};

char *tutFrenchImageList[TUT_NUM_IMAGES] =
{
    "Tut_fr_Next_On",
    "Tut_fr_Next_Off",
    "Tut_fr_Next_Mouse",
    "Tut_fr_previous_on",
    "Tut_fr_previous_off",
    "Tut_fr_previous_mouse",
    "Tut_fr_restart_on",
    "Tut_fr_restart_off",
    "Tut_fr_restart_mouse",
    "Tut_Rightclick",
    "Tut_Key_F",
    "Tut_Key_D",
    "Tut_fr_Key_Shift",
    "Tut_fr_Key_Ctrl",
    "Tut_Key_Alt",
    "Tut_fr_Key_Space",
    "Tut_Key_Tab",
    "Tut_Mouse_ButtonR",
    "Tut_Mouse_ButtonR_MoveLRUD",
    "Tut_Mouse_ButtonR_MoveLR",
    "Tut_Mouse_ButtonR_MoveU",
    "Tut_Mouse_ButtonR_MoveD",
    "Tut_Mouse_ButtonLR_MoveUD",
    "Tut_Mouse_ButtonLR_MoveU",
    "Tut_Mouse_ButtonLR_MoveD",
    "Tut_Mouse_ButtonL",
    "Tut_Mouse_ButtonL_MoveLRUD",
    "Tut_Mouse_ButtonM",
    "Tut_Mouse_Middle_RollD",
    "Tut_Mouse_Middle_RollU",
    "Tut_Mouse_Middle_RollUD",
    "Tut_Mouse_Lbandbox",
    ""
};

char *tutGermanImageList[TUT_NUM_IMAGES] =
{
    "Tut_ger_Next_On",
    "Tut_ger_Next_Off",
    "Tut_ger_Next_Mouse",
    "Tut_ger_previous_on",
    "Tut_ger_previous_off",
    "Tut_ger_previous_mouse",
    "Tut_ger_restart_on",
    "Tut_ger_restart_off",
    "Tut_ger_restart_mouse",
    "Tut_Rightclick",
    "Tut_Key_F",
    "Tut_Key_W",
    "Tut_ger_Key_Shift",
    "Tut_ger_Key_Ctrl",
    "Tut_Key_Alt",
    "Tut_ger_Key_Space",
    "Tut_Key_Tab",
    "Tut_Mouse_ButtonR",
    "Tut_Mouse_ButtonR_MoveLRUD",
    "Tut_Mouse_ButtonR_MoveLR",
    "Tut_Mouse_ButtonR_MoveU",
    "Tut_Mouse_ButtonR_MoveD",
    "Tut_Mouse_ButtonLR_MoveUD",
    "Tut_Mouse_ButtonLR_MoveU",
    "Tut_Mouse_ButtonLR_MoveD",
    "Tut_Mouse_ButtonL",
    "Tut_Mouse_ButtonL_MoveLRUD",
    "Tut_Mouse_ButtonM",
    "Tut_Mouse_Middle_RollD",
    "Tut_Mouse_Middle_RollU",
    "Tut_Mouse_Middle_RollUD",
    "Tut_Mouse_Lbandbox",
    ""
};

char *tutSpanishImageList[TUT_NUM_IMAGES] =
{
    "Tut_sp_Next_On",
    "Tut_sp_Next_Off",
    "Tut_sp_Next_Mouse",
    "Tut_sp_previous_on",
    "Tut_sp_previous_off",
    "Tut_sp_previous_mouse",
    "Tut_sp_restart_on",
    "Tut_sp_restart_off",
    "Tut_sp_restart_mouse",
    "Tut_Rightclick",
    "Tut_Key_F",
    "Tut_Key_D",
    "Tut_sp_Key_Shift",
    "Tut_Key_Ctrl",
    "Tut_Key_Alt",
    "Tut_sp_Key_Space",
    "Tut_Key_Tab",
    "Tut_Mouse_ButtonR",
    "Tut_Mouse_ButtonR_MoveLRUD",
    "Tut_Mouse_ButtonR_MoveLR",
    "Tut_Mouse_ButtonR_MoveU",
    "Tut_Mouse_ButtonR_MoveD",
    "Tut_Mouse_ButtonLR_MoveUD",
    "Tut_Mouse_ButtonLR_MoveU",
    "Tut_Mouse_ButtonLR_MoveD",
    "Tut_Mouse_ButtonL",
    "Tut_Mouse_ButtonL_MoveLRUD",
    "Tut_Mouse_ButtonM",
    "Tut_Mouse_Middle_RollD",
    "Tut_Mouse_Middle_RollU",
    "Tut_Mouse_Middle_RollUD",
    "Tut_Mouse_Lbandbox",
    ""
};

char *tutItalianImageList[TUT_NUM_IMAGES] =
{
    "Tut_it_Next_On",
    "Tut_it_Next_Off",
    "Tut_it_Next_Mouse",
    "Tut_it_previous_on",
    "Tut_it_previous_off",
    "Tut_it_previous_mouse",
    "Tut_it_restart_on",
    "Tut_it_restart_off",
    "Tut_it_restart_mouse",
    "Tut_Rightclick",
    "Tut_Key_F",
    "Tut_Key_M",
    "Tut_Key_Shift",
    "Tut_Key_Ctrl",
    "Tut_Key_Alt",
    "Tut_it_Key_Space",
    "Tut_Key_Tab",
    "Tut_Mouse_ButtonR",
    "Tut_Mouse_ButtonR_MoveLRUD",
    "Tut_Mouse_ButtonR_MoveLR",
    "Tut_Mouse_ButtonR_MoveU",
    "Tut_Mouse_ButtonR_MoveD",
    "Tut_Mouse_ButtonLR_MoveUD",
    "Tut_Mouse_ButtonLR_MoveU",
    "Tut_Mouse_ButtonLR_MoveD",
    "Tut_Mouse_ButtonL",
    "Tut_Mouse_ButtonL_MoveLRUD",
    "Tut_Mouse_ButtonM",
    "Tut_Mouse_Middle_RollD",
    "Tut_Mouse_Middle_RollU",
    "Tut_Mouse_Middle_RollUD",
    "Tut_Mouse_Lbandbox",
    ""
};

// This string structure must match the tutGameEnableFlags struct
char *tutGameEnableString[] =
{
    "KASFrame",     // FOR INTERAL USE ONLY
    "GameRunning",


    "BuildManager",
    "SensorsManager",
    "ResearchManager",
    "PauseGame",
    "Dock",
    "Formation",
    "Launch",
    "Move",
    "MoveIssue",
    "Attack",
    "Harvest",
    "CancelCommand",
    "Scuttle",
    "Retire",
    "ClickSelect",
    "BandSelect",
    "CancelSelect",
    "Special",
    "BuildBuildShips",
    "BuildPauseJobs",
    "BuildCancelJobs",
    "BuildClose",
    "BuildArrows",
    "SensorsClose",
    "ContextMenus",
    "ContextFormDelta",
    "ContextFormBroad",
    "ContextFormX",
    "ContextFormClaw",
    "ContextFormWall",
    "ContextFormSphere",
    "ContextFormCustom",
    "Evasive",
    "Neutral",
    "Agressive",
    "TaskbarOpen",
    "TaskbarClose",
    "ResearchSelectTech",
    "ResearchSelectLab",
    "ResearchResearch",
    "ResearchClearLab",
    "ResearchClose",
    "Focus",
    "FocusCancel",
    "MothershipFocus",
    "LaunchSelectShips",
    "LaunchLaunch",
    "LaunchLaunchAll",
    "LaunchClose",
    ""
};

color       tutTexture[TUT_NUM_IMAGES];
lifheader   *tutImage[TUT_NUM_IMAGES];


static char *tutTutorialNames[4] = {"", "Tutorial1"};

void tutPreInitTutorial(char *dirfile, char *levelfile)
{
    sprintf(dirfile, "Tutorials/%s/", tutTutorialNames[tutorial]);
    sprintf(levelfile, "%s.mission", tutTutorialNames[tutorial]);

    tutEnableEverything();

    //perform a level pass to see what ships we need to load
    levelPreInit(dirfile, levelfile);
}


void tutInitTutorial(char *dirfile, char *levelfile)
{
    levelInit(dirfile, levelfile);
    tutLastLessonName[0] = '\0';
    tutCurrLessonName[0] = '\0';

    switch(tutorial)
    {
    case 1:
        kasMissionStart("tutorial1", Init_Tutorial1, Watch_Tutorial1);
        break;
    }
}

void tutSaveLesson(sdword Num, char *pName)
{
    collUpdateCollBlobs();
    collUpdateObjsInCollBlobs();

    strcpy(tutLastLessonName, tutCurrLessonName);

    sprintf(tutCurrLessonName, "%s%s", TutorialSavedGamesPath, pName);
    SaveGame(tutCurrLessonName);
}

static void SaveLong(sdword Val)
{
    SaveStructureOfSize( &Val, 4 );
}

static sdword LoadLong(void)
{
sdword Val;

    LoadStructureOfSizeToAddress( &Val, 4 );
    return Val;
}

void tutSaveTutorialGame(void)
{
    Save_String(tutCurrLessonName);
    Save_String(tutLastLessonName);

    SaveStructureOfSize( &tutEnable, sizeof(tutGameEnableFlags) );
    SaveStructureOfSize( tutBuildRestrict, sizeof(tutBuildRestrict) );

    SaveLong(tutTextPosX);
    SaveLong(tutTextPosY);
    SaveLong(tutTextSizeX);
    SaveLong(tutTextSizeY);

    SaveStructureOfSize( &tutTextRect, sizeof(rectanglei) );

    SaveLong(tutTextVisible);
    SaveLong(tutNextVisible);
    SaveLong(tutBackVisible);
    SaveLong(FalkosFuckedUpTutorialFlag);
}

void tutLoadTutorialGame(void)
{
    sdword index;

    Load_StringToAddress(tutCurrLessonName);
    Load_StringToAddress(tutLastLessonName);

    LoadStructureOfSizeToAddress( &tutEnable, sizeof(tutGameEnableFlags) );
    LoadStructureOfSizeToAddress( tutBuildRestrict, sizeof(tutBuildRestrict) );

    tutTextPosX = LoadLong();
    tutTextPosY = LoadLong();
    tutTextSizeX = LoadLong();
    tutTextSizeY = LoadLong();

    LoadStructureOfSizeToAddress( &tutTextRect, sizeof(rectanglei) );

    tutTextVisible = LoadLong();
    tutNextVisible = LoadLong();
    tutBackVisible = LoadLong();
    FalkosFuckedUpTutorialFlag = LoadLong();

    tutResetGameMessageQueue();

    tutNextButtonState = 0;
    //tutTextPointerType = 0;

    //clear out all the pointers
    for (index = 0; index < TUT_NumberPointers; index++)
    {
        tutPointer[index].pointerType = TUT_PointerTypeNone;
        tutPointerShip = NULL;
        tutPointerShipGroupRect = tutPointerShipHealthRect = NULL;
    }
    tutFEContextMenuShipType = 0;
}

void tutTutorial1(char *name, featom *atom)
{
    static bool beginning;

    if (FEFIRSTCALL(atom))
    {
        beginning = FALSE;
    }
    else if (FELASTCALL(atom))
    {
        if (!beginning)
        {
            tutorial = 0;
        }
    }
    else
    {
        beginning = TRUE;
        tutorial = 1;
        dbgAssertOrIgnore(startingGame == FALSE);

#ifdef HW_BUILD_FOR_DEBUGGING
        dbgMessage("Tutorial1 started");
#endif

        utySinglePlayerGameStart(name, atom);
    }
}

/*-----------------------------------------------------------------------------
    Name        : tutPointerAllocate
    Description : Allocate a tutorial pointer from the pointer list.
    Inputs      : name - name of pointer.  Will overwrite pointers of same name.
                  type - type of pointer to allocate.
    Outputs     :
    Return      : pointer to pointer.  Quaint isn't it?
----------------------------------------------------------------------------*/
tutpointer *tutPointerAllocate(char *name, sdword type)
{
    sdword index, freeIndex = -1;

    for (index = 0; index < TUT_NumberPointers; index++)
    {                                                       //make sure this name is unique
        if (!strcmp(name, tutPointer[index].name))
        {                                                   //if this one is the same name
            if (tutPointer[index].ship == tutPointerShip)
            {
                tutPointerShip = NULL;
                tutPointerShipGroupRect = tutPointerShipHealthRect = NULL;
            }
            freeIndex = index;
            break;
        }
        if (tutPointer[index].pointerType == TUT_PointerTypeNone)
        {                                                   //if this one is free
            freeIndex = index;
        }
    }
    
#ifdef HW_BUILD_FOR_DEBUGGING
    if (freeIndex < 0)
    {
        dbgFatalf(DBG_Loc, "Cannot allocate tutorial pointer '%s'.", name);
    }
#endif

    memStrncpy(tutPointer[freeIndex].name, name, TUT_PointerNameMax);
    tutPointer[freeIndex].pointerType = type;
    return(&tutPointer[freeIndex]);
}

void tutSetPointerTargetXY(char *name, sdword x, sdword y)
{
    tutpointer *pointer = tutPointerAllocate(name, TUT_PointerTypeXY);
    pointer->x = x;
    pointer->y = y;
}

//sets the tutorial pointer relative to the right side of the screen
void tutSetPointerTargetXYRight(char *name, sdword x, sdword y)
{
    tutSetPointerTargetXY(name, MAIN_WindowWidth+x-640, y);
}

//sets the tutorial pointer relative to the bottom right corner of the screen
void tutSetPointerTargetXYBottomRight(char *name, sdword x, sdword y)
{
    tutSetPointerTargetXY(name, MAIN_WindowWidth+x-640, MAIN_WindowHeight+y-480);
}

void tutSetPointerTargetXYTaskbar(char *name, sdword x, sdword y)
{
    tutSetPointerTargetXY(name, feResRepositionCentredX(x), MAIN_WindowHeight+y-480);
}

void tutSetPointerTargetXYFE(char *name, sdword x, sdword y)
{
    tutSetPointerTargetXY(name, feResRepositionCentredX(x), feResRepositionCentredY(y));
}

void tutSetPointerTargetShip(char *name, ShipPtr ship)
{
    tutpointer *pointer = tutPointerAllocate(name, TUT_PointerTypeShip);
    pointer->ship = ship;
}

void tutSetPointerTargetShipSelection(char *name, SelectCommand *ships)
{
    tutpointer *pointer = tutPointerAllocate(name, TUT_PointerTypeShips);
    pointer->selection = ships;
}

void tutSetPointerTargetShipHealth(char *name, ShipPtr ship)
{
    tutpointer *pointer = tutPointerAllocate(name, TUT_PointerTypeShipHealth);
    pointer->ship = ship;
    tutPointerShip = ship;
    tutPointerShipHealthRect = &pointer->rect;
}

void tutSetPointerTargetShipGroup(char *name, ShipPtr ship)
{
    tutpointer *pointer = tutPointerAllocate(name, TUT_PointerTypeShipGroup);
    pointer->ship = ship;
    tutPointerShip = ship;
    tutPointerShipGroupRect = &pointer->rect;
}

void tutPlayerShipDied(ShipPtr ship)
{
    sdword index;

    for (index = 0; index < TUT_NumberPointers; index++)
    {
        if (tutPointer[index].ship == ship)
        {
            tutPointer[index].pointerType = TUT_PointerTypeNone;
            tutPointer[index].ship = NULL;
        }
    }
    if(ship == tutPointerShip)
    {
        //tutTextPointerType = TUT_PointerTypeNone;
        tutPointerShip = NULL;
        tutPointerShipGroupRect = tutPointerShipHealthRect = NULL;
    }
}

void tutSetPointerTargetFERegion(char *name, char *pAtomName)
{
    regionhandle    reg;

    reg = regFindChildByAtomName(&regRootRegion, pAtomName);

    if(reg)
    {
        tutpointer *pointer = tutPointerAllocate(name, TUT_PointerTypeRegion);
        pointer->rect = reg->rect;
        pointer->rect.x0 -= 1;
        pointer->rect.y0 -= 1;
        pointer->rect.y1 += 2;
    }
}

void tutSetPointerTargetRect(char *name, sdword x0, sdword y0, sdword x1, sdword y1)
{
    tutpointer *pointer = tutPointerAllocate(name, TUT_PointerTypeRegion);
    pointer->rect.x0 = x0;
    pointer->rect.y0 = y0;
    pointer->rect.x1 = x1;
    pointer->rect.y1 = y1;
}

void tutSetPointerTargetAIVolume(char *name, Volume *volume)
{
    if (volume != NULL)
    {
        tutpointer *pointer = tutPointerAllocate(name, TUT_PointerTypeAIVolume);
        pointer->volume = volume;
    }
}

/*-----------------------------------------------------------------------------
    Name        : tutRemovePointerByName
    Description : Turn off a named pointer.
    Inputs      : name - name of pointer to remove
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void tutRemovePointerByName(char *name)
{
    sdword index;

    for (index = 0; index < TUT_NumberPointers; index++)
    {
        if (!strcmp(tutPointer[index].name, name))
        {
            tutPointer[index].pointerType = TUT_PointerTypeNone;
            if (tutPointer[index].ship == tutPointerShip)
            {
                tutPointerShip = NULL;
                tutPointerShipGroupRect = tutPointerShipHealthRect = NULL;
            }
            return;
        }
    }

#ifdef HW_BUILD_FOR_DEBUGGING
    dbgMessagef("tutRemovePointerByName: '%s' not found.", name);
#endif
}

/*-----------------------------------------------------------------------------
    Name        : tutRemoveAllPointers
    Description : Remove all pointers.  Gone.
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void tutRemoveAllPointers(void)
{
    sdword index;

    for (index = 0; index < TUT_NumberPointers; index++)
    {
        if (tutPointer[index].pointerType != TUT_PointerTypeNone)
        {
            tutPointer[index].pointerType = TUT_PointerTypeNone;
            if (tutPointer[index].ship == tutPointerShip)
            {
                tutPointerShip = NULL;
                tutPointerShipGroupRect = tutPointerShipHealthRect = NULL;
            }
        }
    }
}

// This function sets the current text display position and size
void tutSetTextDisplayBox(sdword x, sdword y, sdword width, sdword height, bool bScale)
{
    if(bScale)
    {
        x = (x / 640) * MAIN_WindowWidth;
        y = (y / 480) * MAIN_WindowHeight;
    }
    else
    {
        x += ((MAIN_WindowWidth  - 640) / 2);
        y += ((MAIN_WindowHeight - 480) / 2);
    }

    tutTextPosX  = x+5;
    tutTextPosY  = y+5;
    tutTextSizeX = width-10;
    tutTextSizeY = height-10;

    tutTextRect.x0 = x;
    tutTextRect.y0 = y;
    tutTextRect.x1 = (x + width);
    tutTextRect.y1 = (y + height);
}

/*-----------------------------------------------------------------------------
    Name        : tutSetTextDisplayBoxToSubtitleRegion
    Description : Causes pointers to be clipped to the subtitle region
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void tutSetTextDisplayBoxToSubtitleRegion(void)
{
    tutTextRect = subRegion[STR_LetterboxBar].rect;
}

void tutRootDrawFunction(regionhandle reg)
{
    sdword index;
    regionhandle escapeRegion = NULL;

    if (reg->previous)
    {
        for (index = 0; index <= feStackIndex; index++)
        {
            if (feStack[index].screen != NULL)
            {
                if (strcmp(feStack[index].screen->name, "Construction_manager") &&
                    strcmp(feStack[index].screen->name, "Launch_Manager") &&
                    strcmp(feStack[index].screen->name, "Research_Manager") &&
                    strcmp(feStack[index].screen->name, "Sensors_manager"))
                {                                           //if it's not one of the screens the turorial teaches
                    escapeRegion = feStack[index].baseRegion;
                    break;
                }
            }
        }
        //we want the tutorial stuff to be on top of all menu screens EXCEPT the in-game escape menu and it's minions
        if (escapeRegion == NULL)
        {                                                   //don't put it on top of the build manager or any other base-level manager.
            regSiblingMoveToFront(reg);
        }
    }
}

void tutAllocateRootRegion(void)
{
    if(tutRootRegionCount == 0)
    {
        tutRootRegion = regChildAlloc(ghMainRegion, (smemsize)&tutRootAtom,
            tutRootAtom.x, tutRootAtom.y, tutRootAtom.width, tutRootAtom.height, 0, RPE_DrawEveryFrame);

        tutRootAtom.region = (void*)tutRootRegion;
        tutRootRegion->atom = &tutRootAtom;
        tutRootAtom.pData = NULL;

        regDrawFunctionSet(tutRootRegion, tutRootDrawFunction);
    }

    tutRootRegionCount++;
}

void tutDeallocateRootRegion(void)
{
    tutRootRegionCount--;

    if(tutRootRegionCount == 0)
    {
        if(tutRootRegion)
            regRegionDelete(tutRootRegion);

        tutRootRegion = NULL;
    }
}

// This function will accept a string to be displayed in the current text box.
// The current text box is set up with the tutSetTextDisplayBox() function.
void tutShowText(char *szText)
{
fonthandle  currFont;
sdword      Height;
char        Line[256], *pString;

    if(tutTextVisible)
        tutHideText();

    if (MAIN_WindowWidth >= 1024)
    {
        tutTextFont = frFontRegister("ScrollingText.hff");
    }
    else
    {
        tutTextFont = frFontRegister("SimplixSSK_13.hff");
    }

    Height = 0;
    pString = szText;
    currFont = fontMakeCurrent(tutTextFont);

    do {
        pString = tutGetNextTextLine(Line, pString, tutTextSizeX, 256); // 256 defined above as size of Line
        Height += fontHeight(" ");
    } while(pString && pString[0]);

    fontMakeCurrent(currFont);

    tutAllocateRootRegion();

    tutTextAtom.x = tutTextPosX-5;
    tutTextAtom.y = tutTextPosY-5;
    tutTextAtom.width = tutTextSizeX + 10;
    tutTextAtom.height = max(Height, tutTextSizeY)+10;

    tutTextRect.x0 = tutTextPosX-5;
    tutTextRect.y0 = tutTextPosY-5;
    tutTextRect.x1 = tutTextRect.x0 + tutTextSizeX + 10;
    tutTextRect.y1 = tutTextRect.y0 + max(Height, tutTextSizeY) + 10;

    tutTextRegion = regChildAlloc(tutRootRegion, (smemsize)&tutTextAtom,
        tutTextAtom.x, tutTextAtom.y, tutTextAtom.width, tutTextAtom.height, 0, 0);

    tutTextAtom.region = (void*)tutTextRegion;
    tutTextRegion->atom = &tutTextAtom;
    tutTextAtom.pData = (ubyte *)szText;

    tutTextVisible = TRUE;

    regDrawFunctionSet(tutTextRegion, tutDrawTextFunction);
}

void tutHideText(void)
{
//    sdword index;

    if(tutTextRegion)
    {
        regRegionDelete(tutTextRegion);
        tutDeallocateRootRegion();
    }

    tutTextRegion = NULL;
    tutTextVisible = FALSE;

    tutRemoveAllPointers();

    //kills the speech associated with the text
    speechEventActorStop(SPEECH_ACTOR_FLEET_COMMAND, 0.1f);
}


/*-----------------------------------------------------------------------------
    Name        : tutIsspace
    Description : Returns TRUE if the character is a "space" character
    Inputs      : c - the character to test
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
bool tutIsspace(char c)
{
    if (c==' '||c=='\n')
    {
        return TRUE;
    }
    return FALSE;
}

// This function gets a line of text that is up to Width pixels wide, and returns a
// pointer to the start of the next line.  Assumes the current font is set.
char *tutGetNextTextLine(char *pDest, char *pString, sdword Width, udword pDestSize)
{
    sdword WordLen, StringLen;
    bool done = FALSE;
    char *pstr;
    char temp;

    StringLen = 0;
    WordLen   = 0;
    *pDest    = 0;

    if (pString[0] == 0)
        return NULL;

    if (strlen(pString) < pDestSize) {
	    memStrncpy(pDest, pString, strlen(pString) +1);
    }
    else {
	    memStrncpy(pDest, pString, pDestSize -1);
	    memStrncpy(pDest + pDestSize -1, '\0', 1);
    }

    do {
        // Skip leading whitespace
        pstr = &pDest[StringLen];
        while ( *pstr && *pstr != '\n' && (*pstr == '-' || tutIsspace(*pstr)) )
        {
            WordLen++;
            pstr++;
        }

        if (*pstr && *pstr != '\n')
        {
            while( *pstr && *pstr != '\n' && *pstr != '-' && !tutIsspace(*pstr) )
            {
                WordLen++;
                pstr++;
            }

            temp = *pstr;
            *pstr = 0;
            if(fontWidth(pDest) > Width)
                done = TRUE;
            else
            {
                StringLen += WordLen;
                WordLen = 0;
            }
            *pstr = temp;
        }
        else
        {
            done = TRUE;
            StringLen += WordLen;
        }
    } while (!done);

    if (StringLen)
    {
        // memStrncpy(pDest, pString, StringLen+1);
        pDest[StringLen] = 0;

        while( pString[StringLen] && tutIsspace(pString[StringLen]) )
            StringLen++;
    }
    else
        return NULL;

    return pString + StringLen;
}

static long PulseVal(long Val)
{
    static long pulseDir = TUT_PointerPulseInc;

    Val += pulseDir;
    if(Val >= 255 || Val <= TUT_PointerPulseMin)
    {
        pulseDir = -pulseDir;
        Val += (long)(0.5 + pulseDir / getResFrequencyRelative());
    }
    return Val;
}

// long vector type for the clipper
typedef struct lvector {
    real32 x,y;
} lvector;


// Clips segment to a horizontal line at y.
// Puts the intersection point in the return vector at *pDest
// returns true if clipped, false otherwise
sdword tutClipSegToHorizontal(lvector *pSeg, lvector *pDest, real32 y)
{
    real32 dy0 = pSeg[1].y - pSeg[0].y;
    real32 dy1 = y - pSeg[0].y;
    real32 t   = dy1 / dy0;

    if (t > 0.0f && t < 1.0f) {
        pDest->y = y;
        pDest->x = pSeg[0].x + ((pSeg[1].x - pSeg[0].x) * t + 0.5f);
        return TRUE;
    } else {
        return FALSE;
    }
}

// Clips segment to a vertical line at x.
// Puts the intersection point in the return vector at *pDest
// returns true if clipped, false otherwise
sdword tutClipSegToVertical(lvector *pSeg, lvector *pDest, real32 x)
{
    real32 dx0 = pSeg[1].x - pSeg[0].x;
    real32 dx1 = x - pSeg[0].x;
    real32 t   = dx1 / dx0;

    if (t > 0.0f && t < 1.0f)
    {
        pDest->x = x;
        pDest->y = pSeg[0].y + ((pSeg[1].y - pSeg[0].y) * t + 0.5f);
        return TRUE;
    } else {
        return FALSE;
    }
}


void tutClipSegToTextBox(real32 *x0, real32 *y0, real32 *x1, real32 *y1)
{
    lvector    clipped;
    lvector    seg[2] = { {*x0, *y0}, {*x1,*y1} };
    rectanglef rect   = rectToFloatRect( &tutTextRect );

    if (tutClipSegToHorizontal(seg, &clipped, rect.y0))
    if (clipped.x >= rect.x0 && clipped.x <= rect.x1)
        seg[0] = clipped;

    if (tutClipSegToHorizontal(seg, &clipped, rect.y1))
    if (clipped.x >= rect.x0 && clipped.x <= rect.x1)
        seg[0] = clipped;

    if (tutClipSegToVertical(seg, &clipped, rect.x0))
    if (clipped.y >= rect.y0 && clipped.y <= rect.y1)
        seg[0] = clipped;

    if (tutClipSegToVertical(seg, &clipped, rect.x1))
    if (clipped.y >= rect.y0 && clipped.y <= rect.y1)
        seg[0] = clipped;

    *x0 = seg[0].x; *y0 = seg[0].y;
    *x1 = seg[1].x; *y1 = seg[1].y;
}



static void tutGetScreenSpaceInfoForLineToCircle( tutpointer* tp, real32* sx, real32* sy, real32* sradius ) {
    switch (tp->pointerType) {
        case TUT_PointerTypeShip: {
            *sradius = tp->ship->collInfo.selCircleRadius;
            *sx      = tp->ship->collInfo.selCircleX;
            *sy      = tp->ship->collInfo.selCircleY;
        } break;

        case TUT_PointerTypeShips : {
            selSelectionDimensions( &rndCameraMatrix, &rndProjectionMatrix, tp->selection, sx, sy, sradius );
        } break;

        case TUT_PointerTypeAIVolume: {
            vector wpos    = volFindCenter( tp->volume );
            real32 wradius = volFindRadius( tp->volume );
            selCircleComputeGeneral( &rndCameraMatrix, &rndProjectionMatrix, &wpos, wradius, sx, sy, sradius );
        }
    }
}



// Draw a line from the current text box meeting a circle over the given worldspace position
static void tutDrawTextPointerLineToCircle( tutpointer* tp, rectanglef* rect, real32 thickness, color col ) {
    // Get the screenspace info
    real32 sx, sy, sradius;
    tutGetScreenSpaceInfoForLineToCircle( tp, &sx, &sy, &sradius );

    // Beind the screen? Nope out
    if (sradius <= 0.0f)
        return;
    
    // We need to render a circle here and it actually needs to be round! God damn it people!
    // Save projection and make it a bog-standard orthographic one
    const real32 mrw = (real32) MAIN_WindowWidth;
    const real32 mrh = (real32) MAIN_WindowHeight;
    
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( 0, mrw, mrh, 0, -1, +1 );
    glMatrixMode( GL_MODELVIEW );

    // Circle params
    real32 radiusMin    = TUT_ShipCircleSizeMin * getResDensityRelative();
    real32 radiusScaled = sradius * mrh * 0.5f;
    real32 radius       = max( radiusScaled, radiusMin );
    sdword segs         = pieCircleSegmentsCompute( radius / mrh * 2.0f ); // Takes screenspace arg
    real32 cx           = (real32) primGLToScreenX( sx );
    real32 cy           = (real32) primGLToScreenY( sy );
                    
    // Text box centre
    real32 rcx = ((rect->x0 + rect->x1) / 2.0f);
    real32 rcy = ((rect->y0 + rect->y1) / 2.0f);
                    
    // Line params
    real32 dx     = cx - rcx;
    real32 dy     = cy - rcy;
    real32 dMag   = sqrtf(dx*dx + dy*dy);
    real32 dScale = (dMag - radius) / dMag; // Meet circle edge

    // Line endpoints
    real32 bx = primGLToScreenX(rcx);
    real32 by = primGLToScreenY(rcy);
    real32 ex = primGLToScreenX(rcx + dx * dScale);
    real32 ey = primGLToScreenY(rcy + dy * dScale);

    // Clip the line to the box it's coming from.
    tutClipSegToTextBox( &bx, &by, &ex, &ey );

    // Draw!
    primGLCircleOutline2( cx, cy, radius, segs, col );
    primLine2( bx, by, ex, ey, col );

    // Restore projection
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
}



/*-----------------------------------------------------------------------------
    Name        : tutDrawTextPointers
    Description : Draw all active tutorial pointers
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void tutDrawTextPointers(rectanglef *pRect)
{
    static sdword tutPulse = TUT_PointerPulseMin;

    real32 x0, y0, x1, y1;
    sdword index;
    tutpointer *pointer;

    glEnable( GL_MULTISAMPLE );

    color  pulseWhite = colRGB(tutPulse,   tutPulse, tutPulse  );
    color  pulseGreen = colRGB(tutPulse/2, tutPulse, tutPulse/2);
    real32 thickness  = 1.65f * sqrtf(getResDensityRelative());

    for (pointer = tutPointer, index = 0; index < TUT_NumberPointers; index++, pointer++)
    {                                                       //for each pointer
        rectanglef prectf = rectToFloatRect(&pointer->rect);

        glLineWidth( thickness );

        switch(pointer->pointerType)
        {
            case TUT_PointerTypeNone:
                break;                                      //not active; ignore

            case TUT_PointerTypeXY:
                x0 = (pRect->x0 + pRect->x1) / 2;
                y0 = (pRect->y0 + pRect->y1) / 2;
                x1 = (real32) pointer->x;//tutPointerX;
                y1 = (real32) pointer->y;//tutPointerY;

                tutClipSegToTextBox(&x0, &y0, &x1, &y1);
                primLine2(x0, y0, x1, y1, pulseWhite);
                real32 dx = (real32)(x0 - x1);                     //vector from arrowhead to source
                real32 dy = (real32)(y0 - y1);
                if (dx != 0.0f || dy != 0.0f)
                {                                           //if not zero-length vector
                    real32 magnitude = sqrtf(dx * dx + dy * dy);   //magnitude of vector
                    real32 angle     = atan2f( dy, dx );
                    dx /= magnitude;
                    dy /= magnitude;                        //normalize the vector
                    //now we have angle of main vector; offset that to get arrowhead vectors
                    x0 = x1 + cosf(angle - TUT_ArrowheadAngle) * TUT_ArrowheadLength;
                    y0 = y1 + sinf(angle - TUT_ArrowheadAngle) * TUT_ArrowheadLength;
                    primLine2(x0, y0, x1, y1, pulseWhite);
                    x0 = x1 + cosf(angle + TUT_ArrowheadAngle) * TUT_ArrowheadLength;
                    y0 = y1 + sinf(angle + TUT_ArrowheadAngle) * TUT_ArrowheadLength;
                    primLine2(x0, y0, x1, y1, pulseWhite);
                }
                break;

            case TUT_PointerTypeShipHealth:
                if (pointer->ship->collInfo.selCircleRadius > 0.0)  //if ship on-screen
                {
                    x0 = (pRect->x0 + pRect->x1) / 2;
                    y0 = (pRect->y0 + pRect->y1) / 2;

                    x1 = (prectf.x0 + prectf.x1) / 2;
                    y1 = prectf.y0;

                    tutClipSegToTextBox(&x0, &x1, &y0, &y1);
                    primLine2(x0, y0, x1, y1, pulseGreen);
                    primRectOutline2(&prectf, thickness, pulseGreen);
                }
                break;

            case TUT_PointerTypeShipGroup:
                if (pointer->ship->collInfo.selCircleRadius > 0.0) //if ship on-screen
                {
                    x0 = (pRect->x0 + pRect->x1) / 2;
                    y0 = (pRect->y0 + pRect->y1) / 2;

                    x1 = (prectf.x0 + prectf.x1) / 2;
                    y1 = prectf.y0;

                    tutClipSegToTextBox(&x0, &y0, &x1, &y1);
                    primLine2(x0, y0, x1, y1, pulseGreen);
                    primRectOutline2(&prectf, thickness, pulseGreen);
                }
                break;

            case TUT_PointerTypeRegion:
                primRectOutline2(&prectf, sqrtf(thickness*3.0f), pulseWhite);
                break;

            case TUT_PointerTypeShip:
            case TUT_PointerTypeShips:
            case TUT_PointerTypeAIVolume:
                tutDrawTextPointerLineToCircle( pointer, pRect, thickness, pulseGreen );
                break;
        }
    }
    tutPulse = PulseVal(tutPulse);
    tutPointersDrawnThisFrame = TRUE;

    glLineWidth( 1.0f );
    glDisable( GL_MULTISAMPLE );
}

// This function actually handles drawing the text in the region added by tutShowText
void tutDrawTextFunction(regionhandle reg)
{
    char       Line[256];
    featom*    pAtom = (featom *)reg->atom;
    rectanglei recti = reg->rect;
    rectanglef rectf = rectToFloatRect( &recti );

//    if(tutTextPointerType != TUT_PointerTypeNone)
    if (!tutPointersDrawnThisFrame)
    {
        tutDrawTextPointers(&rectf);
    }

    // Draw the translucent blue box behind the text
    primRectTranslucent2(&rectf, colRGBA(0, 0, 64, 192));

    fonthandle currFont = fontMakeCurrent(tutTextFont);
    sdword x     =  recti.x0+5;
    sdword y     =  recti.y0+5;
    sdword width = (recti.x1 - recti.x0) - 10;

    char* pString = (char *)pAtom->pData;

    do {
        pString = tutGetNextTextLine(Line, pString, width, 256); //256 defined above as size of Line
        if(Line[0])
            fontPrintf(x, y, colRGB(255, 255, 128), "%s", Line);
        y += fontHeight(" ");
    } while(pString && pString[0]);

    fontMakeCurrent(currFont);
}

void tutShowNextButton(void)
{
    if(tutNextVisible)
        tutHideNextButton();

    tutAllocateRootRegion();

    tutNextAtom.x = (sdword)tutTextRect.x0 + 128;
    tutNextAtom.y = (sdword)tutTextRect.y1;
    tutNextAtom.width  = 64;
    tutNextAtom.height = 32;

    tutNextRegion = (buttonhandle)regChildAlloc(tutRootRegion, (smemsize)&tutNextAtom,
        tutNextAtom.x, tutNextAtom.y, tutNextAtom.width, tutNextAtom.height, sizeof(buttonhandle) - sizeof(regionhandle), 0);

    tutNextAtom.region = (void*)tutNextRegion;
    tutNextRegion->reg.atom = &tutNextAtom;
    tutNextRegion->reg.flags |= RPM_MouseEvents;
    tutNextAtom.pData = NULL;

    tutNextRegion->reg.processFunction = uicButtonProcess;
    tutNextRegion->processFunction = tutProcessNextButton;
    tutNextVisible = TRUE;

    regDrawFunctionSet((regionhandle)tutNextRegion, tutDrawNextButtonFunction);
}

void tutHideNextButton(void)
{
    if(tutNextRegion)
    {
        regRegionDelete((regionhandle)tutNextRegion);
        tutDeallocateRootRegion();
    }

    tutNextRegion = NULL;
    tutNextVisible = FALSE;

}

udword tutProcessNextButton(struct tagRegion *reg, smemsize ID, udword event, udword data)
{
    if(event == CM_ButtonClick)
    {
        tutNextButtonState = 1;
        return 1;
    }

    return 0;
}

void tutDrawNextButtonFunction(regionhandle reg)
{
    if(mouseInRect(&reg->rect))
    {
        if(mouseLeftButton())
             trRGBTextureMakeCurrent(tutTexture[TUT_NEXT_ON]);
        else trRGBTextureMakeCurrent(tutTexture[TUT_NEXT_MOUSE]);
    }
    else {
        trRGBTextureMakeCurrent(tutTexture[TUT_NEXT_OFF]);
    }

//  glEnable(GL_BLEND);
    primRectiSolidTextured2(&reg->rect, colWhite);
//  glDisable(GL_BLEND);
}

sdword tutNextButtonClicked(void)
{
sdword  RetVal;

    RetVal = tutNextButtonState;
    tutNextButtonState = 0;

    return RetVal;
}


void tutShowBackButton(void)
{
    if(tutBackVisible)
    {
        tutPrevVisible = FALSE;
        tutHideBackButton();
    }

    tutAllocateRootRegion();

    tutBackAtom.x = (sdword) tutTextRect.x0;
    tutBackAtom.y = (sdword) tutTextRect.y1;
    tutBackAtom.width = 128;
    tutBackAtom.height = 32;

    tutBackRegion = (buttonhandle)regChildAlloc(tutRootRegion, (smemsize)&tutBackAtom,
        tutBackAtom.x, tutBackAtom.y, tutBackAtom.width, tutBackAtom.height, sizeof(buttonhandle) - sizeof(regionhandle), 0);

    tutBackAtom.region = (void*)tutBackRegion;
    tutBackRegion->reg.atom = &tutBackAtom;
    tutBackRegion->reg.flags |= RPM_MouseEvents;
    tutBackAtom.pData = NULL;

    tutBackRegion->reg.processFunction = uicButtonProcess;
    tutBackRegion->processFunction = tutProcessBackButton;
    tutBackVisible = TRUE;

    regDrawFunctionSet((regionhandle)tutBackRegion, tutDrawBackButtonFunction);
}

void tutShowPrevButton(void)
{
    if (tutBackVisible)
    {
        tutHideBackButton();
    }
    tutPrevVisible = TRUE;
    tutShowBackButton();
}

void tutHideBackButton(void)
{
    if(tutBackRegion)
    {
        regRegionDelete((regionhandle)tutBackRegion);
        tutDeallocateRootRegion();
    }

    tutBackRegion = NULL;
    tutBackVisible = FALSE;
    tutPrevVisible = FALSE;

}

udword tutProcessBackButton(struct tagRegion *reg, smemsize ID, udword event, udword data)
{
    if(event == CM_ButtonClick)
    {
        if((tutPrevVisible) && (tutLastLessonName[0]))
            strcpy(tutCurrLessonName, tutLastLessonName);

        if (fileExists(tutCurrLessonName, 0) && (VerifySaveFile(tutCurrLessonName) == VERIFYSAVEFILE_OK))
        {
            tutEnableEverything();
            spMainScreen();
            tbForceTaskbar(FALSE);
            gameEnd();

            tutorial = 1;

            utyLoadSinglePlayerGameGivenFilename(tutCurrLessonName);
            return 1;
        }
    }

    return 0;
}

void tutDrawBackButtonFunction(regionhandle reg)
{
    if (tutPrevVisible)
    {
        //put in pointer to prevlesson texture here
        if(mouseInRect(&reg->rect))
        {
            if(mouseLeftButton())
                trRGBTextureMakeCurrent(tutTexture[TUT_PREV_ON]);
            else
                trRGBTextureMakeCurrent(tutTexture[TUT_PREV_MOUSE]);
        }
        else
            trRGBTextureMakeCurrent(tutTexture[TUT_PREV_OFF]);
    }
    else
    {
        //the back button is displayed as a "Restart Lesson"
        if(mouseInRect(&reg->rect))
        {
            if(mouseLeftButton())
                trRGBTextureMakeCurrent(tutTexture[TUT_REST_ON]);
            else
                trRGBTextureMakeCurrent(tutTexture[TUT_REST_MOUSE]);
        }
        else
            trRGBTextureMakeCurrent(tutTexture[TUT_REST_OFF]);
    }

//  glEnable(GL_BLEND);
    primRectiSolidTextured2(&reg->rect, colWhite);
//  glDisable(GL_BLEND);
}


// Returns the next "StartFrom" value, and copies the token into pDest
long GetNextCommaDelimitedToken(char *pString, char *pDest, long StartFrom)
{
static char *pSrc = NULL;
long Index = 0, Len;

    if(pString)
    {
        pSrc = pString;
        Index = 0;
    }

    if(pSrc)
    {
        Index = StartFrom;
        Len = 0;

        while(pSrc[Index] && pSrc[Index] != ',' && pSrc[Index] != 0x0a && pSrc[Index] != 0x0d)
        {
            pDest[Len] = pSrc[Index];
            Len++;
            Index++;
        }
        pDest[Len] = 0;

        if(pSrc[Index] == ',')
            Index++;
        else
            pSrc = NULL;
    }
    else
        return 0;

    return Index;
}

long tutFindTokenIndex(char *pTokenList[], char *pToken)
{
long    i;

    i = 0;
    while(pTokenList[i][0])
    {
        if(strcasecmp(pTokenList[i], pToken) == 0)
            return i;
        i++;
    }

    return -1;
}

long tutParseImagesIntoIndices(char *szImages)
{
    char szToken[256];

    long count    = 0;
    long strIndex = GetNextCommaDelimitedToken(szImages, szToken, 0);
    while (strIndex)
    {
        long TokenIndex = tutFindTokenIndex(tutImageList, szToken);

        if(TokenIndex != -1)
             szImageIndexList[count] = TokenIndex;
        else szImageIndexList[count] = 0;

        count++;
        strIndex = GetNextCommaDelimitedToken(NULL, szToken, strIndex);
    }

    szImageIndexList[count] = 0;
    return count;
}


/*
This function will accept a comma delimited list of images to be displayed, in
order of appearance in this list, from left to right on the screen.  The image
list is included in the header.
*/
void tutShowImages(char *szImages)
{
long    ImageCount;

    if(tutImageVisible)
        tutHideImages();

    tutAllocateRootRegion();

    ImageCount = tutParseImagesIntoIndices(szImages);

    tutImageAtom.x      = (sword) (MAIN_WindowWidth - ImageCount * 64);
    tutImageAtom.y      = (sword) (MAIN_WindowHeight - 64);
    tutImageAtom.width  = (sword) ImageCount * 64;
    tutImageAtom.height = (sword) 64;

    tutImageRegion = regChildAlloc(tutRootRegion, (smemsize)&tutImageAtom,
        tutImageAtom.x, tutImageAtom.y, tutImageAtom.width, tutImageAtom.height, 0, 0);

    tutImageAtom.region = (void*)tutImageRegion;
    tutImageRegion->atom = &tutImageAtom;
    tutImageAtom.pData = (ubyte *) szImages;

    tutImageVisible = TRUE;

    regDrawFunctionSet(tutImageRegion, tutDrawImageFunction);
}

void tutDrawImageFunction(regionhandle reg)
{
    sdword x = reg->rect.x0;
    sdword y = reg->rect.y0;
    sdword i = 0;

//  glEnable(GL_BLEND);

    while (szImageIndexList[i])
    {
        sdword Index = szImageIndexList[i];

        trRGBTextureMakeCurrent(tutTexture[Index]);
        rectanglei rect;
        rect.x0 = x + (64 * i);
        rect.y0 = y;
        rect.x1 = x + (64 * i) + 64;
        rect.y1 = y + 64;
        primRectiSolidTextured2(&rect, colWhite);

        i++;
    }
//  glDisable(GL_BLEND);
}


void tutHideImages(void)
{
    if(tutImageRegion)
    {
        regRegionDelete(tutImageRegion);
        tutDeallocateRootRegion();
    }

    tutImageRegion = NULL;
    tutImageVisible = FALSE;
}

/*-----------------------------------------------------------------------------
    Name        : tutStartup
    Description : tutorial init function called at game startup
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void tutStartup(void)
{
    sdword index;

    for (index = 0; tutImageList[index][0]; index++)
    {
        tutTexture[index] = TR_InvalidInternalHandle;
        tutImage[index] = NULL;
    }
}

/*-----------------------------------------------------------------------------
    Name        : tutShutdown
    Description : Shutdown function called at game shutdown time
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void tutShutdown(void)
{
    ;
}

/*-----------------------------------------------------------------------------
    Name        : tutInitialize
    Description : Tutorial init function called once for every tutorial load
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void tutInitialize(void)
{
    long i = 0;
    char Filename[256];

    while( tutImageList[i][0] )
    {
        if (tutTexture[i] == TR_InvalidInternalHandle)
        {
            dbgAssertOrIgnore(tutImage[i] == NULL);
            strcpy(Filename, "feman/texdecorative/");

            //load the correct texture depending on language. defaults to english
            switch (strCurLanguage) {
                case languageEnglish : strcat(Filename, tutImageList[i]);        break;
                case languageFrench  : strcat(Filename, tutFrenchImageList[i]);  break;
                case languageGerman  : strcat(Filename, tutGermanImageList[i]);  break;
                case languageSpanish : strcat(Filename, tutSpanishImageList[i]); break;
                case languageItalian : strcat(Filename, tutItalianImageList[i]); break;
                default              : strcat(Filename, tutImageList[i]);        break;
            }
            strcat(Filename, ".LiF");
            tutImage[i] = trLIFFileLoad(Filename, NonVolatile);

            dbgAssertOrIgnore(tutImage[i] != NULL);
            tutTexture[i] = trRGBTextureCreate((color *)tutImage[i]->data, tutImage[i]->width, tutImage[i]->height, TRUE);
            i++;
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : tutUnInitialize
    Description : Tutorial shutdown function called at the end of a tutorial game
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void tutUnInitialize(void)
{
long    i;

    tutHideText();
    tutHideImages();
    tutHideNextButton();
    tutHideBackButton();

    i = 0;
    while( tutImageList[i][0] )
    {
        if(tutImage[i] != NULL)
        {
            memFree(tutImage[i]);
            tutImage[i] = NULL;
        }

        if (tutTexture[i] != TR_InvalidInternalHandle)
        {
            trRGBTextureDelete(tutTexture[i]);
            tutTexture[i] = TR_InvalidInternalHandle;
        }
        i++;
    }
    tutEnableEverything();
}

void tutSetFlagIndex(sdword Index, sdword Val)
{
sdword *pFlagMem;
sdword FlagBit;

    //hack code to get speech to work when game paused
    if (Index == 1)
    {
        FalkosFuckedUpTutorialFlag = !Val;
    }

    pFlagMem = (sdword *)&tutEnable;
	pFlagMem += Index/32;

#if FIX_ENDIAN
	FlagBit = 1 << (31 - Index & 31);
#else
	FlagBit = 1 << (Index & 31);
#endif

    if(Val)
        *pFlagMem |= FlagBit;
    else
        *pFlagMem &= ~FlagBit;
}

void tutEnableEverything(void)
{
long    i;

    for(i=0; i<sizeof(tutGameEnableFlags)*8; i++)
        tutSetFlagIndex(i, 1);
}

void tutDisableEverything(void)
{
long    i;

    for(i=0; i<sizeof(tutGameEnableFlags)*8; i++)
        tutSetFlagIndex(i, 0);
}

void tutSetEnableFlags(char *pFlagString, long Val)
{
char    szToken[256];
long    StrIndex, TokenIndex;

    StrIndex = GetNextCommaDelimitedToken(pFlagString, szToken, 0);
    while(StrIndex)
    {
        TokenIndex = tutFindTokenIndex(tutGameEnableString, szToken);
        tutSetFlagIndex(TokenIndex, Val);
        StrIndex = GetNextCommaDelimitedToken(NULL, szToken, StrIndex);
    }
}

void tutBuilderSetRestrictions(char *pShipTypes, bool bRestricted)
{
ShipType    st;
char    szToken[256];
long    StrIndex;

    StrIndex = GetNextCommaDelimitedToken(pShipTypes, szToken, 0);
    while(StrIndex)
    {
        st = StrToShipType(szToken);
        tutBuildRestrict[st] = bRestricted;

        StrIndex = GetNextCommaDelimitedToken(NULL, szToken, StrIndex);
    }

    // usually BuildArrows are on when the player can build things
    // but the arrows can be disabled seperately if needed
    tutSetEnableFlags("BuildArrows", !bRestricted);
}

void tutBuilderRestrictAll(void)
{
int i;

    for(i=0; i<TOTAL_STD_SHIPS; i++)
        tutBuildRestrict[i] = 1;

    tutSetEnableFlags("BuildArrows", 0);
}

void tutBuilderRestrictNone(void)
{
int i;

    for(i=0; i<TOTAL_STD_SHIPS; i++)
        tutBuildRestrict[i] = 0;

    tutSetEnableFlags("BuildArrows", 1);
}

sdword tutIsBuildShipRestricted(sdword shipType)
{
    return tutBuildRestrict[shipType];
}

sdword tutSelectedContainsShipType(ShipType st)
{
long i, j;

    for(i=0, j=0; i<selSelected.numShips; i++)
    {
        if(selSelected.ShipPtr[i]->shiptype == st)
            j++;
    }
    return j;
}

sdword tutSelectedContainsShipTypes(char *pShipTypes)
{
ShipType    st;
char    szToken[256];
long    StrIndex, Count;

    Count = 0;
    StrIndex = GetNextCommaDelimitedToken(pShipTypes, szToken, 0);
    while(StrIndex)
    {
        st = StrToShipType(szToken);

        Count += tutSelectedContainsShipType(st);

        StrIndex = GetNextCommaDelimitedToken(NULL, szToken, StrIndex);
    }

    return Count;
}

void tutGameMessage(char *eventName)
{
    if (!tutorial)
    {
        return;
    }

#ifdef HW_BUILD_FOR_DEBUGGING
    dbgMessagef("%s: '%s'", __func__, eventName);
#endif

    if( !tutGameMessageInQueue(eventName) && tutGameMessageIndex < 16 )
    {
        strcpy(tutGameMessageList[ tutGameMessageIndex ], eventName);
        tutGameMessageIndex++;
    }
}

bool tutGameSentMessage(char *eventNames)
{
    bool in_queue = tutGameMessageInQueue(eventNames);

    tutResetGameMessageQueue();
    
    return in_queue;
}

bool tutGameMessageInQueue(char *eventNames)
{
    char szToken[256];
    long StrIndex;

    StrIndex = GetNextCommaDelimitedToken(eventNames, szToken, 0);
    while (StrIndex)
    {
        for (long i=0; i<tutGameMessageIndex; i++)
        {
            if (strcasecmp(szToken, tutGameMessageList[i]) == 0)
            {
                return TRUE;
            }
        }
        StrIndex = GetNextCommaDelimitedToken(NULL, szToken, StrIndex);
    }

    return FALSE;
}

void tutResetGameMessageQueue(void)
{
    tutGameMessageIndex = 0;
}

sdword tutContextMenuDisplayedForShipType(char *pShipType)
{
    ShipType st;

    st = StrToShipType(pShipType);
    return (tutFEContextMenuShipType == st);
}

void tutResetContextMenuShipTypeTest(void)
{
    tutFEContextMenuShipType = (ShipType)-1;
}


sdword tutBuildManagerShipTypeInBatchQueue(char *pShipType)
{
    long        i;
    ShipType    st;

    st = StrToShipType(pShipType);
    for (i=0; cmShipsAvailable[i].nJobs != -1; i++)
    {
        if(cmShipsAvailable[i].type == st)
            return cmShipsAvailable[i].nJobs;
    }

    return 0;
}

sdword tutBuildManagerShipTypeInBuildQueue(char *pShipType)
{
    long        index;
    ShipType    st;
    Node        *node;
    Ship        *factoryship;
    shipsinprogress *sinprogress;
    shipinprogress  *progress;

    st = StrToShipType(pShipType);

    node = listofShipsInProgress.head;
    while (node != NULL)
    {
        sinprogress = (shipsinprogress *)listGetStructOfNode(node);
        factoryship = sinprogress->ship;

        if (factoryship->playerowner == universe.curPlayerPtr)
        {
            for (index = 0, progress = &sinprogress->progress[0]; index < TOTAL_STD_SHIPS; index++, progress++)
            {
                if(progress->info && progress->info->shiptype == st)
                    return progress->nJobs;
            }
        }
        node = node->next;
    }

    return 0;
}

//external function definition
shipinprogress *cmSIP(udword index);

/*-----------------------------------------------------------------------------
    Name        : tutBuildManagerShipTypeSelected
    Description : Returns TRUE if the shiptype is selected in the build manager
    Inputs      : pShipType - string of shiptypes
    Outputs     :
    Return      : TRUE or FALSE
----------------------------------------------------------------------------*/

sdword tutBuildManagerShipTypeSelected(char *pShipType)
{
    ShipType shiptype;
    shipinprogress *shipprog;

    shiptype = StrToShipType(pShipType);
    shipprog = cmSIP(shiptype);

    if (shipprog->selected)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



long tutCameraFocusShipCount(ShipType st)
{
long Count = 0;
CameraStackEntry *entry;
long    i;

    entry = currentCameraStackEntry(&universe.mainCameraCommand);

    for(i=0; i<entry->focus.numShips; i++)
    {
        if(entry->focus.ShipPtr[i]->shiptype == st)
            Count++;
    }
    return Count;
}


sdword tutCameraFocusedOnShipType(char *pShipTypes)
{
char    szToken[256];
long    StrIndex, Count;
ShipType st;

    if (smSensorsActive)
    {
        //camera can't be focused on a ship in the Sensors Manager
        return 0;
    }
    Count = 0;
    StrIndex = GetNextCommaDelimitedToken(pShipTypes, szToken, 0);
    while(StrIndex)
    {
        st = StrToShipType(szToken);
        Count += tutCameraFocusShipCount(st);
        StrIndex = GetNextCommaDelimitedToken(NULL, szToken, StrIndex);
    }
    return Count;
}
