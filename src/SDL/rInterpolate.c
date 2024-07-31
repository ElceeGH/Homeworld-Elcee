/*=============================================================================
    Name    : rInterpolate.c
    Purpose : Rendering interpolation timing and utility functions.
    
    The basis of an interpolating render system with a fixed update rate.

    TODO: Trails, fix stability across saves, add config option.

    Created 27/09/2021 by Elcee
=============================================================================*/



#include "rInterpolate.h"
#include "Types.h"
#include "Debug.h"
#include "Vector.h"
#include "SDL.h"
#include "rResScaling.h"
#include "NIS.h"
#include "Universe.h"
#include <stdio.h>




static void clearRenderMap( void );



#define HistoryLen 4
#define RenderEnableDelayFrames 10

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
static udword     updateEnabled = TRUE;  ///< Managed by config
static udword     renderEnabled = FALSE; ///< Managed automatically
static udword     renderTimer   = 0;     ///< Render enable delay timer



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



/// Update the reference time.
/// Call once per universe update.
static void updateTimeReference(void) {
    updateTimingData( &updateTiming );
}



/// Update the interpolation value.
/// Call once per frame.
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
static vector lerp( vector from, vector to, real32 f ) {
    return (vector) {
        from.x + (to.x - from.x) * f,
        from.y + (to.y - from.y) * f,
        from.z + (to.z - from.z) * f
    };
}



/// Interpolate an orientation.
static vector slerp( vector from, vector to, real32 f ) {
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
    vector    pprev;  ///< Previous uninterpolated position
    vector    pcurr;  ///< Current  uninterpolated position
    vector    cprev;  ///< Previous uninterpolated collision position (ships only)
    vector    ccurr;  ///< Current  uninterpolated collision position (ships only)
    vector    eprev;  ///< Previous uninterpolated engine position (ships only)
    vector    ecurr;  ///< Current  uninterpolated engine position (ships only)
    bool      exists; ///< Keepalive flag. Cleared before each update. If not set, entry gets removed from the list.
    bool      enable; ///< Disable interpolation if not set. For effect filtering.
} Interp;

#define InterpLimit 8192              ///< Number of interps allocated at a time
static udword interpCount;            ///< Current count
static udword interpPeak;             ///< Highest count so far
static Interp interps[ InterpLimit ]; ///< Interpolation items



/// Get the limit for searching/iterating over the interps.
static udword getLimit(void) {
    return min( InterpLimit, interpPeak + 2 ); // +2 instead of +1; it needs to find empty spaces not just the last occupied space.
}



/// Find interp with matching obj pointer.
/// Returns NULL on failure
static Interp* interpFind( SpaceObj* obj ) {
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
    //printf( "interpCreate: count=%i, peak=%i\n", interpCount, interpPeak );
}



/// Destroy an interp object
static void interpDestroy( Interp* interp ) {
    interpCount--;
    //printf( "interpDestroy: count=%i, peak=%i\n", interpCount, interpPeak );
    memset( interp, 0x00, sizeof(*interp) );
}



/// Map object to interp slot. If no existing mapping exists, one will be created.
static Interp* interpMap( SpaceObj* obj ) {
    Interp* interp = interpFind( obj );

    if (interp == NULL) {
        interp = interpFind( NULL );
        interpCreate( interp, obj );
    }
     
    return interp;
}



/// Space object callback
typedef void(SpaceObjFunc)(SpaceObj*);

/// Iterate over all space objects in the universe list
static void iterateSpaceObjectsUniverse( SpaceObjFunc* callback ) {
    Node* node = universe.SpaceObjList.head;
    
    while (node != NULL) {
        SpaceObj* obj = (SpaceObj*) listGetStructOfNode(node);
        node = node->next;
        callback( obj );
    }
}

