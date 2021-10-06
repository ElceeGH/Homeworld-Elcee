/*=============================================================================
    Name    : rInterpolate.c
    Purpose : Rendering interpolation timing and utility functions.
    
    The basis of an interpolating render system with a fixed update rate.

    Created 27/09/2021 by Elcee
=============================================================================*/



#include "Types.h"
#include "Debug.h"
#include "Vector.h"
#include "SDL.h"
#include "rResScaling.h"



#define HistoryLen 4

typedef struct TimingData {
    uqword last;                  ///< Last absolute time
    uqword delta;                 ///< Last delta time
    uqword deltaAvg;              ///< Delta time nominal, from average
    real64 frequency;             ///< Frequency computed from delta average
    uqword history[ HistoryLen ]; ///< History of delta times
    udword index;                 ///< Wrapping index into history
} TimingData;



static TimingData updateTiming;
static TimingData frameTiming;
static real32     fraction;



static void updateTimingData( TimingData* td ) {
    // Use the performance counter, need high precision
    const uqword now  = SDL_GetPerformanceCounter();
    const uqword freq = SDL_GetPerformanceFrequency();

    // Update the basics
    td->delta = now - td->last;
    td->last  = now;

    // Write into history ring
    td->history[td->index] = td->delta;
    td->index              = (td->index + 1) % HistoryLen;

    // Sum all deltas in ring
    uqword sum = 0;
    for (udword i=0; i<HistoryLen; i++)
        sum += td->history[i];

    // Produce average, rounded to closest integer
    const uqword half = (HistoryLen / 2);
    td->deltaAvg = (sum + half) / HistoryLen;

    // Compute frequency. Mainly for debugging
    td->frequency = (real64)freq / (real64)max(1,td->deltaAvg);
}



void rintInit(void) {
    memset( &updateTiming, 0x00, sizeof(updateTiming) );
    memset( &frameTiming,  0x00, sizeof(frameTiming)  );

    dbgMessagef( "Refresh frequency: %f Hz\n",   getResFrequency() );
    dbgMessagef( "Timer   frequency: %llu Hz\n", SDL_GetPerformanceFrequency() );
}



/// Update the reference time.
/// Call once per universe update.
void rintUpdateReference(void) {
    updateTimingData( &updateTiming );
}



/// Update the interpolation value.
/// Call once per frame, in-between each frame.
void rintUpdateValue(void) {
    updateTimingData( &frameTiming );

    const uqword now   = SDL_GetPerformanceCounter();
    const real64 rate  = (real64) updateTiming.deltaAvg;
    const real64 delta = (real64) (now - updateTiming.last);
    
    fraction = (real32) (delta / rate);
    fraction = max( fraction, 0.0f );
    fraction = min( fraction, 1.0f );
}



/// Get the current interpolation value for rendering.
real32 rintGetValue(void) {
    return fraction;
}
