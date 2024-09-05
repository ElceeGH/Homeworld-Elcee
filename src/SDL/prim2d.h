/*=============================================================================
    Name    : prim2d.h
    Purpose : Abstraction for drawing 2D primitives.

    Created 6/26/1997 by lmoloney
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#ifndef ___PRIM2D_H
#define ___PRIM2D_H

#include "Color.h"
#include "main.h"

/*=============================================================================
    Functions:
=============================================================================*/
#ifdef HW_BUILD_FOR_DEBUGGING

#define PRIM_ERROR_CHECKING     1               //general error checking

#else

#define PRIM_ERROR_CHECKING     0               //general error checking

#endif


/*=============================================================================
    Define:
=============================================================================*/
#define P2_OvalSegments         16

/*=============================================================================
    Type definitions:
=============================================================================*/
//basic 2D rectangle
typedef struct rectanglef {
    real32 x0, y0, x1, y1;
} rectanglef;

typedef struct rectanglei {
    sdword x0, y0, x1, y1;
} rectanglei;

//basic triangle structure
typedef struct triangle {
    real32 x0, y0, x1, y1, x2, y2;
} triangle;

//basic oval structure
typedef struct oval {
    real32 centreX, centreY;
    real32 radiusX, radiusY;
} oval;

/*=============================================================================
    Data:
=============================================================================*/
extern sdword primModeEnabled;

/*=============================================================================
    Macros:
=============================================================================*/
#define primModeSet2() if (!primModeEnabled) primModeSetFunction2();
#define primModeClear2() if (primModeEnabled) primModeClearFunction2();

#define primScreenToGLX(x) (       ((real32)(x)) / (real32)(MAIN_WindowWidth)  * 2.0f - 1.0f)
#define primScreenToGLY(y) (1.0f - ((real32)(y)) / (real32)(MAIN_WindowHeight) * 2.0f)
#define primScreenToGLScaleX(x) ((real32)(x) / (real32)(MAIN_WindowWidth) * 2.0f)
#define primScreenToGLScaleY(y) ((real32)(y) / (real32)(MAIN_WindowHeight) * 2.0f)
#define primGLToScreenX(x) (MAIN_WindowWidth  / 2 + ((x) * (real32)MAIN_WindowWidth  / 2.0f))
#define primGLToScreenY(y) (MAIN_WindowHeight / 2 - ((y) * (real32)MAIN_WindowHeight / 2.0f))
#define primGLToScreenScaleX(x) (((x) * (real32)MAIN_WindowWidth  / 2.0f))
#define primGLToScreenScaleY(y) (((y) * (real32)MAIN_WindowHeight / 2.0f))

#if PRIM_ERROR_CHECKING
#define primErrorMessagePrint()  primErrorMessagePrintFunction(__FILE__, __LINE__)
#else
#define primErrorMessagePrint()
#endif

/*=============================================================================
    Functions:
=============================================================================*/

//enable/disable primitive drawing mode do not call directly, use macros instead
void primModeSetFunction2(void);
void primModeClearFunction2(void);

//draw a single colored triangle
void primTriSolid2(triangle *tri, color c);
void primTriOutline2(triangle *tri, real32 thickness, color c);

//draw a rectanglef
void primRectSolid2(rectanglef *rect, color c);
void primRectTranslucent2(rectanglef *rect, color c);
void primBeveledRectSolid(rectanglef *rect, color c, uword xb, uword yb);
void primRectOutline2(rectanglef *rect, real32 thickness, color c);
void primRectShaded2(rectanglef *rect, color *c); // color *c is a pointer to an array of 4 color values
void primRectSolidTextured2(rectanglef *rect, color c);

//draw a rectanglei
void primRectiSolid2(rectanglei *rect, color c);
void primRectiTranslucent2(rectanglei *rect, color c);
void primBeveledRectiSolid(rectanglei *rect, color c, uword xb, uword yb);
void primRectiOutline2(rectanglei *rect, real32 thickness, color c);
void primRectiShaded2(rectanglei *rect, color *c); // color *c is a pointer to an array of 4 color values
void primRectiSolidTextured2(rectanglei *rect, color c);

//draw a line
void primLine2(real32 x0, real32 y0, real32 x1, real32 y1, color c);
void primNonAALine2(real32 x0, real32 y0, real32 x1, real32 y1, color c);
void primLineThick2(real32 x0, real32 y0, real32 x1, real32 y1, real32 thickness, color c);

//draw a line, integer coords
void primLinei2(sdword x0, sdword y0, sdword x1, sdword y1, color c);

//draw a line loop
void primLineLoopStart2(real32 thickness, color c);
void primLineLoopPoint3F(real32 x, real32 y);
void primLineLoopEnd2(void);

//draw solid circular things
void primCircleSolid2(real32 x, real32 y, real32 rad, sdword nSlices, color c);
void primCircleBorder(real32 x, real32 y, real32 radInner, real32 radOuter, sdword nSlices, color colInner);

//2d rectangle utility functions
void primRectiUnion2(rectanglei *result, rectanglei *r0, rectanglei *r1);
void primRealRectUnion2(rectanglef *result, rectanglef *r0, rectanglef *r1);

//draw oval arcs
void primOvalArcOutline2(oval *o, real32 radStart, real32 radEnd, real32 thickness, sdword segments, color c);
void primGLCircleOutline2(real32 x, real32 y, real32 radius, sdword nSegments, color c);

//series of successively blended rect outlines
void primSeriesOfRects(rectanglef *rect, uword width, color fore, color back, uword steps);
void primSeriesOfRoundRects(rectanglef *rect, uword width, color fore, color back, uword steps, uword xb, uword yb);

//report errors
void primErrorMessagePrintFunction(char *file, sdword line);

//utility functions
real32     primPointLineIntersection(real32 xp, real32 yp, real32 x0, real32 y0, real32 x1, real32 y1);
rectanglei rectToIntRect( rectanglef* rect );
rectanglef rectToFloatRect( rectanglei* rect );
void       scissorClearDepthInRect( rectanglei* rect );

#endif
