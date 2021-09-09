// =============================================================================
//  Animatic.h
//  - playback of animatics via OpenGL
// =============================================================================
//  Copyright Relic Entertainment, Inc. All rights reserved.
//  Created 2/11/1999 by khent
// =============================================================================

#ifndef ___ANIMATIC_H
#define ___ANIMATIC_H

#include "Types.h"

extern sdword animaticJustPlayed;

void animStartup(void);
void animShutdown(void);
void animPlay(sdword a, sdword b);

#endif
