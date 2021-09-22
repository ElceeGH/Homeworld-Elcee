/*=============================================================================
    Name    : smixer.c
    Purpose : Low level audio mixer routines

    Created 01/10/1998 by salfreds
    Copyright Relic Entertainment, Inc.  All rights reserved.

    Butchered for your enjoyment...
=============================================================================*/

#include <stdio.h>
#include <string.h>
#include "fquant.h"
#include "soundcmn.h"
#include "soundlow.h"
#include "Debug.h"
#include "SoundStructs.h"
#include "Randy.h"
#include "FastMath.h"
#include "main.h"
#include "Globals.h"



/* function prototypes */
static void   isoundmixerprocess(void* buffer, udword bufferSize);
static void   soundmixerDeviceCallback(void *userdata, Uint8 *stream, int len);
static sdword isoundmixerdecodeEffect(sbyte *readptr, real32 *writeptr1, real32 *writeptr2, ubyte *exponent, sdword size, uword bitrate, EFFECT *effect);



udword mixerticks = 0;

static sdword dctmode	   = FQ_MNORM;		  // DCT mode
static sdword dctsize	   = FQ_HSIZE;		  // DCT block size
static sdword dctpanicmode = SOUND_MODE_NORM; // DCT panic mode, normal by default

static udword mixerBufferOutLen = 0;

static real32 timebufferL[FQ_DSIZE];
static real32 timebufferR[FQ_DSIZE];
static real32 temptimeL  [FQ_DSIZE];
static real32 temptimeR  [FQ_DSIZE];
static real32 mixbuffer1L[FQ_SIZE];
static real32 mixbuffer1R[FQ_SIZE];
static real32 mixbuffer2L[FQ_SIZE];
static real32 mixbuffer2R[FQ_SIZE];



extern bool			soundinited;
extern SDL_sem*		streamerThreadSem;
extern CHANNEL		channels[];
extern STREAM		streams[];
extern CHANNEL		speechchannels[];
extern sdword		numstreams;
extern SENTENCELUT* SentenceLUT;
extern real32		MasterEQ[];

extern SOUNDCOMPONENT mixer;
extern SOUNDCOMPONENT streamer;

extern bool bSoundPaused;
extern bool bSoundDeactivated;

extern sdword soundnumvoices;



// DCT mode functions
void soundMixerGetMode(sdword *mode)	// mode SOUND_MODE_NORM or SOUND_MODE_AUTO or SOUND_MODE_LOW
{
	// check DCT mode and panic mode
	if((dctmode == FQ_MHALF) && (dctpanicmode == SOUND_MODE_NORM))	{
		*mode=SOUND_MODE_LOW;
		return;
	}

	// check DCT mode and panic mode
	if((dctmode == FQ_MNORM) && (dctpanicmode == SOUND_MODE_AUTO))	{
		*mode=SOUND_MODE_AUTO;
		return;
	}

	*mode=SOUND_MODE_NORM;
}



void soundMixerSetMode(sdword mode)		// mode SOUND_MODE_NORM or SOUND_MODE_AUTO or SOUND_MODE_LOW
{
	switch (mode) {
		case SOUND_MODE_LOW:
			dctmode		 = FQ_MHALF;		// set DCT mode
			dctsize		 = FQ_QSIZE;	    // set block size for effects
			dctpanicmode = SOUND_MODE_NORM; // set panic mode
			break;

		case SOUND_MODE_AUTO:
			dctmode		 = FQ_MNORM;
			dctsize		 = FQ_HSIZE;
			dctpanicmode = SOUND_MODE_AUTO;
			break;

		default:
			dctmode		 = FQ_MNORM;
			dctsize		 = FQ_HSIZE;
			dctpanicmode = SOUND_MODE_NORM;
			break;
	}
}



