/*=============================================================================
    Name    : Tactical.c
    Purpose : Logic for the tactical overlay

    Created 8/8/1997 by lmoloney
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#include "Tactical.h"

#include <stdio.h>

#include "Alliance.h"
#include "Blobs.h"
#include "CommandDefs.h"
#include "Debug.h"
#include "FastMath.h"
#include "font.h"
#include "main.h"
#include "Memory.h"
#include "render.h"
#include "prim2d.h"
#include "prim3d.h"
#include "Sensors.h"
#include "Ships.h"
#include "SpaceObj.h"
#include "StatScript.h"
#include "Teams.h"
#include "Tweak.h"
#include "Universe.h"
#include "Vector.h"
#include "rResScaling.h"
#include "rStateCache.h"
#include "rInterpolate.h"

#ifdef _MSC_VER
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif

extern fonthandle selGroupFont2;

real32 mineMineTOSize;
real32 toMinimumSize;

/*=============================================================================
    Data:
=============================================================================*/
//basic, default TO icon
toicon toDefaultIcon =
{
    0, {{0.0f, 0.0f}}
};

//to icons for all classes of ships + 1 for mines
toicon *toClassIcon[NUM_CLASSES+1];

//colors for tactical overlay of various players
color toPlayerColor[TO_NumPlayers];

//for legend: track if a particular class is currently displayed (for each player)
bool8 toClassUsed[NUM_CLASSES+1][TO_NumPlayers];

void toNewClassSet(char *directory,char *field,void *dataToFillIn);
void toNumberPointsSet(char *directory,char *field,void *dataToFillIn);
void toVertexAdd(char *directory,char *field,void *dataToFillIn);
scriptEntry toIconTweaks[] =
{
    { "IconClass",      toNewClassSet,  NULL},
    { "nPoints",        toNumberPointsSet, NULL},
    { "vertex",         toVertexAdd,    NULL},
    { "playerColor0",   scriptSetRGBCB, &toPlayerColor[0]},
    { "playerColor1",   scriptSetRGBCB, &toPlayerColor[1]},
    { "playerColor2",   scriptSetRGBCB, &toPlayerColor[2]},
    { "playerColor3",   scriptSetRGBCB, &toPlayerColor[3]},
    { "playerColor4",   scriptSetRGBCB, &toPlayerColor[4]},
    { "playerColor5",   scriptSetRGBCB, &toPlayerColor[5]},
    { "playerColor6",   scriptSetRGBCB, &toPlayerColor[6]},
    { "playerColor7",   scriptSetRGBCB, &toPlayerColor[7]},
    { NULL, NULL, 0 }
};

//retained integrals used during loading
ShipClass toCurrentClass = CLASS_Mothership;
sdword toCurrentPoint = 0;

// x,y location for the player list in the main tactical overlay
sdword TO_PLAYERLIST_Y = 480;
sdword TO_PLAYERLIST_X = 10;

/*=============================================================================
    Functions:
=============================================================================*/
/*-----------------------------------------------------------------------------
    Name        : toStartup
    Description : Startup TO module.
    Inputs      : void
    Outputs     : Initializes the toClassIcon array.
    Return      : void
----------------------------------------------------------------------------*/
void toStartup(void)
{
    sdword index;

    mineMineTOSize = primScreenToGLScaleX(TO_MineMinimumScreenSize);
    toMinimumSize  = primScreenToGLScaleX(TO_MinimumScreenSize);

    for (index = 0; index < (NUM_CLASSES+1); index++)
    {                                                       //set all icons to default
        toClassIcon[index] = &toDefaultIcon;
    }
    scriptSet(NULL, "Tactical.script", toIconTweaks);
}

/*-----------------------------------------------------------------------------
    Name        : toShutdown
    Description : Shut down the TO module.
    Inputs      : void
    Outputs     : Frees the toIconTweaks array.
    Return      : void
----------------------------------------------------------------------------*/
void toShutdown(void)
{
    sdword index;

    for (index = 0; index < (NUM_CLASSES+1); index++)
    {                                                       //set all icons to default
        if (toClassIcon[index] != &toDefaultIcon && toClassIcon[index] != NULL)
        {
            memFree(toClassIcon[index]);
        }
    }
}

