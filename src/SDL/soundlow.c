/*=============================================================================
    Name    : SoundLow.c
    Purpose : Low level sound routines

    Created 7/24/1997 by gshaw
    Copyright Relic Entertainment, Inc.  All rights reserved.

    CUT CUT CUT CUT CUT OMG WTF HAVE I DONE!?!#@
=============================================================================*/

#include <string.h>
#include "Switches.h"
#include "Debug.h"
#include "soundlow.h"
#include "File.h"
#include "soundcmn.h"
#include "main.h"

#define SOUNDFXDIR "SoundFX/"
#define EQ_STEP			0.1
#define SDL_BUFFERSIZE  FQ_SIZE

typedef struct BANKPOINTERS {
    void *start;
    void *end;
} BANKPOINTERS;


void musicEventUpdateVolume(void);

/* internal functions */
static sdword SNDgetchannel(sword patchnum, sdword priority);


/* Globals */
bool soundinited       = FALSE;
bool bSoundPaused      = FALSE;
bool bSoundDeactivated = FALSE;

/* Shared by mixer, sstream, soundlow */
SOUNDCOMPONENT mixer;
SOUNDCOMPONENT streamer;
CHANNEL        channels[SOUND_MAX_VOICES];
sdword         soundnumvoices = SOUND_DEF_VOICES;
sdword         soundvoicemode = SOUND_MODE_NORM; // voice panic mode, normal by default

/* Locals */
static sword        numpatches = 0;
static sdword	    lasthandle = 0;
static BANK *       bank;
static PATCH *      patches;
static sdword       channelsinuse = 0;
static real32       masterEQ[FQ_SIZE];
static sdword       numbanks = 0;
static sdword       numchans[4] = {0,0,0,0};
static BANKPOINTERS bankpointers[4];

/* Imports */
extern real32 cardiod[];
extern udword mixerticks;



// Get the min and max number of voices
void soundGetVoiceLimits(sdword *min,sdword *max)
{
    *min = SOUND_MIN_VOICES;
    *max = SOUND_MAX_VOICES;
}

// Get the current number of voices and mode
void soundGetNumVoices(sdword *num,sdword *mode)
{
    *num  = soundnumvoices;
    *mode = soundvoicemode;
}

// Set the current number of voices and mode
void soundSetNumVoices(sdword num,sdword mode)
{
    num = max( num, SOUND_MIN_VOICES );
    num = min( num, SOUND_MAX_VOICES );
    soundnumvoices = num;
    soundvoicemode = mode;
}



// Called by main.c on before and after[Alt]-[Tab]
// (It was anyway, 20 years ago...)
void sounddeactivate(bool bDeactivate)
{
    /* set flag */
    if (soundinited) {
        bSoundDeactivated=bDeactivate;
        if (! bDeactivate) {
            SDL_PauseAudio(FALSE);
        }
    }
}



/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/
sdword soundinit(void)
{
    // clean up the channels
    for (sdword i = 0; i < soundnumvoices; i++)
        channels[i].status = SOUND_FREE;

    // clean up the masterEQ
    for (sdword i = 0; i < FQ_SIZE; i++)
        masterEQ[i] = 1.0;

    // Reset timeout and status
    mixer   .status  = SOUND_FREE;
    mixer   .timeout = 0;
    streamer.status  = SOUND_FREE;
    streamer.timeout = 0;

    // Set up wave format structure.
    if (isoundmixerinit() != SOUND_OK) {
        dbgMessagef("Unable to init mixer subsystem");
        return SOUND_ERR;
    } else {
        soundinited  = TRUE;
        SDL_PauseAudio(FALSE);
        return SOUND_OK;
    }
}


void soundclose(void)
{
    bSoundPaused = TRUE;
}


/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/
void soundrestore(void)
{
    soundpause(TRUE);
    soundinited = FALSE;
    isoundmixerrestore();
}


