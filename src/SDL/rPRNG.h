


#pragma once
#include "Types.h"
#include "Vector.h"



/// Deterministic RNG state
typedef struct RNG {
    udword seed;  ///< Seed it was created with
    udword state; ///< Current state
} RNG;



void   rngInit         ( RNG* rng, udword seed );
real32 rngNext         ( RNG* rng );
real32 rngRange        ( RNG* rng, real32 a, real32 b );
bool   rngChance       ( RNG* rng, real32 n );
real32 rngChancef      ( RNG* rng, real32 n );
vector rngPointInCube  ( RNG* rng, vector centre, real32 radius );
vector rngPointInSphere( RNG* rng, vector centre, real32 radius );
