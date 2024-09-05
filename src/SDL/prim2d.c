/*=============================================================================
    Name    : prim2d.c
    Purpose : Functions to draw 2D primitives.

    Created 6/26/1997 by lmoloney
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#include "prim2d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "Debug.h"
#include "FastMath.h"
#include "render.h"
#include "rglu.h"
#include "rStateCache.h"

/*=============================================================================
    Data:
=============================================================================*/
sdword primModeEnabled = FALSE;

/*=============================================================================
    Functions:
=============================================================================*/

/*-----------------------------------------------------------------------------
    Name        : primModeSetFunction2
    Description : Enables the primitive drawing mode.
    Inputs      : void
    Outputs     : sets primModeEnabled TRUE
    Return      : void
----------------------------------------------------------------------------*/
void primModeSetFunction2(void)
{
    glShadeModel(GL_FLAT);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);

    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();                                         //perform no transformations on the 2D primitives
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);

    rndLightingEnable(FALSE);             //mouse is self-illuminated
    rndTextureEnable(FALSE);
    primModeEnabled = TRUE;
}

/*-----------------------------------------------------------------------------
    Name        : primModeClearFunction2
    Description : Disables the primitive drawing mode.
    Inputs      : void
    Outputs     : sets primModeEnabled FALSE
    Return      : void
----------------------------------------------------------------------------*/
void primModeClearFunction2(void)
{
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    primModeEnabled = FALSE;
}

