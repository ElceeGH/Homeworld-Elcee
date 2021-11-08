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
#include "Universe.h"



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



/// Update the timing information.
/// A lot of it is just for debug insight.
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



/// Init the interpolation timing system
void rintInit(void) {
    memset( &updateTiming, 0x00, sizeof(updateTiming) );
    memset( &frameTiming,  0x00, sizeof(frameTiming)  );

    dbgMessagef( "\nRender interpolator init:" );
    dbgMessagef( "    Display refresh: %2.f Hz" , getResFrequency()             );
    dbgMessagef( "    Timer frequency: %llu Hz" , SDL_GetPerformanceFrequency() );
    dbgMessagef( "    Universe update: %d Hz"   , UNIVERSE_UPDATE_RATE          );
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



/// Current interpolation value for rendering.
real32 rintGetValue(void) {
    return fraction;
}



/// Signed saturated dot product of normalised vectors.
static real32 unitDotProductClamped( vector a, vector b ) {
    vecNormalize( &a );
    vecNormalize( &b );
    real32 dot = vecDotProduct( a, b );
    dot = min( dot, +1.0f );
    dot = max( dot, -1.0f );
    return dot;
}


/// Interpolate a position.
vector lerp( vector from, vector to, real32 f ) {
    return (vector) {
        from.x + (to.x - from.x) * f,
        from.y + (to.y - from.y) * f,
        from.z + (to.z - from.z) * f
    };
}



/// Interpolation an orientation.
vector slerp( vector from, vector to, real32 f ) {
    const real32 dot = unitDotProductClamped( from, to );

    vector rel = {
        to.x - from.x * dot,
        to.y - from.y * dot,
        to.z - from.z * dot 
    };
    
    vecNormalize( &rel );
    const real32 rads = acosf( dot ) * f;
    const real32 cos  = cosf( rads );
    const real32 sin  = sinf( rads );

    return (vector) {
        from.x * cos + rel.x * sin,
        from.y * cos + rel.y * sin,
        from.z * cos + rel.z * sin
    };
}