/*-----------------------------------------------------------------------------
    Script-read functions to load in the TO icons
-----------------------------------------------------------------------------*/
void toNewClassSet(char *directory,char *field,void *dataToFillIn)
{                                                           //set new ship class
    if (strcasecmp(field,"CLASS_Mine") == 0)
    {
        toCurrentClass = NUM_CLASSES;   // use NUM_CLASSES to indicate mine
    }
    else
    {
        toCurrentClass = StrToShipClass(field);
    }
    dbgAssertOrIgnore(toCurrentClass < (NUM_CLASSES+1));
}
void toNumberPointsSet(char *directory,char *field,void *dataToFillIn)
{                                                           //set number of points and allocate structure
    sdword nPoints, nScanned;

    nScanned = sscanf(field, "%d", &nPoints);               //parse number of points
    dbgAssertOrIgnore(nScanned == 1);
    dbgAssertOrIgnore(nPoints > 0 && nPoints < 32);                 //!!! 32 is some arbitrary number
    toClassIcon[toCurrentClass] = memAlloc(toIconSize(nPoints),//allocate the icon structure
                                           "Tactical Overlay Icon", NonVolatile);
    toClassIcon[toCurrentClass]->nPoints = nPoints;
    toCurrentPoint = 0;                                     //no points loaded yet
}
void toVertexAdd(char *directory,char *field,void *dataToFillIn)
{
    real32 x, y;
    sdword nScanned;

    dbgAssertOrIgnore(toCurrentPoint < toClassIcon[toCurrentClass]->nPoints);//make sure not too many points
    nScanned = sscanf(field, "%f,%f", &x, &y);              //scan in the x/y
    dbgAssertOrIgnore(nScanned == 2);                               //make sure we got 2 numbers
    toClassIcon[toCurrentClass]->loc[toCurrentPoint].x =    //copy the points over
        x * TO_VertexScanFactorX;                           //multiplied by a scaling factor
    toClassIcon[toCurrentClass]->loc[toCurrentPoint].y =
        y * TO_VertexScanFactorY;
    toCurrentPoint++;                                       //next point
}

