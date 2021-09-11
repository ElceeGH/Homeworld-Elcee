// =============================================================================
//  Animatic.c
//  - playback of animatics via OpenGL
// =============================================================================
//  Copyright Relic Entertainment, Inc. All rights reserved.
//  Created 2/11/1999 by khent
// =============================================================================

#include "Animatic.h"

#include "File.h"
#include "glinc.h"
#include "mouse.h"
#include "NIS.h"
#include "SinglePlayer.h"
#include "SoundEvent.h"
#include "soundlow.h"
#include "StringSupport.h"
#include "Subtitle.h"
#include "Universe.h"
#include "video.h"



// Decrementing counter for delaying purposes used by the renderer.
sdword animaticJustPlayed = 0;  

typedef struct animlst {
    char filename[32];
} animlst;

static animlst    animlisting[NUMBER_SINGLEPLAYER_MISSIONS];
static nisheader* animScriptHeader = NULL;
static sdword     animCurrentEvent;
static real32     animTimeElapsed;
static real32     animPreviousSFXVolume;
static real32     animPreviousSpeechVolume;
static real32     animPreviousMusicVolume;
static sdword     animSubY1;

//for localization
extern char *nisLanguageSubpath[];



/*-----------------------------------------------------------------------------
    Name        : animAviEnd
    Description : cleanup after playing a Avi video file as an animatic
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void animCleanup( void )
{
    universe.totaltimeelapsed = animTimeElapsed;
    soundEventStopMusic(0.0f);

    mouseCursorShow();
    rndClear();

    soundstopall(0.0);
    speechEventCleanup();
    subReset();

    soundEventMusicVol ( animPreviousMusicVolume  );
    soundEventSpeechVol( animPreviousSpeechVolume );
    soundEventSFXVol   ( animPreviousSFXVolume    );

    animaticJustPlayed = 8;
}



/*-----------------------------------------------------------------------------
    Name        : animSubtitlesSetup
    Description : setup / reset the GL for displaying subtitles
    Inputs      : on - TRUE or FALSE, setup or reset
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void animSubtitlesSetup(bool on)
{
    static GLint matrixMode;
    static GLfloat projection[16];

    if (on)
    {
        glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
        glGetFloatv(GL_PROJECTION_MATRIX, projection);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
    }
    else
    {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection);

        glMatrixMode(matrixMode);
    }
}



/*-----------------------------------------------------------------------------
    Name        : animSubtitlesClear
    Description : clear (to black) the region that subtitles appear in
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void animSubtitlesClear(void)
{
    if (animSubY1 < 0 || animSubY1 > (MAIN_WindowHeight / 2))
    {
        return;
    }

    animSubtitlesSetup(TRUE);

    //rectangle r;
    //r.x0 = -1;
    //r.y0 =  0;
    //r.x1 = MAIN_WindowWidth;
    //r.y1 = (animSubY1 > 128) ? animSubY1 : 128;
    //primRectSolid2(&r, colBlack);

    animSubtitlesSetup(FALSE);
}



/*-----------------------------------------------------------------------------
    Name        : animSubtitlesDraw
    Description : render any subtitles
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void animSubtitlesDraw(void)
{
    animSubtitlesSetup(TRUE);

    animSubY1 = 0;

    for (sdword index = 0; index < SUB_NumberRegions; index++)
    {
        if (subRegion[index].bEnabled && subRegion[index].cardIndex > 0)
        {
            subTimeElapsed = &universe.totaltimeelapsed;
            subTitlesDraw(&subRegion[index]);
            if (subRegion[index].rect.y1 > animSubY1)
            {
                animSubY1 = subRegion[index].rect.y1;
            }
        }
    }

    animSubtitlesSetup(FALSE);
}



/*-----------------------------------------------------------------------------
    Name        : animLoadNISScript
    Description : process an NIS script file containing time-keyed events
    Inputs      : scriptname - name of script file to process
    Outputs     :
    Return      : nisheader
----------------------------------------------------------------------------*/
static nisheader* animLoadNISScript(char* scriptname)
{
    animCurrentEvent = 0;

    nisheader* newHeader = memAlloc(sizeof(nisheader), "animatic NIS header", NonVolatile);
    memset(newHeader, 0, sizeof(nisheader));

    newHeader->iLookyObject = -1;
    newHeader->length = NIS_FrameRate * 10000;
//    if (scriptname != NULL && fileExists(scriptname, 0))
    if (scriptname != NULL)
    {
        // for localization
        char localisedPath[256];
		strcpy(localisedPath, scriptname);
        if (strCurLanguage >= 1)
        {
            for (char* pString = localisedPath + strlen(localisedPath); pString > localisedPath; pString--)
            {                                               //find the end of the path
                if (*pString == '/')
                {
                    char string[256];
                    strcpy(string, pString + 1);            //save the file name
                    strcpy(pString + 1, nisLanguageSubpath[strCurLanguage]);//add the language sub-path
                    strcat(pString, string);                //put the filename back on
                    break;
                }
            }
        }

		if (fileExists(localisedPath, 0))
		{
			nisEventIndex = 0;
			nisCurrentHeader = newHeader;
			scriptSet(NULL, localisedPath, nisScriptTable);     //load in the script file
//        scriptSet(NULL, scriptname, nisScriptTable);
			nisCurrentHeader = NULL;
			newHeader->nEvents = nisEventIndex;
			if (newHeader->nEvents != 0)
			{
				newHeader->events = nisEventList;
				qsort(newHeader->events, newHeader->nEvents, sizeof(nisevent), nisEventSortCB);
				nisEventList = NULL;
			}
			else
			{
				newHeader->events = NULL;
			}
		}
		else
		{
			// file doesn't exist
			newHeader->nEvents = 0;
			newHeader->events = NULL;
		}
    }
    else
    {
		// script name = null
        newHeader->nEvents = 0;
        newHeader->events = NULL;
    }
    return newHeader;
}



