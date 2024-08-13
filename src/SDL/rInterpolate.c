/*=============================================================================
    Name    : rInterpolate.c
    Purpose : Rendering interpolation timing and utility functions.
    
    Helps generate in-between position and rotation updates for game objects.
    It also exposes the interpolated time fraction so other systems can take
    advantage of it. When disabled, the interpolation fraction is fixed at 0.

    This covers most cases, though if you interpolate between a previous and
    current value then the current one should be selected if interpolation is
    turned off. Otherwise you get into a situation where the object will be 
    one universe update behind, visually, out of sync with others things.

    Created 27/09/2021 by Elcee
=============================================================================*/



#include "rInterpolate.h"
#include "Debug.h"
#include "Matrix.h"
#include "NIS.h"
#include "Options.h"
#include "SinglePlayer.h"
#include "Types.h"
#include "Universe.h"
#include "Vector.h"

#include "rResScaling.h"
#include <stdio.h>



// Defines
#define TimingHistory           4
#define RenderEnableDelayFrames 10



// Forward decl
static void clearRenderList( void );
static void clearInterpList( void );



/// Timing info for update and render
typedef struct TimingData {
    uqword last;                     ///< Last absolute time
    uqword delta;                    ///< Last delta time
    uqword deltaAvg;                 ///< Delta time nominal, from average
    real64 frequency;                ///< Frequency computed from delta average
    uqword history[ TimingHistory ]; ///< History of delta times
    udword index;                    ///< Wrapping index into history
} TimingData;



// Timing vars
static TimingData updateTiming;
static TimingData frameTiming;
static real32     fraction;
static udword     updateEnabled = TRUE;  ///< Managed by config, master enable
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
    td->index              = (td->index + 1) % TimingHistory;

    // Sum all deltas in ring
    uqword sum = 0;
    for (udword i=0; i<TimingHistory; i++)
        sum += td->history[i];

    // Produce average, rounded to closest integer
    const uqword half = (TimingHistory / 2);
    td->deltaAvg = (sum + half) / TimingHistory;

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



/// Signed saturated dot product of normalised vectors.
static real32 dotProductClamped( vector a, vector b ) {
    real32 dot = vecDotProduct( a, b );
    dot = min( dot, +1.0f );
    dot = max( dot, -1.0f );
    return dot;
}



/// Interpolate a scalar.
static real32 lerpf( real32 from, real32 to, real32 f ) {
    return from + (to - from) * f;
}



/// Interpolate a position.
static vector lerpv( vector from, vector to, real32 f ) {
    return (vector) {
        from.x + (to.x - from.x) * f,
        from.y + (to.y - from.y) * f,
        from.z + (to.z - from.z) * f
    };
}



