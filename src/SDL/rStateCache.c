/*=============================================================================
    Name    : rStateCache.c
    Purpose : Cached GL state functions.
    
    Modern drivers run in a separate thread from the application and like to
    batch up a large number of operations. This makes glGet* a blocking sync
    operation which stalls for a very long time, especially since Homeworld
    is ancient and uses parts of GL that are no longer optimised with any real
    effort by the driver developers.
    
    Profiling shows these functions take upwards of 50% of the game's execution time.
    This file caches the most common cases, drastically reducing that impact.

    Created 14/09/2021 by Elcee
=============================================================================*/



#include "rStateCache.h"
#include "rMatrixStack.h"
#include <stdio.h>



enum CacheEntry {
    CacheMatrixMode,
    CacheProjMatrix,
    CacheMvMatrix,
    CacheClearColor,
    CacheLineWidth,
    CachePointSize,
    CacheViewport,
    CacheScissorBox,
    CacheDepthTest,
    CacheAlphaTest,
    CacheStencilTest,
    CacheScissorTest,
    CacheTexture2D,
    CacheBlend,
    CacheLighting,
    CacheCullFace,
    CacheFog,
    CacheNormalize,
    CacheClipPlane0,

    CacheEntryCount,
};



typedef struct CacheMap {
    void*  address; ///< Address of cache entry
    udword size;    ///< Size of cache entry
    udword mask;    ///< Mask of cache entry
} CacheMap;



typedef struct Cache {
    // Cache metadata
    udword flags; ///< Flags get set when value is populated

    // glGetFloatv / glGetIntegerv
    GLint     matrixMode;
    GLfloat   projectionMatrix[16];
    GLfloat   modelViewMatrix[16];
    GLclampf  colorClearValue[4];
    GLfloat   lineWidth;
    GLfloat   pointSize;
    GLint     viewport[4];
    GLint     scissorBox[4];

    // glIsEnabled
    GLboolean depthTest;
    GLboolean alphaTest;
    GLboolean stencilTest;
    GLboolean scissorTest;
    GLboolean texture2D;
    GLboolean blend;
    GLboolean lighting;
    GLboolean cullFace;
    GLboolean fog;
    GLboolean normalizeNormals;
    GLboolean clipPlane0;

    // Cache stats
    udword hits[ CacheEntryCount ];
} Cache;



/// Cache instance
static Cache cache;



/// Map ID to cache data. Not all IDs are mapped, just the ones Homeworld actually uses.
static struct CacheMap cacheMap( GLenum id ) {
    switch (id) {
        case GL_MATRIX_MODE       : return (CacheMap) { &cache.matrixMode        , sizeof(cache.matrixMode      ) , 1<<CacheMatrixMode        };
        case GL_PROJECTION_MATRIX : return (CacheMap) { &cache.projectionMatrix  , sizeof(cache.projectionMatrix) , 1<<CacheProjMatrix        };
        case GL_MODELVIEW_MATRIX  : return (CacheMap) { &cache.modelViewMatrix   , sizeof(cache.modelViewMatrix ) , 1<<CacheMvMatrix          };
        case GL_COLOR_CLEAR_VALUE : return (CacheMap) { &cache.colorClearValue   , sizeof(cache.colorClearValue ) , 1<<CacheClearColor        };
        case GL_LINE_WIDTH        : return (CacheMap) { &cache.lineWidth         , sizeof(cache.lineWidth       ) , 1<<CacheLineWidth         };
        case GL_POINT_SIZE        : return (CacheMap) { &cache.pointSize         , sizeof(cache.pointSize       ) , 1<<CachePointSize         };
        case GL_VIEWPORT          : return (CacheMap) { &cache.viewport          , sizeof(cache.viewport        ) , 1<<CacheViewport          };
        case GL_SCISSOR_BOX       : return (CacheMap) { &cache.scissorBox        , sizeof(cache.scissorBox      ) , 1<<CacheScissorBox        };
        case GL_DEPTH_TEST        : return (CacheMap) { &cache.depthTest         , sizeof(cache.depthTest       ) , 1<<CacheDepthTest         };
        case GL_ALPHA_TEST        : return (CacheMap) { &cache.alphaTest         , sizeof(cache.alphaTest       ) , 1<<CacheAlphaTest         };
        case GL_STENCIL_TEST      : return (CacheMap) { &cache.stencilTest       , sizeof(cache.stencilTest     ) , 1<<CacheStencilTest       };
        case GL_SCISSOR_TEST      : return (CacheMap) { &cache.scissorTest       , sizeof(cache.scissorTest     ) , 1<<CacheScissorTest       };
        case GL_TEXTURE_2D        : return (CacheMap) { &cache.texture2D         , sizeof(cache.texture2D       ) , 1<<CacheTexture2D         };
        case GL_BLEND             : return (CacheMap) { &cache.blend             , sizeof(cache.blend           ) , 1<<CacheBlend             };
        case GL_LIGHTING          : return (CacheMap) { &cache.lighting          , sizeof(cache.lighting        ) , 1<<CacheLighting          };
        case GL_CULL_FACE         : return (CacheMap) { &cache.cullFace          , sizeof(cache.cullFace        ) , 1<<CacheCullFace          };
        case GL_FOG               : return (CacheMap) { &cache.fog               , sizeof(cache.fog             ) , 1<<CacheFog               };
        case GL_NORMALIZE         : return (CacheMap) { &cache.normalizeNormals  , sizeof(cache.normalizeNormals) , 1<<CacheNormalize         };
        case GL_CLIP_PLANE0       : return (CacheMap) { &cache.clipPlane0        , sizeof(cache.clipPlane0      ) , 1<<CacheClipPlane0        };
        default                   : return (CacheMap) { NULL, 0, 0 };
    }
}



