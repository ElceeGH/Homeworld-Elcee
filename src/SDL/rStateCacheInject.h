/*=============================================================================
    Name    : rStateCacheInject.h
    Purpose : Injects cached GL state functions by redefining the GL ones.

    No biggie. Though if you have GL errors, disabling this is definitely a
    good place to start looking. It's purely an optimisation and has no
    functional effect - if everything works as intended that is.

    Created 14/08/2024 by Elcee
=============================================================================*/



#pragma once



#ifndef NO_GL_CACHE_INJECT

    // Generic gets
    #define glGetFloatv     glccGetFloatv  
    #define glGetIntegerv   glccGetIntegerv

    // Matrix control
    #define glLoadIdentity  glccLoadIdentity
    #define glPushMatrix    glccPushMatrix  
    #define glPopMatrix     glccPopMatrix   
    #define glMatrixMode    glccMatrixMode  
    #define glLoadMatrixf   glccLoadMatrixf 
    #define glMultMatrixf   glccMultMatrixf 
    #define glRotatef       glccRotatef     
    #define glScalef        glccScalef      
    #define glTranslatef    glccTranslatef  
    #define glFrustum       glccFrustum     
    #define glOrtho         glccOrtho       

    // State control
    #define glEnable        glccEnable   
    #define glDisable       glccDisable  
    #define glIsEnabled     glccIsEnabled

    // Points and lines
    #define glPointSize     glccPointSize
    #define glLineWidth     glccLineWidth

    // Viewport and scissor
    #define glViewport      glccViewport
    #define glScissor       glccScissor 

    // Clear col
    #define glClearColor    glccClearColor

#endif