/*-----------------------------------------------------------------------------
    Name        : toAllShipsDraw
    Description : Draw tactical overlays for all ships.
    Inputs      :
    Outputs     : Draw a tactical display icon overlay for all on-screen ships
    Return      : void
----------------------------------------------------------------------------*/
void toAllShipsDraw(void)
{
    Node *objnode = universe.RenderList.head;
    Ship *ship = NULL;
    real32 radius = 0.0, scale = 0.0;
    sdword index = 0, player = 0;
    toicon *icon = NULL;
    vector zero = {0,0,0};
    color c = colBlack;
    udword intScale = 0;

     // reset usage of classes (for legend)
    for (index = 0; index < (NUM_CLASSES+1); index++)
    {
          for (player = 0; player < TO_NumPlayers; ++player)
          {
               toClassUsed[index][player] = 0;
          }
    }

    rndTextureEnable(FALSE);
    rndLightingEnable(FALSE);

    while (objnode != NULL)
    {
          ship = (Ship *) listGetStructOfNode(objnode);

        if (ship->flags & SOF_Dead)
        {
            goto nextnode;
        }

        if (ship->collInfo.selCircleRadius <= 0.0f)
        {                                                   //if behind camera
            goto nextnode;                                  //don't draw it
        }

        if(ship->flags & SOF_Slaveable)
        {
            if(ship->slaveinfo->flags & SF_SLAVE)
            {
                goto nextnode;          //don't draw TO for SLAVES of some master ship
            }
        }

        if (ship->objtype == OBJ_MissileType)
        {
#define mine ((Missile *)ship)
            if (mine->missileType != MISSILE_Mine)
            {
                goto nextnode;
            }

            if (mine->playerowner != universe.curPlayerPtr)
            {
                goto nextnode;      // only show player's own mines
            }

            radius = mine->collInfo.selCircleRadius;

            if (radius < TO_MaxFadeSize)
            {

                //Get an index of the player to get the color.
#if TO_STANDARD_COLORS
                if (mine->playerowner == universe.curPlayerPtr)
                {
                    c = teFriendlyColor;
                }
                else if (allianceArePlayersAllied(mine->playerowner, universe.curPlayerPtr))
                {
                    c = teAlliedColor;
                }
                else
                {
                    c = teHostileColor;
                }
#else
                c = teColorSchemes[mine->colorScheme].tacticalColor;
#endif

                if (radius > TO_FadeStart)
                {
                    // scale the color - start fading out the overlay
                    scale = (radius - TO_MinFadeSize) * TO_FadeRate;
                    intScale = (udword)scale;
                    c = colRGB(colRed(c) / intScale, colGreen(c) / intScale, colBlue(c) / intScale);
                }

                icon = toClassIcon[NUM_CLASSES];    // NUM_CLASSES indicates Mine

                primLineLoopStart2(1, c);

                radius = max(mineMineTOSize, radius);   //ensure radius not too small

                for (index = icon->nPoints - 1; index >= 0; index--)
                {
                    primLineLoopPoint3F(mine->collInfo.selCircleX + icon->loc[index].x * radius,
                                        mine->collInfo.selCircleY + icon->loc[index].y * radius);
                }

                primLineLoopEnd2();
            }

        }
        else if (ship->objtype == OBJ_ShipType)
        {
            if (bitTest(ship->flags,SOF_Cloaked) && ship->playerowner != universe.curPlayerPtr)
            {                                                   //if cloaked, and doesn't belong to current player
                if(!proximityCanPlayerSeeShip(universe.curPlayerPtr,ship))
                {
                    goto nextnode;                                  //don't draw it
                }
            }

            if (ship->shiptype == Drone)
            {
                goto nextnode;
            }

            // tactical overlay (fades out when the ship gets large enough)
            radius = ship->collInfo.selCircleRadius * ship->staticinfo->tacticalSelectionScale;

            if (radius < TO_MaxFadeSize)
            {
                //Get an index of the player to get the color.
#if TO_STANDARD_COLORS
                if (ship->playerowner == universe.curPlayerPtr)
                {
                    c = teFriendlyColor;
                }
                else if (allianceIsShipAlly(ship, universe.curPlayerPtr))
                {
                    c = teAlliedColor;
                }
                else
                {
                    c = teHostileColor;
                }
#else
                c = teColorSchemes[ship->colorScheme].tacticalColor;
#endif

                if (radius > TO_FadeStart)
                {
                    // scale the color - start fading out the overlay
                    scale = 255.0f - ((radius - TO_MinFadeSize) * TO_FadeRate);
                    intScale = (udword)scale;
                    c = colRGB((colRed(c) * intScale) >> 8, (colGreen(c) * intScale) >> 8, (colBlue(c) * intScale) >> 8);
                }

                icon = toClassIcon[((ShipStaticInfo *)ship->staticinfo)->shipclass];
                primLineLoopStart2(sqrtf(getResDensityRelative()), c);

                radius = max(toMinimumSize, radius);   //ensure radius not too small

                for (index = icon->nPoints - 1; index >= 0; index--)
                {
                    primLineLoopPoint3F(ship->collInfo.selCircleX + icon->loc[index].x * radius,
                                        ship->collInfo.selCircleY + icon->loc[index].y * radius);
                }

                primLineLoopEnd2();

                // mark class as used for this player (for legend)
                toClassUsed[((ShipStaticInfo *)ship->staticinfo)->shipclass][ship->playerowner->playerIndex] = TRUE;
            }

            //for moving ships that belong to the current player, draw the moveline
            if ((!vecAreEqual(ship->moveTo, zero)) && (ship->playerowner == universe.curPlayerPtr))
            {
                toMoveLineDraw(ship, scale);
            }

            //Draw Special TO's for Special Ships
            if(ship->playerowner == universe.curPlayerPtr)
            {
                switch (ship->shiptype)
                {
                    case CloakGenerator:
                        if( ((CloakGeneratorSpec *)ship->ShipSpecifics)->CloakOn)
                        {
                            toFieldSphereDraw(ship,((CloakGeneratorStatics *) ((ShipStaticInfo *)(ship->staticinfo))->custstatinfo)->CloakingRadius, TW_CLOAKGENERATOR_SPHERE_COLOUR);
                        }
                        break;
                    case DFGFrigate:
                        toFieldSphereDraw(ship,((DFGFrigateStatics *) ((ShipStaticInfo *)(ship->staticinfo))->custstatinfo)->DFGFrigateFieldRadius, TW_DFGF_SPHERE_COLOUR);
                        break;
                    case GravWellGenerator:
                        if (((GravWellGeneratorSpec *)ship->ShipSpecifics)->GravFieldOn)
                        {
                            toFieldSphereDraw(ship,((GravWellGeneratorStatics *) ((ShipStaticInfo *)(ship->staticinfo))->custstatinfo)->GravWellRadius, TW_GRAVWELL_SPHERE_COLOUR);
                        }
                        break;
                    case ProximitySensor:
                        if( ((ProximitySensorSpec *)ship->ShipSpecifics)->sensorState == SENSOR_SENSED
                        ||  ((ProximitySensorSpec *)ship->ShipSpecifics)->sensorState == SENSOR_SENSED2)
                        {
                            toActiveProxSensorDraw(ship,((ProximitySensorStatics *) ((ShipStaticInfo *)(ship->staticinfo))->custstatinfo)->SearchRadius, TW_PROXIMITY_RING_COLOUR);
                            toFieldSphereDraw     (ship,((ProximitySensorStatics *) ((ShipStaticInfo *)(ship->staticinfo))->custstatinfo)->SearchRadius, TW_PROXIMITY_SPHERE_COLOUR_FOUND);
                        }
                        else
                        {
                            toFieldSphereDraw(ship,((ProximitySensorStatics *) ((ShipStaticInfo *)(ship->staticinfo))->custstatinfo)->SearchRadius, TW_PROXIMITY_SPHERE_COLOUR);
                        }
                        break;
                    default:
                        break;
                }
            }
        }

nextnode:
        objnode = objnode->next;
    }

#define DEBUG_DRAW_BLOBS 0

#if DEBUG_DRAW_BLOBS


    {
        blob *Blob;

        objnode = universe.collBlobList.head;
        while (objnode != NULL)
        {
            Blob = (blob *)listGetStructOfNode(objnode);

            toFieldSphereDrawGeneral(Blob->centre,Blob->radius,colReddish);

            objnode = objnode->next;
        }
    }
#endif
}