void soundpause(bool bPause)
{
    if (soundinited)
    {
        if (bPause)
        {
            mixer   .timeout = mixerticks + SOUND_FADE_MIXER;
            streamer.timeout = mixerticks + SOUND_FADE_MIXER;
        }
        /* No SDL_PauseAudio(TRUE) immediately here; that
           happens in smixer.c after the above timeout */

        bSoundPaused = bPause;

        if (bPause)
        {
            int timeout=SOUND_PAUSE_BREAKOUT;
            soundstopall(SOUND_FADE_STOPALL);

            while ((mixer.status != SOUND_STOPPED) && (--timeout))
            {
                musicEventUpdateVolume();
                SDL_Delay(SOUND_PAUSE_DELAY);
            }
            if (!timeout) {
                dbgMessagef("WARNING: Sound refused to pause in %d*%d ms, forcing exit", SOUND_PAUSE_BREAKOUT, SOUND_PAUSE_DELAY);
            }
        } else {
            SDL_PauseAudio(FALSE);
        }
    }
}

void soundstopallSFX(real32 fadetime, bool stopStreams)
{
    for (sdword i = 0; i < soundnumvoices; i++) {
        if (channels[i].handle > SOUND_DEFAULT) {
            soundstop(channels[i].handle, fadetime);
        }
    }

    if (stopStreams) {
        soundstreamstopall(fadetime);
    }
}

/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
udword soundbankadd(void *bankaddress)
{
    /* point the global bank pointer to the bank header */
    bank = bankaddress;

    /* check bank ID */
    const sdword numpatches = bank->numpatches;

    /* point the global patches pointer to the start of the patches */
    patches = (PATCH *)&bank->firstpatch;

    /* figure out where the patch data starts */
    for (sdword i = 0; i < numpatches; i++) {
        patches[i].dataoffset = (smemsize)bankaddress + patches[i].dataoffset;
        patches[i].loopstart += patches[i].dataoffset;
        patches[i].loopend   += patches[i].dataoffset;
        patches[i].datasize  += patches[i].dataoffset;
    }

    bankpointers[numbanks].start = bankaddress;
    bankpointers[numbanks++].end = (void *)patches[numpatches - 1].datasize;

    return bank->checksum;
}


/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
bool soundover(sdword handle)
{
    if (handle < SOUND_OK) {
        return TRUE;
    }

    const CHANNEL* pchan = &channels[SNDchannel(handle)];

    if (pchan != NULL) {
        if (pchan->handle != handle) {
            return TRUE;
        }
    
        if (pchan->status <= SOUND_STOPPED) {
            return TRUE;
        }
        
        return FALSE;
    }
    
    return TRUE;
}

/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/
sdword soundstop(sdword handle, real32 fadetime)
{
    CHANNEL *pchan;
    sdword channel;
    sdword fadeblocks = 0;

    if (!soundinited)
    {
        return (SOUND_ERR);
    }
    
    channel = SNDchannel(handle);

    if (channel < SOUND_OK)
    {
        return (SOUND_ERR);
    }
    
    pchan = &channels[channel];

    if (pchan->handle != handle)
    {
        return (SOUND_ERR);
    }

    if (pchan->status == SOUND_FREE)
    {
        return (SOUND_ERR);
    }

    if (pchan != NULL)
    {
        if ((pchan->looping) && (pchan->ppatch->datasize > pchan->ppatch->loopend) && (fadetime > 0.0f))
        {
            pchan->status = SOUND_LOOPEND;
        }
        else
        {
            fadeblocks = (sdword)(fadetime * SOUND_FADE_TIMETOBLOCKS);
            
            if (fadeblocks < NUM_FADE_BLOCKS)
            {
                fadeblocks = NUM_FADE_BLOCKS;
            }
        
            pchan->status = SOUND_STOPPING;
            pchan->voltarget = -1;
            pchan->volticksleft = fadeblocks;
            pchan->volfade = (real32)(pchan->voltarget - pchan->volume) / (real32)pchan->volticksleft;

            if (pchan->volfade == 0.0f)
            {
                pchan->volfade = 0.01f;
                if (pchan->voltarget < pchan->volume)
                {
                    pchan->volfade = -0.01f;
                }
            }
        }
        return (SOUND_OK);
    }

    return (SOUND_ERR);
}


