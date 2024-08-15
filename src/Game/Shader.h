/*=============================================================================
        Name    : shader.h
        Purpose : shader provides specialized shading models for non-rGL renderers

Created 22/06/1998 by khent
Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#ifndef ___SHADER_H
#define ___SHADER_H

#include "Color.h"
#include "Matrix.h"
#include "Types.h"
#include "Vector.h"

typedef struct {
    ubyte c[4];
} shColor;

typedef enum shSpecMode {
    shSpecModeOff     = -1, ///< No specular lighting
    shSpecModeNormalZ =  0, ///< Light intensity is taken from vertex normal z axis. (No dot product used)
    shSpecModeLights  =  1, ///< Standard specular, light vectors dotted with normals.
    shSpecModeCamera  =  2, ///< Standard specular, but uses the camera view vector as the light vector.
} SpecMode;

void shStartup(void);
void shShutdown(void);
void shTransformNormal(vector* out, vector* in, real32* m);
void shTransformVertex(vector* out, vector* in, real32* m);
void shSpecularColour(sdword specInd, sdword side, vector* vobj, vector* norm,
                      ubyte* color, real32* m, real32* minv);
void shColour(sdword side, vector* norm, ubyte* color, real32* minv);
void shColourSet(sdword side, vector* norm, real32* minv);
void shColourSet0(vector* norm);
void shInvertMatrix(real32* out, real32 const* m);
void shSetExponent(sdword index, real32 exponent);
void shSetLightPosition(sdword index, real32* position, real32* m);
void shSetGlobalAmbient(real32* ambient);
void shGetGlobalAmbient(real32* ambient);
void shSetMaterial(real32* ambient, real32* diffuse);
void shSetLighting(sdword index, real32* diffuse, real32* ambient);
void shUpdateLighting(void);
void shGammaUp(void);
void shGammaDown(void);
void shGammaReset(void);
void shGammaSet(sdword gamma);
sdword shGammaGet(void);

void shGrowBuffers(sdword nVertices);
void shShadeBuffer(sdword nVertices, hvector* source);

void shDockLight(real32 t);
void shDockLightColor(color c);

void shPushLightMatrix(hmatrix *pMat);
void shPopLightMatrix(void);

#endif
