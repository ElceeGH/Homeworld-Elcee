/*=============================================================================
    Name    : TimeoutTimer.c
    Purpose : Implementation of Timeout Timers

    Created 98/11/13 by gshaw
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#include "SDL.h"

#include "TimeoutTimer.h"
#include "utility.h"

extern Uint32 utyTimerDivisor;

void TTimerInit(TTimer *timer)
{
    timer->enabled = FALSE;
}

void TTimerClose(TTimer *timer)
{
    timer->enabled = FALSE;
}

void TTimerDisable(TTimer *timer)
{
    timer->enabled = FALSE;
}

bool TTimerUpdate(TTimer *timer)
{
    if (!timer->enabled)
    {
        return FALSE;
    }

    if (timer->timedOut)
    {
        return TRUE;
    }

    uqword timerval   = SDL_GetTicks();
    uqword difference = timerval - timer->timerLast;
    uqword nTicks     = difference / utyTimerDivisor;

    if (nTicks >= timer->timeoutTicks)
    {
        timer->timedOut = TRUE;
        return TRUE;
    }

    return FALSE;
}

bool TTimerIsTimedOut(TTimer *timer)
{
    if (!timer->enabled)
    {
        return FALSE;
    }

    return timer->timedOut;
}

void TTimerReset(TTimer *timer)
{
    if (!timer->enabled)
    {
        return;
    }

    uqword timerval  = SDL_GetTicks();
    timer->timedOut  = FALSE;
    timer->timerLast = timerval;
}

void TTimerStart(TTimer *timer,real32 timeout)
{
    uqword timerval = SDL_GetTicks();

    timer->enabled = TRUE;
    timer->timedOut = FALSE;
    timer->timerLast = timerval;
    timer->timeoutTicks = (uqword) (timeout * UTY_TimerResolutionMax);
}

void GetRawTime(uqword *time)
{
    *time = SDL_GetTicks();
}