/*-----------------------------------------------------------------------------
    Name		: soundrestart
    Description	: If this is a looping sound, it will reset the play pointer
                    to the start of the sound.
    Inputs		: handle - the handle to a sound returned by soundplay
    Outputs		:
    Return		: SOUND_OK if successful, SOUND_ERR on error
----------------------------------------------------------------------------*/	
sdword soundrestart(sdword handle)
{
    CHANNEL *pchan;
    sdword channel;

    if (!soundinited)
    {
        channel = SNDchannel(handle);
        if (channel < SOUND_OK)
        {
            pchan = &channels[channel];

            if (pchan != NULL)
            {
                if (pchan->handle != handle)
                {
                    return (SOUND_ERR);
                }

                if ((pchan->looping == TRUE) && (pchan->status == SOUND_PLAYING))
                {
                    pchan->status = SOUND_RESTART;
                    return (SOUND_OK);
                }
            }
        }
    }

    return (SOUND_ERR);
}


/*-----------------------------------------------------------------------------
    Name		: soundvolume
    Description	:
    Inputs		: handle - the handle to a sound returned by soundplay
                  vol - the volume to set this sound to (range of SOUND_MIN_VOL - SOUND_MAX_VOL)
    Outputs		:
    Return		: SOUND_OK if successful, SOUND_ERR on error
----------------------------------------------------------------------------*/	
sdword soundvolumeF(sdword handle, sword vol, real32 fadetime)
{
    CHANNEL *pchan;
    sdword channel;
    sdword fadeblocks = 0;

    if (!soundinited)
    {
        return (SOUND_ERR);
    }
    
    if (vol > SOUND_VOL_MAX)
    {
        vol = SOUND_VOL_MAX;
    }
    else if (vol <= SOUND_VOL_MIN)
    {
        soundstop(handle, TRUE);
        return (SOUND_OK);
    }

    channel = SNDchannel(handle);

    if (channel < SOUND_OK)
    {
        return (SOUND_ERR);
    }

    pchan = &channels[channel];

    if (pchan != NULL)
    {
        if (pchan->handle != handle)
        {
            return (SOUND_ERR);
        }

        if (vol != pchan->voltarget)
        {
            if (vol == (sword)pchan->volume)
            {
                pchan->voltarget = vol;
                pchan->volticksleft = 0;
                pchan->volfade = 0.0f;
            }
            else
            {
                fadeblocks = (sdword)(fadetime * SOUND_FADE_TIMETOBLOCKS);

                if (fadeblocks < NUM_FADE_BLOCKS)
                {
                    fadeblocks = NUM_FADE_BLOCKS;
                }

                pchan->voltarget = vol;
                pchan->volticksleft = fadeblocks;
dbgAssertOrIgnore(pchan->volticksleft != 0);
                pchan->volfade = (real32)(pchan->voltarget - pchan->volume) / (real32)pchan->volticksleft;

                if (pchan->volfade == 0.0f)
                {
                    pchan->volfade = 0.01f;
                    if (pchan->voltarget < pchan->volume)
                    {
                        pchan->volfade = -0.01f;
                    }
                }
            }
        }
    }

    return (SOUND_OK);
}


/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
sdword soundpanF(sdword handle, sword pan, real32 fadetime)
{
    CHANNEL *pchan;
    sdword channel;
    sdword fadeblocks = 0;

    if (!soundinited)
    {
        return (SOUND_ERR);
    }
    
    channel = SNDchannel(handle);

    if (channel < SOUND_OK)
    {
        return (SOUND_ERR);
    }
    
    pchan = &channels[channel];

    if (pchan != NULL)
    {
        if (pchan->handle != handle)
        {
            return (SOUND_ERR);
        }

        if (pan == pchan->pan)
        {
            pchan->pantarget = pan;
            pchan->panticksleft = 0;
            pchan->panfade = 0;
        }
        else
        {
            fadeblocks = (sdword)(fadetime * SOUND_FADE_TIMETOBLOCKS);

            if (fadeblocks < NUM_FADE_BLOCKS)
            {
                fadeblocks = NUM_FADE_BLOCKS;
            }

            pchan->pantarget = pan;
            pchan->panticksleft = fadeblocks;
            pchan->panfade = (pchan->pantarget - pchan->pan) / pchan->panticksleft;

            if (pchan->panfade > 180)
            {
                pchan->panfade -= (pchan->panfade - 180);
            }
            else if (pchan->panfade < -180)
            {
                pchan->panfade -= (pchan->panfade + 180);
            }

            if (pchan->panfade == 0)
            {
                pchan->panfade = 1;
                if (pchan->pantarget < pchan->pan)
                {
                    pchan->panfade = -1;
                }
            }
        }
    }

    return (SOUND_OK);
}


