


#pragma once
#include "Types.h"
#include "Vector.h"



/// Deterministic RNG state
typedef struct RNG {
    udword seed;  ///< Seed it was created with
    udword state; ///< Current state
} RNG;



void   rngInit             ( RNG* rng, udword seed );
real32 rngNext             ( RNG* rng );
real32 rngNextRange        ( RNG* rng, real32 a, real32 b );
bool   rngNextChance       ( RNG* rng, real32 n );
vector rngNextPointInCube  ( RNG* rng, vector centre, real32 radius );
vector rngNextPointInSphere( RNG* rng, vector centre, real32 radius );
