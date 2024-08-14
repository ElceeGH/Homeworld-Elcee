


#pragma once
#include "glinc.h"



// Init
void ccglInit( void );

// Generic gets
void ccglGetFloatv  ( GLenum id, GLfloat* out );
void ccglGetIntegerv( GLenum id, GLint* out );

// Matrix control
void ccglLoadIdentity( void );
void ccglPushMatrix  ( void );
void ccglPopMatrix   ( void );
void ccglMatrixMode  ( GLenum mode );
void ccglLoadMatrixf ( const GLfloat* mat );
void ccglMultMatrixf ( const GLfloat* mat );
void ccglRotatef     ( GLfloat angle, GLfloat x, GLfloat y,  GLfloat z );
void ccglScalef      ( GLfloat x, GLfloat y, GLfloat z );
void ccglTranslatef  ( GLfloat x, GLfloat y, GLfloat z );
void ccglFrustum     ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearv, GLdouble farv );
void ccglOrtho       ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearv, GLdouble farv );

// State control
void      ccglEnable   ( GLenum id );
void      ccglDisable  ( GLenum id );
GLboolean ccglIsEnabled( GLenum id );

// Points and lines
void ccglPointSize( GLfloat size  );
void ccglLineWidth( GLfloat width );

// Viewport and scissor
void ccglViewport( GLint x, GLint y, GLint w, GLint h );
void ccglScissor ( GLint x, GLint y, GLint w, GLint h );

// Clear col
void ccglClearColor( GLclampf r, GLclampf g, GLclampf b, GLclampf a );