/*-----------------------------------------------------------------------------
    Name        : primTriSolid2
    Description : Draw a solid 2D triangle
    Inputs      : tri - pointer to triangle structure containing coordinates of corners
                  c - color to draw it in
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void primTriSolid2(triangle *tri, color c)
{
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glBegin(GL_TRIANGLES);
    glVertex2f(primScreenToGLX(tri->x0), primScreenToGLY(tri->y0));
    glVertex2f(primScreenToGLX(tri->x1), primScreenToGLY(tri->y1));
    glVertex2f(primScreenToGLX(tri->x2), primScreenToGLY(tri->y2));
    glEnd();
}

/*-----------------------------------------------------------------------------
    Name        : primTriOutline2
    Description : Draw a 2D triangle outline
    Inputs      : tri - pointer to triangle structure containing coordinates of corners
                  thickness - thickness of the lines
                  c - color to draw it in
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void primTriOutline2(triangle *tri, real32 thickness, color c)
{
    GLfloat linewidth;
    glGetFloatv(GL_LINE_WIDTH, &linewidth);
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glLineWidth((GLfloat)thickness);
    glBegin(GL_LINE_LOOP);
    glVertex2f(primScreenToGLX(tri->x0), primScreenToGLY(tri->y0));
    glVertex2f(primScreenToGLX(tri->x1), primScreenToGLY(tri->y1));
    glVertex2f(primScreenToGLX(tri->x2), primScreenToGLY(tri->y2));
    glEnd();
    glLineWidth(linewidth);
}

/*-----------------------------------------------------------------------------
    Name        : primRectSolidTextured2
    Description : Draw a solid 2d rectanglef with the current texture mapped thereupon.
    Inputs      : rect - pointer to rectanglef structure containing coordinates.
                : c    - color of the rectangle
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
#define COORD(S,T,X,Y) \
    glTexCoord2f(S, T); \
    glVertex2f(primScreenToGLX(X), primScreenToGLY(Y));

void primRectSolidTextured2(rectanglef *rect, color c)
{
    glColor4ub(colRed(c), colGreen(c), colBlue(c), colAlpha(c));

    rndTextureEnable(TRUE);

    glBegin(GL_QUADS);
    COORD(0.0f, 0.0f, rect->x0, rect->y0);
    COORD(0.0f, 1.0f, rect->x0, rect->y1);
    COORD(1.0f, 1.0f, rect->x1, rect->y1);
    COORD(1.0f, 0.0f, rect->x1, rect->y0);
    glEnd();

    rndTextureEnable(FALSE);
}
#undef COORD

/*-----------------------------------------------------------------------------
    Name        : primRectSolid2
    Description : Draw a solid 2d rectanglef.
    Inputs      : rect - pointer to rectanglef structure containing coordinates.
                  c - color to draw it in.
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void primRectSolid2(rectanglef *rect, color c)
{
    glColor4ub(colRed(c), colGreen(c), colBlue(c), colAlpha(c));
    glBegin(GL_QUADS);
    glVertex2f(primScreenToGLX(rect->x0), primScreenToGLY(rect->y0));
    glVertex2f(primScreenToGLX(rect->x0), primScreenToGLY(rect->y1));
    glVertex2f(primScreenToGLX(rect->x1), primScreenToGLY(rect->y1));
    glVertex2f(primScreenToGLX(rect->x1), primScreenToGLY(rect->y0));
    glEnd(); 
}

/*-----------------------------------------------------------------------------
    Name        : primRectTranslucent2
    Description : Draw a translucent 2d rectanglef.
    Inputs      : rect - pointer to rectanglef structure containing coordinates.
                  c - color to draw it in.
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void primRectTranslucent2(rectanglef* rect, color c)
{
    GLboolean blendOn;

    blendOn = glIsEnabled(GL_BLEND);
    if (!blendOn) glEnable(GL_BLEND);
    glColor4ub(colRed(c), colGreen(c), colBlue(c), colAlpha(c));
    glBegin(GL_QUADS);
    glVertex2f(primScreenToGLX(rect->x0), primScreenToGLY(rect->y0));
    glVertex2f(primScreenToGLX(rect->x0), primScreenToGLY(rect->y1));
    glVertex2f(primScreenToGLX(rect->x1), primScreenToGLY(rect->y1));
    glVertex2f(primScreenToGLX(rect->x1), primScreenToGLY(rect->y0));
    glEnd();
    
    if (!blendOn) glDisable(GL_BLEND);
}

/*-----------------------------------------------------------------------------
    Name        : primRectOutline2
    Description : Draw an outline 2d rectanglef.
    Inputs      : rect - pointer to rectanglef structure containing coordinates.
                  thickness - thickness of the lines
                  c - color to draw it in.
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void primRectOutline2(rectanglef *rect, real32 thickness, color c)
{
    real32 bottom = rect->y1 - 1;
    GLfloat linewidth;
    glGetFloatv(GL_LINE_WIDTH, &linewidth);

    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glLineWidth(thickness);

    glBegin(GL_LINE_LOOP);
    glVertex2f(primScreenToGLX(rect->x0), primScreenToGLY(rect->y0));
    glVertex2f(primScreenToGLX(rect->x1), primScreenToGLY(rect->y0));
    glVertex2f(primScreenToGLX(rect->x1), primScreenToGLY(bottom));
    glVertex2f(primScreenToGLX(rect->x0), primScreenToGLY(bottom));
    glEnd();

    glLineWidth(linewidth);
}

/*-----------------------------------------------------------------------------
    Name        : primRectShaded2
    Description : Draw a shaded 2d rectanglef.
    Inputs      : rect - pointer to rectanglef structure containing coordinates.
                  c - pointer to 4 color values to draw it in.
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void primRectShaded2(rectanglef *rect, color *c)
{
    glShadeModel(GL_SMOOTH);

    glBegin(GL_QUADS);

    glColor3ub(colRed(c[0]), colGreen(c[0]), colBlue(c[0]));
    glVertex2f(primScreenToGLX(rect->x0), primScreenToGLY(rect->y0));

    glColor3ub(colRed(c[1]), colGreen(c[1]), colBlue(c[1]));
    glVertex2f(primScreenToGLX(rect->x0), primScreenToGLY(rect->y1));

    glColor3ub(colRed(c[2]), colGreen(c[2]), colBlue(c[2]));
    glVertex2f(primScreenToGLX(rect->x1), primScreenToGLY(rect->y1));

    glColor3ub(colRed(c[3]), colGreen(c[3]), colBlue(c[3]));
    glVertex2f(primScreenToGLX(rect->x1), primScreenToGLY(rect->y0));

    glEnd();
}

/*-----------------------------------------------------------------------------
    Name        : primRectUnion2
    Description : Compute the union of 2 2d rectangles
    Inputs      : result - destination of union rect
                  r0, r1 - rectangles to compute union of
    Outputs     : fills reslult with union rect
    Return      : void
    Notes       : The x1/y1 parameters of the rectangles should be larger
                    than the x0/y0 members.
                  If no union exists (rectangles do not overlap), the x0
                    and/or y0 members of the reslult will be the same as
                    the x1 and/or y1 members.
----------------------------------------------------------------------------*/
void primRectiUnion2(rectanglei *result, rectanglei *r0, rectanglei *r1)
{
    result->x0 = max(r0->x0, r1->x0);                       //get min/max bounds
    result->y0 = max(r0->y0, r1->y0);
    result->x1 = min(r0->x1, r1->x1);
    result->y1 = min(r0->y1, r1->y1);

    result->x0 = min(result->x0, result->x1);               //make sure not negative width/height
    result->y0 = min(result->y0, result->y1);
}

