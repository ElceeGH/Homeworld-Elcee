/*=============================================================================
    Name    : NavLights.c
    Purpose : Control operation of NAV lights on ships.

    Created 6/21/1997 by agarden
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#include "NavLights.h"

#include "glinc.h"
#include "Matrix.h"
#include "Memory.h"
#include "prim3d.h"
#include "render.h"
#include "Universe.h"
#include "rResScaling.h"

#ifndef SW_Render
    #ifdef _WIN32
        #include <windows.h>
    #endif
#endif

/*-----------------------------------------------------------------------------
    Name        : navLightBillboardEnable
    Description : setup modelview matrix for rendering billboarded sprites
    Inputs      : s - ship to obtain coordinate system from
                  nls - NAV Light static info
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void navLightBillboardEnable(Ship *s, NAVLightStatic *nls)
{
    vector src, dst;

    //object -> worldspace
    src = nls->position;
    matMultiplyMatByVec(&dst, &s->rotinfo.coordsys, &src);
    vecAddTo(dst, s->posinfo.position);

    //setup billboarding
    glPushMatrix();
    rndBillboardEnable(&dst);
    glDisable(GL_CULL_FACE);
}

/*-----------------------------------------------------------------------------
    Name        : navLightBillboardDisable
    Description : reset modelview matrix to non-billboard state
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void navLightBillboardDisable(void)
{
    //undo billboarding
    rndBillboardDisable();
    glPopMatrix();
    glEnable(GL_CULL_FACE);
}

/*-----------------------------------------------------------------------------
    Name        : navLightStaticInfoDelete
    Description : Delete the static info of a set of nav lights
    Inputs      : staticInfo - array of navlight structures to free
    Outputs     : unregisters the navlight textures, if any.
    Return      :
----------------------------------------------------------------------------*/
void navLightStaticInfoDelete(NAVLightStaticInfo *staticInfo)
{
    sdword i, num = staticInfo->numNAVLights;
    NAVLightStatic *navLightStatic = staticInfo->navlightstatics;

    dbgAssertOrIgnore(staticInfo != NULL);

    for( i=0 ; i < num ; i++, navLightStatic++)
    {
        if (navLightStatic->texturehandle != TR_InvalidHandle)
        {
            trTextureUnregister(navLightStatic->texturehandle);
        }
    }
    memFree(staticInfo);
}

/*-----------------------------------------------------------------------------
    Name        : RenderNAVLights
    Description : TODO: render sorted by projected depth value so alpha sorts correctly
    Inputs      : ship - the ship whose navlights we are to render
    Outputs     :
    Return      :

    Notes:
        This is a bit different than the original game which textured the lights up close (barely) but used solid coloured points far away.
        Positioning of the lights is totally broken on the good LODs when far away when using the textured versions, so it's points all the way now.
        In order to replicate the look of the original the points scale with distance and gain a white centre up close.
        The lights still scale individually per ship too, but clamp the minimum scale to a certain level to remain nicely visible as in the original.
----------------------------------------------------------------------------*/
void RenderNAVLights(Ship* ship) {
    extern bool bFade;
    extern real32 meshFadeAlpha;

    ShipStaticInfo* shipStaticInfo = ship->staticinfo;
    NAVLightInfo*   navLightInfo   = ship->navLightInfo;

    if (shipStaticInfo->navlightStaticInfo && navLightInfo != NULL)    {
        NAVLightStaticInfo* navLightStaticInfo = shipStaticInfo->navlightStaticInfo;
        NAVLightStatic*     navLightStatic     = navLightStaticInfo->navlightstatics;
        NAVLight*           navLight           = navLightInfo->navLights;

        // State/inputs
        real32 fade = bFade ? meshFadeAlpha : 1.0f;

        // Scaling params
        real32 resScaling = getResDensityRelative();
        real32 nearThresh = 50;
        real32 farThresh  = 4000;
        real32 nearScale  = 5.0f * resScaling;
        real32 farScale   = 1.0f * resScaling;

        // Box step
        real32 dist = sqrtf(ship->cameraDistanceSquared);
        dist = max(dist, nearThresh);
        dist = min(dist, farThresh);
        real32 delta  = farThresh - nearThresh;
        real32 linear = (dist - nearThresh) / delta;
        real32 frac   = 1.0f - linear;

        // Lerp
        real32 scaleDelta    = nearScale - farScale;
        real32 lightSizeBase = farScale + scaleDelta * (frac*frac);

        // Render each light
        bool lightOn = rndLightingEnable(FALSE);
        glDepthMask(GL_FALSE);
        glEnable(GL_POINT_SMOOTH);
        rndAdditiveBlends(TRUE);
        rndTextureEnable(FALSE);
        
        for (sdword i=0; i<navLightStaticInfo->numNAVLights; i++, navLight ++, navLightStatic ++)
        {
            // Account for the startdelay.
            if (navLight->lastTimeFlashed == navLightStatic->startdelay)
            {
                navLight->lastTimeFlashed = universe.totaltimeelapsed + navLightStatic->startdelay;
            }

            if (universe.totaltimeelapsed > navLight->lastTimeFlashed)
            {
                real32 timeadd = (navLight->lightstate == 1) ? navLightStatic->flashrateoff : navLightStatic->flashrateon;
                navLight->lastTimeFlashed = universe.totaltimeelapsed + timeadd;
                navLight->lightstate      = 1 - navLight->lightstate;
            }

            if (navLight->lightstate)
            {
                real32 lightScale = navLightStatic->size / 32.0f; // Scale up big lights on the Mothership, Scaffold etc.
                real32 lightSize  = lightSizeBase * max(1.0f,lightScale); // Clamp scale though or it will look dumb!
                real32 fadeCentre = fade * frac * 0.75f;
                color  white      = colRGB(255, 255, 255);
                color  tempColor  = colRGB(colRed  (navLightStatic->color) * 2 / 3,
                                           colGreen(navLightStatic->color) * 2 / 3,
                                           colBlue (navLightStatic->color) * 2 / 3);
                
                primPointSize3Fade(&navLightStatic->position, lightSize,      tempColor, fade);
                primPointSize3Fade(&navLightStatic->position, lightSize*0.5f, white,     fadeCentre);
            }
        }

        rndLightingEnable(lightOn);
        rndAdditiveBlends(FALSE);
        glDisable(GL_POINT_SMOOTH);
        glDepthMask(GL_TRUE);
    }
}

