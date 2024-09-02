


#pragma once
#include "Types.h"
#include "SpaceObj.h"



void   rintInit                            ( void );
bool   rintIsEnabled                       ( void );
real32 rintFraction                        ( void );
real32 rintUniverseElapsedTime             ( void );
void   rintMarkHsDiscontinuity             ( SpaceObj* obj, real32 clipT );

void   rintUnivUpdatePreMove               ( void );
void   rintUnivUpdatePostDestroy           ( void );

void   rintRenderBeginAndInterpolate       ( void );
void   rintRenderEndAndRestore             ( void );
void   rintRenderClearAndDisableTemporarily( void );
void   rintRenderEnableDeferred            ( void );

real32 lerpf( real32 from, real32 to, real32 f );
vector lerpv( vector from, vector to, real32 f );
vector slerp( vector from, vector to, real32 f );
void   slerpm( matrix* out, const matrix* from, const matrix* to, real32 f );