/*-----------------------------------------------------------------------------
    Name		: soundfrequency
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
sdword soundfrequency(sdword handle, real32 freq)
{
    CHANNEL *pchan;
    sdword channel;

    if (!soundinited) {
        return SOUND_ERR;
    }
    
    channel = SNDchannel(handle);

    if (channel < SOUND_OK) {
        return SOUND_ERR;
    }

    pchan = &channels[channel];

    if (pchan != NULL) {
        if (pchan->handle != handle) {
            return SOUND_ERR;
        }

        if (freq != pchan->pitchtarget) {
            if (freq == pchan->pitch) {
                pchan->pitchtarget    = freq;
                pchan->pitchticksleft = 0;
                pchan->pitchfade      = 0.0f;
            } else {
                pchan->pitchtarget    = freq;
                pchan->pitchticksleft = NUM_FADE_BLOCKS;
                pchan->pitchfade      = (pchan->pitchtarget - pchan->pitch) / pchan->pitchticksleft;
            }
        }
    }

    return SOUND_OK;
}


/*-----------------------------------------------------------------------------
    Name		: soundequalize
    Description	:
    Inputs		: handle - the handle to a sound returned by soundplay
                  eq - array[SOUND_EQ_SIZE] of floats range of 0.0 to 1.0
    Outputs		:
    Return		: SOUND_OK if successful, SOUND_ERR on error
----------------------------------------------------------------------------*/	
sdword soundequalize(sdword handle, real32 *eq)
{
    if (!soundinited) {
        return SOUND_ERR;
    }
    
    if (eq == NULL) {
        return SOUND_ERR;
    }

    sdword channel = SNDchannel(handle);
    if (channel < 0) {
        return SOUND_ERR;
    }

    CHANNEL* pchan = &channels[channel];
    if (pchan != NULL) {
        if (pchan->handle != handle) {
            return SOUND_ERR;
        }

        for (sdword i = 0; i < SOUND_EQ_SIZE; i++) {
            pchan->filter[i] = eq[i];
        }
    }

    return SOUND_OK;
}


/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
sdword soundshipheading(sdword handle, sword heading, sdword highband, sdword lowband, real32 velfactor, real32 shipfactor)
{
    if (!soundinited) {
        return SOUND_ERR;
    }
    
    const sdword channel = SNDchannel(handle);
    if (channel < SOUND_OK) {
        return SOUND_ERR;
    }

    CHANNEL* const pchan = &channels[channel];
    if (pchan == NULL) {
        return SOUND_OK;
    }
    
    if (pchan->heading != handle) {
        return SOUND_ERR;
    }
    
    pchan->heading = heading;

    const real32 factor        = ((cardiod[heading]       - 1.0f) * velfactor * shipfactor + 1.0f);
    const real32 inversefactor = ((cardiod[180 - heading] - 1.0f) * velfactor * shipfactor + 1.0f);

    for (sdword i = 0; i < lowband; i++) {
        pchan->cardiodfilter[i] = factor;
    }

    const real32 diff = (inversefactor - factor) / (highband - lowband);

    for (sdword i = lowband; i < highband; i++) {
        pchan->cardiodfilter[i] = factor + (diff * (i - lowband));
    }

    for (sdword i = highband; i < SOUND_EQ_SIZE; i++) {
        pchan->cardiodfilter[i] = inversefactor;
    }
    
    pchan->usecardiod = TRUE;
    return SOUND_OK;
}