/*-----------------------------------------------------------------------------
	Name		:
	Description	:
	Inputs		:
	Outputs		:
	Return		:
----------------------------------------------------------------------------*/	
sdword isoundmixerinit(void)
{
	// Initialize codec
	fqInitDequant();

	// Set random number generator for effects
	fqRand(ranRandomFnSimple,RANDOM_SOUND);

	// Set square root function for effects
	fqSqrt(sqrt);

	// Set block size for effects
	fqSize(dctsize);

	// init the iDCT function
	if (SOUND_OK != fqDecBlock(mixbuffer1L, mixbuffer2L, timebufferL, temptimeL, FQ_MINIT, FQ_MINIT)) return SOUND_ERR;
	if (SOUND_OK != fqDecBlock(mixbuffer1R, mixbuffer2R, timebufferR, temptimeR, FQ_MINIT, FQ_MINIT)) return SOUND_ERR;

	// Set up wave format structure.
    SDL_AudioSpec aspec = {
        .freq     = FQ_RATE,
        .format   = AUDIO_S16,
        .channels = 2,
        .samples  = FQ_SIZE,
        .callback = soundmixerDeviceCallback,
        .userdata = NULL,
    };

    if (SDL_OpenAudio(&aspec, NULL) < 0) {
        dbgMessagef("Couldn't open audio: %s", SDL_GetError());
        return SOUND_ERR;
    }

	// When using NULL for the "obtained" parameter to OpenAudio, SDL guarantees the parameters are exactly as requested.
	// So we don't need to go farting around with ring buffers to deal with mismatched buffer lengths / block sizes.
	mixerBufferOutLen = aspec.size;

	// Done
	mixer.status = SOUND_PLAYING;
	return SOUND_OK;
}



/*-----------------------------------------------------------------------------
	Name		:
	Description	:
	Inputs		:
	Outputs		:
	Return		:
----------------------------------------------------------------------------*/	
void isoundmixerrestore(void)
{
	mixer.timeout = 0;
}



/*-----------------------------------------------------------------------------
	Name		:
	Description	:
	Inputs		:
	Outputs		:
	Return		:
----------------------------------------------------------------------------*/
static sdword isoundmixerdecodeEffect(sbyte *readptr, real32 *writeptr1, real32 *writeptr2, ubyte *exponent, sdword size, uword bitrate, EFFECT *effect)
{
    char tempblock[FQ_LEN];
	memset(tempblock, 0,       sizeof(tempblock) );
	memcpy(tempblock, readptr, bitrate / 8       );

	// Check size
	if (size > dctsize) 
		size = dctsize;
	
	if (effect != NULL)
		fqGenQNoiseE(tempblock, bitrate, effect);

	fqDequantBlock(tempblock, writeptr1, writeptr2, exponent, FQ_LEN, bitrate, size);

	return bitrate / 8;
}



