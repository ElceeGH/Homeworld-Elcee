


#pragma once
#include "glinc.h"



// Init
void glccInit( void );

// Generic gets
void glccGetFloatv  ( GLenum id, GLfloat* out );
void glccGetIntegerv( GLenum id, GLint* out );

// Matrix control
void glccLoadIdentity( void );
void glccPushMatrix  ( void );
void glccPopMatrix   ( void );
void glccMatrixMode  ( GLenum mode );
void glccLoadMatrixf ( const GLfloat* mat );
void glccMultMatrixf ( const GLfloat* mat );
void glccRotatef     ( GLfloat angle, GLfloat x, GLfloat y,  GLfloat z );
void glccScalef      ( GLfloat x, GLfloat y, GLfloat z );
void glccTranslatef  ( GLfloat x, GLfloat y, GLfloat z );
void glccFrustum     ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearv, GLdouble farv );
void glccOrtho       ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearv, GLdouble farv );

// State control
void      glccEnable   ( GLenum id );
void      glccDisable  ( GLenum id );
GLboolean glccIsEnabled( GLenum id );

// Points and lines
void glccPointSize( GLfloat size  );
void glccLineWidth( GLfloat width );

// Viewport and scissor
void glccViewport( GLint x, GLint y, GLint w, GLint h );
void glccScissor ( GLint x, GLint y, GLint w, GLint h );

// Clear col
void glccClearColor( GLclampf r, GLclampf g, GLclampf b, GLclampf a );


