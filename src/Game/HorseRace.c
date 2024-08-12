/*=============================================================================
    Name    : HorseRace.c
    Purpose : Logic for the horse race and loading chat screens.

    Created 6/16/1998 by ddunlop
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#include "HorseRace.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "AutoDownloadMap.h"
#include "Chatting.h"
#include "CommandNetwork.h"
#include "Debug.h"
#include "Demo.h"
#include "devstats.h"
#include "ETG.h"
#include "FEFlow.h"
#include "FEReg.h"
#include "File.h"
#include "font.h"
#include "FontReg.h"
#include "interfce.h"
#include "LinkedList.h"
#include "main.h"
#include "Memory.h"
#include "mouse.h"
#include "MultiplayerGame.h"
#include "NIS.h"
#include "Options.h"
#include "prim2d.h"
#include "Region.h"
#include "render.h"
#include "SinglePlayer.h"
#include "StringSupport.h"
#include "Task.h"
#include "Teams.h"
#include "texreg.h"
#include "TimeoutTimer.h"
#include "Titan.h"
#include "Tweak.h"
#include "UIControls.h"
#include "Universe.h"
#include "utility.h"
#include "rStateCache.h"

#ifdef _WIN32
    #define strcasecmp _stricmp
#else
    #include <dirent.h>
    #include <unistd.h>
#endif



extern Uint32 utyTimerLast;

/*=============================================================================
    defines:
=============================================================================*/

#ifdef _MACOSX
    #define HR_SCALE_MISSION_LOADING_SCREENS  TRUE
#else
    #define HR_SCALE_MISSION_LOADING_SCREENS  FALSE
#endif

#define HR_PacketRateLimitTime 50   // One packet every N milliseconds, max
#define HR_MinRenderDelta      10   // One render every N milliseconds, max
#define HR_MinOverallTime      1600 // Must be at least 1.5 seconds total to work properly with the audio system.
#define HR_PlayerNameFont      "Arial_12.hff"
#define MAX_CHAT_TEXT          64
#define NUM_CHAT_LINES         10

real32 HorseRacePlayerDropoutTime = 10.0f;     // tweakable
color  HorseRaceDropoutColor      = colRGB(75,75,75);

/*=============================================================================
    data:
=============================================================================*/

static rectangle hrSinglePlayerPos;
static color hrSinglePlayerLoadingBarColor = colRGB(255,63,63);  // pastel red

static sdword JustInit;
static udword startTimeRef;
static udword lastRenderTime;
static sdword localbar;

// Pixels and info about the background image chosen
static long hrBackXSize, hrBackYSize;
static GLfloat hrBackXFrac, hrBackYFrac;
static GLuint hrBackgroundTexture = 0;

static bool hrScaleMissionLoadingScreens = HR_SCALE_MISSION_LOADING_SCREENS;

static regionhandle hrDecRegion;

HorseStatus horseracestatus;
static udword hrProgressCounter = 0;
static udword hrLastPacketSendTime;

TTimer hrPlayerDropoutTimers[MAX_MULTIPLAYER_PLAYERS];

fibfileheader *hrScreensHandle = NULL;

fonthandle      playernamefont=0;
color           hrBackBarColor=HR_BackBarColor;
color           hrChatTextColor=HR_ChatTextColor;

bool            PlayersAlreadyDrawnDropped[MAX_MULTIPLAYER_PLAYERS];

textentryhandle ChatTextEntryBox = NULL;

regionhandle   hrBaseRegion       = NULL;
regionhandle   hrProgressRegion   = NULL;
regionhandle   hrChatBoxRegion    = NULL;
regionhandle   hrAbortLoadConfirm = NULL;

region horseCrapRegion =
{
    {-1,-1,-1,-1},             // rectangle
    NULL,                      // draw function
    NULL,                      // process function
    NULL, NULL,                // parent, child
    NULL, NULL,                // previous, next
    0, 0,                      // flags, status,
    0,                         // nKeys
    {0},                       // keys
    0,                         // userID
#if REG_ERROR_CHECKING
    REG_ValidationKey,         // validation key
#endif
    0,                         // tabstop
    UNINITIALISED_LINKED_LIST, // cutouts;
    {0,0},                     // drawstyle[2];
    0,                         // lastframedrawn;
    NULL                       // atom;
};

bool hrRunning=FALSE;

ChatPacket chathistory[NUM_CHAT_LINES];

udword      chatline=0;

void horseRaceRender(void);
void hrDrawPlayersProgress(featom *atom, regionhandle region);
void hrDrawChatBox(featom *atom, regionhandle region);
void hrDrawFile(char* filename, sdword x, sdword y);
void hrChatTextEntry(char *name, featom *atom);
void hrAbortLoadingYes(char *name, featom *atom);
void hrAbortLoadingNo(char *name, featom *atom);

real32 horseRaceGetPacketPercent(real32 barPercent);

//HR_ChatTextEntry
//extern nisheader *utyTeaserHeader;



// Hack crap to get it to build
bool feShouldSaveMouseCursor( void ) { return 0; }
void mouseStoreCursorUnder  ( void ) {};
void mouseRestoreCursorUnder( void ) {};





/*=============================================================================
    Functions
=============================================================================*/
fedrawcallback hrDrawCallback[] =
{
    {hrDrawPlayersProgress,     "HR_DrawPlayersProgress"    },
    {hrDrawChatBox,             "HR_DrawChatBox"            },
    {NULL,          NULL}
};

fecallback hrCallBack[] =
{
    {hrChatTextEntry,           "HR_ChatTextEntry"          },
    {hrAbortLoadingYes,         "HR_AbortLoadingYes"        },
    {hrAbortLoadingNo,          "HR_AbortLoadingNo"         },
    {NULL,          NULL}
};