/// Interpolate an orientation. Vectors must be unit length.
static vector slerp( vector from, vector to, real32 f ) {
    const real32 dot = dotProductClamped( from, to );

    // If they're almost exactly the same, just lerp.
    if (fabsf(dot) > 0.99f)
        return lerpv( from, to, f );

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



/// Interpolate an orientation matrix.
static void lerpm( matrix* out, const matrix* from, const matrix* to, real32 f ) {
    vector upA, upB, rightA, rightB, headA, headB;
    matGetVectFromMatrixCol1( upA,    *from );
    matGetVectFromMatrixCol1( upB,    *to   );
    matGetVectFromMatrixCol2( rightA, *from );
    matGetVectFromMatrixCol2( rightB, *to   );
    matGetVectFromMatrixCol3( headA,  *from );
    matGetVectFromMatrixCol3( headB,  *to   );

    vector up    = slerp( upA,    upB,    f );
    vector right = slerp( rightA, rightB, f );
    vector head  = slerp( headA,  headB,  f );

    matPutVectIntoMatrixCol1( up,    *out );
    matPutVectIntoMatrixCol2( right, *out );
    matPutVectIntoMatrixCol3( head,  *out );
}



/// Ship-specific interpolation info.
typedef struct ShipInterp {
    real32 hsClipT; ///< Hyperspace clipping parameter value (Note: discontinuous!)
} ShipInfo;



/// Interpolation metadata
typedef struct Interp {
    SpaceObj* obj;    ///< Object being interpolated, null if an unused slot
    bool      exists; ///< Keepalive flag. Cleared before each update. If not set, entry gets removed from the list.
    vector    pprev;  ///< Previous uninterpolated position
    vector    pcurr;  ///< Current  uninterpolated position
    vector    cprev;  ///< Previous uninterpolated collision position
    vector    ccurr;  ///< Current  uninterpolated collision position
    matrix    hprev;  ///< Previous uninterpolated coordsys matrix
    matrix    hcurr;  ///< Current  uninterpolated coordsys matrix
    ShipInfo  sprev;  ///< Previous ship-specifics
    ShipInfo  scurr;  ///< Current  ship-specific
} Interp;



// Interp memory
static udword  interpCount = 0;    ///< Current count
static udword  interpPeak  = 0;    ///< Highest count so far
static udword  interpAlloc = 0;    ///< Allocated count
static udword  interpLimit = 0;    ///< Highest used count
static Interp* interpData  = NULL; ///< Interpolation items

// Interps to apply, in the same order as the render list.
static Interp** renderList      = NULL;  ///< List of interps, in the same order as the universe render list. The same number of items allocated as with interps.
static Interp** renderListEnd   = NULL;  ///< List exclusive end / append pointer
static bool     renderListReady = FALSE; ///< Whether list is built already. Only needs to be done once after a universe update.



/// Get the limit for searching/iterating over the interps (exclusive end).
static udword interpRangeLimit(void) {
    return min( interpAlloc, interpLimit + 1 ); // +1 because it needs to iterate over empty spaces too, not just the last occupied space.
}



/// Allocate memory and update vars
static void rintAllocMemory( udword count ) {
    udword interpBytes = count * sizeof(Interp);
    udword renderBytes = count * sizeof(Interp*);

    // Allocate
    interpAlloc   = count;
    interpData    = malloc( interpBytes );
    renderList    = malloc( renderBytes );
    renderListEnd = renderList;

    // Zero
    memset( interpData, 0x00, interpBytes );
    memset( renderList, 0x00, renderBytes );

    printf( "Interpolator allocated %i items, %i bytes.\n", interpAlloc, interpBytes+renderBytes );
}



/// Grows allocated memory.
static void rintGrowMemory( void ) {
    // Save old pointer to copy data from
    void*  oldMem   = interpData;
    udword oldAlloc = interpAlloc;

    // Grow to 150% of original size
    udword newAlloc = (udword) (interpAlloc * 1.5);
    rintAllocMemory( newAlloc );

    // Copy previous data.  Not needed for the render list because it's created fresh after every update.
    memcpy( interpData, oldMem, oldAlloc*sizeof(Interp) );
}



/// Find interp with matching obj pointer.
/// Returns NULL on failure
static Interp* interpFind( SpaceObj* obj ) {
    for (udword i=0; i<interpRangeLimit(); i++)
        if (interpData[i].obj == obj)
            return &interpData[i];

    return NULL;
}



/// Find a free slot. If none exists, more memory will be allocated automatically.
static Interp* interpFindFreeSlot( void ) {
    // Try to find free slot
    Interp* interp = interpFind( NULL );

    // No free slots. Well shit, better make some more.
    if (interp == NULL) {
        rintGrowMemory();
        interp = interpFind( NULL );
    }

    return interp;
}



/// Create a new interp object mapped to the given spaceobj
static void interpCreate( Interp* interp, SpaceObj* obj ) {
    // Set object pointer
    interp->obj = obj;

    // Track stats
    interpCount++;
    interpPeak = max( interpPeak, interpCount );

    // Track range usage
    udword index = interp - interpData;
    interpLimit = max( index+1, interpLimit );

    //printf( "interpCreate: count=%i, peak=%i, alloc=%i, limit=%i\n", interpCount, interpPeak, interpAlloc, interpLimit );
}



/// Destroy an interp object
static void interpDestroy( Interp* interp ) {
    interpCount--;
    memset( interp, 0x00, sizeof(*interp) );

    //printf( "interpDestroy: count=%i, peak=%i, alloc=%i, limit=%i\n", interpCount, interpPeak, interpAlloc, interpLimit );
}



/// Space object callbacks
typedef void(SpaceObjIter  )(SpaceObj*);
typedef bool(SpaceObjFilter)(SpaceObj*);



/// Iterate over all space objects in the universe list
static void iterateSpaceObjectsUniverse( SpaceObjIter iter, SpaceObjFilter filter ) {
    Node* node = universe.SpaceObjList.head;
    
    while (node != NULL) {
        SpaceObj* obj = (SpaceObj*) listGetStructOfNode(node);
        node = node->next;

        if (filter( obj ))
            iter( obj );
    }
}



/// Iterate over all space objects in the render list
static void iterateSpaceObjectsRender( SpaceObjIter iter, SpaceObjFilter filter ) {
    Node* node = universe.RenderList.head;
    
    while (node != NULL) {
        SpaceObj* obj = (SpaceObj*) listGetStructOfNode(node);
        node = node->next;

        if (filter( obj ))
            iter( obj );
    }
}



/// Decide whether the object should be interpolated.
static bool filterInterpAllowed( SpaceObj* obj ) {
    switch (obj->objtype) {
        // Some effects shouldn't be interpolated, since they don't move, and it creates weird visual artifacts.
        case OBJ_EffectType:
            return 0 != (((Effect*) obj)->flags       & (SOF_AttachPosition | SOF_AttachVelocity | SOF_AttachCoordsys))
                || 0 != (((Effect*) obj)->effectFlags & (EAF_Position       | EAF_Velocity       | EAF_Coordsys      ));

        // These are normal objects.
        case OBJ_ShipType:
        case OBJ_BulletType:
        case OBJ_MissileType:
        case OBJ_AsteroidType:
        case OBJ_DerelictType:
            return TRUE;

        // Some types of objects just never move/rotate.
        case OBJ_NebulaType:
        case OBJ_GasType:
        case OBJ_DustType:
            return FALSE;

        default:
            return FALSE;
    }
}



/// Some objects can have their orientation interpolated.
/// Applies only for SpaceObjRot compatible objects.
static bool canInterpOrientation( SpaceObj* obj ) {
    switch (obj->objtype) {
        case OBJ_ShipType:     // Spinning like hell all the time
        case OBJ_MissileType:  // You can barely see them but hell, go for it.
        case OBJ_DerelictType: // They spin too
        case OBJ_AsteroidType: // Also spinning all over
        case OBJ_EffectType:   // Only really needed for ion beams, but the overhead is low anyway.
            return TRUE;

        default:
            return FALSE;
    }
}



/// Some objects need their collision interpolated since you can select them and whatnot.
/// It's also important for camera following.
/// Applies only for SpaceObjRotImp compatible objects.
static bool canInterpCollision( SpaceObj* obj ) {
    switch (obj->objtype) {
        case OBJ_ShipType:
        case OBJ_DerelictType:
        case OBJ_AsteroidType:
            return TRUE;

        default:
            return FALSE;
    }
}



/// Clear all the exist flags.
static void clearExistFlags(void) {
    for (udword i=0; i<interpRangeLimit(); i++)
        interpData[i].exists = FALSE;
}



/// Remove interps which no longer exist.
static void cleanInterps(void) {
    for (udword i=0; i<interpRangeLimit(); i++)
        if (interpData[i].exists == FALSE)
        if (interpData[i].obj    != NULL)
            interpDestroy( &interpData[i] );
}



/// Clear all interpolation objects to empty
static void clearInterpList( void ) {
    memset( interpData, 0x00, sizeof(Interp) * interpAlloc );
    interpCount = 0;
    interpLimit = 0;
    interpPeak  = 0;
}



/// Set the previous position
/// Adds objects to the interp list.
static void setPrevPosAndAdd( SpaceObj* obj ) {
    // Find the interp data for object in the list
    Interp* interp = interpFind( obj );

    // Doesn't exist? Create one
    if (interp == NULL) {
        interp = interpFindFreeSlot();
        interpCreate( interp, obj );
    }

    // Populate initial info
    interp->pprev = obj->posinfo.position;

    if (canInterpCollision( obj )) {
        SpaceObjRotImp* sori = (SpaceObjRotImp*) obj;
        interp->cprev = sori->collInfo.collPosition;
    }

    if (canInterpOrientation( obj )) {
        SpaceObjRot* sor = (SpaceObjRot*) obj;
        interp->hprev = sor->rotinfo.coordsys;
    }

    if (obj->objtype == OBJ_ShipType) {
        Ship* ship = (Ship*) obj;
        interp->sprev.hsClipT = ship->shipSinglePlayerGameInfo->clipt;
    }
}



/// Set the current position and mark it as alive
/// DOES NOT add objects to the interp list, since it can create visual artifacts if they don't have the previous values to lerp from.
static void setCurPosAndMarkExists( SpaceObj* obj ) {
    // Find the related interpolation data
    Interp* interp = interpFind( obj );

    // Doesn't exist? Don't do anything.
    if (interp == NULL)
        return;

    // Mark as existing, store current pos
    interp->exists = TRUE;
    interp->pcurr  = obj->posinfo.position;
    
    // Handle the more advanced cases
    if (canInterpCollision( obj )) {
        SpaceObjRotImp* sori = (SpaceObjRotImp*) obj;
        interp->ccurr = sori->collInfo.collPosition;
    }

    if (canInterpOrientation( obj )) {
        SpaceObjRot* sor = (SpaceObjRot*) obj;
        interp->hcurr = sor->rotinfo.coordsys;
    }

    if (obj->objtype == OBJ_ShipType) {
        Ship* ship = (Ship*) obj;
        interp->scurr.hsClipT = ship->shipSinglePlayerGameInfo->clipt;
    }
}



/// Interpolate the values of the associated spaceobj
static void interpolateObject( const Interp* interp ) {
    SpaceObj*    obj = interp->obj;
    const real32 f   = rintFraction();

    const vector pa = interp->pprev;
    const vector pb = interp->pcurr;
    obj->posinfo.position = lerpv( pa, pb, f );

    if (canInterpCollision( obj )) {
        SpaceObjRotImp* sori = (SpaceObjRotImp*) obj;
        const vector ca = interp->cprev;
        const vector cb = interp->ccurr;
        sori->collInfo.collPosition = lerpv( ca, cb, f );
    }

    if (canInterpOrientation( obj )) {
        SpaceObjRot* sor = (SpaceObjRot*) obj;
        const matrix* ha = &interp->hprev;
        const matrix* hb = &interp->hcurr;
        lerpm( &sor->rotinfo.coordsys, ha, hb, f );
    }

    if (obj->objtype == OBJ_ShipType) {
        Ship* ship = (Ship*) obj;
        const real32 ta = interp->sprev.hsClipT;
        const real32 tb = interp->scurr.hsClipT;
        ship->shipSinglePlayerGameInfo->clipt = lerpf( ta, tb, f );
    }
}



/// Restore the original values of the associated spaceobj
static void restoreObject( const Interp* interp ) {
    SpaceObj* obj = interp->obj;
    obj->posinfo.position = interp->pcurr;

    if (canInterpCollision( obj )) {
        SpaceObjRotImp* sori = (SpaceObjRotImp*) obj;
        sori->collInfo.collPosition = interp->ccurr;
    }

    if (canInterpOrientation( obj )) {
        SpaceObjRot* sor = (SpaceObjRot*) obj;
        sor->rotinfo.coordsys = interp->hcurr;
    }

    if (obj->objtype == OBJ_ShipType) {
        Ship* ship = (Ship*) obj;
        ship->shipSinglePlayerGameInfo->clipt = interp->scurr.hsClipT;
    }
}



/// Check whether it's safe to do interpolation on the render side.
/// It's always safe on the universe update end.
static bool interpRenderAllowed() {
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

    // Alloc initial memory.
    if (interpData == NULL)
        rintAllocMemory( 1024 );

    clearInterpList();
    memset( &updateTiming, 0x00, sizeof(updateTiming) );
    memset( &frameTiming,  0x00, sizeof(frameTiming)  );
}



/// Check if interpolation is enabled.
/// Not affected by temporary render disables.
bool rintIsEnabled( void ) {
    return updateEnabled;
}



/// Get the interpolation factor (0:1 inclusive)
/// If interpolation is disabled, returns the fixed value 0.
real32 rintFraction( void ) {
    if (updateEnabled)
         return fraction;
    else return 0.0f;
}



/// Get the current universe time extrapolated forward using rintFraction().
/// This is the real time, but it should only be used for visual things while rendering.
/// If interpolation isn't enabled, it's the same as universe.totaltimeelapsed.
real32 rintUniverseElapsedTime( void ) {
    return universe.totaltimeelapsed + UNIVERSE_UPDATE_PERIOD * rintFraction();
}



/// Mark point of discontinuity in hsClipT.
/// Otherwise the effect will bounce around stupidly at those moments.
void rintMarkHsDiscontinuity( SpaceObj* obj, real32 clipT ) {
    // Don't do anything stupid, kid
    if (obj == NULL)
        return;

    // Map the object
    Interp* interp = interpFind( obj );
    if (interp == NULL)
        return;

    // Make the values equal
    interp->sprev.hsClipT = clipT;
    interp->scurr.hsClipT = clipT;
}



/// Disable interpolation temporarily during rendering. Call when starting/loading a new game.
/// If it's enabled on the very first frame, things will go to hell.
/// There are also certain times when interpolation can interfere in a negative way, e.g. the Quick Dock button when hyperspacing.
void rintRenderClearAndDisableTemporarily( void ) {
    clearInterpList();
    renderEnabled = FALSE;
    renderTimer   = RenderEnableDelayFrames;
}



/// Enable interpolation (call in renderer before render begin/end function pair)
/// The enable doesn't take effect instantly, as doing the interpolation trickery on the very first frame will crash the game.
/// Also, even if you enable it here, nothing will happen if interpolated isn't turned on.
void rintRenderEnableDeferred( void ) {
    if (renderTimer != 0)
         renderTimer--;
    else renderEnabled = TRUE;
}



/// Update interpolation state
/// Pre-move phase where the previous position and keepalive flag are set
void rintUnivUpdatePreMove( void ) {
    // Use the option to decide whether interpolation is enabled
    updateEnabled = opRenderInterpolation;

    if ( ! updateEnabled)
        return;

    updateTimeReference();
    clearExistFlags();
    iterateSpaceObjectsUniverse( setPrevPosAndAdd, filterInterpAllowed );
}



/// Update interpolation state
/// Post-destroy phase where nonexistent objects have been destroyed
/// Last event before rendering begins
void rintUnivUpdatePostDestroy( void ) {
    if ( ! updateEnabled)
        return;

    iterateSpaceObjectsUniverse( setCurPosAndMarkExists, filterInterpAllowed );
    cleanInterps();
    clearRenderList(); // Must be last event, just before rendering!
}



///////////////////////////////////////////////////////////////////////////////
/// Render side
///////////////////////////////////////////////////////////////////////////////

/// Add mapping to the list
static void generateRenderListElement( SpaceObj* obj ) {
    Interp* interp = interpFind( obj );

    if (interp)
    if (interp->obj)
        *renderListEnd++ = interp;
}



/// The mapping of objects to interps only needs to be done once per universe update.
/// So generate the list just once on the first frame.
static void generateRenderList( void ) {
    if (renderListReady)
        return;

    clearRenderList();
    iterateSpaceObjectsRender( generateRenderListElement, filterInterpAllowed );
    renderListReady = TRUE;
}



/// Reset the render interp map. Do this once per universe update.
static void clearRenderList( void ) {
    renderListEnd   = renderList;
    renderListReady = FALSE;
}



/// Move everything to the interpolated positions.
void rintRenderBeginAndInterpolate( void ) {
    // Set the render fraction
    updateTimeFraction();

    // Don't do anything when NIS cutscenes are running, or the game isn't running
    if ( ! interpRenderAllowed())
        return;

    // Generate the interpolation render list, if needed
    generateRenderList();

    // Interpolate everything
    for (Interp** interp=renderList; interp!=renderListEnd; interp++)
        interpolateObject( *interp );
}



/// Move everything back to its original position.
void rintRenderEndAndRestore( void ) {
    // May not be allowed
    if ( ! interpRenderAllowed())
        return;

    // Restore everything
    for (Interp** interp=renderList; interp!=renderListEnd; interp++)
        restoreObject( *interp );
}