/*-----------------------------------------------------------------------------
    Name        : primRealRectUnion2
    Description : Same as above, just with real numbers instead
    Inputs      :
    Outputs     :
    Return      :
    Notes       :
----------------------------------------------------------------------------*/
void primRealRectUnion2(rectanglef *result, rectanglef *r0, rectanglef *r1)
{
    result->x0 = max(r0->x0, r1->x0);                       //get min/max bounds
    result->y0 = max(r0->y0, r1->y0);
    result->x1 = min(r0->x1, r1->x1);
    result->y1 = min(r0->y1, r1->y1);

    result->x0 = min(result->x0, result->x1);               //make sure not negative width/height
    result->y0 = min(result->y0, result->y1);
}

/*-----------------------------------------------------------------------------
    Name        : primOvalArcOutline2
    Description : Draw an outline section of an oval.
    Inputs      : o - oval structure describing location and size on-screen
                  degStarts, degEnd - degree stations of start and end of line
                  thickness - thickness of lines
                  segments - number of line segments for a complete oval
                  c - color to draw outline in
    Outputs     : ..
    Return      : void
    Note        : The coordinate system used will be the engineering system
                    where up (x = 0, y = -1) is 0 rad.
----------------------------------------------------------------------------*/
void primOvalArcOutline2(oval *o, real32 radStart, real32 radEnd, real32 thickness, sdword segments, color c)
{
    sdword segment, endSegment;
    real32 angle, angleInc;
    real32 centreX, centreY, width, height;
    real32 x, y;
    GLfloat linewidth;
    glGetFloatv(GL_LINE_WIDTH, &linewidth);

    centreX = primScreenToGLX(o->centreX);                  //get floating-point version of oval attributes
    centreY = primScreenToGLY(o->centreY);
    width  = primScreenToGLScaleX(o->radiusX);
    height = primScreenToGLScaleY(o->radiusY);

    segment    = (sdword)(radStart * (real32)segments / (2.0f * PI));//get starting segment
    endSegment = (sdword)(radEnd   * (real32)segments / (2.0f * PI) - 0.01f);//get ending segment

    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glLineWidth((GLfloat)thickness);
    glBegin(GL_LINE_STRIP);

    x = centreX + sinf(radStart) * width;    //first vertex
    y = centreY + cosf(radStart) * height;
 
    glVertex2f(x, y);
    
    segment++;
    angle    = (real32)segment * (2.0f * PI / (real32)segments);
    angleInc = (2.0f * PI / (real32)segments);

    for (; segment <= endSegment; segment++)
    {                                                       //for start and all complete segments
        x = centreX + sinf(angle) * width;
        y = centreY + cosf(angle) * height;

        glVertex2f(x, y);
        
        angle += angleInc;                                  //update angle
    }
    x = centreX + sinf(radEnd) * width;
    y = centreY + cosf(radEnd) * height;

    glVertex2f(x, y);                                       //draw last vertex
    
    glEnd();
    glLineWidth(linewidth);
}

