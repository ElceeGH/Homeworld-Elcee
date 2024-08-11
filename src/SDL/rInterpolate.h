


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
