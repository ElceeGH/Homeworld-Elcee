


#pragma once
#include "Types.h"



void rintInit                 ( void );
void rintUnivUpdatePreMove    ( void );
void rintUnivUpdatePostDestroy( void );
void rintRenderBegin          ( void );
void rintRenderEnd            ( void );

vector lerp ( vector from, vector to, real32 f );
vector slerp( vector from, vector to, real32 f );