void hrDirtyProgressBar()
{
    if (hrRunning)
    {
        if (hrProgressRegion != NULL)
        {
#ifdef DEBUG_STOMP
            regVerify(hrProgressRegion);
#endif
            hrProgressRegion->status |= RSF_DrawThisFrame;
        }
    }
}

void hrDirtyChatBox()
{
    if (hrRunning)
    {
        if (hrChatBoxRegion != NULL)
        {
#ifdef DEBUG_STOMP
            regVerify(hrChatBoxRegion);
#endif
            hrChatBoxRegion->status |= RSF_DrawThisFrame;
        }
    }
}

void hrBarDraw(rectangle *rect, color back, color fore, real32 percent)
{
    rectangle temp;
//  primRectSolid2(rect, back);

    if (percent > 1.0)
    {
        percent = 1.0;
    }

    temp.x0 = rect->x0;
    temp.y0 = rect->y0;
    temp.x1 = rect->x0 + (sdword)((rect->x1-rect->x0)*percent);
    temp.y1 = rect->y1;

    primRectSolid2(&temp, fore);
}

void hrDrawPlayersProgress(featom *atom, regionhandle region)
{
    sdword     index;
    rectangle pos; 
    rectangle outline;
    real32 percent;
    fonthandle currentfont;
    bool droppedOut;

    hrProgressRegion = region;

    pos = region->rect;

//  primRectSolid2(&pos, colRGBA(0, 0, 0, 64));

    pos.y0+=fontHeight(" ");
    pos.y1=pos.y0+8;

    currentfont = fontMakeCurrent(playernamefont);

    if (multiPlayerGame)
    {
        dbgAssertOrIgnore(sigsNumPlayers == tpGameCreated.numPlayers);
        for (index=0;index<sigsNumPlayers;index++)
        {
            droppedOut = playerHasDroppedOutOrQuit(index);

            outline = pos;
            outline.x0 -= 5;
            outline.x1 += 10;
            outline.y0 -= 3;
            outline.y1 = outline.y0 + fontHeight(" ")*2 - 2;

            PlayersAlreadyDrawnDropped[index] = droppedOut;

            primRectSolid2(&outline, colBlack);

            if (droppedOut)
            {
                fontPrintf(pos.x0,pos.y0,colBlack,"%s",tpGameCreated.playerInfo[index].PersonalName);
                fontPrintf(pos.x0,pos.y0,HorseRaceDropoutColor,"%s",
                            (playersReadyToGo[index] == PLAYER_QUIT) ? strGetString(strQuit) : strGetString(strDroppedOut));
            }
            else
            {
                fontPrintf(pos.x0,pos.y0,tpGameCreated.playerInfo[index].baseColor,"%s",tpGameCreated.playerInfo[index].PersonalName);

                if (horseracestatus.hrstatusstr[index][0])
                {
                    fontPrintf(pos.x0+150,pos.y0,tpGameCreated.playerInfo[index].baseColor,"%s",horseracestatus.hrstatusstr[index]);
                }
            }

            primRectOutline2(&outline, 1, (droppedOut) ? HorseRaceDropoutColor : tpGameCreated.playerInfo[index].stripeColor);

            pos.y0+=fontHeight(" ");
            pos.y1=pos.y0+4;

            percent = horseracestatus.percent[index];

            hrBarDraw(&pos,hrBackBarColor,(droppedOut) ? HorseRaceDropoutColor : tpGameCreated.playerInfo[index].baseColor,percent);

            pos.y0+=fontHeight(" ");
        }
    }
    else if (singlePlayerGame)
    {
        pos = hrSinglePlayerPos;

        // progress bar
        if (pos.x0 != 0)
        {
            percent = horseracestatus.percent[0];

            //dbgMessagef("percent %f",percent);

            hrBarDraw(&pos, colBlack, hrSinglePlayerLoadingBarColor/*teColorSchemes[0].textureColor.base*/, percent);
        }

        // blinking hyperspace destination (render every other call)
        const udword wrapAt = 100;
        hrProgressCounter = (hrProgressCounter + 1) % wrapAt;
        if (hrProgressCounter > wrapAt / 2) // Blink
        {
            // hyperspace destination circled in first-person view  
            #define SP_LOADING_HYPERSPACE_DEST_CIRCLE_X  115
            #define SP_LOADING_HYPERSPACE_DEST_CIRCLE_Y  342
            
            // hyperspace destination arrowed in "as the bird flies" view 
            #define SP_LOADING_HYPERSPACE_DEST_ARROWS_X  195
            #define SP_LOADING_HYPERSPACE_DEST_ARROWS_Y  134
        
            // NB: hrDrawFile deals with coordinate mapping
        
            hrDrawFile("feman/loadscreen/ring.lif",
                SP_LOADING_HYPERSPACE_DEST_CIRCLE_X, SP_LOADING_HYPERSPACE_DEST_CIRCLE_Y
            );
            hrDrawFile("feman/loadscreen/arrows.lif",
                SP_LOADING_HYPERSPACE_DEST_ARROWS_X, SP_LOADING_HYPERSPACE_DEST_ARROWS_Y
            );
        }
    }
    else
    {
        outline = pos;
        outline.x0 -= 5;
        outline.x1 += 10;
        outline.y0 -= 3;
        outline.y1 = outline.y0 + fontHeight(" ")*2 - 2;

        primRectTranslucent2(&outline, colRGBA(0,0,0,64));
        primRectOutline2(&outline, 1, teColorSchemes[0].textureColor.detail);

        fontPrintf(pos.x0,pos.y0,teColorSchemes[0].textureColor.base,"%s",playerNames[0]);

        pos.y0+=fontHeight(" ");
        pos.y1=pos.y0+4;

        percent = horseracestatus.percent[0];

        hrBarDraw(&pos,hrBackBarColor,teColorSchemes[0].textureColor.base,percent);
    }
    fontMakeCurrent(currentfont);
}