/*-----------------------------------------------------------------------------
    Name        : animStartup
    Description : reads animatics.lst from the Movies directory
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void animStartup(void)
{
    memset(animlisting, 0, NUMBER_SINGLEPLAYER_MISSIONS*sizeof(animlst));

    if (!fileExists("Movies/animatics.lst", 0))
    {
        return;
    }

    
    char line[128];
    filehandle lst = fileOpen("Movies/animatics.lst", FF_TextMode);
    while (fileLineRead(lst, line, sizeof(line)-1) != FR_EndOfFile)
    {
        if (strlen(line) < 5 || line[0] == ';' || line[0] == '/')
        {
            continue;
        }

        sdword level;
        char temp[64];
        sscanf(line, "%d %s", &level, temp);
        dbgAssertOrIgnore(level >= 0 && level < NUMBER_SINGLEPLAYER_MISSIONS);
        memStrncpy(animlisting[level].filename, temp, 31);
    }
    fileClose(lst);
}



/*-----------------------------------------------------------------------------
    Name        : animShutdown
    Description : releases memory required by the animatics listing file
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void animShutdown(void)
{
    memset( animlisting, 0x00, sizeof(animlisting) );
}



/*-----------------------------------------------------------------------------
    Name        : animUpdateCallback
    Description : callback once per frame decode
    Inputs      : frame - current frame number of animation
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void animUpdateCallback( VideoStatus status ) {
    if (animScriptHeader == NULL)
        return;

    const real32 timeElapsed = (real32)status.frameIndex / status.frameRate;
    universe.totaltimeelapsed = timeElapsed;

    nisevent* event = &animScriptHeader->events[ animCurrentEvent ];
    while (animCurrentEvent < animScriptHeader->nEvents &&
           event->time <= timeElapsed)
    {
        dbgAssertOrIgnore(nisEventDispatch[event->code] != NULL);
        nisEventDispatch[event->code](NULL, event);
        animCurrentEvent++;
        event++;
    }

    speechEventUpdate();
    subTitlesUpdate();
}



/*-----------------------------------------------------------------------------
    Name        : animRenderCallback
    Description : display callback for a frame of video
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void animRenderCallback( VideoStatus video ) {
    animSubtitlesClear();
    animSubtitlesDraw();
}



static bool animSetup( sdword a, sdword b ) {
    // Sanity check indexes
    dbgAssertOrIgnore(a >= 0);
    dbgAssertOrIgnore(b < NUMBER_SINGLEPLAYER_MISSIONS);

    // Global counter reset.
    animaticJustPlayed = 0;

    // Don't show animatics when entering tutorials.
    if (tutorial == 1)
        return FALSE;

    // Reset subtitles.
    subReset();

    // If there's no listing for this, don't try to play anything.
    if (animlisting[a].filename[0] == '\0')
        return FALSE;

    // Load animatic script.
    char scriptFile[ PATH_MAX ];
    snprintf( scriptFile, sizeof(scriptFile), "Movies/%s.script", animlisting[a].filename );
    animScriptHeader = animLoadNISScript( scriptFile );

    // Stop sounds
    soundEventStopMusic(0.0f);
    soundstopall(0.0f);

    // Clear the screen (todo: remove this, it's pointless? Then again I don't actually clear the screen, do I...)
    rndSetClearColor(colBlack);
    rndClear();

    // Hide the cursor for da movies
    mouseCursorHide();

    // Pause time for the universe.
    animTimeElapsed = universe.totaltimeelapsed;
    universe.totaltimeelapsed = 0.0f;

    // Save the previous volume settings
    soundEventGetVolume( &animPreviousSFXVolume, &animPreviousSpeechVolume, &animPreviousMusicVolume );

    // Hey, we got this far.
    return TRUE;
}



/*-----------------------------------------------------------------------------
    Name        : animPlay
    Description : plays a Bink video file
    Inputs      : a, b - levels this video is playing between
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void animPlay(sdword a, sdword b)
{
    // Set up the animatic script, subtitles etc.
    if ( ! animSetup( a, b ))
        return;

    // Create the video path.
    char videoFile[ PATH_MAX ];
    snprintf( videoFile, sizeof(videoFile), "Movies/%s.bik", animlisting[a].filename );

    // Play the video and run callbacks as we go.
    videoPlay( videoFile, animUpdateCallback, animRenderCallback, TRUE );

    // Clean up now that the video is finished.
    animCleanup();
}

