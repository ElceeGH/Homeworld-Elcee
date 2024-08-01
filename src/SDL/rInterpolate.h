


#pragma once
#include "Types.h"



void   rintInit                 ( void );
real32 rintFraction             ( void );
void   rintClear                ( void );
void   rintUnivUpdatePreMove    ( void );
void   rintUnivUpdatePostDestroy( void );
void   rintRenderBegin          ( void );
void   rintRenderEnd            ( void );
void   rintRenderDisable        ( void );
void   rintRenderEnableDeferred ( void );