/// Iterate over all space objects in the universe list
static void iterateSpaceObjectsRender( SpaceObjFunc* callback ) {
    Node* node = universe.RenderList.head;
    
    while (node != NULL) {
        SpaceObj* obj = (SpaceObj*) listGetStructOfNode(node);
        node = node->next;
        callback( obj );
    }
}



/// Exclude objects which can't be interpolated without causing problems (hopping around visually).
/// This boils down to excluding all effects other than bullets.
/// @todo It would be nice not to exclude effects attached to ships, to avoid jerky attachments in rare cases
static void filterObjects() {
    Effect*  allowList = malloc( InterpLimit * sizeof(Effect*) );
    Effect** allowEnd  = allowList;

    // First pass, disable all effects and mark bullet effects
    for (udword i=0; i<interpCount; i++) {
        Interp* interp = &interps[i];
        
        if (interp->exists && interp->obj) {
            // Enable by default
            interp->enable = TRUE;
            
            // For effects, disable by default
            if (interp->obj->objtype == OBJ_EffectType)
                interp->enable = FALSE;

            // For bullets, remember the linked effect
            if (interp->obj->objtype == OBJ_BulletType) {
                const Bullet* bullet = (Bullet*) interp->obj;
                if (bullet->effect)
                    *allowEnd++ = bullet->effect;
            }
        }
    }

    // Second pass, enable effects linked with bullets
    for (Effect** allow=allowList; allow!=allowEnd; allow++) {
        Interp* interp = interpFind( (SpaceObj*) *allow );
        if (interp)
            interp->enable = TRUE;
    }

    // Cleanup
    free( allowList );
}



/// Clear all the exist flags.
static void clearExistFlags(void) {
    for (udword i=0; i<getLimit(); i++)
        interps[i].exists = FALSE;
}



/// Remove interps which no longer exist.
static void cleanInterps(void) {
    for (udword i=0; i<getLimit(); i++)
        if (interps[i].exists == FALSE)
        if (interps[i].obj    != NULL)
            interpDestroy( &interps[i] );
}



/// Set the previous position
static void setPrevPos( SpaceObj* obj ) {
    Interp* interp = interpMap( obj );
    interp->pprev  = obj->posinfo.position;

    if (obj->objtype == OBJ_ShipType) {
        interp->cprev = ((Ship *)obj)->collInfo.collPosition;
        interp->eprev = ((Ship *)obj)->enginePosition;
    }
}



/// Set the current position and mark it as alive
static void setCurPosAndMarkExists( SpaceObj* obj ) {
    Interp* interp = interpMap( obj );
    interp->pcurr  = obj->posinfo.position;
    interp->exists = TRUE;

    if (obj->objtype == OBJ_ShipType) {
        interp->ccurr = ((Ship *)obj)->collInfo.collPosition;
        interp->ecurr = ((Ship *)obj)->enginePosition;
    }
}



/// Interpolate the position of the associated spaceobj
static void interpolatePosition( const Interp* interp ) {
    SpaceObj*    obj = interp->obj;
    const vector pa  = interp->pprev;
    const vector pb  = interp->pcurr;
    const real32 f   = getFraction();
    obj->posinfo.position = lerp( pa, pb, f );

    if (obj->objtype == OBJ_ShipType) {
        const vector ca = interp->cprev;
        const vector cb = interp->ccurr;
        const vector ea = interp->eprev;
        const vector eb = interp->ecurr;
        ((Ship *)obj)->collInfo.collPosition = lerp( ca, cb, f );
        ((Ship *)obj)->enginePosition        = lerp( ea, eb, f );
    }
}



/// Restore the original position of the associated spaceobj
static void restorePosition( const Interp* interp ) {
    SpaceObj* obj = interp->obj;
    obj->posinfo.position = interp->pcurr;

    if (obj->objtype == OBJ_ShipType) {
        ((Ship *)obj)->collInfo.collPosition = interp->ccurr;
        ((Ship *)obj)->enginePosition        = interp->ecurr;
    }
}