static void mixerMixStreams( void ) 
{
	for (sdword i=0; i<numstreams; i++) {
		if (speechchannels[i].status < SOUND_PLAYING) {
			continue;
		}
		
		STREAM*      const pstream = &streams[i];
		CHANNEL*     const pchan   = &speechchannels[i];
		STREAMQUEUE* const pqueue  = &pstream->queue[pstream->playindex];

		if (pstream->blockstatus[pstream->readblock] == 0) {
			if (pchan->status == SOUND_STOPPING) {
				pstream->writepos  = 0;
				pstream->playindex = pstream->writeindex;
				pstream->numtoplay = 0;
				pchan->status      = SOUND_INUSE;
			}
			continue;
		}
	
		if (pstream->blockstatus[pstream->readblock] == 1) {
			pstream->blockstatus[pstream->readblock] = 2;
		}
	
		const sdword amountread = isoundmixerdecodeEffect( pchan->currentpos,  pchan->mixbuffer1,  pchan->mixbuffer2, pchan->exponentblockL, pchan->fqsize,  pchan->bitrate,  pqueue->effect );
		pchan->currentpos += amountread;
		pchan->amountread += amountread;

		real32 scaleLevel = 1.0f;
			
		/* figure out any volume fades, pan fades, etc */
		if (pchan->volticksleft) {
			pchan->volticksleft--;
			pchan->volume += pchan->volfade;
				
			if (pchan->volticksleft <= 0) {
				pchan->volfade	    = 0.0f;
				pchan->volume       = (real32)pchan->voltarget;
				pchan->volticksleft = 0;
				pchan->voltarget	= -1;
			}
			
			if ((sword)pchan->volume > SOUND_VOL_MAX) pchan->volume = SOUND_VOL_MAX;
			if ((sword)pchan->volume < SOUND_VOL_MIN) pchan->volume = SOUND_VOL_MIN;
			
			if ((pchan->status == SOUND_STOPPING) && (pchan->volume == SOUND_VOL_MIN))
			{
				if (pstream->numqueued == 0) {
					pstream->status     = SOUND_STREAM_INUSE;
					pstream->queueindex = 0;
					pstream->writeindex = 0;
					pstream->playindex  = 0;
					pstream->numqueued  = 0;
				} else {
					pstream->status     = SOUND_STREAM_STARTING;
					pstream->writeindex = pstream->queueindex - pstream->numqueued;
					if (pstream->writeindex < 0) {
						pstream->writeindex += SOUND_MAX_STREAM_QUEUE;
					}
					pstream->playindex = pstream->writeindex;
				}
				
				pchan->status = SOUND_FREE;
				continue;
			}
			
			SNDcalcvolpan(pchan);
		}
		
		if (pchan->numchannels < SOUND_STEREO) {// added this check because of a strange bug that caused the music to
												// have a mixHandle of 0, which played a buzzing in the left channel
												// this doesn't fix the bug, but it'll keep the buzzing from happening
			/* mix in SFX */
			if (pqueue->mixHandle >= SOUND_OK) {
				sdword chan = SNDchannel(pqueue->mixHandle);
				fqMix(pchan->mixbuffer1,channels[chan].mixbuffer1, 1.0f);
				fqMix(pchan->mixbuffer2,channels[chan].mixbuffer2, 1.0f);
			}
	
			/* do the EQ and Delay or Acoustic Model stuff */
			if (pqueue->eq && pqueue->eq->flags && STREAM_FLAGS_EQ) {
				/* equalize this sucker */
				fqEqualize(pchan->mixbuffer1, pqueue->eq->eq);
				fqEqualize(pchan->mixbuffer2, pqueue->eq->eq);
			}
			
		
			if (pqueue->effect != NULL) {
				// Add tone
				fqAddToneE(pchan->mixbuffer1, pqueue->effect);
				fqAddToneE(pchan->mixbuffer2, pqueue->effect);
			
				// Generate break
				fqAddBreakE(pchan->mixbuffer1, pqueue->effect);
				fqAddBreakE(pchan->mixbuffer2, pqueue->effect);
					
				// Add noise
				fqAddNoiseE(pchan->mixbuffer1, pqueue->effect);
				fqAddNoiseE(pchan->mixbuffer2, pqueue->effect);
				
				// Limit
				fqLimitE(pchan->mixbuffer1, pqueue->effect);
				fqLimitE(pchan->mixbuffer2, pqueue->effect);
	
				// Filter
				fqFilterE(pchan->mixbuffer1, pqueue->effect);
				fqFilterE(pchan->mixbuffer2, pqueue->effect);
	
				scaleLevel = pqueue->effect->fScaleLev;
			}
		
			if (pqueue->delay && pqueue->delay->flags & STREAM_FLAGS_ACMODEL) {
				fqAcModel(pchan->mixbuffer1, pqueue->delay->eq, pqueue->delay->duration, pstream->delaybuffer1, DELAY_BUF_SIZE, &(pstream->delaypos1));
				fqAcModel(pchan->mixbuffer2, pqueue->delay->eq, pqueue->delay->duration, pstream->delaybuffer2, DELAY_BUF_SIZE, &(pstream->delaypos2));
			}
			else if (pqueue->delay &&  pqueue->delay->flags & STREAM_FLAGS_DELAY) {
				fqDelay(pchan->mixbuffer1, pqueue->delay->level, pqueue->delay->duration, pstream->delaybuffer1, DELAY_BUF_SIZE, &(pstream->delaypos1));
				fqDelay(pchan->mixbuffer2, pqueue->delay->level, pqueue->delay->duration, pstream->delaybuffer2, DELAY_BUF_SIZE, &(pstream->delaypos2));
			}
		}
	
		/* add it to mix buffer */
		fqMix(mixbuffer1L,pchan->mixbuffer1,pchan->volfactorL * scaleLevel);
		fqMix(mixbuffer2L,pchan->mixbuffer2,pchan->volfactorL * scaleLevel);

		/* if this is stereo, do the right channel */
		if (pchan->numchannels == SOUND_STEREO) {
			const sdword amountread = isoundmixerdecodeEffect(pchan->currentpos, pchan->mixbuffer1R, pchan->mixbuffer2R, pchan->exponentblockR, pchan->fqsize, pchan->bitrate, NULL);
			pchan->currentpos += amountread;
			pchan->amountread += amountread;
			fqMix(mixbuffer1R,pchan->mixbuffer1R,pchan->volfactorR * scaleLevel);
			fqMix(mixbuffer2R,pchan->mixbuffer2R,pchan->volfactorR * scaleLevel);
		} else {
			fqMix(mixbuffer1R,pchan->mixbuffer1,pchan->volfactorR * scaleLevel);
			fqMix(mixbuffer2R,pchan->mixbuffer2,pchan->volfactorR * scaleLevel);
		}
	
		/* this is a clock for the frequency filtering/effects */
		if (pqueue->effect != NULL) {
			pqueue->effect->nClockCount++;
		}
	
		if ((pchan->currentpos == (sbyte *)(pstream->buffer + (pstream->blocksize * (pstream->readblock + 1)))) ||
			(pchan->currentpos == (sbyte *)pstream->writepos)) {
			pstream->blockstatus[pstream->readblock++] = 0;
			pstream->readblock %= 2;
		}
	
		if (pchan->amountread == pqueue->size) {
			pchan->amountread = 0;
			if (pstream->numtoplay > 0) {
				/* finished this queued stream, go on to the next */
				pstream->numtoplay--;
				pstream->playindex = (pstream->playindex + 1) % SOUND_MAX_STREAM_QUEUE;

				if (pqueue->mixHandle >= SOUND_OK) {
					if (pqueue->pmixPatch != pstream->queue[pstream->playindex].pmixPatch) {
						/* we're mixing in a patch to this stream, shut it down */
						soundstop(pqueue->mixHandle, 0.0f);
						pqueue->mixHandle = SOUND_DEFAULT;
						pqueue->pmixPatch = NULL;
						pqueue->mixLevel  = SOUND_VOL_MIN;
					} else {
						pstream->queue[pstream->playindex].mixHandle = pqueue->mixHandle;
					}
				}
			}
		}
		
		if (pchan->currentpos >= pchan->endpos) {
			pchan->currentpos = pchan->freqdata; /* get the next block */
		}
	}
}



