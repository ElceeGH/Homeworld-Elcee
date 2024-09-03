/*=============================================================================
    Name    : rPRNG.h
    Purpose : Fast float RNG functions.

    Not to be used for gameplay. This is for rendering and I didn't want to
    touch the game's RNGs in case it affects determinism in some way. Plus
    these methods are just a really nice part of the ole' toolbox.

    Created 03/09/2024 by Elcee
=============================================================================*/



#include "rPRNG.h"
#include "rInterpolate.h"



/// Init PRNG.
/// Seed of zero will be changed to 1 if given.
void rngInit( RNG* rng, udword seed ) {
    rng->seed  = max( 1, seed );
    rng->state = rng->seed;
}



/// Random float in range [0:1] exclusive.
real32 rngNext( RNG* rng ) {
    // RNG update
    const udword prime = 16807; // Prime to multiply state with
    rng->state *= prime;

    // The ole' hackaroo
    union { udword i; real32 f; } fu;
    
    // Generate float
    const udword oneF  = 0x3F800000;     // Binary32 1.0f
    const udword shift = 8 + 1;          // Binary32 bits in exponent + sign
    fu.i = (rng->state >> shift) | oneF; // Range [1:2] exclusive
    return fu.f - 1.0f;                  // Range [0:1] exclusive
}



/// Random float in given exclusive range.
real32 rngRange( RNG* rng, real32 a, real32 b ) {
    return lerpf( a, b, rngNext(rng) );
}



/// Returns true/false with uniform probability 1/n
bool rngChance( RNG* rng, real32 n ) {
    return 0 == (sdword)( rngNext(rng) * n );
}



/// Returns 1 or 0 with uniform probability 1/n
real32 rngChancef( RNG* rng, real32 n ) {
    return (real32) rngChance( rng, n );
}



/// Random point uniformly distributed inside a cube.
vector rngPointInCube( RNG* rng, vector centre, real32 radius ) {
    return (vector) { centre.x + rngRange( rng, -radius, +radius ),
                      centre.y + rngRange( rng, -radius, +radius ),
                      centre.z + rngRange( rng, -radius, +radius ) };
}



/// Random point uniformly distributed inside a sphere.
/// Based on Karthik Karanth's great post: https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/
vector rngPointInSphere( RNG* rng, vector centre, real32 radius ) {
    // Create a random point on the unit cube and normalise it into an orientation vector
    vector vec = { rngNext(rng), rngNext(rng), rngNext(rng) };
    vecNormalize( &vec );

    // Scale by the cube root of another independent random number to create point inside unit sphere
    const real32 rcr = cbrtf( rngNext(rng) );
    vecMultiplyByScalar( vec, rcr );

    // Scale up radius, offset to centre
    vecMultiplyByScalar( vec, radius );
    vecAddTo( vec, centre );
    return vec;
}




