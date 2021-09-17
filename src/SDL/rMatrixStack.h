


#pragma once
#include "Types.h"



/// 4x4 matrix. OpenGL layout.
typedef struct Matrix {
    float m[ 16 ];
} Matrix;



/// A matrix stack.
typedef struct MatrixStack {
    Matrix data[ 64 ]; ///< Stack data, newest stack at highest address
    sdword index;      ///< Stack index, counts up, initially zero
} MatrixStack;



void         msInitStacks  ( void );
MatrixStack* msGetStack    ( void );
Matrix*      msGetMatrix   ( void );
void         msSetSelect   ( sdword sel );
sdword       msGetSelect   ( void );
void         msPopMatrix   ( void );
void         msPushMatrix  ( void );
void         msMultMatrix  ( Matrix* m );
void         msSetIdentity ( Matrix* m );
void         msSetRotate   ( Matrix* m, float angle, float ax, float ay, float az );
void         msSetScale    ( Matrix* m, float x, float y, float z );
void         msSetTranslate( Matrix* m, float x, float y, float z );
void         msSetOrtho    ( Matrix* m, float l, float r, float b, float t, float n, float f );
void         msSetFrustum  ( Matrix* m, float l, float r, float b, float t, float n, float f );