static void mixerMixSFX(void) 
{
	for (sdword i=0; i<soundnumvoices; i++) {
		if (channels[i].status < SOUND_PLAYING)
			continue;
		
		CHANNEL* const pchan = &channels[i];

		if ( ! pchan->looping){
			if (pchan->currentpos >= (sbyte *)pchan->ppatch->datasize) {
				SNDreleasebuffer(pchan);
				continue;
			}
		}
		
		if (pchan->status == SOUND_LOOPEND) {
			pchan->status     = SOUND_PLAYING;
			pchan->currentpos = (sbyte *)pchan->ppatch->loopend;
			pchan->looping    = FALSE;
		}
		else if (pchan->status == SOUND_RESTART) {
			pchan->status     = SOUND_PLAYING;
			pchan->currentpos = (sbyte *)pchan->ppatch->dataoffset;
		}

		const sdword amountread = isoundmixerdecodeEffect(pchan->currentpos, pchan->mixbuffer1, pchan->mixbuffer2, pchan->exponentblockL, pchan->fqsize, pchan->ppatch->bitrate,NULL);
		pchan->currentpos += amountread;

		if (pchan->looping) {
			if (pchan->currentpos >= (sbyte *)pchan->ppatch->loopend) {
				pchan->currentpos  = (sbyte *)pchan->ppatch->loopstart;
			}
		}

		/* figure out any volume fades, pan fades, etc */
		if (pchan->volticksleft) {
			pchan->volume       += pchan->volfade;
			pchan->volticksleft -= 1;
			
			if (pchan->volticksleft <= 0) {
				pchan->volfade		= 0.0f;
				pchan->volume	    = (real32)pchan->voltarget;
				pchan->volticksleft = 0;
				pchan->voltarget	= -1;
			}

			if ((sword)pchan->volume > SOUND_VOL_MAX) pchan->volume = SOUND_VOL_MAX;
			if ((sword)pchan->volume < SOUND_VOL_MIN) pchan->volume = SOUND_VOL_MIN;

			if ((pchan->status == SOUND_STOPPING) && ((sword)pchan->volume == SOUND_VOL_MIN)) {
				SNDreleasebuffer(pchan);
				continue;
			}
			
			SNDcalcvolpan(pchan);
		}

		if (pchan->panticksleft) {
			pchan->pan += pchan->panfade;
			pchan->panticksleft--;
			
			if (pchan->panticksleft <= 0) {
				pchan->panfade	    = 0;
				pchan->pan			= pchan->pantarget;
				pchan->panticksleft = 0;
			}
		
			if (pchan->pan > SOUND_PAN_MAX) pchan->pan = SOUND_PAN_MIN + (pchan->pan - SOUND_PAN_MAX);
			if (pchan->pan < SOUND_PAN_MIN) pchan->pan = SOUND_PAN_MAX + (pchan->pan - SOUND_PAN_MIN);
			SNDcalcvolpan(pchan);
		}

		if (pchan->pitchticksleft) {
			pchan->pitch		  += pchan->pitchfade;
			pchan->pitchticksleft -= 1;
			
			if (pchan->pitchticksleft <= 0) {
				pchan->pitchfade	  = 0.0f;
				pchan->pitch		  = pchan->pitchtarget;
				pchan->pitchticksleft = 0;
			}
		}
		
		/* do pitch shift */
		fqPitchShift(pchan->mixbuffer1, pchan->pitch);
		fqPitchShift(pchan->mixbuffer2, pchan->pitch);

		if (pchan->usecardiod) {
			/* apply cardiod filter for fake doppler */
			fqEqualize(pchan->mixbuffer1, pchan->cardiodfilter);
			fqEqualize(pchan->mixbuffer2, pchan->cardiodfilter);
		}
		
		/* equalize this sucker */
		fqEqualize(pchan->mixbuffer1, pchan->filter);
		fqEqualize(pchan->mixbuffer2, pchan->filter);

		if (!pchan->mute) {
			fqMix(mixbuffer1L,pchan->mixbuffer1,pchan->volfactorL);
			fqMix(mixbuffer2L,pchan->mixbuffer2,pchan->volfactorL);
			fqMix(mixbuffer1R,pchan->mixbuffer1,pchan->volfactorR);
			fqMix(mixbuffer2R,pchan->mixbuffer2,pchan->volfactorR);
		}
	}
}