/*-----------------------------------------------------------------------------
    Name        : toLegendDraw
    Description : Draw tactical overlay legend.
    Inputs      : toClassUsed is set with a previous call to toAllShipsDraw()
    Outputs     : Draw a legend for the tactical display icons.
    Return      : void
----------------------------------------------------------------------------*/
void toLegendDraw(void)
{
    sdword rowHeight, shipClass, pl, y, index, x;
    toicon *icon;
    color col;
    real32 radius;
    fonthandle fhSave;
    rectangle playerColorRect;

    fhSave = fontCurrentGet();                  //save the current font
    fontMakeCurrent(selGroupFont2);  // use a common, fairly small font

    rowHeight = fontHeight(" "); // used to space the legend
    y = rowHeight;
    radius = primScreenToGLScaleX(rowHeight)/2;
    x = (sdword)(rowHeight * 2.5);

#if TO_STANDARD_COLORS
    col = teNeutralColor;
#else
    col = teColorSchemes[universe.curPlayerIndex].tacticalColor;
#endif
    col = colRGB(colRed(col)/TO_IconColorFade, colGreen(col)/TO_IconColorFade, colBlue(col)/TO_IconColorFade);

    // draw legend entries for any classes currently displayed
    real32 thickness = 0.75f * sqrtf(getResDensityRelative());

    for (shipClass = CLASS_Mothership+1; shipClass <= CLASS_NonCombat; ++shipClass)
    {
        for (pl = 0; pl < TO_NumPlayers; ++pl)
        {
            if (toClassUsed[shipClass][pl])
                break;
        }
        if (pl < TO_NumPlayers)
        {
            // icon
            icon = toClassIcon[shipClass];
            primLineLoopStart2(thickness, col);

            for (index = icon->nPoints - 1; index >= 0; index--)
            {
               primLineLoopPoint3F(primScreenToGLX(rowHeight*1.5)   + icon->loc[index].x * radius,
                                   primScreenToGLY(y + rowHeight/2) + icon->loc[index].y * radius);
            }
            primLineLoopEnd2();

            // text
            fontPrint(x, y,  TO_TextColor, ShipClassToNiceStr((ShipClass)shipClass));
            y += rowHeight + 1;
        }
    }

    // draw player names in the bottom left of the screen.

    if (!singlePlayerGame)
    {
        playerColorRect.x0 = TO_PLAYERLIST_X;
        playerColorRect.x1 = MAIN_WindowWidth;
        playerColorRect.y0 = 0;
        playerColorRect.y1 = MAIN_WindowHeight-TO_PLAYERLIST_Y;

        smPlayerNamesDraw(&playerColorRect);
    }

    fontMakeCurrent(fhSave);
}



