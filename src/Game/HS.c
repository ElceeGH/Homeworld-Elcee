/*=============================================================================
        Name    : hs.c
        Purpose : hyperspace rendering & management routines

Created 7/10/1998 by khent
Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#include <ctype.h>
#include <math.h>

#include "AIVar.h"
#include "Damage.h"
#include "Debug.h"
#include "FEReg.h"
#include "glinc.h"
#include "Globals.h"
#include "GravWellGenerator.h"
#include "HS.h"
#include "Matrix.h"
#include "Memory.h"
#include "Particle.h"
#include "prim3d.h"
#include "render.h"
#include "SaveGame.h"
#include "Shader.h"
#include "SinglePlayer.h"
#include "UnivUpdate.h"
#include "rShaderProgram.h"
#include "rResScaling.h"

//scalar that determines distance out of bbox the window will travel from / to
#define HS_DIST   1.08f
#define HS_DIST_2 (HS_DIST / 2.0f)
#define hsDefaultColor colRGB(70,90,255)



extern udword rndTextureEnviron;
extern bool   rndTextureEnabled;

//guaranteed to be at least 1 gate when the singleplayer game is running
static sdword        hsStaticNumGates = 0;
static hsStaticGate* hsStaticData     = NULL;
static bool          hsGateState      = TRUE;



typedef struct HsProgramState {
    GLuint* program;            ///< Shader program pointer
    bool    programActive;      ///< Whether program is bound and active for drawing
    bool    programEnable;      ///< Whether program will be bound when hsProgramUpdate() is called
    GLfloat clipPlane[4];       ///< Hyperspace clip plane
    GLfloat glowColour[4];      ///< Hyperspace glow colour
    GLfloat crossColour[4];     ///< Hyperspace cross section colour
    GLint   locTex;
    GLint   locTexMode;
    GLint   locTexEnable;
    GLint   locClipPlane;
    GLint   locProjInv;
    GLint   locViewport;
    GLint   locGlowDist;
    GLint   locGlowCol;
    GLint   locCrossCol;
} HsProgramState;

static HsProgramState hsp = { 0 };



static void hsProgramMeshMatCallback( void ) {
    // Get the texturing mode. Homeworld does fullbright by using texture replace mode.
    glUniform1i( hsp.locTexMode,   rndTextureEnviron == RTE_Modulate );
    glUniform1i( hsp.locTexEnable, rndTextureEnabled );

    // Override backface culling. Must be off for hyperspace shader. It gets restored automatically.
    rndBackFaceCullEnable( FALSE );
}



void hsProgramUpdate( void ) {
    // Only use the shader when the clipping plane is active.
    if ( ! hsp.programEnable)
        return;

    // Initialise if needed
    if ( ! hsp.program) {
        hsp.program      = loadShaderProgram( "hyperspace.frag" );
        hsp.locTex       = glGetUniformLocation( *hsp.program, "uTex"       );
        hsp.locTexMode   = glGetUniformLocation( *hsp.program, "uTexMode"   );
        hsp.locTexEnable = glGetUniformLocation( *hsp.program, "uTexEnable" );
        hsp.locClipPlane = glGetUniformLocation( *hsp.program, "uClipPlane" );
        hsp.locProjInv   = glGetUniformLocation( *hsp.program, "uProjInv"   );
        hsp.locViewport  = glGetUniformLocation( *hsp.program, "uViewport"  );
        hsp.locGlowDist  = glGetUniformLocation( *hsp.program, "uGlowDist"  );
        hsp.locGlowCol   = glGetUniformLocation( *hsp.program, "uGlowCol"   );
        hsp.locCrossCol  = glGetUniformLocation( *hsp.program, "uCrossCol"  );
    }

    // Add callback to update the shader texture environment and such.
    meshAddMatCallback( hsProgramMeshMatCallback );

    // Remember to clean up after.
    hsp.programActive = TRUE;

    // Make viewport vector
    const hvector viewport = { 0.0f, 0.0f, (real32) MAIN_WindowWidth, (real32) MAIN_WindowHeight };

    // Inverst projection matrix for vertex position reconstruction.
    hmatrix per,inv;
    glGetFloatv( GL_PROJECTION_MATRIX, &per.m11 );
    shInvertMatrix( &inv.m11, &per.m11 );

    // Use shader program and update uniforms
    glUseProgram( *hsp.program );
    glUniform1i       ( hsp.locTex,          0                  ); // HW only uses texture unit 0
    glUniform4fv      ( hsp.locClipPlane, 1, hsp.clipPlane      );
    glUniform4fv      ( hsp.locViewport,  1, &viewport.x        );
    glUniformMatrix4fv( hsp.locProjInv,   1, GL_FALSE, &inv.m11 );
    glUniform1f       ( hsp.locGlowDist,     32.0f              );
    glUniform4fv      ( hsp.locGlowCol,   1, hsp.crossColour    );
    glUniform4fv      ( hsp.locCrossCol,  1, hsp.crossColour    );
}



void hsProgramCleanup() {
    if (hsp.programActive) {
        hsp.programActive = FALSE;
        glUseProgram( 0 );
        meshRemoveMatCallback( hsProgramMeshMatCallback );
    }
}



/// @todo Do the clipping entirely in the shader and do not use the clipping planes at all.
/// @todo Matrix state affects the clip plane. Do the transform directly instead of getting it from GL.
static void hsProgramEnable( real32 plane[] ) {
    GLdouble dplane[4] = { plane[0], plane[1], plane[2], plane[3] };

#ifdef HW_ENABLE_GLES
    glClipPlanef(GL_CLIP_PLANE0, plane);
    glEnable(GL_CLIP_PLANE0);
#else
    glClipPlane(GL_CLIP_PLANE0, dplane);
    glEnable(GL_CLIP_PLANE0);
#endif

    // Update program state to match.
    glGetClipPlane( GL_CLIP_PLANE0, dplane );
    hsp.clipPlane[0] = (real32) dplane[0];
    hsp.clipPlane[1] = (real32) dplane[1];
    hsp.clipPlane[2] = (real32) dplane[2];
    hsp.clipPlane[3] = (real32) dplane[3];
    hsp.programEnable = TRUE;
}



static void hsProgramDisable(void) {
    // Set GL clip plane (todo: remove the need for this)
    glDisable(GL_CLIP_PLANE0);

    // Update program state to match
    hsp.programEnable = FALSE;
}



static void hsProgramSetCrossColour( color col ) {
    hsp.crossColour[0] = colRed  (col) / 255.0f;
    hsp.crossColour[1] = colGreen(col) / 255.0f;
    hsp.crossColour[2] = colBlue (col) / 255.0f;
    hsp.crossColour[3] = 1.0f;

    hsp.glowColour[0] = hsp.crossColour[0] * 0.5f;
    hsp.glowColour[1] = hsp.crossColour[1] * 0.5f;
    hsp.glowColour[2] = hsp.crossColour[2] * 0.5f;
    hsp.glowColour[3] = 1.0f;
}









/*-----------------------------------------------------------------------------
    Name        : hsOrientEffect
    Description : ensures that effect is pointing in the appropriate direction.
                  *NOTE* worldspace effects only
    Inputs      : ship - the ship whose effect will be oriented
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsOrientEffect(Ship* ship)
{
    sdword    index;
    psysPtr   system;
    particle* part;

    ShipSinglePlayerGameInfo* ssinfo = ship->shipSinglePlayerGameInfo;
    StaticCollInfo* sinfo = &ship->staticinfo->staticheader.staticCollInfo;
    Effect* effect = ssinfo->hyperspaceEffect;

    dbgAssertOrIgnore(effect != NULL);

    for (index = 0; index < effect->iParticleBlock; index++)
    {
        if (effect->particleBlock[index] != NULL)
        {
            system = (psysPtr) effect->particleBlock[index];
            part = (particle*)((ubyte*)system + partHeaderSize(system));

            bitSet(part->flags, PART_XYZSCALE);

            part->wVel.x = 0.0f;
            part->wVel.y = -1.0f;
            part->wVel.z = 0.0f;

            part->tumble[0] = HS_DIST * sinfo->uplength;
            part->tumble[1] = HS_DIST * sinfo->rightlength;
            part->tumble[2] = 0.25f * sinfo->forwardlength;

            switch (ssinfo->hsState)
            {
            case HS_POPUP_INTO:
            case HS_POPUP_OUTOF:
                part->tumble[0] *= 1.0f - ssinfo->clipt;
                break;

            case HS_COLLAPSE_INTO:
            case HS_COLLAPSE_OUTOF:
                part->tumble[0] *= ssinfo->clipt;
            }
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : hsShouldDisplayEffect
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
bool hsShouldDisplayEffect(Ship* ship)
{
    Ship* host;
    bool  displayEffect;

    if (ship->clampInfo != NULL)
    {
        host = (Ship*)ship->clampInfo->host;
        if (host != NULL)// && host->shiptype == AdvanceSupportFrigate)
        {
            displayEffect = FALSE;
        }
        else
        {
            displayEffect = TRUE;
        }
    }
    else
    {
        displayEffect = TRUE;
    }

    return displayEffect;
}

/*-----------------------------------------------------------------------------
    Name        : hsStart
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsStart(Ship* ship, real32 cliptDelta, bool into, bool displayEffect)
{
    ShipSinglePlayerGameInfo* ssinfo = ship->shipSinglePlayerGameInfo;
    StaticCollInfo*           sinfo  = &ship->staticinfo->staticheader.staticCollInfo;

    ssinfo->clipt            = 1.0f;
    ssinfo->cliptDelta       = cliptDelta;
    ssinfo->hsState          = into ? HS_POPUP_INTO : HS_POPUP_OUTOF;
    ssinfo->hyperspaceEffect = NULL;

    dmgStopEffect(ship, DMG_All);

    if (!displayEffect)
    {
        //don't bother with the tons-of-alpha effect if the horsepower isn't available
        return;
    }

#if ETG_DISABLEABLE
    if (etgEffectsEnabled)
#endif
    {
        etgeffectstatic* stat = etgHyperspaceEffect->level[0];
        if (stat != NULL)
        {
            vector vecToRotate, vecRotated;

            vecToRotate.x = 0.0f;
            vecToRotate.y = 0.0f;
            vecToRotate.z = (sinfo->forwardlength);
            vecToRotate.z *= ship->magnitudeSquared;
            vecToRotate.z -= sinfo->collsphereoffset.z;

            matMultiplyMatByVec(&vecRotated, &ship->rotinfo.coordsys, &vecToRotate);

            vector  location = ship->collInfo.collPosition;
            vecAddTo(location, vecRotated);

            real32 floatUpScale    = 0.0f;//HS_DIST * sinfo->uplength;
            real32 floatRightScale = HS_DIST * sinfo->rightlength;
            real32 floatDepthScale = 0.25f   * sinfo->forwardlength;

            sdword intUpScale    = Real32ToSdword(floatUpScale);
            sdword intRightScale = Real32ToSdword(floatRightScale);
            sdword intDepthScale = Real32ToSdword(floatDepthScale);

            Effect* effect = etgEffectCreate(stat, ship,
                                     &location,
                                     &ship->posinfo.velocity,
                                     &ship->rotinfo.coordsys, 1.0f,
                                     EAF_NLips, 3, SCALECAST(intUpScale), SCALECAST(intRightScale), SCALECAST(intDepthScale));

            ssinfo->hyperspaceEffect = effect;

            hsOrientEffect(ship);
        }
    }
}

void hsGetEquation(Ship* ship, GLfloat equation[])
{
    StaticCollInfo*           sinfo  = &ship->staticinfo->staticheader.staticCollInfo;
    ShipSinglePlayerGameInfo* ssinfo = ship->shipSinglePlayerGameInfo;

    equation[0] = 0.0f;
    equation[1] = 0.0f;
    equation[2] = 1.0f;
    equation[3] = 0.0f;

    switch (ssinfo->hsState)
    {
    case HS_SLICING_INTO:
        equation[2] = -1.0f;
        equation[3] = ssinfo->clipt * (sinfo->forwardlength);
        break;
    case HS_COLLAPSE_INTO:
        equation[2] = -1.0f;
        equation[3] = -(sinfo->forwardlength);
        break;
    case HS_POPUP_OUTOF:
        equation[2] = 1.0f;
        equation[3] = -(sinfo->forwardlength);
        break;
    case HS_SLICING_OUTOF:
        equation[2] = 1.0f;
        equation[3] = -ssinfo->clipt * (sinfo->forwardlength);
        break;
    case HS_COLLAPSE_OUTOF:
        equation[2] = 1.0f;
        equation[3] = (sinfo->forwardlength);
    }
}

/*-----------------------------------------------------------------------------
    Name        : hsContinue
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsContinue(Ship* ship, bool displayEffect)
{
    real32                    equation[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    StaticCollInfo*           sinfo       = &ship->staticinfo->staticheader.staticCollInfo;
    ShipSinglePlayerGameInfo* ssinfo      = ship->shipSinglePlayerGameInfo;

    if (!displayEffect)
    {
        if (ship->clampInfo != NULL)
        {
            Ship* host = (Ship*)ship->clampInfo->host;
            if (host != NULL)// && host->shiptype == AdvanceSupportFrigate)
            {
                //we're in NLIP'ed shipspace;
                hmatrix coordMatrixForGL, prevModelview;
                glGetFloatv(GL_MODELVIEW_MATRIX, &prevModelview.m11);
                glPopMatrix();
                glPushMatrix();
                
                //put glstack info similar state as before
                hmatMakeHMatFromMat(&coordMatrixForGL,&host->rotinfo.coordsys);
                hmatPutVectIntoHMatrixCol4(host->posinfo.position,coordMatrixForGL);
                glMultMatrixf(&coordMatrixForGL.m11);//ship's rotation matrix

                real32 nliphak = host->magnitudeSquared;
                glScalef(nliphak,nliphak,nliphak);

                hsGetEquation(host, equation);
                hsProgramEnable( equation );

                glLoadMatrixf((GLfloat*)&prevModelview);
            }
        }
        return;
    }

    if (ssinfo->hyperspaceEffect != NULL)
    {
        //appropriately orient effect
        hsOrientEffect(ship);
    }

    switch (ssinfo->hsState)
    {
        case HS_POPUP_INTO:
            return;
        case HS_SLICING_INTO:
            equation[2] = -1.0f;
            equation[3] = ssinfo->clipt * (sinfo->forwardlength);
            break;
        case HS_COLLAPSE_INTO:
            if (!singlePlayerGameInfo.hyperspaceFails)
            {
                equation[2] = -1.0f;
                equation[3] = -(sinfo->forwardlength);
            }
            break;
        case HS_POPUP_OUTOF:
            equation[2] = 1.0f;
            equation[3] = -(sinfo->forwardlength);//0.0f;
            break;
        case HS_SLICING_OUTOF:
            equation[2] = 1.0f;
            equation[3] = -ssinfo->clipt * (sinfo->forwardlength);
            break;
        case HS_COLLAPSE_OUTOF:
            equation[2] = 1.0f;
            equation[3] = (sinfo->forwardlength);
            break;
        default:
            dbgFatalf(DBG_Loc, "unknown hyperspace state %d", ssinfo->hsState);
    }

    if (ssinfo->hyperspaceEffect != NULL)
    {
        vector vecToRotate = { 0.0f, 0.0f, -equation[2] * equation[3] * ship->magnitudeSquared - sinfo->collsphereoffset.z};
        vector vecRotated;
        matMultiplyMatByVec(&vecRotated, &ship->rotinfo.coordsys, &vecToRotate);

        Effect* effect = ssinfo->hyperspaceEffect;
        effect->posinfo.position = ship->collInfo.collPosition;
        vecAddTo(effect->posinfo.position, vecRotated);

        //manhandle the effect into position
        for (sdword index = 0; index < effect->iParticleBlock; index++)
        {
            if (effect->particleBlock[index] != NULL)
            {
                psysPtr   system = (psysPtr) effect->particleBlock[index];
                particle* part   = (particle*) ((ubyte*)system + partHeaderSize(system));
                part->position   = effect->posinfo.position;
            }
        }
    }

    if (!singlePlayerGameInfo.hyperspaceFails)
    {
        hsProgramEnable( equation );
    }
}

/*-----------------------------------------------------------------------------
    Name        : hsRectangle
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsRectangle(vector* origin, real32 rightlength, real32 uplength, ubyte alpha, bool outline, color c)
{
    real32 rlen = HS_DIST_2 * uplength;
    real32 ulen = HS_DIST_2 * rightlength;
    real32 v[12] = { origin->x - rlen, origin->y + ulen, origin->z,
                     origin->x + rlen, origin->y + ulen, origin->z,
                     origin->x - rlen, origin->y - ulen, origin->z,
                     origin->x + rlen, origin->y - ulen, origin->z };
    ubyte red   = colRed(c);
    ubyte green = colGreen(c);
    ubyte blue  = colBlue(c);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, v);

    glColor4ub(red, green, blue, alpha);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (outline) {
        ubyte l[4] = { 0, 1, 3, 2 };
        glLineWidth(2.0f * sqrtf(getResDensityRelative()));
        glColor4ub((ubyte)(red + 40), (ubyte)(green + 40), blue, alpha);
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_BYTE, l);
        glLineWidth(1.0f);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
}

/*-----------------------------------------------------------------------------
    Name        : hsDesat
    Description :
    Inputs      : r, g, b - output colours
                  c - input colour
                  scale - scalar for saturation
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsDesat(ubyte* r, ubyte* g, ubyte* b, color c, real32 scale)
{
    real32 h, s, v;
    real32 fr, fg, fb;

    colRGBToHSV(&h, &s, &v,
                colUbyteToReal(colRed(c)),
                colUbyteToReal(colGreen(c)),
                colUbyteToReal(colBlue(c)));
    s *= scale;
    colHSVToRGB(&fr, &fg, &fb, h, s, v);

    *r = colRealToUbyte(fr);
    *g = colRealToUbyte(fg);
    *b = colRealToUbyte(fb);
}

/*-----------------------------------------------------------------------------
    Name        : hsLine
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsLine(vector* origin, real32 rightlength, real32 uplength, ubyte alpha, color c)
{
    real32 rlen = 0.8f * rightlength;
    real32 v[6] = { origin->x, origin->y - rlen, origin->z,
                    origin->x, origin->y + rlen, origin->z };
    ubyte  red, green, blue;

    hsDesat(&red, &green, &blue, c, 0.70f);
    glColor4ub(red, green, blue, alpha);

    glLineWidth(uplength * sqrtf(getResDensityRelative()));
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, v);
    glDrawArrays(GL_LINES, 0, 2);
    glDisableClientState(GL_VERTEX_ARRAY);
    glLineWidth(1.0f);
}

/*-----------------------------------------------------------------------------
    Name        : hsUpdate
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsUpdate(Ship* ship)
{
    real32 minimum = 0.0, delta;
    ShipSinglePlayerGameInfo* ssinfo = ship->shipSinglePlayerGameInfo;
    extern real32 HS_CLIPT_NEG_THRESH;
    extern real32 HS_CLIPT_NEG_SCALAR;
    extern real32 HS_CLIPT_NEG_FINISH;
    extern real32 HS_CLIPT_POS_THRESH;
    extern real32 HS_CLIPT_POS_SCALAR;
    extern real32 HS_CLIPT_POS_START;

    if (ssinfo->hsState == HS_INACTIVE ||
        ssinfo->hsState == HS_FINISHED)
    {
        return;
    }

    switch (ssinfo->hsState)
    {
    case HS_POPUP_INTO:
    case HS_POPUP_OUTOF:
        ssinfo->clipt -= 2.5f * ssinfo->cliptDelta;
        minimum = 0.0f;
        break;
    case HS_COLLAPSE_INTO:
    case HS_COLLAPSE_OUTOF:
        ssinfo->clipt -= 2.5f * ssinfo->cliptDelta;
        minimum = 0.0f;
        break;
    case HS_SLICING_INTO:
    case HS_SLICING_OUTOF:
        delta = ssinfo->cliptDelta;
        if (ssinfo->clipt < 0.0f)
        {
            if (ssinfo->clipt > HS_CLIPT_NEG_THRESH)
            {
                delta *= HS_CLIPT_NEG_SCALAR;
            }
            else
            {
                delta *= HS_CLIPT_NEG_FINISH;
            }
        }
        else
        {
            if (ssinfo->clipt < HS_CLIPT_POS_THRESH)
            {
                delta *= HS_CLIPT_POS_SCALAR;
            }
            else
            {
                delta *= HS_CLIPT_POS_START;
            }
        }
        ssinfo->clipt -= delta;
        minimum = -1.0f;
        break;
    }

    if (ssinfo->clipt < minimum)
    {
        if (ssinfo->hsFlags & HSF_WAIT)
        {
            ssinfo->clipt = minimum;
        }
        else
        {
            ssinfo->clipt = 1.0f;
            switch (ssinfo->hsState)
            {
            case HS_POPUP_INTO:
                if(!singlePlayerGame)
                {
                    if(gravwellIsShipStuckForHyperspaceing(ship))
                    {
                        //ship is stuck because of gravwell near by
                        ssinfo->clipt = 0.0f;
                        goto noupdateHSMPOUT;
                    }
                }
                ssinfo->hsState = HS_SLICING_INTO;
noupdateHSMPOUT:
                break;
                case HS_POPUP_OUTOF:
                if (singlePlayerGame && spHoldHyperspaceWindow)
                {
                        //ship is stuck 'cause I want it to
                        ssinfo->clipt = 0.0f;
                        break;  // don't change state
                }
                ssinfo->hsState = HS_SLICING_OUTOF;
                break;
            case HS_SLICING_INTO:
                ssinfo->hsState = HS_COLLAPSE_INTO;
                break;
            case HS_SLICING_OUTOF:
                ssinfo->hsState = HS_COLLAPSE_OUTOF;
                break;
            case HS_COLLAPSE_INTO:
            case HS_COLLAPSE_OUTOF:
                hsFinish(ship);
                break;
            }
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : hsEnd
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsEnd(Ship* ship, bool displayEffect)
{
    StaticCollInfo*           sinfo  = &ship->staticinfo->staticheader.staticCollInfo;
    ShipSinglePlayerGameInfo* ssinfo = ship->shipSinglePlayerGameInfo;
    
    hsProgramDisable();

    if (!hsGateState)
    {
        return;
    }

    if (!displayEffect)
    {
        return;
    }

    switch (ssinfo->hsState)
    {
    case HS_POPUP_INTO:
    case HS_POPUP_OUTOF:
    case HS_COLLAPSE_INTO:
    case HS_COLLAPSE_OUTOF:
    case HS_SLICING_INTO:
    case HS_SLICING_OUTOF:
        break;

    default:
        return;
    }

    //setup our coordinate system so the rectangles agree w/ the ETG effect
    glPushMatrix();
    hmatrix hcoordsys;
    hmatMakeHMatFromMat(&hcoordsys, &ship->rotinfo.coordsys);
    hmatPutVectIntoHMatrixCol4(ship->collInfo.collPosition, hcoordsys);
    glMultMatrixf((GLfloat*)&hcoordsys);

    rndAdditiveBlends(TRUE);
    rndBackFaceCullEnable(FALSE);
    rndTextureEnable(FALSE);
    bool lightEnabled = rndLightingEnable(FALSE);

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    color c = hsDefaultColor;
    if (ship->staticinfo->hyperspaceColor != colBlack)
         c = ship->staticinfo->hyperspaceColor;
    hsProgramSetCrossColour( c );

    //monkey with the effect origin so it scales properly and starts in proper spot
    //vector origin = {0.0f, 0.0f, sinfo->forwardlength * ship->magnitudeSquared - sinfo->collsphereoffset.z};
    //
    //switch (ssinfo->hsState)
    //{
    //    case HS_POPUP_INTO:
    //    case HS_POPUP_OUTOF: {
    //        real32 t = 1.0f - ssinfo->clipt;
    //        hsRectangle(&origin, sinfo->rightlength, t * sinfo->uplength, 90, TRUE, c);
    //        if (ssinfo->hyperspaceEffect != NULL)
    //        {
    //            //monkey effect position
    //            vector vecTemp;
    //            matMultiplyMatByVec(&vecTemp, &ship->rotinfo.coordsys, &origin);
    //            ssinfo->hyperspaceEffect->posinfo.position = ship->collInfo.collPosition;
    //            vecAddTo(ssinfo->hyperspaceEffect->posinfo.position, vecTemp);
    //        }
    //        hsLine(&origin, sinfo->rightlength, 1.5f, (ubyte)(255.0f * ssinfo->clipt), c);
    //    } break;
    //
    //    case HS_COLLAPSE_INTO:
    //    case HS_COLLAPSE_OUTOF: {
    //        //monkey with the effect origin so it scales properly and starts in proper spot
    //        if ( ! singlePlayerGameInfo.hyperspaceFails)
    //            origin.z = -sinfo->forwardlength * ship->magnitudeSquared - sinfo->collsphereoffset.z;
    //
    //        real32 t = ssinfo->clipt;
    //        hsRectangle(&origin, sinfo->rightlength, t * sinfo->uplength, 90, TRUE, c);
    //        hsLine(&origin, sinfo->rightlength, 1.5f, (ubyte)(255.0f * (1.0f - t)), c);
    //    } break;
    //
    //    case HS_SLICING_INTO:
    //    case HS_SLICING_OUTOF: {
    //        hsRectangle(&origin, sinfo->rightlength, sinfo->uplength, 90, TRUE, c);
    //    } break;
    //}

    vector origin = { 0,0,0 };
    switch (ssinfo->hsState)
    {
    case HS_POPUP_INTO:
    case HS_POPUP_OUTOF: {
        //monkey with the effect origin so it scales properly and starts in proper spot
        origin.z = sinfo->forwardlength * ship->magnitudeSquared - sinfo->collsphereoffset.z;
        real32 t = 1.0f - ssinfo->clipt;
        hsRectangle(&origin, sinfo->rightlength, t * sinfo->uplength, 90, TRUE, c);
        if (ssinfo->hyperspaceEffect != NULL)
        {
            //monkey effect position
            vector vecTemp;
            matMultiplyMatByVec(&vecTemp, &ship->rotinfo.coordsys, &origin);
            ssinfo->hyperspaceEffect->posinfo.position = ship->collInfo.collPosition;
            vecAddTo(ssinfo->hyperspaceEffect->posinfo.position, vecTemp);
        }

        hsLine(&origin, sinfo->rightlength, 1.5f, (ubyte)(255.0f * ssinfo->clipt), c);
    } break;

    case HS_COLLAPSE_INTO:
    case HS_COLLAPSE_OUTOF:
        origin.z = (singlePlayerGameInfo.hyperspaceFails) ? sinfo->forwardlength : -sinfo->forwardlength;
        real32 t = ssinfo->clipt;
        //monkey with the effect origin so it scales properly and starts in proper spot
        origin.z *= ship->magnitudeSquared;
        origin.z -= sinfo->collsphereoffset.z;
        hsRectangle(&origin, sinfo->rightlength, t * sinfo->uplength, 90, TRUE, c);
        hsLine(&origin, sinfo->rightlength, 1.5f, (ubyte)(255.0f * (1.0f - t)), c);
        break;

    case HS_SLICING_INTO:
    case HS_SLICING_OUTOF:
        origin.z = ssinfo->clipt * (sinfo->forwardlength) * ship->magnitudeSquared - sinfo->collsphereoffset.z;
        //monkey with the effect origin so it scales properly and starts in proper spot
        hsRectangle(&origin, sinfo->rightlength, sinfo->uplength, 90, TRUE, c);
        break;
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    rndLightingEnable(lightEnabled);
    rndBackFaceCullEnable(TRUE);
    rndAdditiveBlends(FALSE);

    glPopMatrix();
}

/*-----------------------------------------------------------------------------
    Name        : hsFinish
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsFinish(Ship* ship)
{
    ShipSinglePlayerGameInfo* ssinfo = ship->shipSinglePlayerGameInfo;
    ssinfo->clipt      = -2.0f;
    ssinfo->cliptDelta = 0.0f;
    ssinfo->hsState    = (ssinfo->hsState == HS_COLLAPSE_INTO) ? HS_FINISHED : HS_INACTIVE;

    if (ssinfo->hyperspaceEffect != NULL)
    {
        etgEffectDelete(ssinfo->hyperspaceEffect);
        univDeleteEffect(ssinfo->hyperspaceEffect);
        ssinfo->hyperspaceEffect = NULL;
    }
}

/*-----------------------------------------------------------------------------
    Name        : hsStaticReset
    Description : resets (clears) static gate data
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsStaticReset(void)
{
    if (hsStaticData != NULL)
    {
        memFree(hsStaticData);
        hsStaticData = NULL;
        hsStaticNumGates = 0;
    }
}

//whether a label denotes a gate
bool hsIsStaticGate(char* label)
{
    return (tolower(label[0]) == 'g')
        && (tolower(label[1]) == 'a')
        && (tolower(label[2]) == 't')
        && (tolower(label[3]) == 'e');
}

/*-----------------------------------------------------------------------------
    Name        : hsStaticInit
    Description : iterates thru KAS's AI point list to find static hyperspace gates
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsStaticInit(sdword nVectors)
{
    hsStaticGate* pGate;
    extern LabelledVector** LabelledVectors;

    hsStaticReset();

    //iterate thru KAS labelled vectors
    hsStaticNumGates = 0;
    for (sdword i=0; i<nVectors;i++)
    {
        if (hsIsStaticGate(LabelledVectors[i]->label))
        {
            hsStaticNumGates++;
        }
    }

    if (hsStaticNumGates > 0)
    {
        hsStaticData = memAlloc(hsStaticNumGates * sizeof(hsStaticGate), "hs statics", NonVolatile);
        pGate = hsStaticData;

        for (sdword i=0; i<nVectors; i++)
        {
            if (hsIsStaticGate(LabelledVectors[i]->label))
            {
                pGate->active = TRUE;
                pGate->position.x = LabelledVectors[i]->hvector->x;
                pGate->position.y = LabelledVectors[i]->hvector->y;
                pGate->position.z = LabelledVectors[i]->hvector->z;
                pGate->rotation = LabelledVectors[i]->hvector->w;
                pGate->state = HS_STATIC_ACTIVE;
                pGate->counter = 0;
                memStrncpy(pGate->label,LabelledVectors[i]->label,HSGATE_LABEL_LEN-1);
                pGate->derelict = univAddHyperspaceGateAsDerelict(LabelledVectors[i]->hvector);
                aivarValueSet(aivarCreate(pGate->label),(sdword)pGate->derelict->health);
                pGate++;
            }
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : hsStaticDestroy
    Description : "destroy" a static gate
    Inputs      : point - used to ID which gate
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsStaticDestroy(hvector* point)
{
    sdword i;
    hsStaticGate* pGate;

    for (i = 0, pGate = (hsStaticGate*)hsStaticData; i < hsStaticNumGates; i++, pGate++)
    {
        if (pGate->position.x == point->x &&
            pGate->position.y == point->y &&
            pGate->position.z == point->z)
        {
            pGate->active = FALSE;
            pGate->state = HS_STATIC_COLLAPSING;
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : hsStaticGateRender
    Description : render a single static hyperspace gate
    Inputs      : gate - the static hyperspace gate to render
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsStaticGateRender(hsStaticGate* gate)
{
    if (singlePlayerHyperspacingInto)
    {
        return;
    }

    Derelict* derelict = gate->derelict;
    if (derelict == NULL)
    {
        return;
    }

    if (!univSpaceObjInRenderList((SpaceObj*)derelict))
    {
        return;
    }

    if (derelict != NULL)
    {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        hmatrix hmat;
        hmatMakeHMatFromMat(&hmat, &derelict->rotinfo.coordsys);
        hmatPutVectIntoHMatrixCol4(derelict->collInfo.collPosition, hmat);
        glMultMatrixf((GLfloat*)&hmat);
    }

    hsProgramDisable();
    rndAdditiveBlends(TRUE);
    rndBackFaceCullEnable(FALSE);
    rndTextureEnable(FALSE);
    bool lightEnabled = rndLightingEnable(FALSE);

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    vector origin = { 0.0f, 0.0f, 0.0f };
    color  col    =  colRGB( 70, 90, 255 );
    hsRectangle(&origin, HYPERSPACEGATE_WIDTH, HYPERSPACEGATE_HEIGHT, 90, TRUE, col);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (derelict != NULL)
    {
        glPopMatrix();
    }

    rndLightingEnable(lightEnabled);
    rndBackFaceCullEnable(TRUE);
    rndAdditiveBlends(FALSE);
}

/*-----------------------------------------------------------------------------
    Name        : hsStaticRender
    Description : render all static hyperspace gates
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsStaticRender(void)
{
    sdword i;
    hsStaticGate* pGate;

    for (i = 0, pGate = (hsStaticGate*)hsStaticData; i < hsStaticNumGates; i++, pGate++)
    {
        if (pGate->active)
        {
            hsStaticGateRender(pGate);
        }
    }
}

void hsDerelictTakesDamage(Derelict *derelict)
{
    sdword i;
    hsStaticGate* pGate;

    for (i = 0, pGate = (hsStaticGate*)hsStaticData; i < hsStaticNumGates; i++, pGate++)
    {
        if (pGate->derelict == derelict)
        {
            aivarValueSet(aivarFind(pGate->label),(sdword)derelict->health);
            break;
        }
    }
}

void hsDerelictDied(Derelict *derelict)
{
    sdword i;
    hsStaticGate* pGate;

    for (i = 0, pGate = (hsStaticGate*)hsStaticData; i < hsStaticNumGates; i++, pGate++)
    {
        if (pGate->derelict == derelict)
        {
            aivarValueSet(aivarFind(pGate->label),-1);
            pGate->derelict = NULL;
        }
    }
}

Derelict *GetHyperspaceGateFromVector(vector *compare)
{
    sdword i;
    hsStaticGate* pGate;

    for (i = 0, pGate = (hsStaticGate*)hsStaticData; i < hsStaticNumGates; i++, pGate++)
    {
        if (vecAreEqual(*compare,pGate->position))
        {
            return pGate->derelict;
        }
    }

    return NULL;
}

/*-----------------------------------------------------------------------------
    Name        : hsNoGate
    Description :
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsNoGate(bool state)
{
    hsGateState = !state;
}

/*=============================================================================
    Save Game stuff
=============================================================================*/