void hrDrawChatBox(featom *atom, regionhandle region)
{
    fonthandle currentfont;
    sdword     x,y,i;
    char       name[128];

    hrChatBoxRegion = region;

    currentfont = fontMakeCurrent(playernamefont);

    primRectSolid2(&region->rect,colRGB(0,0,0));
    feStaticRectangleDraw(region);

    x = region->rect.x0+10;
    y = region->rect.y0;

    for (i=0;i<NUM_CHAT_LINES;i++)
    {
        if (chathistory[i].message[0]!=0) 
        {
            x = region->rect.x0;
            sprintf(name,"%s >",tpGameCreated.playerInfo[chathistory[i].packetheader.frame].PersonalName);
            fontPrintf(x,y,tpGameCreated.playerInfo[chathistory[i].packetheader.frame].baseColor,"%s",name);
            x+=fontWidth(name)+10;
            //fontShadowSet(FS_E | FS_SE | FS_S);
            fontPrintf(x,y,hrChatTextColor,"%s",chathistory[i].message);
            //fontShadowSet(FS_NONE);
            y+= fontHeight(" ");
        }
    }

    fontMakeCurrent(currentfont);
}

void hrChooseSinglePlayerBitmap(char* pFilenameBuffer)
{
    char fname[128];
    sdword x, y, width, height;

    memset(&hrSinglePlayerPos, 0, sizeof(hrSinglePlayerPos));

    //image itself
#if defined(HW_GAME_RAIDER_RETREAT)
    if (spGetCurrentMission() == MISSION_5B_TURANIC_RAIDER_PLANETOID)
    {
        sprintf(fname, "SinglePlayer/mission05_OEM/loading.jpg");
    }
    else
#endif
    {
        sprintf(fname, "SinglePlayer/mission%02d/loading.jpg", spGetCurrentMission());
    }

    if (!fileExists(fname, 0))
    {
        pFilenameBuffer[0] = '\0';
        return;
    }

    strcpy(pFilenameBuffer, fname);

    // loading bar position
    // NB: the single player loading images are not all aligned properly...
    #define SP_LOADING_IMAGE_PROGRESS_BAR_X        43
    #define SP_LOADING_IMAGE_PROGRESS_BAR_Y       133
    #define SP_LOADING_IMAGE_PROGRESS_BAR_WIDTH   151
    #define SP_LOADING_IMAGE_PROGRESS_BAR_HEIGHT    2

    x      = hrScaleMissionLoadingScreens
           ? feResRepositionScaledX (SP_LOADING_IMAGE_PROGRESS_BAR_X)
           : feResRepositionCentredX(SP_LOADING_IMAGE_PROGRESS_BAR_X);
      
    y      = hrScaleMissionLoadingScreens
           ? feResRepositionScaledY (SP_LOADING_IMAGE_PROGRESS_BAR_Y)
           : feResRepositionCentredY(SP_LOADING_IMAGE_PROGRESS_BAR_Y);
    
    width  = hrScaleMissionLoadingScreens
           ? (SP_LOADING_IMAGE_PROGRESS_BAR_WIDTH  * FE_SCALE_TO_FIT_FACTOR_RELIC_SCREEN)
           :  SP_LOADING_IMAGE_PROGRESS_BAR_WIDTH;
    
    height = hrScaleMissionLoadingScreens
           ? (SP_LOADING_IMAGE_PROGRESS_BAR_HEIGHT * FE_SCALE_TO_FIT_FACTOR_RELIC_SCREEN)
           :  SP_LOADING_IMAGE_PROGRESS_BAR_HEIGHT;
    
    hrSinglePlayerPos.x0 = x;
    hrSinglePlayerPos.y0 = y;
    hrSinglePlayerPos.x1 = x + width;
    hrSinglePlayerPos.y1 = y + height;
}

