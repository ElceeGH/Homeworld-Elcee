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
#define SHADER_MAX_COUNT   32               ///< Max number of simultaneously loaded shader programs



//////////////////////////////////////////
// Types ////////////////////////////////
////////////////////////////////////////

/// Mapping of filename to GL handle
typedef struct ProgramMap {
    GLuint program;        ///< Handle of shader program. Zero if no program is allocated.
    bool   isJustLoaded;   ///< Flag that clears when it's checked, to help with setting uniforms.
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

static char*  loadSourceFromFile( const char* name, GLuint type );
static GLuint compileShader     ( const char* name, const char* source, GLuint type );
static GLuint linkProgram       ( const char* name, GLuint shaderVert, GLuint shaderFrag );
static void   checkCompile      ( const char* name, GLuint handle );
static void   checkLink         ( const char* name, GLuint handle );



/// Initialise the OpenGL function pointers.
void shaderProgramLoadGLFunctions() {
    // This can be called multiple times, so don't be leakin'
    unloadAllShaderPrograms();

    // Load em
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
    map->program      = program;
    map->isJustLoaded = TRUE;
    return map;
}

/// Destruct a mapping.
static void mapDestruct( GLuint handle ) {
    ProgramMap* map = mapFind( handle );
    snprintf( map->name, sizeof(map->name), "%s", "<undefined>" );
    map->program      = 0;
    map->isJustLoaded = FALSE;
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



/// Reload and recompile all extant shader programs from source.
/// This should be done at a fixed point in the rendering sequence.
/// Beginning or end of the frame is good.
void shaderProgramReloadAll( void ) {
    for (size_t i=0; i<SHADER_MAX_COUNT; i++) {
        ProgramMap* map = &programMap[i];

        if (map->program) {
            unloadShaderProgramInternal( map->program );
            map->program      = loadShaderProgramInternal( map->name );
            map->isJustLoaded = TRUE;
        }
    }
}



/// Unload all shader programs.
static void unloadAllShaderPrograms( void ) {
    for (size_t i=0; i<SHADER_MAX_COUNT; i++)
        if (programMap[i].program)
            shaderProgramUnload( &programMap[i].program );
}



/// Check if a shader was freshly loaded since this function was last called.
/// In other words, it's a self-clearing flag that gets set again whenever the shader program is reloaded.
/// Useful for checking whether to remap uniform locations when shader autoreloading is enabled.
/// If the pointer is NULL it returns false.
bool shaderProgramWasJustLoaded( GLuint* program ) {
    if (program == NULL)
        return FALSE;

    ProgramMap* map = mapFind( *program );
    bool        ijl = map->isJustLoaded;
    map->isJustLoaded = FALSE;
    return ijl;
}



/// Load a shader program. Returns a pointer to the handle.
/// The name doesn't include the file extension. It will look for .vert and .frag and deal with both.
/// If there's only .frag, or only .vert, that's fine too.
/// The pointer is valid until the program is unloaded.
/// Do not store the dereferenced handle anywhere across multiple frames.
/// This allows the shader to be dynamically reloaded which makes shader development much easier.
GLuint* shaderProgramLoad( const char* name ) {
    GLuint      program = loadShaderProgramInternal( name ); // Create the program
    ProgramMap* map     = mapConstruct( name, program );     // Record name/handle mapping
    return &map->program;                                    // Give pointer to the mapped handle
}



/// Load a shader program, compile it, link it and return the resulting handle.
/// Name doesn't include the extension. It automatically looks for .vert and .frag.
/// Dies if neither source file can be located, but not if they can't be compiled. (Because it would interfere with realtime editing horribly)
static GLuint loadShaderProgramInternal( const char* name ) {
    // Load the file and compile it
    void* sourceVert = loadSourceFromFile( name, GL_VERTEX_SHADER   );
    void* sourceFrag = loadSourceFromFile( name, GL_FRAGMENT_SHADER );

    // If neither could be loaded, error out.
    if (!sourceVert && !sourceFrag)
        dbgFatalf( DBG_Loc, "Failed to load any shader sources for program '%s'.", name );

    // Compile whichever ones existed.
    GLuint shaderVert = compileShader( name, sourceVert, GL_VERTEX_SHADER   );
    GLuint shaderFrag = compileShader( name, sourceFrag, GL_FRAGMENT_SHADER );
    free( sourceVert );
    free( sourceFrag );
    
    // Link and return the fully formed program.
    return linkProgram( name, shaderVert, shaderFrag );
}



/// Unload shader and free resources used by it.
void shaderProgramUnload( GLuint* handle ) {
    unloadShaderProgramInternal( *handle );
    mapDestruct( *handle );
}



/// Unload shader and free resources used by it.
static void unloadShaderProgramInternal( GLuint handle ) {
    // This can be called before we actually know how to delete things!
    if (glDeleteProgram)
        glDeleteProgram( handle );
}



/// Get FILE* for shader source.
/// Returns NULL if it can't be found.
static FILE* openShaderSourceFile( const char* name, GLuint type ) {
    char        pathDev[MAX_PATH] = { '\0' };
    char        pathRel[MAX_PATH] = { '\0' };
    const char* ext               = (type == GL_FRAGMENT_SHADER) ? "frag" : "vert";
    
    snprintf( pathDev, sizeof(pathDev), "%s/%s.%s", SHADER_DIR_DEV,     name, ext );
    snprintf( pathRel, sizeof(pathRel), "%s/%s.%s", SHADER_DIR_RELEASE, name, ext );

    // Try dev path first, then release, and if that fails then return NULL.
    FILE*        file = fopen( pathDev, "rb" );
    if ( ! file) file = fopen( pathRel, "rb" );
    if ( ! file) return NULL;

    return file;
}



/// Load shader source for compilation.
static char* loadSourceFromFile( const char* name, GLuint type ) {
    FILE* file = openShaderSourceFile( name, type );

    if (file == NULL)
        return NULL;

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
/// If the source is null, 0 is returned.
static GLuint compileShader( const char* name, const char* source, GLuint type ) {
    // Don't do anything for NULL case
    if (source == NULL)
        return 0;

    // Create, load source, compile it
    GLuint handle = glCreateShader( type );
    glShaderSource( handle, 1, &source, NULL );
    glCompileShader( handle );
    checkCompile( name, handle );
    return handle;
}



/// Link the shaders into a program. Pass zero if there's no shader.
/// Prints any message from the shader linker to stdout.
static GLuint linkProgram( const char* name, GLuint shaderVert, GLuint shaderFrag ) {
    // Create, attach and link
    GLuint handle = glCreateProgram();
    if (shaderVert) glAttachShader( handle, shaderVert );
    if (shaderFrag) glAttachShader( handle, shaderFrag );
    glLinkProgram( handle );

    // Check
    checkLink( name, handle );
    
    // Detach and delete (has no effect on linked shader program)
    if (shaderVert) glDetachShader( handle, shaderVert );
    if (shaderFrag) glDetachShader( handle, shaderFrag );
    if (shaderVert) glDeleteShader( shaderVert );
    if (shaderFrag) glDeleteShader( shaderFrag );

    // Return handle to use with glUseProgram
    return handle;
}



/// Check the results of shader compilation.
static void checkCompile( const char* name, GLuint handle ) {
    // Check if everything went well, and if there's any log output.
    GLint compileGood=0, logLen=0;
    glGetShaderiv( handle, GL_COMPILE_STATUS,  &compileGood );
    glGetShaderiv( handle, GL_INFO_LOG_LENGTH, &logLen   );

    // If present, output log.
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

    // Output log.
    if ( ! linkGood  || logLen > 1) {
        const char* what = linkGood ? "message" : "error";
        printf( "Program link %s for program '%s'. Log:\n", what, name );

        char* log = malloc( logLen );
        glGetProgramInfoLog( handle, logLen, NULL, log );
        printf( "%s\n\n", log );
        free( log );
    }
}