/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
static sdword SNDresetchannel(CHANNEL *pchan)
{
    memset( pchan, 0, sizeof(*pchan) );

    pchan->priority	   = SOUND_PRIORITY_NORMAL;
    pchan->handle	   = SOUND_DEFAULT;
    pchan->numchannels = SOUND_MONO;
    pchan->volfactorL  = 1.0f;
    pchan->volfactorR  = 1.0f;
    
    memset(pchan->filter,        1, SOUND_EQ_SIZE);
    memset(pchan->cardiodfilter, 1, SOUND_EQ_SIZE);
    pchan->usecardiod = FALSE;

    return (SOUND_OK);
}



/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
sdword SNDreleasebuffer(CHANNEL *pchan)
{
    for (sdword i = 0; i < numbanks; i++) {
        if ((bankpointers[i].start <= (void *)pchan->ppatch) &&
            (bankpointers[i].end   >= (void *)pchan->ppatch)) {
            numchans[i]--;
            break;
        }
    }
    
    pchan->handle   = SOUND_DEFAULT;
    pchan->ppatch   = NULL;
    pchan->status   = SOUND_FREE;
    pchan->priority = SOUND_PRIORITY_MIN;

    channelsinuse--;

    return 0;
}



static sdword SNDusefreechannel( sdword priority ) {
    for (sdword i=0; i<soundnumvoices; i++) {
        if (channels[i].status == SOUND_FREE) {
            channels[i].status   = SOUND_INUSE;
            channels[i].priority = priority;
            channelsinuse++;
            return i;
        }
    }

    return SOUND_DEFAULT;
}


/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
static sdword SNDgetchannel(sword patchnum, sdword priority)
{
    sdword channel = SOUND_DEFAULT;

    if ((channelsinuse < soundnumvoices-2) || (priority > SOUND_PRIORITY_MAX)) {// Keep at least 2 voices available
        channel = SNDusefreechannel(priority);
        
        if (channel != SOUND_DEFAULT)
            return channel;
    }

    sdword lowchannel  = SOUND_DEFAULT;
    real32 lowvolume   = (real32)SOUND_VOL_MAX;
    sdword lowticks    = 255;
    sdword lowpriority = max( priority, SOUND_PRIORITY_LOW );

    /* find the channel to steal */
    for (sdword i = 0; i < soundnumvoices; i++) {
        CHANNEL* const pchan = &channels[i];
        if (pchan->status == SOUND_PLAYING) /* don't want one that is already stopping */
        if (pchan->priority     <  lowpriority)
        if (pchan->volume       <= lowvolume)
        if (pchan->volticksleft <= lowticks) {
            lowpriority = pchan->priority;
            lowticks    = pchan->volticksleft;
            lowvolume   = pchan->volume;
            lowchannel  = i;
        }
    }

    if (lowchannel > SOUND_DEFAULT) {
        /* stop the sound with the lowest priority */
        soundstop(channels[lowchannel].handle, SOUND_FADE_STOPNOW);
        
        /* find an empty channel */
        channel = SNDusefreechannel(priority);
    }
    
    return channel;
}


sdword SNDcreatehandle(sdword channel)
{
    if ((channel > SOUND_MAX_VOICES) || (channel < SOUND_OK)) {
        return (SOUND_ERR);
    }

    return ((lasthandle++ << 8) + channel);
}


sdword SNDchannel(sdword handle)
{
    if (handle < SOUND_OK)
        return (SOUND_ERR);

    const sdword channel = handle & 0xFF;
    if ((channel > SOUND_MAX_VOICES) || (channel < SOUND_OK)) {
        return (SOUND_ERR);
    }
    
    return channel;
}


