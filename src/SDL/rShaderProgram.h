


#pragma once
#include "SDL_opengl.h"



// Functions loaded by loadShaderFunctions()
extern PFNGLUSEPROGRAMPROC         glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC          glUniform1i;
extern PFNGLUNIFORM1FPROC          glUniform1f;
extern PFNGLUNIFORM2FVPROC         glUniform2fv;
extern PFNGLUNIFORM3FVPROC         glUniform3fv;
extern PFNGLUNIFORM4FVPROC         glUniform4fv;
extern PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv;


// Prototypes
void    loadShaderFunctions( void );
GLuint* loadShaderProgram( const char* name );
void    unloadShaderProgram( GLuint* program );
void    reloadAllShaderPrograms( void );