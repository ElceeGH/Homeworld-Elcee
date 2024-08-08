


#pragma once
#include "Types.h"



void   rintInit                            ( void );
bool   rintIsEnabled                       ( void );
real32 rintFraction                        ( void );
void   rintUnivUpdatePreMove               ( void );
void   rintUnivUpdatePostDestroy           ( void );
void   rintRenderBeginAndInterpolate       ( void );
void   rintRenderEndAndRestore             ( void );
void   rintRenderClearAndDisableTemporarily( void );
void   rintRenderEnableDeferred            ( void );