//
// Move Line Functions
// 

#define TO_MOVE_LINE_COLOR              TW_MOVETO_LINE_COLOR
#define TO_MOVE_LINE_PULSE_COLOR        TW_MOVETO_PULSE_COLOR
#define TO_ATT_MOVE_LINE_COLOR          TW_ATTMOVETO_LINE_COLOR
#define TO_ATT_MOVE_LINE_PULSE_COLOR    TW_ATTMOVETO_PULSE_COLOR
#define TO_PULSE_CLOSE_SHRINK           0.8f
#define TO_FADE_SIZE                    2  //*pulsesize
#define TO_MOVE_LINE_RADIUS_STRETCH     TW_MOVETO_CIRCLE_RADIUS
#define TO_PULSE_SPEED_SCALE            TW_MOVETO_PULSE_SPEED_SCALE
#define TO_MOVETO_ENDCIRCLE_RADIUS      TW_MOVETO_ENDCIRCLE_RADIUS

//a kludgy global variable
bool pulse_at_beginning = FALSE;

/*-----------------------------------------------------------------------------
    Name        : toDrawPulsedLine
    Description : Draws a line with a cool ass pulse flying down it at incredible velocities
    Inputs      : linestart  - where the line starts
                  lineend    - where the line ends
                  pulsesize  - how big that cool ass pulse is
                  linecolor  - the color of the line
                  pulsecolor - the color of the cool ass pulse
    Outputs     : See "description"
    Return      : void
----------------------------------------------------------------------------*/
bool toDrawPulsedLine(vector linestart, vector lineend, real32 pulsesize, color linecolor, color pulsecolor)
{
    static real32 lasttime = 0.0, pulsestartfraction = 0.0;
    real32 distance, pulsedistance, fadesize;
    vector dirvect, pulsestart, pulseend, fadestart, fadeend;
    bool pulse_at_end = FALSE;
    bool draw_pulse_beg = TRUE, draw_fadein = FALSE, draw_fadeout = FALSE;

    pulse_at_beginning = FALSE;

    real32 now = rintUniverseElapsedTime();

    if (now != lasttime)
    {
        pulsestartfraction += (now - lasttime)/TO_PULSE_SPEED_SCALE;
        lasttime            = now;

        // - first check is for the very first iteration of toDrawPulsedLine where
        //   "universe.totaltimeelapsed" will be much larger that "lasttime" and
        //   therefore "pulsestartfraction" is quite large
        // - the second check is to fix a very obscure bug where "lasttime" somehow
        //   gets set to a value larger than "universe.totaltimeelapsed".  We can't
        //   figure out why "lasttime" does this, though...
        if ((pulsestartfraction > 1.0) || (pulsestartfraction < 0.0))
        {
            pulsestartfraction = 0.0;
            pulse_at_beginning = TRUE;
        }
    }

    //calculate the pulse
    //get the unit direction vector of the pulse line
    vecSub(dirvect, lineend, linestart);
    distance = (real32) sqrtf(vecMagnitudeSquared(dirvect));
    vecNormalize(&dirvect);

    //find the start and end points of the pulse
    pulsedistance = pulsestartfraction * distance;

    while (pulsesize > distance/2)
    {
        pulsesize *= TO_PULSE_CLOSE_SHRINK;
    }

    //if the pulse is at the end of the moveto line
    if (pulsedistance > (distance + pulsesize))
    {
        pulsestartfraction = 0.0;
        pulse_at_beginning = TRUE;
    }

    //calculate the fade regions
    fadesize = TO_FADE_SIZE*pulsesize;

    //if the pulse is far enough away from the beginning of the moveto line to have a fadein region
    if (pulsedistance > fadesize)
    {
        vecScalarMultiply(fadestart, dirvect, pulsedistance - fadesize);
        vecAddTo(fadestart, linestart);
        draw_fadein = TRUE;
    }
    else if (pulsedistance < pulsesize + fadesize)
    {
        pulse_at_beginning = TRUE;

        if (pulsedistance < pulsesize)
        {
            draw_pulse_beg = FALSE;
        }
    }

    //if the pulse is far enough away from the end of the moveto line to have a fadeout region
    if (pulsedistance < (distance - fadesize - pulsesize))
    {
        vecScalarMultiply(fadeend, dirvect, (pulsedistance + pulsesize + fadesize));
        vecAddTo(fadeend, linestart);
        draw_fadeout = TRUE;
    }

    if ((pulsedistance > (distance - fadesize/3 - pulsesize)) ||
        (distance < TW_MOVETO_ENDCIRCLE_RADIUS))
    {
        pulse_at_end = TRUE;
    }

    //find the point locations of the start and end of the pulse
    vecScalarMultiply(pulsestart, dirvect, (pulsedistance /*- (pulsestartfraction * pulsesize)*/));
    vecAddTo(pulsestart, linestart);
    vecScalarMultiply(pulseend, dirvect, (pulsedistance + (pulsestartfraction * pulsesize)));
    vecAddTo(pulseend, linestart);

    glBegin(GL_LINES);
    {
        if (draw_pulse_beg)
        {
            //draw the line from the ship to the pulse
            glColor3ub(colRed(linecolor), colGreen(linecolor), colBlue(linecolor));
            glVertex3fv((GLfloat *)&linestart);

            //draw the fadein
            if (draw_fadein)
            {
                glVertex3fv((GLfloat *)&fadestart);
                glVertex3fv((GLfloat *)&fadestart);
            }
            glColor3ub(colRed(pulsecolor), colGreen(pulsecolor), colBlue(pulsecolor));
            glVertex3fv((GLfloat *)&pulsestart);

        }
        else
        {
            glColor3ub(colRed(pulsecolor), colGreen(pulsecolor), colBlue(pulsecolor));
            glVertex3fv((GLfloat *)&linestart);
            glVertex3fv((GLfloat *)&pulsestart);
        }

        //draw the pulse
        glVertex3fv((GLfloat *)&pulsestart);

        if (!pulse_at_end)
        {
            glVertex3fv((GLfloat *)&pulseend);

            //draw the line from the pulse to the end of the line
            glVertex3fv((GLfloat *)&pulseend);
            glColor3ub(colRed(linecolor), colGreen(linecolor), colBlue(linecolor));

            //draw the fadeout
            if (draw_fadeout)
            {
                glVertex3fv((GLfloat *)&fadeend);
                glVertex3fv((GLfloat *)&fadeend);
            }
        }
        glVertex3fv((GLfloat *)&lineend);
    }
    glEnd();

    return pulse_at_end;
}