void hsPreFixStaticData(ubyte *data)
{
    sdword i;
    hsStaticGate* pGate;

    for (i = 0, pGate = (hsStaticGate*)data; i < hsStaticNumGates; i++, pGate++)
    {
        pGate->derelict = (DerelictPtr) SpaceObjRegistryGetID((SpaceObj *)pGate->derelict);
    }
}

void hsFixStaticData(hsStaticGate *data)
{
    sdword i;
    hsStaticGate* pGate;

    for (i = 0, pGate = data; i < hsStaticNumGates; i++, pGate++)
    {
        pGate->derelict = (Derelict *)SpaceObjRegistryGetObj((sdword)pGate->derelict);
    }
}

/*-----------------------------------------------------------------------------
    Name        : hsSetStaticData
    Description : set static hyperspace gate data, for loading
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void hsSetStaticData(sdword size, ubyte* data)
{
    hsStaticReset();
    if (size == 0)
    {
        return;
    }
    hsStaticData = (hsStaticGate*)memAlloc(size, "hs static", NonVolatile);
    memcpy(hsStaticData, data, size);
    hsStaticNumGates = size / sizeof(hsStaticGate);
}

void SaveHyperspaceGates()
{
    SaveInfoNumber(hsGateState);

    sdword size = hsStaticNumGates * sizeof(hsStaticGate);
    if (size == 0)
    {
        SaveInfoNumber(0);
        return;
    }

    ubyte* data = memAlloc(size, "temp hs static", Pyrophoric);
    memcpy(data, hsStaticData, size);

    hsPreFixStaticData(data);
    SaveInfoNumber(size);
    SaveStructureOfSize(data, size);

    memFree(data);
}

void LoadHyperspaceGates()
{
    hsGateState = LoadInfoNumber();
    sdword size = LoadInfoNumber();

    if (size == 0)
    {
        hsStaticReset();
        return;
    }

    ubyte* data = LoadStructureOfSize(size);
    hsSetStaticData(size, data);
    hsFixStaticData(hsStaticData);
    memFree(data);
}

