/*=============================================================================
    Name    : rShaderProgram.c
    Purpose : OpenGL shader functionality.

    Created 04/09/2021 by Elcee
=============================================================================*/



#include "rShaderProgram.h"
#include "Debug.h"
#include "SDL.h"
#include <stdio.h>



//////////////////////////////////////////
// Constants ////////////////////////////
////////////////////////////////////////

#define SHADER_DIR_DEV     "../src/Shaders" ///< Checked first
#define SHADER_DIR_RELEASE "Shaders"        ///< Checked second
#define SHADER_MAX_COUNT   256               ///< Max number of simultaneously loaded shader programs



//////////////////////////////////////////
// Types ////////////////////////////////
////////////////////////////////////////

/// Mapping of filename to GL handle
typedef struct ProgramMap {
    GLuint program;        ///< Handle of shader program. Zero if no program is allocated.
    char   name[MAX_PATH]; ///< Name of shader program excluding the path etc.
} ProgramMap;



//////////////////////////////////////////
// Variables ////////////////////////////
////////////////////////////////////////

/// All mappings
static ProgramMap programMap[SHADER_MAX_COUNT];

// Private GL
static PFNGLCREATESHADERPROC      glCreateShader;
static PFNGLSHADERSOURCEPROC      glShaderSource;
static PFNGLCOMPILESHADERPROC     glCompileShader;
static PFNGLATTACHSHADERPROC      glAttachShader;
static PFNGLDETACHSHADERPROC      glDetachShader;
static PFNGLDELETESHADERPROC      glDeleteShader;
static PFNGLGETSHADERIVPROC       glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC  glGetShaderInfoLog;
static PFNGLCREATEPROGRAMPROC     glCreateProgram;
static PFNGLLINKPROGRAMPROC       glLinkProgram;
static PFNGLDELETEPROGRAMPROC     glDeleteProgram;
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



//////////////////////////////////////////
// Local function decls /////////////////
////////////////////////////////////////

static ProgramMap* mapConstruct( const char* name, GLuint program );
static void        mapDestruct ( GLuint program );
static ProgramMap* mapFind     ( GLuint program );

static GLuint loadShaderProgramInternal  ( const char* name );
static void   unloadShaderProgramInternal( GLuint handle );
static void   unloadAllShaderPrograms    ( void );

static char*  loadSourceFromFile( const char* name );
static GLuint compileShader     ( const char* name, const char* source );
static GLuint linkProgram       ( const char* name, GLuint shader );
static void   checkCompile      ( const char* name, GLuint handle );
static void   checkLink         ( const char* name, GLuint handle );



/// Initialise the OpenGL function pointers.
void loadShaderFunctions() {
    unloadAllShaderPrograms();

    glCreateShader      = SDL_GL_GetProcAddress( "glCreateShader"      );
    glShaderSource      = SDL_GL_GetProcAddress( "glShaderSource"      );
    glCompileShader     = SDL_GL_GetProcAddress( "glCompileShader"     );
    glAttachShader      = SDL_GL_GetProcAddress( "glAttachShader"      );
    glDetachShader      = SDL_GL_GetProcAddress( "glDetachShader"      );
    glDeleteShader      = SDL_GL_GetProcAddress( "glDeleteShader"      );
    glGetShaderiv       = SDL_GL_GetProcAddress( "glGetShaderiv"       );
    glGetShaderInfoLog  = SDL_GL_GetProcAddress( "glGetShaderInfoLog"  );
    glCreateProgram     = SDL_GL_GetProcAddress( "glCreateProgram"     );
    glLinkProgram       = SDL_GL_GetProcAddress( "glLinkProgram"       );
    glDeleteProgram     = SDL_GL_GetProcAddress( "glDeleteProgram"     );
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



/// Construct a new mapping in a free slot.
static ProgramMap* mapConstruct( const char* name, GLuint program ) {
    ProgramMap* map = mapFind( 0 );
    snprintf( map->name, sizeof(map->name), "%s", name );
    map->program = program;
    return map;
}

/// Destruct a mapping.
static void mapDestruct( GLuint handle ) {
    ProgramMap* map = mapFind( handle );
    snprintf( map->name, sizeof(map->name), "%s", "<undefined>" );
    map->program = 0;
}

/// Find mapping by its program handle.
/// A zero handle represents an unused slot.
/// If no free slot exists, it kills the game via dbgFatalf.
static ProgramMap* mapFind( GLuint program ) {
    for (size_t i=0; i<SHADER_MAX_COUNT; i++) {
        ProgramMap* map = &programMap[i];
        if (map->program == program)
            return map;
    }

    dbgFatalf( DBG_Loc, "No shader program with handle %i exists in map.", program );
    return NULL;
}



/// Recompile all extant shader programs.
/// This should be done in a fixed point in the rendering sequence.
/// Beginning or end of the frame is good.
void reloadAllShaderPrograms( void ) {
    for (size_t i=0; i<SHADER_MAX_COUNT; i++) {
        ProgramMap* map = &programMap[i];

        if (map->program) {
            unloadShaderProgramInternal( map->program );
            map->program = loadShaderProgramInternal( map->name );
        }
    }
}



/// Unload all shader programs.
static void unloadAllShaderPrograms( void ) {
    for (size_t i=0; i<SHADER_MAX_COUNT; i++)
        if (programMap[i].program)
            unloadShaderProgram( &programMap[i].program );
}



/// Load a shader program. Returns a pointer to the handle.
/// The pointer is valid until the program is unloaded.
/// Do not store the dereferenced handle anywhere across multiple frames.
/// This allows the shader to be dynamically reloaded which makes shader development much easier.
GLuint* loadShaderProgram( const char* name ) {
    // Create the program
    GLuint program = loadShaderProgramInternal( name );

    // Record name/handle mapping
    ProgramMap* map = mapConstruct( name, program );

    // Give pointer to the mapped handle
    return &map->program;
}



/// Load a shader program, compile it, link it and return the resulting handle.
static GLuint loadShaderProgramInternal( const char* name ) {
    // Load the file and compile it
    void*  source = loadSourceFromFile( name );
    GLuint shader = compileShader( name, source );
    free( source );
    
    // Link and return the fully formed program.
    return linkProgram( name, shader );
}



/// Unload shader and free resources used by it.
void unloadShaderProgram( GLuint* handle ) {
    unloadShaderProgramInternal( *handle );
    mapDestruct( *handle );
}



/// Unload shader and free resources used by it.
static void unloadShaderProgramInternal( GLuint handle ) {
    glDeleteProgram( handle );
}



/// Load shader source for compilation.
static char* loadSourceFromFile( const char* name ) {
    char  pathDev[MAX_PATH] = { '\0' };
    char  pathRel[MAX_PATH] = { '\0' };
    
    snprintf( pathDev, sizeof(pathDev), "%s/%s", SHADER_DIR_DEV,     name );
    snprintf( pathRel, sizeof(pathRel), "%s/%s", SHADER_DIR_RELEASE, name );

    // Try dev path first
    FILE* file = fopen( pathDev, "rb" );

    // Otherwise try release
    if ( ! file) 
        file = fopen( pathRel, "rb" ); 

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
    
    // Detach and delete (has no effect on linked shader program)
    glDetachShader( handle, shader );
    glDeleteShader( shader );

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