void SNDcalcvolpan(CHANNEL *pchan)
{
    pchan->volfactorL = pchan->volfactorR = pchan->volume / (real32)SOUND_VOL_MAX;
    
         if (pchan->pan < SOUND_PAN_CENTER) pchan->volfactorR *= (real32)(SOUND_PAN_RIGHT + pchan->pan) / SOUND_PAN_RIGHT; /* panned left so attenuate right */
    else if (pchan->pan > SOUND_PAN_CENTER) pchan->volfactorL *= (real32)(SOUND_PAN_RIGHT - pchan->pan) / SOUND_PAN_RIGHT; /* panned right so attenuate left */

    pchan->volfactorL = min( pchan->volfactorL,  1.0f );
    pchan->volfactorL = max( pchan->volfactorL, -1.0f );
    pchan->volfactorR = min( pchan->volfactorR,  1.0f );
    pchan->volfactorR = max( pchan->volfactorR, -1.0f );
}


/*-----------------------------------------------------------------------------
    Name		:
    Description	:
    Inputs		:
    Outputs		:
    Return		:
----------------------------------------------------------------------------*/	
sdword splayFPRVL(void *bankaddress, sdword patnum, real32 *eq, real32 freq, sword pan, sdword priority, sword vol, bool startatloop, bool fadein, bool mute)
{
    PATCH* ppatch = NULL;

    if (patnum >= SOUND_OK) {
        ppatch = SNDgetpatch(bankaddress, patnum);
    }
    else if (patnum == SOUND_FLAGS_PATCHPOINTER) { /* compare patch ID */
        ppatch = (PATCH *)bankaddress;
    } else {
        return SOUND_ERR;
    }

    if (ppatch == NULL) {
        return SOUND_ERR;
    }

    if (vol == SOUND_DEFAULT) {
        vol = SOUND_VOL_MAX;
    }

    if ((vol <= SOUND_VOL_MIN) || (vol > SOUND_VOL_MAX)) {
        return SOUND_ERR;
    }

    if (freq == SOUND_DEFAULT) {
        freq = 1.0f;
    }


    const sdword channel = SNDgetchannel((sword)patnum, priority);
    if (channel < SOUND_OK){
        return SOUND_ERR;
    }

    /* create handle here */
    const sdword handle = SNDcreatehandle(channel);

    for (sdword i = 0; i < numbanks; i++) {
        if ((bankpointers[i].start <= (void *)ppatch) &&
            (bankpointers[i].end >= (void *)ppatch))
        {
            numchans[i]++;
            break;
        }
    }

    CHANNEL	*pchan = &channels[channel];
    SNDresetchannel(pchan);
    pchan->handle  = handle;
    pchan->ppatch  = ppatch;
    pchan->looping = ppatch->flags;
    pchan->pitch   = freq;
    pchan->mute    = mute;

    if (eq != NULL) {
        for (sdword i = 0; i < SOUND_EQ_SIZE; i++) {
            pchan->filter[i] = eq[i];
        }
    } else {
        for (sdword i = 0; i < SOUND_EQ_SIZE; i++) {
            pchan->filter[i] = 1.0f;
        }
    }
    
    for (sdword i = 0; i < SOUND_EQ_SIZE; i++) {
        pchan->cardiodfilter[i] = 1.0f;
    }

    pchan->usecardiod = FALSE;

    if (!fadein || !pchan->looping) {
        pchan->volume = vol;
    }

    soundvolume(handle, vol);
    soundpan(handle, pan);
    SNDcalcvolpan(pchan);
    soundfrequency(handle, freq);
    
// NEWLOOP
    if (startatloop)
         pchan->currentpos = (sbyte *)ppatch->loopstart;
    else pchan->currentpos = (sbyte *)ppatch->dataoffset;
    
    if (ppatch->waveformat.frequency < FQ_RATE)
        pchan->fqsize = FQ_QSIZE;
        pchan->fqsize = FQ_HSIZE;

    pchan->status = SOUND_PLAYING;
    return handle;
}



PATCH *SNDgetpatch(void *bankaddress, sdword patnum)
{
    if ( ! soundinited)
        return NULL;
    
    if (patnum >= 0) {
        BANK* tempbank = (BANK *)bankaddress;

        if (patnum < tempbank->numpatches) {
            PATCH* temppatches = (PATCH *)&tempbank->firstpatch;
            return &temppatches[ patnum ];
        }
    }

    return NULL;
}