void hrChooseRandomBitmap(char *pFilenameBuffer)
{
#ifdef _WIN32
    struct _finddata_t  FindData;
    long hFind;
#else
    DIR *dp;
    struct dirent* dir_entry;
    FILE* fp;
#endif
    filehandle handle;
    long userScreenShotCount = 0, BigFileCount = 0,
         chosenFileIndex = 0, currentFileIndex = 0, Result;
    char BigName[PATH_MAX], CurDir[PATH_MAX], NewDir[PATH_MAX];

    // Remember the current directory
    getcwd(CurDir, PATH_MAX);

    NewDir[0] = 0;
    strcpy(NewDir, filePathPrepend("ScreenShots", FF_UserSettingsPath));

    // Prefer user screenshots over pre-saved ones; they're more likely to be 
    // at a higher resolution, not to mention more interesting...

    // Switch to the screenshots directory and count the ones in there
    /*SetCurrentDirectory(NewDir);*/
    chdir(NewDir);
#ifdef _WIN32
    hFind = _findfirst("*.jpg", &FindData);
    if(hFind != -1)
    {
        do {
            if( ((FindData.attrib & _A_SUBDIR) == 0) &&
                ((FindData.attrib & _A_HIDDEN) == 0) )
                userScreenShotCount++;
        } while (_findnext(hFind, &FindData) == 0);
        _findclose(hFind);
    }
#else
    dp = opendir(".");

    if (dp)
    {
        unsigned int dir_str_len;

        while ((dir_entry = readdir(dp)))
        {
            if (dir_entry->d_name[0] == '.')
                continue;
            dir_str_len = strlen(dir_entry->d_name);
            if (dir_str_len < 4 ||
                strcasecmp(dir_entry->d_name + dir_str_len - 4, ".jpg"))
                continue;

            /* See if the current process can actually open the file (simple
               check for read permissions and if it's a directory). */
            if (!(fp = fopen(dir_entry->d_name, "rb")))
                continue;
            fclose(fp);

            userScreenShotCount++;
        }

        closedir(dp);
    }
#endif

    if (userScreenShotCount > 0)
    {
        chosenFileIndex = (utyTimerLast % 32777) % userScreenShotCount;

#ifdef _WIN32
        hFind = _findfirst("*.jpg", &FindData);
        if(hFind != -1)
        {
            do {
                if( ((FindData.attrib & _A_SUBDIR) == 0) &&
                    ((FindData.attrib & _A_HIDDEN) == 0) )
                {
                    if(currentFileIndex == chosenFileIndex)
                    {
                        _findclose(hFind);
                        strcpy(pFilenameBuffer, filePathPrepend("ScreenShots\\", FF_UserSettingsPath));
                        strcat(pFilenameBuffer, FindData.name);
                        break;
                    }
                    else
                        currentFileIndex++;
                }
            } while (_findnext(hFind, &FindData) == 0);
        }
#else
        dp = opendir(".");

        if (dp)
        {
            unsigned int dir_str_len;

            while ((dir_entry = readdir(dp)))
            {
                if (dir_entry->d_name[0] == '.')
                    continue;
                dir_str_len = strlen(dir_entry->d_name);
                if (dir_str_len < 4 ||
                    strcasecmp(dir_entry->d_name + dir_str_len - 4, ".jpg"))
                    continue;
                if (!(fp = fopen(dir_entry->d_name, "rb")))
                    continue;
                fclose(fp);

                if (currentFileIndex != chosenFileIndex)
                {
                    currentFileIndex++;
                    continue;
                }

                strcat(pFilenameBuffer, filePathPrepend("ScreenShots/", FF_UserSettingsPath));
                strcat(pFilenameBuffer, dir_entry->d_name);
                
                break;
            }

            closedir(dp);
        }
#endif
    }
    else // look in the big file for fallback images
    {   
        // First, find screen shots listed in the BigFile
        handle = fileOpen("ScreenShots/ShotList.script", FF_ReturnNULLOnFail | FF_TextMode);
        if(handle)
        {
            do {
                Result = fileLineRead(handle, BigName, 512);

                if(Result != FR_EndOfFile && Result > 0)    // Found one!
                    BigFileCount++;

            } while(Result != FR_EndOfFile);

            fileClose(handle);
        }
        
        if (BigFileCount > 0)
        {
            chosenFileIndex = (utyTimerLast % 32777) % BigFileCount;
            handle          = fileOpen("ScreenShots/ShotList.script", FF_ReturnNULLOnFail | FF_TextMode);
            if(handle)
            {
                do {
                    Result = fileLineRead(handle, BigName, 512);

                    if(Result != FR_EndOfFile && Result > 0)    // Found one!
                        chosenFileIndex--;

                } while( (chosenFileIndex >= 0) && (Result != FR_EndOfFile));
                strcpy(pFilenameBuffer, "ScreenShots/");
                strcat(pFilenameBuffer, BigName);
            }
        }
    }

    // Did we find any at all?
    if(userScreenShotCount == 0 && BigFileCount == 0)
    {
        pFilenameBuffer[0] = 0;
    }

    /*SetCurrentDirectory(CurDir);*/
    chdir(CurDir);
}


long hrShipsToLoadForRace(ShipRace shiprace)
{
    long ShipsToLoad = 0;
    ShipType shiptype;
    ShipType firstshiptype;
    ShipType lastshiptype;
    ShipStaticInfo *shipstaticinfo;

    firstshiptype = FirstShipTypeOfRace[shiprace];
    lastshiptype = LastShipTypeOfRace[shiprace];
    for (shiptype=firstshiptype;shiptype<=lastshiptype;shiptype++)
    {
        shipstaticinfo = GetShipStaticInfo(shiptype,shiprace);

        if( bitTest(shipstaticinfo->staticheader.infoFlags, IF_InfoNeeded) && !bitTest(shipstaticinfo->staticheader.infoFlags, IF_InfoLoaded) )
            ShipsToLoad++;
    }

    return ShipsToLoad;
}

