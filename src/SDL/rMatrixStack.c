/*=============================================================================
    Name    : rMatrixStack.c
    Purpose : Performs the matrix functions which used to be done by OpenGL.
    
    This is used by the GL state caching system. It performs the same matrix
    operations OpenGL does, but without interacting with the driver. It ties
    into the rStateCache system.

    Created 15/09/2021 by Elcee
=============================================================================*/



#include "rMatrixStack.h"
#include <math.h>
#include <xmmintrin.h>



/// Let's make those SSE intrinsics actually readable...
typedef __m128 vec4;
#define SWIZMASK(w,z,y,x) _MM_SHUFFLE(w,z,y,x)
#define addps    _mm_add_ps
#define mulps    _mm_mul_ps
#define shufps   _mm_shuffle_ps
#define loadups  _mm_loadu_ps
#define storeups _mm_storeu_ps





/// Matrix stacks
typedef struct MatrixState {
    MatrixStack stacks[ 2 ]; ///< Matrix stacks
    sdword      select;      ///< Selects which stack to use
} MatrixState;



/// Internal state
static MatrixState matrixState;



/// Init the matrix stack state
void msInitStacks( void ) {
    memset( &matrixState, 0x00, sizeof(matrixState) );
    msSetIdentity( matrixState.stacks[0].data );
    msSetIdentity( matrixState.stacks[1].data );
}



/// Populate 4x4 matrix
static void msSet4x4( Matrix* matrix,
    float x0, float y0, float z0, float w0, 
    float x1, float y1, float z1, float w1,
    float x2, float y2, float z2, float w2,
    float x3, float y3, float z3, float w3 ) {
    float* m = matrix->m;
    m[0]=x0; m[4]=y0; m[ 8]=z0; m[12]=w0;
    m[1]=x1; m[5]=y1; m[ 9]=z1; m[13]=w1;
    m[2]=x2; m[6]=y2; m[10]=z2; m[14]=w2;
    m[3]=x3; m[7]=y3; m[11]=z3; m[15]=w3;
}



/// Populate 4x4 matrix with 3x3 matrix inputs. The rest get identity values.
static void msSet3x3( Matrix* m,
    float x0, float y0, float z0, 
    float x1, float y1, float z1,
    float x2, float y2, float z2 ) {
    msSet4x4( m, 
        x0, y0, z0, 0,
        x1, y1, z1, 0,
        x2, y2, z2, 0,
        0,  0,  0,  1
    );
}



/// Get current matrix stack determined by the mode.
MatrixStack* msCurrentStack( void ) {
    return &matrixState.stacks[ matrixState.select ];
}



/// Get current matrix accounting for the stack and mode.
Matrix* msGetMatrix( void ) {
    MatrixStack* stack = msCurrentStack();
    return &stack->data[ stack->index ];
}



/// Set the active matrix stack.
void msSetSelect( sdword sel ) {
    matrixState.select = sel;
}



/// Get selected matrix stack.
sdword msGetSelect( void ) {
    return matrixState.select;
}



/// Pop the current matrix
void msPopMatrix( void ) {
    msCurrentStack()->index--;
}



/// Push the current matrix, copying it
void msPushMatrix( void ) {
    Matrix* cur = msGetMatrix();
    memcpy( cur, cur+1, sizeof(*cur) );
    msCurrentStack()->index++;
}



/// Based on Ryg's code.
static void linearCombine( vec4* out, vec4 a, const vec4 b[] ) {
    vec4 result = mulps(               shufps(a,a,0x00), b[0]  );
         result = addps( result, mulps(shufps(a,a,0x55), b[1]) );
         result = addps( result, mulps(shufps(a,a,0xaa), b[2]) );
         result = addps( result, mulps(shufps(a,a,0xff), b[3]) );
    *out = result;
}



/// Multiply current matrix by another matrix
void msMultMatrix( Matrix* matrix ) {
    float*       fa = msGetMatrix()->m;
    const float* fb = matrix->m;

    vec4 a[4];
    a[0] = loadups( fa+ 0 );
    a[1] = loadups( fa+ 4 );
    a[2] = loadups( fa+ 8 );
    a[3] = loadups( fa+12 );

    vec4 b[4];
    b[0] = loadups( fb+ 0 );
    b[1] = loadups( fb+ 4 );
    b[2] = loadups( fb+ 8 );
    b[3] = loadups( fb+12 );

    vec4 out[4];
    linearCombine( &out[0], b[0], a );
    linearCombine( &out[1], b[1], a );
    linearCombine( &out[2], b[2], a );
    linearCombine( &out[3], b[3], a );

    storeups( fa+ 0, out[0] );
    storeups( fa+ 4, out[1] );
    storeups( fa+ 8, out[2] );
    storeups( fa+12, out[3] );
}



void msSetIdentity( Matrix* m ) {
    msSet3x3( m,
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    );
}



void msSetRotate( Matrix* m, float angle, float ax, float ay, float az ) {
    const float r  = angle * (float) M_PI / 180.0f;
    const float c  = cosf( r );
    const float s  = sinf( r );
    const float ci = 1.0f - c;

    const float n  = 1.0f / sqrtf(ax*ax + ay*ay + az*az);
    const float x  = ax * n;
    const float y  = ay * n;
    const float z  = az * n;
    const float dx = c + (x*x * ci);
    const float dy = c + (y*y * ci);
    const float dz = c + (z*z * ci);
    
    msSet3x3( m,
        dx,          x*y*ci-z*s,  x*z*ci+y*s,
        x*y*ci+z*s,  dy,          y*z*ci-x*s,
        x*z*ci-y*s,  z*y*ci+x*s,  dz
    );
}



void msSetScale( Matrix* m, float x, float y, float z ) {
    msSet3x3( m,
        x, 0, 0,
        0, y, 0,
        0, 0, z
    );
}



void msSetTranslate( Matrix* m, float x, float y, float z ) {
    msSet4x4( m,
        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1
    );
}



void msSetOrtho( Matrix* m, float l, float r, float b, float t, float n, float f ) {
    const float sumX   = r + l;
    const float sumY   = t + b;
    const float sumZ   = n + f;
    const float deltaX = r - l;
    const float deltaY = t - b;
    const float deltaZ = n - f;

    const float tx = -(sumX / deltaX);
    const float ty = -(sumY / deltaY);
    const float tz = -(sumZ / deltaZ);
    const float sx = 2.0f / deltaX;
    const float sy = 2.0f / deltaY;
    const float sz = 2.0f / deltaZ;

    msSet4x4( m,
        sx, 0,  0,  tx,
        0,  sy, 0,  ty,
        0,  0,  sz, tz,
        0,  0,  0,  1
    );
}



void msSetFrustum( Matrix* m, float l, float r, float b, float t, float n, float f ) {
    const float ma = (r + l) / (r - l);
    const float mb = (t + b) / (t - b);
    const float mc = (f + n) / (f - n);
    const float md = (2*f*n) / (f - n);
    const float mp = (2*n)   / (r - l);
    const float mq = (2*n)   / (t - b);

    msSet4x4( m,
        mp, 0,  ma, 0,
        0,  mq, mb, 0,
        0,  0,  mc, md,
        0,  0,  -1, 0
    );
}