/*-----------------------------------------------------------------------------
	Name		: isoundmixerprocess
	Description	: mix the audio into the target buffer.
	Inputs		:
	Outputs		:
	Return		:
----------------------------------------------------------------------------*/	
static void isoundmixerprocess( void* buffer, udword bufferSize )
{
	/* clear the mixbuffers */
	memset(mixbuffer1L, 0, sizeof(mixbuffer1L) );
	memset(mixbuffer1R, 0, sizeof(mixbuffer1R) );
	memset(mixbuffer2L, 0, sizeof(mixbuffer2L) );
	memset(mixbuffer2R, 0, sizeof(mixbuffer2R) );
	
	/* mix the speech first */
	mixerMixStreams();
	
	/* mix the SFX channels */
	mixerMixSFX();
	
	/* Equalize */
	fqEqualize(mixbuffer1L, MasterEQ);
	fqEqualize(mixbuffer2L, MasterEQ);
	fqEqualize(mixbuffer1R, MasterEQ);
	fqEqualize(mixbuffer2R, MasterEQ);

	/* Magic */
	fqDecBlock(mixbuffer1L, mixbuffer2L, timebufferL, temptimeL, dctmode, FQ_MNORM);
	fqDecBlock(mixbuffer1R, mixbuffer2R, timebufferR, temptimeR, dctmode, FQ_MNORM);

	/* select channel targets */
	real32* const left  = reverseStereo ? timebufferR : timebufferL;
	real32* const right = reverseStereo ? timebufferL : timebufferR;
	fqWriteTBlock(left, right, 2, buffer, bufferSize, NULL, 0 );

	/* update tick count */
	mixerticks++;
}



static void soundmixerDeviceCallback(void *userdata, Uint8 *stream, int len)
{
	// This should never happena according to the SDL spec.
	if (len != mixerBufferOutLen) {
		dbgFatalf( DBG_Loc, "Buffer length in audio mixer callback does not match specified length!" );
	}

	if (mixer.status >= SOUND_PLAYING) {
		// Mix into the audio device.
		isoundmixerprocess( stream, len );

		// Wake up the stream thread so it will soon fetch data for the mixer.
		// This doesn't actually have to happen RIGHT NOW, it's just a convenient place to wake the thread up.
		SDL_SemPost( streamerThreadSem );
	}
	else {
		// No mixing! Write silence.
		memset( stream, 0x00, len );

		if (mixer.status == SOUND_STOPPED && !bSoundPaused && !bSoundDeactivated) {
			mixer.status = SOUND_PLAYING;
		}
	}

	if (mixer.status == SOUND_STOPPING) {
		/* check and see if its done yet */
		if (mixer.timeout <= mixerticks) {
			mixer.timeout = 0;
			mixer.status  = SOUND_STOPPED;
			SDL_PauseAudio(TRUE);
		}
	}

	if (bSoundPaused && (mixer.status == SOUND_PLAYING)) {
		mixer.status = SOUND_STOPPING;
	}

	if (bSoundDeactivated && (mixer.status == SOUND_PLAYING)) {
		mixer.status = SOUND_STOPPED;
		SDL_PauseAudio(TRUE);
	}
}
