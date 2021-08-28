//
//  Mission11.h
//
//  Finite state machine for "Mission11" mission
//
//  Copyright (C) 1998 Relic Entertainment Inc.
//  All rights reserved
//
//  This code was autogenerated from Mission11.kp by KAS2C Version 2.05sdl
//


#ifndef __Mission11_H
#define __Mission11_H


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
extern const void* Mission11_FunctionPointers[];
extern const unsigned int Mission11_FunctionPointerCount;


//
//  FSM prototypes
//
void Init_Mission11(void);
void Watch_Mission11(void);


#endif