static char* idToName( GLenum id ) {
    switch (id) {
        case GL_MATRIX_MODE       : return "GL_MATRIX_MODE"      ;
        case GL_PROJECTION_MATRIX : return "GL_PROJECTION_MATRIX";
        case GL_MODELVIEW_MATRIX  : return "GL_MODELVIEW_MATRIX" ;
        case GL_COLOR_CLEAR_VALUE : return "GL_COLOR_CLEAR_VALUE";
        case GL_LINE_WIDTH        : return "GL_LINE_WIDTH"       ;
        case GL_POINT_SIZE        : return "GL_POINT_SIZE"       ;
        case GL_VIEWPORT          : return "GL_VIEWPORT"         ;
        case GL_SCISSOR_BOX       : return "GL_SCISSOR_BOX"      ;
        case GL_DEPTH_TEST        : return "GL_DEPTH_TEST"       ;
        case GL_ALPHA_TEST        : return "GL_ALPHA_TEST"       ;
        case GL_STENCIL_TEST      : return "GL_STENCIL_TEST"     ;
        case GL_SCISSOR_TEST      : return "GL_SCISSOR_TEST"     ;
        case GL_TEXTURE_2D        : return "GL_TEXTURE_2D"       ;
        case GL_BLEND             : return "GL_BLEND"            ;
        case GL_LIGHTING          : return "GL_LIGHTING"         ;
        case GL_CULL_FACE         : return "GL_CULL_FACE"        ;
        case GL_FOG               : return "GL_FOG"              ;
        case GL_NORMALIZE         : return "GL_NORMALIZE"        ;
        case GL_CLIP_PLANE0       : return "GL_CLIP_PLANE0"      ;
        default                   : return "<UNKNOWN!>"          ;
    }
}



/// Attempt to read from the cache. Returns whether it succeeded.
static udword cacheRead( GLenum id, void* out ) {
    const CacheMap map = cacheMap( id );

    if (cache.flags & map.mask) {
        memcpy( out, map.address, map.size );
        //printf( "GLCC cache read: HIT, %s, id=0x%04x, size=%i\n", idToName(id), id, map.size );
        return TRUE;
    }

    //printf( "GLCC cache read: MISS, %s, id=0x%04x\n", idToName(id), id );
    return FALSE;
}

/// Attempt to write to the cache. Returns whether it succeeded.
static udword cacheWrite( GLenum id, const void* in ) {
    const CacheMap map = cacheMap( id );

    if (map.mask) {
        cache.flags |= map.mask;
        memcpy( map.address, in, map.size );
        return TRUE;
    }

    return FALSE;
}





void glccInit( void ) {
    memset( &cache, 0x00, sizeof(cache) );
    
}