void hrInitBackground(void)
{
    char CurDir[PATH_MAX], NewDir[PATH_MAX];
    char hrImageName[PATH_MAX];
    filehandle handle;
    JPEGDATA    jp;
    unsigned char *pTempImage;
    udword i;

    getcwd(CurDir, PATH_MAX);

    hrImageName[0] = 0;
    if (singlePlayerGame)
    {
        hrChooseSinglePlayerBitmap(hrImageName);
    }
    else
    {
        hrChooseRandomBitmap(hrImageName);
    }

    getcwd(NewDir, PATH_MAX);

    dbgAssertOrIgnore(strcasecmp(CurDir,NewDir) == 0);

    // Load the bitmap image
    handle = fileOpen(hrImageName, FF_ReturnNULLOnFail|FF_IgnorePrepend);
    if (handle)
    {
        memset(&jp, 0, sizeof(jp));
        jp.input_file = handle;
        JpegInfo(&jp);

        fileSeek(handle, 0, SEEK_SET);

        hrBackXSize = jp.width;
        hrBackYSize = jp.height;

        jp.ptr = (unsigned char *)memAllocAttempt((hrBackXSize) * (hrBackYSize) * 3, "BackgroundTemp", NonVolatile);
        if (jp.ptr == NULL)
        {
            return;
        }
        
        JpegRead(&jp);
        fileClose(handle);

        hrBackXSize = 1; hrBackYSize = 1;
        while (hrBackXSize < jp.width) hrBackXSize <<= 1;
        while (hrBackYSize < jp.height) hrBackYSize <<= 1;
        pTempImage = (unsigned char *)memAllocAttempt(hrBackXSize * hrBackYSize * 3, "BackgroundTemp", NonVolatile);
        memset(pTempImage, 0, hrBackXSize * hrBackYSize * 3);
        for (i = 0; i < jp.height; i++)
            memcpy(pTempImage + (hrBackXSize * 3 * i), jp.ptr + (jp.width * 3 * i), jp.width * 3);

        glGenTextures(1, &hrBackgroundTexture);
        trClearCurrent();
        glBindTexture(GL_TEXTURE_2D, hrBackgroundTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, hrBackXSize, hrBackYSize, 0, GL_RGB, GL_UNSIGNED_BYTE, pTempImage);
        
        memFree(jp.ptr);
        memFree(pTempImage);
        
        hrBackXFrac = (GLfloat)jp.width / (GLfloat)hrBackXSize;
        hrBackYFrac = (GLfloat)jp.height / (GLfloat)hrBackYSize;
        hrBackXSize = jp.width;
        hrBackYSize = jp.height;
    }
}

void hrRectSolidTextured2(rectangle *rect)
{
    GLfloat t[8] = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    GLfloat v[8] = { primScreenToGLX(rect->x0), primScreenToGLY(rect->y1 - 1),
                     primScreenToGLX(rect->x1), primScreenToGLY(rect->y1 - 1),
                     primScreenToGLX(rect->x0), primScreenToGLY(rect->y0),
                     primScreenToGLX(rect->x1), primScreenToGLY(rect->y0) };

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    rndTextureEnvironment(RTE_Replace);
    rndTextureEnable(TRUE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, v);
    glTexCoordPointer(2, GL_FLOAT, 0, t);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    rndTextureEnable(FALSE);
    rndTextureEnvironment(RTE_Modulate);
}

//don't mind if this is inefficient as it only
//gets called once per image anyway
void hrDrawFile(char* filename, sdword x, sdword y)
{
    udword handle;
    rectangle rect;
    lifheader* lif = trLIFFileLoad(filename, Pyrophoric);

    rndTextureEnable(TRUE);
    rndAdditiveBlends(FALSE);
    glccEnable(GL_BLEND);

    glGenTextures(1, &handle);
    trClearCurrent();
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lif->width, lif->height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, lif->data);
    
    // FIXME: there's no LiF scaling here, just translation of the image
    x = hrScaleMissionLoadingScreens
      ? feResRepositionScaledX(x)
      : feResRepositionCentredX(x);
      
    y = hrScaleMissionLoadingScreens
      ? feResRepositionScaledY(y)
      : feResRepositionCentredY(y);
    
    x -= lif->width  >> 1;
    y -= lif->height >> 1;
    rect.x0 = x;
    rect.y0 = y;
    rect.x1 = x + lif->width;
    rect.y1 = y + lif->height;
    hrRectSolidTextured2(&rect);

    glDeleteTextures(1, &handle);
    memFree(lif);

    glccDisable(GL_BLEND);
}

void hrDrawBackground(void)
{
    rndClearToBlack();

    if (hrBackgroundTexture)
    {
        real32 x = -((real32)hrBackXSize / (real32)MAIN_WindowWidth);
        real32 y = -((real32)hrBackYSize / (real32)MAIN_WindowHeight);
        GLfloat v[8], t[8];

        sdword oldTex = rndTextureEnable(TRUE);
        udword oldMode = rndTextureEnvironment(RTE_Replace);
        bool cull = glccIsEnabled(GL_CULL_FACE) ? TRUE : FALSE;
        bool blend = glccIsEnabled(GL_BLEND) ? TRUE : FALSE;
        glccDisable(GL_CULL_FACE);
        glccEnable(GL_BLEND);

        trClearCurrent();
        glBindTexture(GL_TEXTURE_2D, hrBackgroundTexture);

        t[0] = 0.0f;        t[1] = 0.0f;
        t[2] = hrBackXFrac; t[3] = 0.0f;
        t[4] = 0.0f;        t[5] = hrBackYFrac;
        t[6] = hrBackXFrac; t[7] = hrBackYFrac;

        v[0] = primScreenToGLX(hrScaleMissionLoadingScreens ? feResRepositionScaledX(0) : feResRepositionCentredX(0));
        v[1] = primScreenToGLY(hrScaleMissionLoadingScreens ? feResRepositionScaledY(0) : feResRepositionCentredY(0));
        v[2] = primScreenToGLX(hrScaleMissionLoadingScreens ? feResRepositionScaledX(640) : feResRepositionCentredX(640));
        v[3] = primScreenToGLY(hrScaleMissionLoadingScreens ? feResRepositionScaledY(0) : feResRepositionCentredY(0));
        v[4] = primScreenToGLX(hrScaleMissionLoadingScreens ? feResRepositionScaledX(0) : feResRepositionCentredX(0));
        v[5] = primScreenToGLY(hrScaleMissionLoadingScreens ? feResRepositionScaledY(480) : feResRepositionCentredY(480));
        v[6] = primScreenToGLX(hrScaleMissionLoadingScreens ? feResRepositionScaledX(640) : feResRepositionCentredX(640));
        v[7] = primScreenToGLY(hrScaleMissionLoadingScreens ? feResRepositionScaledY(480) : feResRepositionCentredY(480));

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, t);
        glVertexPointer(2, GL_FLOAT, 0, v);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        rndTextureEnvironment(oldMode);
        rndTextureEnable(oldTex);
        if (cull) glccEnable(GL_CULL_FACE);
        if (!blend) glccDisable(GL_BLEND);
    }
}

