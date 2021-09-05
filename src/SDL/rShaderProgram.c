/*=============================================================================
    Name    : rShaderProgram.c
    Purpose : OpenGL shader loading functions.

    Created 04/09/2021 by Elcee
=============================================================================*/



#include "rShaderProgram.h"
#include "SDL.h"
#include <stdio.h>



static char*  loadSourceFromFile( const char* name );
static GLuint compileShader     ( const char* name, const char* source );
static GLuint linkProgram       ( const char* name, GLuint shader );
static void   checkCompile      ( const char* name, GLuint handle );
static void   checkLink         ( const char* name, GLuint handle );



// Private GL
static PFNGLCREATESHADERPROC      glCreateShader;
static PFNGLSHADERSOURCEPROC      glShaderSource;
static PFNGLCOMPILESHADERPROC     glCompileShader;
static PFNGLCREATEPROGRAMPROC     glCreateProgram;
static PFNGLATTACHSHADERPROC      glAttachShader;
static PFNGLLINKPROGRAMPROC       glLinkProgram;
static PFNGLDETACHSHADERPROC      glDetachShader;
static PFNGLGETSHADERIVPROC       glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC  glGetShaderInfoLog;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
static PFNGLGETPROGRAMIVPROC      glGetProgramiv;

// Public GL
PFNGLUSEPROGRAMPROC         glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1IPROC          glUniform1i;
PFNGLUNIFORM1FPROC          glUniform1f;
PFNGLUNIFORM2FVPROC         glUniform2fv;
PFNGLUNIFORM3FVPROC         glUniform3fv;
PFNGLUNIFORM4FVPROC         glUniform4fv;
PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv;



/// Initialise the OpenGL function pointers.
void loadShaderFunctions() {
    glCreateShader      = SDL_GL_GetProcAddress( "glCreateShader"      );
    glShaderSource      = SDL_GL_GetProcAddress( "glShaderSource"      );
    glCompileShader     = SDL_GL_GetProcAddress( "glCompileShader"     );
    glCreateProgram     = SDL_GL_GetProcAddress( "glCreateProgram"     );
    glAttachShader      = SDL_GL_GetProcAddress( "glAttachShader"      );
    glLinkProgram       = SDL_GL_GetProcAddress( "glLinkProgram"       );
    glDetachShader      = SDL_GL_GetProcAddress( "glDetachShader"      );
    glGetShaderiv       = SDL_GL_GetProcAddress( "glGetShaderiv"       );
    glGetShaderInfoLog  = SDL_GL_GetProcAddress( "glGetShaderInfoLog"  );
    glGetProgramInfoLog = SDL_GL_GetProcAddress( "glGetProgramInfoLog" );
    glGetProgramiv      = SDL_GL_GetProcAddress( "glGetProgramiv"      );

    glUseProgram         = SDL_GL_GetProcAddress( "glUseProgram"         );
    glGetUniformLocation = SDL_GL_GetProcAddress( "glGetUniformLocation" );
    glUniform1i          = SDL_GL_GetProcAddress( "glUniform1i"          );
    glUniform1f          = SDL_GL_GetProcAddress( "glUniform1f"          );
    glUniform2fv         = SDL_GL_GetProcAddress( "glUniform2fv"         );
    glUniform3fv         = SDL_GL_GetProcAddress( "glUniform3fv"         );
    glUniform4fv         = SDL_GL_GetProcAddress( "glUniform4fv"         );
    glUniform4fv         = SDL_GL_GetProcAddress( "glUniform4fv"         );
    glUniformMatrix4fv   = SDL_GL_GetProcAddress( "glUniformMatrix4fv"   );
}



/// Load a shader program, compile it, link it and return the resulting handle.
GLuint loadShaderProgram( const char* name ) {
    // Load the file and compile it
    void*  source = loadSourceFromFile( name );
    GLuint shader = compileShader( name, source );
    free( source );
    
    // Link for usable program
    return linkProgram( name, shader );
}



/// Load the shader source for compilation.
static char* loadSourceFromFile( const char* name ) {
    // Show what's happening
    printf( "Loading shader source file: %s\n", name );

    FILE* file = fopen( name, "r" );
    fseek( file, 0, SEEK_END);
    size_t size = ftell( file );
    fseek( file, 0, SEEK_SET);
    size_t sizeWithNull = size + 1;
    char* source = malloc( sizeWithNull );
    memset( source, '\0', sizeWithNull );
    fread( source, sizeof(char), size, file );
    fclose( file );
    return source;
}



/// Compile the shader.
/// Prints any message from the shader compiler to stdout.
static GLuint compileShader( const char* name, const char* source ) {
    // Create, load source, compile it
    GLuint handle = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( handle, 1, &source, NULL );
    glCompileShader( handle );
    checkCompile( name, handle );
    return handle;
}



/// Link the shader into a program. For now this only supports fragment shaders.
/// Prints any message from the shader linker to stdout.
static GLuint linkProgram( const char* name, GLuint shader ) {
    // Create, attach and link
    GLuint handle = glCreateProgram();
    glAttachShader( handle, shader );
    glLinkProgram( handle );

    // Check
    checkLink( name, handle );
    
    // Detach (has no effect on linked shader program)
    glDetachShader( handle, shader );

    // Return handle to use with glUseProgram
    return handle;
}



/// Check the results of shader compilation.
static void checkCompile( const char* name, GLuint handle ) {
    // Check if everything went well, and if there's any log output.
    GLint compileGood=0, logLen=0;
    glGetShaderiv( handle, GL_COMPILE_STATUS,  &compileGood );
    glGetShaderiv( handle, GL_INFO_LOG_LENGTH, &logLen   );

    // If present, output log on standard error stream.
    if ( ! compileGood  ||  logLen > 1) { // +1 for null terminator
        const char* what = compileGood ? "message" : "error";
        printf( "Shader compile %s for shader '%s'. Log:\n", what, name );

        char* log = malloc( logLen );
        glGetShaderInfoLog( handle, logLen, NULL, log );
        printf( "%s\n\n", log );
        free( log );
    }
}



/// Check the results of program linking.
static void checkLink( const char* name, GLuint handle ) {
    // Check
    GLint linkGood=0, logLen=0;
    glGetProgramiv( handle, GL_LINK_STATUS,     &linkGood );
    glGetProgramiv( handle, GL_INFO_LOG_LENGTH, &logLen   );

    // Output log on standard error stream.
    if ( ! linkGood  || logLen > 1) {
        const char* what = linkGood ? "message" : "error";
        printf( "Program link %s for program '%s'. Log:\n", what, name );

        char* log = malloc( logLen );
        glGetProgramInfoLog( handle, logLen, NULL, log );
        printf( "%s\n\n", log );
        free( log );
    }
}
