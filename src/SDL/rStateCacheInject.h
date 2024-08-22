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
    #define glGetFloatv     ccglGetFloatv  
    #define glGetIntegerv   ccglGetIntegerv

    // Matrix control
    #define glLoadIdentity  ccglLoadIdentity
    #define glPushMatrix    ccglPushMatrix  
    #define glPopMatrix     ccglPopMatrix   
    #define glMatrixMode    ccglMatrixMode  
    #define glLoadMatrixf   ccglLoadMatrixf 
    #define glMultMatrixf   ccglMultMatrixf 
    #define glRotatef       ccglRotatef     
    #define glScalef        ccglScalef      
    #define glTranslatef    ccglTranslatef  
    #define glFrustum       ccglFrustum     
    #define glOrtho         ccglOrtho       

    // State control
    #define glEnable        ccglEnable   
    #define glDisable       ccglDisable  
    #define glIsEnabled     ccglIsEnabled

    // Points and lines
    #define glPointSize     ccglPointSize
    #define glLineWidth     ccglLineWidth

    // Viewport and scissor
    #define glViewport      ccglViewport
    #define glScissor       ccglScissor 

    // Clear col
    #define glClearColor    ccglClearColor

    // Shade model
    #define glShadeModel    ccglShadeModel

#endif