/*-----------------------------------------------------------------------------
    Name        : toDrawMoveToCircle
    Description : Draws a circle at the destination of the ship
    Inputs      : ship - the ship
    Outputs     : See "description"
    Return      : void
----------------------------------------------------------------------------*/
void toDrawMoveToEndCircle(ShipPtr ship, bool pulse, color linecolor, color pulsecolor)
{
    if (pulse)
    {
        primCircleOutline3(&ship->moveTo, TO_MOVETO_ENDCIRCLE_RADIUS,
                           32, 0, pulsecolor, Z_AXIS);
    }
    else
    {
        primCircleOutline3(&ship->moveTo, TO_MOVETO_ENDCIRCLE_RADIUS,
                            32, 0, linecolor, Z_AXIS);
    }
}



/*-----------------------------------------------------------------------------
    Name        : toDrawMoveToLine
    Description : Draws a single move line from a ship to it's move point
    Inputs      : ship - the aformentioned ship
    Outputs     : See "description"
    Return      : void
----------------------------------------------------------------------------*/
void toDrawMoveToLine(ShipPtr ship, color linecolor, color pulsecolor)
{
    vector shipfront, shipright, dirfrontproj, dirrightproj, dirvect, dirproj,
           zero = {0,0,0}, negdir;
    real32 distance, pulsesize, shipradius;
    bool pulse = FALSE;

    //initialize a bunch of vectors and distances
    shipradius  = ship->magnitudeSquared*ship->staticinfo->staticheader.staticCollInfo.approxcollspheresize;
    pulsesize   = 2*shipradius;

    matGetVectFromMatrixCol3(shipfront, ship->rotinfo.coordsys);
    matGetVectFromMatrixCol2(shipright, ship->rotinfo.coordsys);

    vecSub(dirvect, ship->moveTo, ship->collInfo.collPosition);
    distance = (real32) sqrtf(vecMagnitudeSquared(dirvect));

    //find the moveto starting point
    vecScalarMultiply(dirfrontproj, shipfront, vecDotProduct(dirvect, shipfront));
    vecScalarMultiply(dirrightproj, shipright, vecDotProduct(dirvect, shipright));
    vecAdd(dirproj, dirfrontproj, dirrightproj);
    vecNormalize(&dirproj);

    //calculate the edge of the MoveToEndCircle
    //find the negative of the direction
    vecSub(negdir, zero, dirvect);
    negdir.z = 0;
    vecNormalize(&negdir);
    vecMultiplyByScalar(negdir, TO_MOVETO_ENDCIRCLE_RADIUS);
    vecAddTo(negdir, ship->moveTo);

    vecMultiplyByScalar(dirproj, (shipradius)*TO_MOVE_LINE_RADIUS_STRETCH);
    vecAddTo(dirproj, ship->collInfo.collPosition);


    //if the ship is pretty close to the end of the move, don't draw the line
    if (distance > pulsesize)
    {
        pulse = toDrawPulsedLine(dirproj, negdir, pulsesize, linecolor, pulsecolor);
    }

    if (distance > ((shipradius*TO_MOVE_LINE_RADIUS_STRETCH) + TO_MOVETO_ENDCIRCLE_RADIUS))
    {
        toDrawMoveToEndCircle(ship, pulse, linecolor, pulsecolor);
    }


}