void hrShutdownBackground(void)
{
    // free the memory required by the cached background bitmap

    if (hrBackgroundTexture != 0)
    {
        glDeleteTextures(1, &hrBackgroundTexture);
        hrBackgroundTexture = 0;
    }
}

void hrAbortLoadingYes(char *name, featom *atom)
{
    hrAbortLoadingGame = TRUE;

    if (multiPlayerGame)
        SendDroppingOutOfLoad(sigsPlayerIndex);

    if (hrAbortLoadConfirm)
    {
        feScreenDelete(hrAbortLoadConfirm);
        hrAbortLoadConfirm = NULL;
    }
}

void hrAbortLoadingNo(char *name, featom *atom)
{
    if (hrAbortLoadConfirm)
    {
        feScreenDelete(hrAbortLoadConfirm);
        hrAbortLoadConfirm = NULL;
    }
}

void hrChatTextEntry(char *name, featom *atom)
{
    char *string;
    ChatPacket temp;
    sdword     width;
    fonthandle fhsave;
    char  testwidth[MAX_CHATSTRING_LENGTH+40];

    if (FEFIRSTCALL(atom))
    {
        // initialize button here
        ChatTextEntryBox = (textentryhandle)atom->pData;
        uicTextEntryInit(ChatTextEntryBox,UICTE_NoLossOfFocus|UICTE_ChatTextEntry);
        uicTextBufferResize(ChatTextEntryBox,MAX_CHATSTRING_LENGTH-2);
        return;
    }

    switch (uicTextEntryMessage(atom))
    {
        case CM_AcceptText :
            string = ((textentryhandle)atom->pData)->textBuffer;
            sendChatMessage(ALL_PLAYER_MASK^PLAYER_MASK(sigsPlayerIndex),string,(uword)sigsPlayerIndex);
            dbgMessagef("text entry: %s",string);
            strcpy(temp.message,string);
            temp.packetheader.frame = (uword)sigsPlayerIndex;
            hrProcessPacket((struct ChatPacket *)&temp);
            uicTextEntrySet(ChatTextEntryBox,"",0);
        break;
        case CM_KeyPressed :
            fhsave = fontMakeCurrent(((textentryhandle)atom->pData)->currentFont); //select the appropriate font
            sprintf(testwidth, "%s >  %s", playerNames[sigsPlayerIndex], ((textentryhandle)atom->pData)->textBuffer);
            width = fontWidth(testwidth);
            fontMakeCurrent(fhsave);
            if (width > (atom->width-30))
            {
                uicBackspaceCharacter((textentryhandle)atom->pData);
            }
        break;
    }
}

HorseRaceBars horseBarInfo;
extern bool8 etgHasBeenStarted;

void horseGetNumBars(HorseRaceBars *horsebars)
{
    sdword i;
    bool enablebar[MAX_POSSIBLE_NUM_BARS];
    real32 totalperc;

    dbgAssertOrIgnore(horseTotalNumBars.numBars > 0);
    for (i=0;i<horseTotalNumBars.numBars;i++)
    {
        switch (i)
        {
            case DOWNLOADMAP_BAR:
                enablebar[i] = autodownloadmapRequired();
                break;

            case UNIVERSE_BAR:
                enablebar[i] = TRUE;
                break;

            case ETG_BAR:
                enablebar[i] = etgHasBeenStarted ? FALSE : TRUE;
                break;

            case TEXTURE1_BAR:
            case TEXTURE2_BAR:
#if TR_NIL_TEXTURE
                enablebar[i] = GLOBAL_NO_TEXTURES ? FALSE : TRUE;
#else
                enablebar[i] = TRUE;
#endif
                break;

            default:
                dbgFatalf(DBG_Loc,"You must specify whether or not to load bar %i here",i);
                break;
        }
    }

    horsebars->numBars = 0;

    // now, based on enablebar[i], selectively add bars that will be loaded:

    for (i=0;i<horseTotalNumBars.numBars;i++)
    {
        if (enablebar[i])
        {
            horsebars->perc[horsebars->numBars++] = horseTotalNumBars.perc[i];
        }
    }

    dbgAssertOrIgnore(horsebars->numBars > 0);

    // now renormalize percentages to 1.0
    for (totalperc = 0.0f,i=0;i<horsebars->numBars;i++)
    {
        totalperc += horsebars->perc[i];
    }

    dbgAssertOrIgnore(totalperc > 0.0f);
    if (totalperc != 1.0f)
    {
        for (i=0;i<horsebars->numBars;i++)
        {
            horsebars->perc[i] /= totalperc;
        }
    }
}

void horseRaceInit()
{
    udword i;
    horseGetNumBars(&horseBarInfo);

    //initialize current bar to the 0th bar
    localbar=0;

    for(i=0;i<MAX_MULTIPLAYER_PLAYERS;i++)
    {
        horseracestatus.barnum[i] = 0;
        horseracestatus.percent[i] = 0;
        horseracestatus.hrstatusstr[i][0] = 0;
        if (autodownloadmapRequired())
            TTimerStart(&hrPlayerDropoutTimers[i],HorseRacePlayerDropoutTime*2.0f);     // give double time for autodownloads
        else
            TTimerStart(&hrPlayerDropoutTimers[i],HorseRacePlayerDropoutTime);
        PlayersAlreadyDrawnDropped[i] = FALSE;
    }

    for (i=0;i<NUM_CHAT_LINES;i++)
    {
        chathistory[i].message[0] = 0;
    }

    listInit(&horseCrapRegion.cutouts);

    JustInit = TRUE;
    startTimeRef   = SDL_GetTicks();
    lastRenderTime = 0;

    if (!hrScreensHandle)
    {
        feCallbackAddMultiple(hrCallBack);
        feDrawCallbackAddMultiple(hrDrawCallback);
        hrScreensHandle = feScreensLoad(HR_FIBFile);
    }

    mouseCursorHide();

    if (singlePlayerGame)
    {
        hrBaseRegion = feScreenStart(&horseCrapRegion, HR_SingleRaceScreen);
    }
    else
    {
        hrBaseRegion = feScreenStart(&horseCrapRegion, (multiPlayerGame) ? HR_RaceScreen : HR_RaceScreenNotNetwork);
    }

    playernamefont = frFontRegister(HR_PlayerNameFont);

    hrRunning=TRUE;
}

