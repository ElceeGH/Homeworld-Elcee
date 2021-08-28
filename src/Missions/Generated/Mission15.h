//
//  Mission15.h
//
//  Finite state machine for "Mission15" mission
//
//  Copyright (C) 1998 Relic Entertainment Inc.
//  All rights reserved
//
//  This code was autogenerated from Mission15.kp by KAS2C Version 2.05sdl
//


#ifndef __Mission15_H
#define __Mission15_H


//
//  types and exposed game functions
//
#include <string.h>
#include "AIMoves.h"
#include "AITeam.h"
#include "Attributes.h"
#include "CommandWrap.h"
#include "KASFunc.h"
#include "Objectives.h"
#include "Timer.h"
#include "TradeMgr.h"
#include "Types.h"
#include "Vector.h"
#include "Volume.h"


//
//  level function pointer list
//
extern const void* Mission15_FunctionPointers[];
extern const unsigned int Mission15_FunctionPointerCount;


//
//  FSM prototypes
//
void Init_Mission15(void);
void Watch_Mission15(void);


#endif