/// Check whether it's safe to do interpolation on the render side.
/// It's always safe on the universe update end.
/// @todo This doesn't cover everything yet. Load the Great Wastes fight and then the Gardens fight and the game will crash with an access violation.
static udword interpRenderAllowed() {
    return updateEnabled
        && renderEnabled
        && ! nisIsRunning
        && gameIsRunning;
}



/// Init the interpolation timing system
void rintInit( void ) {
    dbgMessagef( "\nRender interpolator init:" );
    dbgMessagef( "    Display refresh: %2.f Hz" , getResFrequency()             );
    dbgMessagef( "    Universe update: %d Hz"   , UNIVERSE_UPDATE_RATE          );
    dbgMessagef( "    Timer frequency: %llu Hz" , SDL_GetPerformanceFrequency() );

    rintClear();
    memset( &updateTiming, 0x00, sizeof(updateTiming) );
    memset( &frameTiming,  0x00, sizeof(frameTiming)  );
}



/// Disable interpolation (call when starting a new game)
void rintRenderDisable( void ) {
    renderEnabled = FALSE;
    renderTimer   = RenderEnableDelayFrames;
    dbgMessagef( "\nRender interpolation disabled." );
}



/// Enable interpolation (call in renderer before render begin/end functions)
/// The enable doesn't take effect instantly, as doing the render trickery on the very first frame will crash the game.
void rintRenderEnable( void ) {
    if (renderTimer != 0)
         renderTimer--;
    else renderEnabled = TRUE;
}



/// Clear all interpolation objects to empty
void rintClear( void ) {
    printf( "interpClear()\n" );
    memset( interps, 0x00, sizeof(interps) );
    interpCount = 0;
    interpPeak  = 0;
}



/// Update interpolation state
/// Pre-move phase where the previous position and keepalive flag are set
void rintUnivUpdatePreMove( void ) {
    updateTimeReference();
    clearExistFlags();
    iterateSpaceObjectsUniverse( setPrevPos );
}



/// Update interpolation state
/// Post-destroy phase where nonexistent objects have been destroyed
/// Last event before rendering begins
void rintUnivUpdatePostDestroy( void ) {
    iterateSpaceObjectsUniverse( setCurPosAndMarkExists );
    cleanInterps();
    filterObjects();
    clearRenderMap(); // Must be last event, just before rendering!
}



///////////////////////////////////////////////////////////////////////////////
/// Render side
///////////////////////////////////////////////////////////////////////////////

// Interps to apply, in the same order as the render list
static Interp*  renderList[InterpLimit];
static Interp** renderListEnd   = renderList;
static bool     renderListReady = FALSE;



/// Add mapping to the list
static void generateRenderListElement( SpaceObj* obj ) {
    Interp* interp = interpFind( obj );

    if (interp)
    if (interp->obj)
    if (interp->enable)
        *renderListEnd++ = interp;
}



/// The mapping of objects to interps only needs to be done once per universe update.
/// So generate the list just once on the first frame.
static void generateRenderList( void ) {
    if (renderListReady)
        return;

    clearRenderMap();
    iterateSpaceObjectsRender( generateRenderListElement );
    renderListReady = TRUE;
}



/// Reset the render interp map. Do this once per universe update.
static void clearRenderMap( void ) {
    renderListEnd   = renderList;
    renderListReady = FALSE;
}



void rintRenderBegin( void ) {
    // Set the render fraction
    updateTimeFraction();

    // Don't do anything when NIS cutscenes are running, or the game isn't running
    if ( ! interpRenderAllowed())
        return;

    // Generate the interpolation render list, if needed
    generateRenderList();

    // Interpolate everything
    for (Interp** interp=renderList; interp!=renderListEnd; interp++)
        interpolatePosition( *interp );
}



void rintRenderEnd( void ) {
    // May not be allowed
    if ( ! interpRenderAllowed())
        return;

    // Restore everything
    for (Interp** interp=renderList; interp!=renderListEnd; interp++)
        restorePosition( *interp );
}