void horseRaceShutdown()
{
    mouseCursorShow();

    for(sdword i=0;i<MAX_MULTIPLAYER_PLAYERS;i++)
    {
        TTimerClose(&hrPlayerDropoutTimers[i]);
    }

    hrRunning=FALSE;
    if (hrAbortLoadConfirm)
    {
        feScreenDelete(hrAbortLoadConfirm);
        hrAbortLoadConfirm = NULL;
    }

    feScreenDelete(hrBaseRegion);

    hrShutdownBackground();
    hrProgressCounter = 0;
    
    hrBaseRegion = NULL;
    hrProgressRegion = NULL;
    hrChatBoxRegion = NULL;
    ChatTextEntryBox = NULL;
    
}

void horseRaceWaitForNetworkGameStartShutdown()
{
    if (multiPlayerGame)
    {
        while (!multiPlayerGameUnderWay) ;
    }
}

void HorseRaceBeginBar(uword barnum)
{
    if(JustInit)
    {
        JustInit = FALSE;
    }
    else
    {
        localbar++;
    }

    //send packet
    HorsePacket packet = {
        .packetheader.type = PACKETTYPE_HORSERACE,
        .playerindex       = (uword) sigsPlayerIndex,
        .barnum            = (uword) localbar,
        .percent           = horseRaceGetPacketPercent(0.0f),
    };

    if (multiPlayerGame)
    {
        SendHorseRacePacket((ubyte *)&packet,sizeof(HorsePacket));
    }
    else
    {
        recievedHorsePacketCB((ubyte *)&packet,sizeof(HorsePacket));
    }
}

extern void mousePoll();
extern udword regRegionProcess(regionhandle reg, udword mask);
extern sdword regProcessingRegions;
extern sdword regRenderEventIndex;

void hrUncleanDecorative(void)
{
    if (hrDecRegion != NULL)
    {
        hrDecRegion->drawFunction = ferDrawDecorative;
    }
}

void horseRaceRender()
{
    regRenderEventIndex = 0;

/*   if (demDemoRecording)
    {
        memcpy((ubyte *)&keyScanCode[0], (ubyte *)&keySaveScan[0], sizeof(keyScanCode));//freeze a snapshot of the key state
        demStateSave();
    }
    else if (demDemoPlaying)
    {
        demStateLoad();
    }*/

    // Make sure the Homeworld text gets drawn on the correct frames
    regRecursiveSetDirty(&horseCrapRegion);
    hrDecRegion = NULL;

    if (TitanActive)
        titanPumpEngine();

    if (!SDL_PollEvent(0))
    {
        regProcessingRegions = TRUE;
        regRegionProcess(horseCrapRegion.child, 0xffffffff);
        regProcessingRegions = FALSE;
        if (ChatTextEntryBox!=NULL)
        {
            bitSet(ChatTextEntryBox->reg.status,RSF_KeyCapture);
            keyBufferClear();
        }
    }
    else
    {
        regRegionProcess(horseCrapRegion.child, 0xffffffff);
        while (SDL_PollEvent(0))
        {
            SDL_Event e;
            if (SDL_WaitEvent(&e))
            {
                HandleEvent(&e);

                if (multiPlayerGame)
                {
                    if (keyIsStuck(ESCKEY))
                    {
                        keyClearSticky(ESCKEY);                      //clear the sticky bit
                        if (!hrAbortLoadConfirm)
                        {
                            if (!hrAbortLoadingGame)    // if not already aborting
                            {
                                hrAbortLoadConfirm = feScreenStart(&horseCrapRegion, "AbortLoadConfirm");
                            }
                        }
                        else
                        {
                            feScreenDelete(hrAbortLoadConfirm);
                            hrAbortLoadConfirm = NULL;
                        }
                    }
                }

                regProcessingRegions = TRUE;
                if (hrAbortLoadConfirm!=NULL)
                {
                    ;
                }
                else if (ChatTextEntryBox!=NULL)
                {
                    regRegionProcess(&ChatTextEntryBox->reg, 0xffffffff);
                }
                regProcessingRegions = FALSE;
            }

            if (ChatTextEntryBox!=NULL)
            {
                bitSet(ChatTextEntryBox->reg.status,RSF_KeyCapture);
                keyBufferClear();
            }

            if (TitanActive)
                titanPumpEngine();

            SDL_Delay(0);
        }
    }

    // If background isn't loaded yet, do it now
    if (hrBackgroundTexture == 0)
    {
        hrInitBackground();
    }

    // When there's no background loaded yet, it fills the screen with black
    hrDrawBackground();

    regFunctionsDraw();                                //render all regions
    primErrorMessagePrint();
    hrUncleanDecorative();
    rndFlush();
    primErrorMessagePrint();
}

//function that returns the doctored percentage for conversion of
//multiple bars to a single bar.

