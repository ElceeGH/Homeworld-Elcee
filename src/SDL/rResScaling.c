/*=============================================================================
    Name    : rResScaling.c
    Purpose : Resolution-adaptive point/line scaling for modern systems

    Created 03/09/2021 by Elcee
=============================================================================*/


#include "rResScaling.h"
#include "SDL.h"



// Window size globals
extern int MAIN_WindowWidth;
extern int MAIN_WindowHeight;



/// This is kind of an arbitrary metric that is a proxy for pixel density.
/// It just needs to scale with resolution and avoid dependency on aspect ratio.
/// Note: This is not a linear factor and isn't supposed to be.
static real32 sqrtSum( real32 x, real32 y ) {
    return sqrtf( x * y );
}



/// Get the screen pixel density
real32 getResDensity( void ) {
    return sqrtSum( (real32) MAIN_WindowWidth, (real32) MAIN_WindowHeight );
}

/// Get the screen pixel density as a fraction of the original expected density (800x600)
/// Always at least 1.0f but unbounded upwards.
real32 getResDensityRelative( void ) {
    const real32 baseDensity = sqrtSum( 800.0f, 600.0f );
    const real32 ratio       = getResDensity() / baseDensity;
    return max( 1.0f, ratio );
}



/// Get the actual refresh frequency.
/// Clamped in range 24Hz to 1000Hz.
real32 getResFrequency( void ) {
    SDL_DisplayMode info;
    SDL_GetCurrentDisplayMode( 0, &info ); /// @todo Get the correct displayindex.
    real32 freq = (real32) info.refresh_rate;
    freq = max( freq,   24.0f );
    freq = min( freq, 1000.0f );
    return freq;
}

/// Get the relative refresh frequency compared with the typical 60Hz the game was designed for.
real32 getResFrequencyRelative( void ) {
    return getResFrequency() / 60.0f;
}



/// Get the height of the letterboxes when doing cutscenes.
/// No matter how wide the native aspect, there's still a minimum height just so you can see a cutscene is happening.
 udword getResLetterboxHeight( void ) {
    const real32 cinema = 2.35f; // How cinematic.
    const real32 minH   = 32.0f; // Need to know cutscenes are happening though.
    const real32 resW   = (real32) MAIN_WindowWidth;
    const real32 resH   = (real32) MAIN_WindowHeight;
    const real32 aspect = resW / resH;
    
    // If the screen is already C-I-N-E-M-A-S-C-O-P-E as hell, maintain only a minimal boxing.
    if (aspect >= cinema)
        return (udword) minH;

    const real32 target = resW / cinema;
    const real32 delta  = resH - target;
    return (udword) max( minH, delta / 2.0f );
}