/*-----------------------------------------------------------------------------
    Name        : toDrawMoveFromLine
    Description : Draws a single move line from a ship's movefrom point to the ship
    Inputs      : ship - the aformentioned ship
    Outputs     : See "description"
    Return      : void
----------------------------------------------------------------------------*/
void toDrawMoveFromLine(ShipPtr ship)
{
    vector shipback, shippos, dirvect;
    real32 distance,shipradius;

    shipradius = TO_MOVE_LINE_RADIUS_STRETCH*ship->staticinfo->staticheader.staticCollInfo.approxcollspheresize;

    //calculate the line
    matGetVectFromMatrixCol3(shipback, ship->rotinfo.coordsys);
    vecMultiplyByScalar(shipback, (shipradius));
    shippos = ship->collInfo.collPosition;
    vecSubFrom(shippos, shipback);
    shipback = shippos;

    vecSub(dirvect, shipback, ship->moveFrom);
    distance = (real32) sqrtf(vecMagnitudeSquared(dirvect));

    if (distance < shipradius)
    {
        return;
    }

    glBegin(GL_LINES);
    {
        //draw the line from the ship to the pulse
        glColor3ub(colRed(TO_MOVE_LINE_COLOR), colGreen(TO_MOVE_LINE_COLOR), colBlue(TO_MOVE_LINE_COLOR));
        glVertex3fv((GLfloat *)&shipback);
        glVertex3fv((GLfloat *)&ship->moveFrom);
    }
    glEnd();


}



/*-----------------------------------------------------------------------------
    Name        : toDrawMoveCircle
    Description : Draws a circle around the moving ship
    Inputs      : ship - the ship
                  scale - the tactical overlay color scale factor to fade in the move
                          circle (not implemented)
    Outputs     : See "description"
    Return      : void
----------------------------------------------------------------------------*/
void toDrawMoveCircle(ShipPtr ship, real32 scale, color linecolor, color pulsecolor)
{
    real32  shipradius;
    hmatrix rotmat;
    vector origin = {0.0, 0.0, 0.0};

    hmatMakeHMatFromMat(&rotmat, &ship->rotinfo.coordsys);
    hmatPutVectIntoHMatrixCol4(ship->collInfo.collPosition, rotmat);

    glPushMatrix();
    glMultMatrixf((GLfloat *)&rotmat);

    shipradius = ship->magnitudeSquared*TO_MOVE_LINE_RADIUS_STRETCH*ship->staticinfo->staticheader.staticCollInfo.approxcollspheresize;

    if ((ship->formationcommand)&&(ship->formationcommand->formation.pulse))
    {
        primCircleOutline3(&origin, shipradius, 32, 0, pulsecolor, X_AXIS);
    }
    else
    {
        primCircleOutline3(&origin, shipradius, 32, 0, linecolor, X_AXIS);
    }

    glPopMatrix();

}




