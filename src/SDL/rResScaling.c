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
    return sqrtSum( (float) MAIN_WindowWidth, (float) MAIN_WindowHeight );
}

/// Get the screen pixel density as a fraction of the original expected density (800x600)
/// Always at least 1.0f but unbounded upwards.
real32 getResDensityRelative( void ) {
    const real32 baseDensity = sqrtSum( 800.0f, 600.0f );
    const real32 ratio       = rglGetResDensity() / baseDensity;
    return max( 1.0f, ratio );
}



/// Get the actual refresh frequency.
/// Clamped in range 24Hz to 1000Hz.
real32 getResFrequency( void ) {
    SDL_DisplayMode info;
    SDL_GetCurrentDisplayMode( 0, &info ); /// @todo Get the correct displayindex.
    sdword freq = info.refresh_rate;
    freq = max( freq,   24.0f );
    freq = min( freq, 1000.0f );
    return freq;
}

/// Get the relative refresh frequency compared with the typical 60Hz the game was designed for.
real32 getResFrequencyRelative( void ) {
    return getResFrequency() / 60.0f;
}



/// Extrapolate the curve originally specified for the size of big stars in BTG, proportional to screen resolution.
/// It was originally based on only the width of the screen as a proxy for resolution but that's no good nowadays.
real32 getBigStarSize( void ) {
    return sqrtf( getResDensity() * 0.0064f );
}