void glccGetFloatv( GLenum id, GLfloat* out ) {
    if ( ! cacheRead( id, out ))
        glGetFloatv( id, out );
}

void glccGetIntegerv( GLenum id, GLint* out ) {
    if ( ! cacheRead( id, out ))
        glGetIntegerv( id, out );
}



void glccEnable( GLenum id ) {
    const GLboolean value = GL_TRUE;
    cacheWrite( id, &value );
    glEnable( id );
}

void glccDisable( GLenum id ) {
    const GLboolean value = GL_FALSE;
    cacheWrite( id, &value );
    glDisable( id );
}

GLboolean glccIsEnabled( GLenum id ) {
    GLboolean value = 0;
    if (cacheRead( id, &value ))
         return value;
    
    value = glIsEnabled( id );
    cacheWrite( id, &value );
    return value;
}





void glccPointSize( const GLfloat size ) {
    cacheWrite( GL_POINT_SIZE, &size );
    glPointSize( size );

}

void glccLineWidth( GLfloat width ) {
    cacheWrite( GL_LINE_WIDTH, &width );
    glPointSize( width );
}



void glccViewport( GLint x, GLint y, GLint w, GLint h ) {
    GLint vp[4] = {x,y, w,h};
    cacheWrite( GL_VIEWPORT, vp );
    glViewport( x,y, w,h );
}



void glccScissor( GLint x, GLint y, GLint w, GLint h ) {
    GLint sb[4] = {x,y, w,h};
    cacheWrite( GL_SCISSOR_BOX, sb );
    glScissor( x,y, w,h );
}



void glccClearColor( GLclampf r, GLclampf g, GLclampf b, GLclampf a ) {
    GLclampf col[4] = {r,g,b,a};
    cacheWrite( GL_COLOR_CLEAR_VALUE, col );
    glClearColor( r,g,b,a );
}




static sdword mapGlMatrixModeToSelect( GLenum mode ) {
    return mode - GL_MODELVIEW;
}

static GLenum mapSelectToGlMatrix( sdword sel ) {
    return GL_MODELVIEW_MATRIX + sel;
}

// Update the GL matrix and cache it.
static void updateGlMatrix( void ) {
    const Matrix* mat = msGetMatrix();
    const sdword  sel = msGetSelect();
    const GLenum  id  = mapSelectToGlMatrix( sel );

    glLoadMatrixf( mat->m );
    cacheWrite( id, mat );
}



void glccLoadIdentity( void ) {
    msSetIdentity( msGetMatrix() );
    updateGlMatrix();
}

void glccMatrixMode( GLenum mode ) {
    msSetSelect( mapGlMatrixModeToSelect(mode) );
    cacheWrite( GL_MATRIX_MODE, &mode );
    glMatrixMode( mode );
}

void glccPushMatrix( void ) {
    msPushMatrix();
    updateGlMatrix();
}

void glccPopMatrix( void ) {
    msPopMatrix();
    updateGlMatrix();
}

void glccLoadMatrixf( const GLfloat* mat ) {
    memcpy( msGetMatrix(), mat, sizeof(Matrix) );
    updateGlMatrix();
}

void glccMultMatrixf( const GLfloat* mat ) {
    msMultMatrix( (Matrix*) mat );
    updateGlMatrix();
}

void glccRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
    Matrix rotation;
    msSetRotate( &rotation, angle, x, y, z );
    glccMultMatrixf( rotation.m );
}

void glccScalef( GLfloat x, GLfloat y, GLfloat z ) {
    Matrix scale;
    msSetScale( &scale, x, y, z );
    glccMultMatrixf( scale.m );
}

void glccTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
    Matrix trans;
    msSetTranslate( &trans, x, y, z );
    glccMultMatrixf( trans.m );
}

void glccFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearv, GLdouble farv ) {
    Matrix frustum;
    msSetFrustum( &frustum, (float)left, (float)right, (float)bottom, (float)top, (float)nearv, (float)farv );
    glccMultMatrixf( frustum.m );
}

void glccOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearv, GLdouble farv ) {
    Matrix ortho;
    msSetOrtho( &ortho, (float)left, (float)right, (float)bottom, (float)top, (float)nearv, (float)farv );
    glccMultMatrixf( ortho.m );
}