/*-----------------------------------------------------------------------------
    Name        : toMoveLineDraw
    Description : Draws a line with a cool pulse from the ship's current position to
                  it's moveTo position.  Also draws a circle around the moving ship
    Inputs      : ship - the ship
                  scale - the tactical overlay color scale factor to fade in the move
                          circle (not used at the moment)
    Outputs     : See "description"
    Return      : void
----------------------------------------------------------------------------*/
void toMoveLineDraw(ShipPtr ship, real32 scale)
{
    vector temp1;
    real32 temp2;
    bool   primEnabled;
    color  linecolor;
    color  pulsecolor;

    if ((ship->staticinfo->shipclass != CLASS_Corvette) &&
        (ship->staticinfo->shipclass != CLASS_Fighter))
    {
        //do da graphics setting up stuff
        primEnabled = primModeEnabled;
        if (primEnabled)
        {
            primModeClear2();
        }

        glLineWidth( sqrtf(getResDensityRelative()) );

        //setup color: attack moves different from normal moves
        if ((ship->command != NULL) && (ship->command->ordertype.order == COMMAND_ATTACK))
        {
            linecolor  = TO_ATT_MOVE_LINE_COLOR;
            pulsecolor = TO_ATT_MOVE_LINE_PULSE_COLOR;
        }
        else
        {
            linecolor  = TO_MOVE_LINE_COLOR;
            pulsecolor = TO_MOVE_LINE_PULSE_COLOR;
        }

        //if a ship is in a formation but not the leader
        if ((ship->formationcommand)&&(ship!=ship->formationcommand->selection->ShipPtr[0]))
        {
            //only draw the moveto circle
            toDrawMoveCircle(ship, scale, linecolor, pulsecolor);
        }
        else if(ship->shiptype == ResearchShip)
        {
            toFakeOneShip(ship,&temp1, &temp2);

            //den draw da linez
            toDrawMoveToLine(ship, linecolor, pulsecolor);
            //toDrawMoveFromLine(ship);
            toDrawMoveCircle(ship, scale, linecolor, pulsecolor);

            toUnFakeOneShip(ship,&temp1, &temp2);

        }
        else
        {
            //den draw da linez
            toDrawMoveToLine(ship, linecolor, pulsecolor);

            if (ship->formationcommand)
            {
                ship->formationcommand->formation.pulse = pulse_at_beginning;
            }

            //toDrawMoveFromLine(ship);
            toDrawMoveCircle(ship, scale, linecolor, pulsecolor);
        }

        //den do sum more graphics non-setting up stuff
        if (primEnabled)
        {
            primModeSet2();
        }
    }
}

void toFieldSphereDraw(ShipPtr ship, real32 radius, color colour)
{
    //Turn off 2D primmode
    primModeClear2();

    udword segs   = 32 * 8;
    udword spokes = 4;
    primCircleOutline3( &ship->posinfo.position, radius, segs, spokes, colour, X_AXIS );
    primCircleOutline3( &ship->posinfo.position, radius, segs, spokes, colour, Y_AXIS );
    primCircleOutline3( &ship->posinfo.position, radius, segs, spokes, colour, Z_AXIS );

    //Turn on 2D primmode
    primModeSet2();
}

// Draw a circle around the Z axis which moves downwards. IE outlining a sphere slice
void toActiveProxSensorDraw( ShipPtr ship, real32 sphereRadius, color col )
{
    // Constants
    const real32 dirMove    = -1.0f;                            // Moves downwards over time
    const real32 distPerSec = 7500.0f;                          // Worldspace distance covered on Z axis per second
    const real32 timeDelay  = 0.55f;                            // Time between movements in secs
    const real32 timeMove   = sphereRadius * 2.0f / distPerSec; // Time overall movement takes in secs
    const real32 timeTotal  = timeMove + timeDelay;             // Time total per cycle

    // Where are we now?
    real32 state = fmodf( rintUniverseElapsedTime(), timeTotal );

    // Draw nothing in delay phase
    if (state > timeMove)
        return;

    // Transform range into useful parameters
    real32 frac = state / timeMove;   // Range  0 :  1
    real32 ucy  = frac * 2.0f - 1.0f; // Range -1 : +1, this will be our unit circle Y

    // Now unit sphere slice radius as a function of Y.
    // We have one coordinate on the unit circle, so conjure the other.
    real32 ucx = sqrtf( 1.0f - sqr(ucy) ); // Range 0 : 1 : 0

    // Position and scale into the sphere
    vector pos    = ship->posinfo.position;
    real32 radius = ucx * sphereRadius;
    pos.z        += ucy * sphereRadius * dirMove;

    // Draw
    primModeClear2();
    primCircleOutline3( &pos, radius, 32*8, 0, col, Z_AXIS );
    primModeSet2();
}