/*-----------------------------------------------------------------------------
    Name        : primCircleOutline2
    Description : Draw a circle
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void primGLCircleOutline2(real32 x, real32 y, real32 radius, sdword nSegments, color c)
{
    float  angleInc = 2.0f * PI / (real32)nSegments;
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glBegin(GL_LINE_STRIP);

    sdword index;
    float angle;;
    for (index=0, angle=angleInc;  index<=nSegments;  index++, angle+=angleInc)
    {
        glVertex2f(x + sinf(angle) * radius,
                   y + cosf(angle) * radius);
    }
    glEnd();
}

/*-----------------------------------------------------------------------------
    Name        : primErrorMessagePrint
    Description : Print any errors generated by rendering system
    Inputs      : line, file - location function called from
    Outputs     : checks error messages and prints out any errors found
    Return      :
----------------------------------------------------------------------------*/
void primErrorMessagePrintFunction(char *file, sdword line)
{
    GLenum errorEnum;
    //there can be multiple errors simultaneously.  detect them all
    //in one call to this fn for debugging simplicity
    for (;;)
    {
        errorEnum = glGetError();
        if (errorEnum == GL_NO_ERROR)
        {
            return;
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : primLine2
    Description : Draw a line 1 pixel wide
    Inputs      : c - attributes of line to be drawn
                  x0, y0, x1, y1 - start/end of line segment
    Outputs     : Sets GL_COLOR
    Return      : void
----------------------------------------------------------------------------*/
void primLine2(real32 x0, real32 y0, real32 x1, real32 y1, color c)
{
    bool blendon;

    blendon = glIsEnabled(GL_BLEND);
    if (!blendon) glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glBegin(GL_LINES);
    glVertex2f(primScreenToGLX(x0), primScreenToGLY(y0));
    glVertex2f(primScreenToGLX(x1), primScreenToGLY(y1));
    glEnd();
    glDisable(GL_LINE_SMOOTH);
    if (!blendon) glDisable(GL_BLEND);
}

/*-----------------------------------------------------------------------------
    Name        : primLineThick2
    Description : Draw a line "thickness" pixels wide
    Inputs      : c - attributes of line to be drawn
                  x0, y0, y1, y1 - start/end of line segment
                  thickness - width of line
    Outputs     : Sets GL_COLOR
    Return      : void
----------------------------------------------------------------------------*/
void primLineThick2(real32 x0, real32 y0, real32 x1, real32 y1, real32 thickness, color c)
{
    GLfloat linewidth;
    glGetFloatv(GL_LINE_WIDTH, &linewidth);
    glLineWidth((GLfloat)thickness);
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glBegin(GL_LINES);
    glVertex2f(primScreenToGLX(x0), primScreenToGLY(y0));
    glVertex2f(primScreenToGLX(x1), primScreenToGLY(y1));
    glEnd();
    glLineWidth(linewidth);
}

void primLinei2(sdword x0, sdword y0, sdword x1, sdword y1, color c) {
    primLine2( (real32)x0, (real32)y0, (real32)x1, (real32)y1, c );
}

/*-----------------------------------------------------------------------------
    Name        : primLineLoopStart2
    Description : Start a line-loop by setting up color and thickness
    Inputs      : thickness, c - attributes of lines to be drawn
    Outputs     : Sets GL_COLOR and GL_LINE_THICKNESS
    Return      : void
    Note        : Must be matched with a primLineLoopEnd2 and have only
                    primLineLoopPoint calls inbetween
----------------------------------------------------------------------------*/
static bool LLblendon;
static GLfloat LLlinewidth;
void primLineLoopStart2(real32 thickness, color c)
{
    glGetFloatv(GL_LINE_WIDTH, &LLlinewidth);
    LLblendon = glIsEnabled(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    if (!LLblendon) glEnable(GL_BLEND);
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glLineWidth((GLfloat)thickness);
    glBegin(GL_LINE_LOOP);
}

/*-----------------------------------------------------------------------------
    Name        : primLineLoopPoint3F
    Description : Insert a point in a line-loop
    Inputs      : x, y - unconverted floating-point screen coordinates
    Outputs     : calls glVertex with the coordinates
    Return      :
----------------------------------------------------------------------------*/
void primLineLoopPoint3F(real32 x, real32 y)
{
    glVertex2f(x, y);
}

/*-----------------------------------------------------------------------------
    Name        : primLineLoopEnd2
    Description : End a line - loop sequence
    Inputs      : void
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void primLineLoopEnd2(void)
{
    glEnd();
    glLineWidth(LLlinewidth);
    if (!LLblendon) glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}

/*-----------------------------------------------------------------------------
    Name        : decRect
    Description : decrease size of a rectanglef
    Inputs      : rect - the rectanglef, width - amount to dec
    Outputs     : rect is modified
    Return      :
----------------------------------------------------------------------------*/
void decRect(rectanglef *rect, uword width)
{
    rect->x0 = rect->x0 + width;
    rect->y0 = rect->y0 + width;
    rect->x1 = rect->x1 - width;
    rect->y1 = rect->y1 - width;
}

/*-----------------------------------------------------------------------------
    Name        : primSeriesOfRects
    Description : draws a series of rects decreasing in size and color
    Inputs      : rect - starting rectanglef, width - width,
                  fore - fg, back - fg, steps - number of rects
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void primSeriesOfRects(rectanglef *rect, uword width,
                       color fore, color back, uword steps)
{
    // ignore width
    rectanglef drect;
    real32 cfrac, frac;
    uword i;
    color Color;
    cfrac = (real32)1.0 / (real32)steps;
    steps++;
    frac = (real32)1.0;
    Color = fore;
    memcpy(&drect, rect, sizeof(rectanglef));
    for (i = 0; i < steps; i++)
    {
        decRect(&drect, 1);
        primRectOutline2(&drect, 2, Color);
        frac -= cfrac;
        Color = colBlend(fore, back, frac);
    }
}

#define SX(x) primScreenToGLX(x)
#define SY(y) primScreenToGLY(y)
#define X0 rect->x0
#define Y0 rect->y0
#define X1 rect->x1
#define Y1 rect->y1
#define SEGS 4
/*-----------------------------------------------------------------------------
    Name        : primRectSolid2
    Description : Draw a solid 2d rectanglef.
    Inputs      : rect - pointer to rectanglef structure containing coordinates.
                  c - color to draw it in, xb - x offset, yb - y offset
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void primBeveledRectSolid(rectanglef *rect, color c, uword xb, uword yb)
{
    bool cull;

    cull = glIsEnabled(GL_CULL_FACE) ? TRUE : FALSE;
    glDisable(GL_CULL_FACE);
    glBegin(GL_POLYGON);
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glVertex2f(SX(X0+xb), SY(Y0));
    glVertex2f(SX(X1-xb), SY(Y0));
    glVertex2f(SX(X1), SY(Y0+yb));
    glVertex2f(SX(X1), SY(Y1-yb));
    glVertex2f(SX(X1-xb), SY(Y1));
    glVertex2f(SX(X0+xb), SY(Y1));
    glVertex2f(SX(X0), SY(Y1-yb));
    glVertex2f(SX(X0), SY(Y0+yb));
    glEnd();
    if (cull)
    {
        glEnable(GL_CULL_FACE);
    }
}


/*-----------------------------------------------------------------------------
    Name        : primRoundRectOutline
    Description : Draw an outline 2d rounded rectanglef.
    Inputs      : rect - pointer to rectanglef structure containing coordinates.
                  thickness - thickness of the lines
                  c - color to draw it in, xb - x offset, yb - y offset
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void primRoundRectOutline(rectanglef *rect, real32 thickness, color c, uword xb, uword yb)
{
    oval o;
    sdword segs = SEGS;
    GLfloat linewidth;
    glGetFloatv(GL_LINE_WIDTH, &linewidth);

    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glLineWidth((GLfloat)thickness);
    glBegin(GL_LINES);
    glVertex2f(SX(X0+xb), SY(Y0));
    glVertex2f(SX(X1-xb), SY(Y0));
    glVertex2f(SX(X1), SY(Y0+yb));
    glVertex2f(SX(X1), SY(Y1-yb));
    glVertex2f(SX(X1-xb), SY(Y1));
    glVertex2f(SX(X0+xb), SY(Y1));
    glVertex2f(SX(X0), SY(Y1-yb));
    glVertex2f(SX(X0), SY(Y0+yb));
    glEnd();
    glLineWidth(linewidth);

//    if (xb > 4 || yb > 4)
//        segs *= 2;

    // upper left
    o.centreX = X0+xb;
    o.centreY = Y0+yb;
    o.radiusX = xb;
    o.radiusY = yb;
    primOvalArcOutline2(&o, 3*PI/2, TWOPI, 2, segs, c);

    // upper right
    o.centreX = X1-xb;
    primOvalArcOutline2(&o, (real32)0, PI/2, 2, segs, c);

    // lower right
    o.centreY = Y1-yb;
    primOvalArcOutline2(&o, PI/2, PI, 2, segs, c);

    // lower left
    o.centreX = X0+xb;
    primOvalArcOutline2(&o, PI, 3*PI/2, 2, segs, c);
}
/*-----------------------------------------------------------------------------
    Name        : primSeriesOfRoundRects
    Description : draws a series of rects decreasing in size and color
    Inputs      : rect - starting rectanglef, width - width,
                  fore - fg, back - fg, steps - number of rects,
                  xb - x offset, yb - y offset
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void primSeriesOfRoundRects(rectanglef *rect, uword width,
                            color fore, color back, uword steps,
                            uword xb, uword yb)
{
    // ignore width
    rectanglef drect;
    real32 cfrac, frac;
    uword i;
    color Color;
    cfrac = (real32)1.0 / (real32)steps;
    steps++;
    frac = (real32)1.0;
    Color = fore;
    memcpy(&drect, rect, sizeof(rectanglef));
    for (i = 0; i < steps; i++)
    {
        decRect(&drect, 1);
        primRoundRectOutline(&drect, 2, Color, xb, yb);
        frac -= cfrac;
        Color = colBlend(fore, back, frac);
    }
}

/*-----------------------------------------------------------------------------
    Name        : partCircleSolid2
    Description : Render a 2d circle, like the 3D one.
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void primCircleSolid2(real32 x, real32 y, real32 rad, sdword nSlices, color c)
{
    sdword index;
    GLfloat v[2];
    real32 theta;
    vector centre;
    real32 radiusX, radiusY;
    bool cull;

    cull = glIsEnabled(GL_CULL_FACE) ? TRUE : FALSE;

    centre.x = primScreenToGLX(x);
    centre.y = primScreenToGLY(y);
    radiusX = primScreenToGLScaleX(rad);
    radiusY = primScreenToGLScaleY(rad);

    glColor4ub(colRed(c), colGreen(c), colBlue(c), colAlpha(c));
    v[0] = centre.x;
    v[1] = centre.y;
    glDisable(GL_CULL_FACE);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(v[0], v[1]);
    for (index = 0, theta = 0.0; index < nSlices; index++)
    {
        v[0] = centre.x + sinf(theta) * radiusX;
        v[1] = centre.y + cosf(theta) * radiusY;
        theta += 2.0f * PI / (real32)nSlices;
        glVertex2f(v[0], v[1]);
    }
    v[0] = centre.x;
    v[1] = centre.y + radiusY;
    glVertex2f(v[0], v[1]);
    glEnd();
    if (cull)
    {
        glEnable(GL_CULL_FACE);
    }
}

/*-----------------------------------------------------------------------------
    Name        : primCircleBorder
    Description : Draw a special type of circle with a solid inner with a
                    alpha ramped outer ring.
    Inputs      : x, y - location of centre of image
                  radInner - radius of the inner circle
                  radOuter - total radius of the inner and outer circles
                  nSlices - number of radial slices must be multiple of 2
                  colInner - color of inner circle
                  colOuter - color of the outer edge
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void primCircleBorder(real32 x, real32 y, real32 radInner, real32 radOuter, sdword nSlices, color colInner)
{
    sdword index;
    real32 centreX, centreY;
    real32 theta, addAmount;
    real32 radXInner, radXOuter, radYInner, radYOuter;
    real32 x0, y0, x1, y1, x2, y2;
    real32 sinTheta, cosTheta;
    ubyte red = colRed(colInner);
    ubyte green = colGreen(colInner);
    ubyte blue = colBlue(colInner);

    dbgAssertOrIgnore(nSlices >= 3);

    centreX = primScreenToGLX(x);                           //make floating-point versions of parameters
    centreY = primScreenToGLY(y);
    radXInner = primScreenToGLScaleX(radInner);
    radYInner = primScreenToGLScaleY(radInner);
    radXOuter = primScreenToGLScaleX(radOuter);
    radYOuter = primScreenToGLScaleY(radOuter);
    addAmount = (real32)(2.0f * PI / (real32)(nSlices - 1));
    theta = addAmount;

    x0 = x2 = centreX;
    y0 = centreY - radYInner;
    y2 = centreY - radYOuter;

    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    for (index = 0; index < nSlices; index++)
    {
        glBegin(GL_TRIANGLE_FAN);
        glColor4ub(red, green, blue, 255);

        glVertex2f(x0, y0);                                 //2 common points
        glVertex2f(centreX, centreY);

        sinTheta = sinf(theta);
        cosTheta = cosf(theta);

        x0 = centreX + sinTheta * radXInner;
        y0 = centreY + cosTheta * radYInner;
        glVertex2f(x0, y0);                                 //complete
        glColor4ub(red, green, blue, 0);

        x1 = centreX + sinTheta * radXOuter;
        y1 = centreY + cosTheta * radYOuter;

        glVertex2f(x1, y1);                                 //complete
        glVertex2f(x2, y2);
        x2 = x1;
        y2 = y1;

        glEnd();
        theta += addAmount;
    }
    glDisable(GL_BLEND);
    glShadeModel(GL_FLAT);
}




void primRectiSolid2(rectanglei *rect, color c) {
    rectanglef rectf = rectToFloatRect( rect );
    primRectSolid2( &rectf, c );
}

void primRectiTranslucent2(rectanglei *rect, color c) {
    rectanglef rectf = rectToFloatRect( rect );
    primRectTranslucent2( &rectf, c );
}

void primBeveledRectiSolid(rectanglei *rect, color c, uword xb, uword yb) {
    rectanglef rectf = rectToFloatRect( rect );
    primBeveledRectSolid( &rectf, c, xb, yb );
}

void primRectiOutline2(rectanglei *rect, real32 thickness, color c) {
    rectanglef rectf = rectToFloatRect( rect );
    primRectOutline2( &rectf, thickness, c );
}

void primRectiShaded2(rectanglei *rect, color *c) {
    rectanglef rectf = rectToFloatRect( rect );
    primRectShaded2( &rectf, c );
}

void primRectiSolidTextured2(rectanglei *rect, color c) {
    rectanglef rectf = rectToFloatRect( rect );
    primRectSolidTextured2( &rectf, c );
}




/*-----------------------------------------------------------------------------
    Name        : primPointLineIntersection
    Description : Computes an intersection of a point and a line segment
    Inputs      : xp/yp - location of point
                  x/y|0/1 - endpoints of the line segment
    Outputs     :
    Return      : positive/negative for different sides
----------------------------------------------------------------------------*/
real32 primPointLineIntersection(real32 xp, real32 yp, real32 x0, real32 y0, real32 x1, real32 y1)
{
    return((y0 - y1) * xp + (x1 - x0) * yp + (x0 * y1 - y0 * x1));
}

rectanglei rectToIntRect( rectanglef* rect ) {
    return (rectanglei) {
        (sdword) rect->x0,
        (sdword) rect->y0,
        (sdword) rect->x1,
        (sdword) rect->y1
    };
}

rectanglef rectToFloatRect( rectanglei* rect ) {
    return (rectanglef) {
        (real32) rect->x0,
        (real32) rect->y0,
        (real32) rect->x1,
        (real32) rect->y1
    };
}

void scissorClearDepthInRect( rectanglei* rect ) {
    glEnable(GL_SCISSOR_TEST);
    glScissor( rect->x0, MAIN_WindowHeight-rect->y1, rect->x1-rect->x0, rect->y1-rect->y0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}