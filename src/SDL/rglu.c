/*=============================================================================
    Name    : rglu.c
    Purpose : glu* functions for Homeworld

    Created 7/6/1998 by khent
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/


#include "rglu.h"
#include <math.h>
#include "Shader.h"
#include "FastMath.h"



#ifndef M_PI
#define M_PI 3.14159265358979324
#endif


#ifdef HW_ENABLE_GLES
    #define glFrustum glFrustumf
#endif



void rgluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
    GLfloat ymax = zNear * tanf(fovy * PI / 360.0f);
    GLfloat ymin = -ymax;

    GLfloat xmax = ymax * aspect;
    GLfloat xmin = -xmax;

    glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}



void rgluLookAt(vector eye, vector centre, vector up)
{
    vector n;
    vecSub( n, eye, centre );
    vecNormalize( &n );

    vector u;
    vecCrossProduct( u, up, n );
    vecNormalize( &u );

    vector v;
    vecCrossProduct( v, n, u );
    vecNormalize( &v );

    vector t = {
        -vecDotProduct(eye,u),
        -vecDotProduct(eye,v),
        -vecDotProduct(eye,n),
    };

    GLfloat m[16] = {
        [0]=u.x, [4]=u.y, [ 8]=u.z, [12]=t.x,
        [1]=v.x, [5]=v.y, [ 9]=v.z, [13]=t.y,
        [2]=n.x, [6]=n.y, [10]=n.z, [14]=t.z,
        [3]=0,   [7]=0,   [11]=0,   [15]=1
    };

    glMultMatrixf(m);
}
