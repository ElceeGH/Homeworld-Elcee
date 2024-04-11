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
#include <stdio.h>



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
static void updateTimeReference(void) {
    updateTimingData( &updateTiming );
}



/// Update the interpolation value.
/// Call once per frame, in-between each frame.
static void updateTimeFraction(void) {
    updateTimingData( &frameTiming );

    const uqword now   = SDL_GetPerformanceCounter();
    const real64 rate  = (real64) updateTiming.deltaAvg;
    const real64 delta = (real64) (now - updateTiming.last);
    
    fraction = (real32) (delta / rate);
    fraction = max( fraction, 0.0f );
    fraction = min( fraction, 1.0f );
}



/// Current interpolation value for rendering.
static real32 getFraction(void) {
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



/// Interpolate an orientation.
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



typedef struct Interp {
    SpaceObj* obj;    ///< Object being interpolated, null if an unused slot
    vector    last;   ///< Last uninterpolated position.
    vector    cur;    ///< Current uninterpolated position.
    bool      exists; ///< Keepalive flag. Cleared before each update. If not set, gets removed from the list.
} Interp;

#define InterpLimit   4096            ///< Maximum interpolated object count
static udword interpCount;            ///< Current count
static udword interpPeak;             ///< Highest count so far
static Interp interps[ InterpLimit ]; ///< Interpolation state



/// Get the limit for searching/iterating over the interps.
static udword getLimit(void) {
    return min( InterpLimit, interpPeak + 2 ); // +2 because it needs to find empty spaces as well.
}



/// Find interp with matching pointer.
/// Returns NULL on failure
static Interp* find( SpaceObj* obj ) {
    for (udword i=0; i<InterpLimit; i++)
        if (interps[i].obj == obj)
            return &interps[i];

    return NULL;
}



/// Create a new interp object mapped to the given spaceobj
static void interpCreate( Interp* interp, SpaceObj* obj ) {
    interp->obj = obj;
    interpCount++;
    interpPeak = max( interpPeak, interpCount );
    printf( "interpCreate: count=%i, peak=%i\n", interpCount, interpPeak );
}



/// Destroy an interp object
static void interpDestroy( Interp* interp ) {
    if (interpCount == 0) {
        printf( "interpDestroy: trying to destroy when counter is zero!\n" );
    }

    interpCount--;
    printf( "interpDestroy: count=%i, peak=%i\n", interpCount, interpPeak );
    memset( interp, 0x00, sizeof(*interp) );
}



/// Map object to interp slot. If no existing mapping exists, one will be created.
static Interp* interpMap( SpaceObj* obj ) {
    Interp* interp = find( obj );

    if (interp == NULL) {
        interp = find( NULL );
        interpCreate( interp, obj );
    }
     
    return interp;
}



/// Space object callback
typedef void(SpaceObjFunc)(SpaceObj*);

/// Iterate over all space objects in the universe list
static void iterateSpaceObjects( SpaceObjFunc* callback ) {
    Node* node = universe.SpaceObjList.head;
    
    while (node != NULL) {
        SpaceObj* obj = (SpaceObj*) listGetStructOfNode(node);
        callback( obj );
        node = node->next;
    }
}



/// Clear all the exist flags.
static void clearExistFlags(void) {
    for (udword i=0; i<getLimit(); i++)
        interps[i].exists = FALSE;
}



/// Remove interps which no longer exist.
static void cleanInterps(void) {
    for (udword i=0; i<getLimit(); i++)
        if (interps[i].obj    != NULL)
        if (interps[i].exists == FALSE)
            interpDestroy( &interps[i] );
}





/// Set the previous position
static void setPrevPos( SpaceObj* obj ) {
    Interp* interp = interpMap( obj );
    interp->last   = obj->posinfo.position;
}



/// Set the current position and mark it as alive
static void setCurPosAndMarkExists( SpaceObj* obj ) {
    Interp* interp = interpMap( obj );
    interp->cur    = obj->posinfo.position;
    interp->exists = TRUE;
}



/// Interpolate the position of the associated spaceobj
static void interpolatePosition( const Interp* interp ) {
    SpaceObj*    obj = interp->obj;
    const vector a   = interp->last;
    const vector b   = interp->cur;
    const real32 f   = getFraction();
    obj->posinfo.position = lerp( a, b, f );
}



/// Restore the original position of the associated spaceobj
static void restorePosition( const Interp* interp ) {
    SpaceObj* obj = interp->obj;
    obj->posinfo.position = interp->cur;
}





/// Update interpolation state
/// Pre-move phase where the previous position and keepalive flag are set
void rintUnivUpdatePreMove( void ) {
    updateTimeReference();
    clearExistFlags();
    iterateSpaceObjects( setPrevPos );
}



/// Update interpolation state
/// Post-destroy phase where nonexistent objects have been destroyed
void rintUnivUpdatePostDestroy( void ) {
    iterateSpaceObjects( setCurPosAndMarkExists );
    cleanInterps();
}



/// Replace all positions with the interpolated ones
void rintRenderBegin( void ) {
    // Set the render fraction
    updateTimeFraction();

    // Interpolate all the positions
    for (udword i=0; i<getLimit(); i++)
        if (interps[i].obj)
            interpolatePosition( &interps[i] );
}



/// Restore all the positions to the uninterpolated current ones
void rintRenderEnd( void ) {
    for (udword i=0; i<getLimit(); i++)
        if (interps[i].obj)
            restorePosition( &interps[i] );
}








