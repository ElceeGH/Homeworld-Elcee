//
//  Mission05_OEM.h
//
//  Finite state machine for "Mission05_OEM" mission
//
//  Copyright (C) 1998 Relic Entertainment Inc.
//  All rights reserved
//
//  This code was autogenerated from Mission05_OEM.kp by KAS2C Version 2.05sdl
//


#ifndef __Mission05_OEM_H
#define __Mission05_OEM_H


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
extern const void* Mission05_OEM_FunctionPointers[];
extern const unsigned int Mission05_OEM_FunctionPointerCount;


//
//  FSM prototypes
//
void Init_Mission05_OEM(void);
void Watch_Mission05_OEM(void);


#endif
