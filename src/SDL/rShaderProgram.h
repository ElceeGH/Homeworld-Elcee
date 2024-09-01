


#pragma once
#include "SDL_opengl.h"
#include "Types.h"



// Functions loaded by shaderProgramLoadGLFunctions()
extern PFNGLUSEPROGRAMPROC         glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC          glUniform1i;
extern PFNGLUNIFORM1FPROC          glUniform1f;
extern PFNGLUNIFORM2FVPROC         glUniform2fv;
extern PFNGLUNIFORM3FVPROC         glUniform3fv;
extern PFNGLUNIFORM4FVPROC         glUniform4fv;
extern PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv;


// Prototypes
void    shaderProgramLoadGLFunctions( void );
GLuint* shaderProgramLoad( const char* name );
void    shaderProgramUnload( GLuint* program );
bool    shaderProgramWasJustLoaded( GLuint* program );
void    shaderProgramReloadAll( void );