real32 horseRaceGetPacketPercent(real32 barPercent)
{
    real32 percent=0.0f;
    for(sdword i=0;i<localbar;i++)
    {
        //add previous bar percentiles...
        percent += horseBarInfo.perc[i];
    }
    //add on current bars contribution scaled by appropriate ammount
    percent += barPercent*horseBarInfo.perc[localbar];
    return percent;
}

bool HorseRaceNext(real32 percent)
{
    percent = max( percent, 0.0f );
    percent = min( percent, 1.0f );

    if (TitanActive)
        titanPumpEngine();
    
    HorsePacket packet = {
        .packetheader.type = PACKETTYPE_HORSERACE,
        .playerindex       = (uword) sigsPlayerIndex,
        .barnum            = (uword) localbar,
        .percent           = horseRaceGetPacketPercent(percent)
    };

    if ( ! multiPlayerGame) {
        recievedHorsePacketCB((ubyte *)&packet, sizeof(HorsePacket));
    }

    if (multiPlayerGame) {
        const udword timeNow     = SDL_GetTicks();
        const udword timeDelta   = timeNow - hrLastPacketSendTime;
        const bool   timeElapsed = timeDelta >= HR_PacketRateLimitTime;
        const bool   alwaysSend  = (startingGameState == AUTODOWNLOAD_MAP) && (sigsPlayerIndex == 0); // always when autouploading map

        if (timeElapsed || alwaysSend) {
            SendHorseRacePacket((ubyte *)&packet, sizeof(HorsePacket));
            hrLastPacketSendTime = timeNow;
        }

        if (timeElapsed) {
            for (sdword i=0;i<sigsNumPlayers;i++) {
                if (i != sigsPlayerIndex) // don't check myself
                if (!playerHasDroppedOutOrQuit(i))
                if (TTimerUpdate(&hrPlayerDropoutTimers[i]))
                    PlayerDroppedOut(i,TRUE);
            }

            if (localbar == (horseBarInfo.numBars - 1)) {
                for (sdword i=0;i<sigsNumPlayers;i++) {
                    if (!playerHasDroppedOutOrQuit(i))
                    if (horseracestatus.percent[i] < 0.999f)
                        return FALSE;
                }
            }
        }
    }

    // Get the overall loading progress
    const real32 overallRatio = horseRaceGetPacketPercent( percent );

    // Don't try to render too fast. It does take some time to render things, and we don't want to slow down /too/ much.
    const udword lastTimeMin   = HR_MinRenderDelta;
    const udword lastTimeRef   = SDL_GetTicks();
    const udword lastTimeDelta = lastRenderTime - lastTimeRef;
    const bool   isFirstRender = lastRenderTime == 0;
    const bool   isComplete    = overallRatio >= 0.99f;

    // Render decision
    if (lastTimeDelta >= lastTimeMin // Render if enough time elapsed
    || isFirstRender                 // Render if the bar is empty
    || isComplete) {                 // Render if the bar is full
        horseRaceRender();
        lastRenderTime = SDL_GetTicks();
    }

    // Based on overall loading progress, apply a delay to meet the minimum time.
    // For example if we're 75% done, we should also be 75% of the way through the minimum load time.
    // The progress updates are very fine-grained so extrapolating or compensating for gaps in time isn't necessary.
    const udword timeNow        = SDL_GetTicks();
    const udword timeDelta      = timeNow - startTimeRef;
    const udword timeDeltaMin   = max( HR_MinOverallTime, opLoadTimeMinMs );
    const real32 timeRatio      = min( 1.0f, (real32) timeDelta / (real32) timeDeltaMin );
    const real32 timeRatioError = overallRatio - timeRatio;
    const bool   isProgLeading  = timeRatioError > 0.0f;

    // If the progress ratio is leading the target time, add a proportional delay to compensate.
    if (isProgLeading) {
        const udword timeDeltaError = (udword) ceilf((real32) timeDeltaMin * timeRatioError);
        SDL_Delay( timeDeltaError );
    }

    return TRUE;
}

void hrProcessPacket(struct ChatPacket *packet)
{
    ChatPacket* cp   = (ChatPacket *)packet;
    bool        done = FALSE;

    for (sdword i=0;i<NUM_CHAT_LINES;i++)
    {
        if (chathistory[i].message[0]==0)
        {
            strcpy(chathistory[i].message,cp->message);
            chathistory[i].packetheader.frame = cp->packetheader.frame;
            done = TRUE;
            break;
        }
    }

    if (!done)
    {
        for (sdword i=0;i<NUM_CHAT_LINES-1;i++)
        {
            chathistory[i] = chathistory[i+1];
            strcpy(chathistory[i].message,chathistory[i+1].message);
        }
        strcpy(chathistory[NUM_CHAT_LINES-1].message,cp->message);
        chathistory[NUM_CHAT_LINES-1].packetheader.frame = cp->packetheader.frame;
    }

    hrDirtyChatBox();
}

void recievedHorsePacketCB(ubyte *packet,udword sizeofpacket)
{
    //captain should have recieved a packet here...
    HorsePacket *hp = (HorsePacket *)packet;
    udword i = hp->playerindex;
    dbgAssertOrIgnore(i < MAX_MULTIPLAYER_PLAYERS);

    if (multiPlayerGame)
    {
        titanDebug("hr: recv hrpacket from %d status %d",i,playersReadyToGo[i]);
        if (!playerHasDroppedOutOrQuit(i))
        {
            TTimerReset(&hrPlayerDropoutTimers[i]);
        }
    }

    hrDirtyProgressBar();

    horseracestatus.barnum[i]  = hp->barnum;
    horseracestatus.percent[i] = hp->percent;
}

