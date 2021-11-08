


#pragma once
#include "Types.h"



void   rintInit(void);
void   rintUpdateReference(void);
void   rintUpdateValue(void);
real32 rintGetValue(void);

vector lerp ( vector from, vector to, real32 f );
vector slerp( vector from, vector to, real32 f );
