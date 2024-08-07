// =============================================================================
//  Animatic.c
//  - playback of animatics via OpenGL
// =============================================================================
//  Copyright Relic Entertainment, Inc. All rights reserved.
//  Created 2/11/1999 by khent
// =============================================================================

#include "Animatic.h"

#include "File.h"
#include "mouse.h"
#include "NIS.h"
#include "SinglePlayer.h"
#include "SoundEvent.h"
#include "SoundEventPrivate.h" // musicEventUpdateVolume required
#include "soundlow.h"
#include "StringSupport.h"
#include "Subtitle.h"
#include "Universe.h"
#include "video.h"
#include "rStateCache.h"



/// Animatic list entry
typedef struct animlst {
    char filename[32];
} animlst;



// Globals
sdword animaticJustPlayed = 0;     ///< Decrementing counter for delaying purposes used by the renderer.
sdword animaticIsPlaying  = FALSE; ///< For sound hackery.

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
        glccGetIntegerv(GL_MATRIX_MODE, &matrixMode);
        glccGetFloatv(GL_PROJECTION_MATRIX, projection);

        glccMatrixMode(GL_PROJECTION);
        glccLoadIdentity();

        glccMatrixMode(GL_MODELVIEW);
        glccPushMatrix();
        glccLoadIdentity();
    }
    else
    {
        glccMatrixMode(GL_MODELVIEW);
        glccPopMatrix();

        glccMatrixMode(GL_PROJECTION);
        glccLoadMatrixf(projection);

        glccMatrixMode(matrixMode);
    }
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

    // Get current elapsed time for the animatic (in seconds)
    const real32 timeElapsed = (real32)status.frameIndex / status.frameRate;

    // Search for events to trigger based on their index and time
    for (udword i=animCurrentEvent; i<animScriptHeader->nEvents; i++) {
        nisevent* event = &animScriptHeader->events[ i ];

        if (event->time <= timeElapsed) {
            dbgAssertOrIgnore(nisEventDispatch[event->code] != NULL);
            universe.totaltimeelapsed = timeElapsed;
            nisEventDispatch[event->code](NULL, event);
            animCurrentEvent++;
        }
    }

    universe.totaltimeelapsed = timeElapsed;
    musicEventUpdateVolume();
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
    animSubtitlesDraw();
}



static bool animSetup( sdword a, sdword b ) {
    // Sanity check indexes
    dbgAssertOrIgnore(a >= 0);
    dbgAssertOrIgnore(b < NUMBER_SINGLEPLAYER_MISSIONS);

    // Global counter reset.
    animaticIsPlaying  = FALSE;
    animaticJustPlayed = 0;
    animCurrentEvent   = 0;

    // Don't show animatics when entering tutorials.
    if (tutorial == 1)
        return FALSE;

    // Reset subtitles.
    soundstopall(0.0);
    speechEventUpdate();
    speechEventCleanup();
    subReset();

    // If there's no listing for this, don't try to play anything.
    if (animlisting[a].filename[0] == '\0')
        return FALSE;

    // Load animatic script.
    char scriptFile[ PATH_MAX ];
    snprintf( scriptFile, sizeof(scriptFile), "Movies/%s.script", animlisting[a].filename );
    animScriptHeader = animLoadNISScript( scriptFile );

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
    animaticIsPlaying = TRUE;
    return TRUE;
}



/*-----------------------------------------------------------------------------
    Name        : animCleanup
    Description : cleanup after playing a video file as an animatic
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void animCleanup( void )
{
    universe.totaltimeelapsed = animTimeElapsed;

    mouseCursorShow();
    rndSetClearColor(colBlack);
    rndClear();
    rndFlush();

    soundstopall(0.0);
    speechEventUpdate();
    speechEventCleanup();
    subReset();

    soundEventMusicVol ( animPreviousMusicVolume  );
    soundEventSpeechVol( animPreviousSpeechVolume );
    soundEventSFXVol   ( animPreviousSFXVolume    );

    animaticIsPlaying  = FALSE;
    animaticJustPlayed = 8;
}



/*-----------------------------------------------------------------------------
    Name        : animPlay
    Description : plays an animatic
    Inputs      : a, b - levels this video is playing between
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

