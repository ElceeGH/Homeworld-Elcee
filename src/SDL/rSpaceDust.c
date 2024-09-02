/*=============================================================================
    Name    : rSpaceDust.c
    Purpose : Renders fancy high-density spacedust.
    
    Adds space dust which is drawn as fine lines with motion blur.
    It helps show motion in space. Plus it looks cool.

    Density of the dust varies with level.
    Of course, this all optional, since Homeworld didn't have this.

    Created 27/08/2024 by Elcee
=============================================================================*/



#include "rSpaceDust.h"
#include "Debug.h"
#include "Matrix.h"
#include "NIS.h"
#include "Options.h"
#include "render.h"
#include "SinglePlayer.h"
#include "Tweak.h"
#include "Universe.h"
#include "Vector.h"

#include "rInterpolate.h"
#include "rResScaling.h"
#include "rShaderProgram.h"
#include "rStateCache.h"
#include <stdio.h>



// Anything not in this table gets density == 1.
static real32 levelDensity[] = {
    [MISSION_1_KHARAK_SYSTEM             ] = 0.3f, // Clean fresh space
    [MISSION_2_OUTSKIRTS_OF_KHARAK_SYSTEM] = 0.8f,
    [MISSION_3_RETURN_TO_KHARAK          ] = 2.5f, // Post-battle. TODO don't do dust when NIS is running since it's recounting events
    [MISSION_4_GREAT_WASTELANDS_TRADERS  ] = 1.0f,
    [MISSION_5_GREAT_WASTELANDS_REVENGE  ] = 1.0f,
    [MISSION_6_DIAMOND_SHOALS            ] = 4.0f, // Absolute filth-zone
    [MISSION_7_THE_GARDENS_OF_KADESH     ] = 2.0f, // Dense, but not dirty
    [MISSION_8_THE_CATHEDRAL_OF_KADESH   ] = 2.0f,
    [MISSION_9_SEA_OF_LOST_SOULS         ] = 1.0f, // Normal space
    [MISSION_10_SUPER_NOVA_STATION       ] = 8.0f, // Intense density. TODO make density dynamically lower outside the dusty areas
    [MISSION_11_TENHAUSER_GATE           ] = 1.0f,
    [MISSION_12_GALACTIC_CORE            ] = 1.0f,
    [MISSION_13_THE_KAROS_GRAVEYARD      ] = 7.0f, // So messy even the stars are obscured
    [MISSION_14_BRIDGE_OF_SIGHS          ] = 0.7f, // Wide open space, middle of nowhere
    [MISSION_15_CHAPEL_PERILOUS          ] = 1.0f,
    [MISSION_16_HIIGARA                  ] = 0.8f,
};



// Get level-specific density scaling factor.
// Defaults to 1 for any non-specified cases.
static real32 getLevelDensity(void) {
    const udword level = spGetCurrentMission();
    const udword count = sizeof(levelDensity) / sizeof(levelDensity[0]);

    if (singlePlayerGame && level < count)
         return levelDensity[ level ];
    else return 1.0f;
}



// Get level-specific seed.
// Why? To make the space dust always look the same for any given level.
static udword getLevelSeed(void) {
    if (singlePlayerGame)
         return 1 + spGetCurrentMission();
    else return 1;
}



// Deterministic RNG state
typedef struct PRNG {
    udword seed;
    udword state;
} PRNG;



// Init PRNG.
static void rngInit( PRNG* prng, udword seed ) {
    prng->seed  = max( 1, seed );
    prng->state = prng->seed;
}



// Random float in range [0:1] exclusive.
static real32 rngNext( PRNG* prng ) {
    // RNG update
    const udword prime = 16807; // Prime to multiply state with
    prng->state *= prime;

    // The ole' hackaroo
    union { udword i; real32 f; } fu;
    
    // Generate float
    const udword oneF  = 0x3F800000;      // Binary32 1.0f
    const udword shift = 8 + 1;           // Binary32 bits in exponent + sign
    fu.i = (prng->state >> shift) | oneF; // Range [1:2] exclusive
    return fu.f - 1.0f;                   // Range [0:1] exclusive
}



// Random float in given exclusive range.
static real32 rngNextRange( PRNG* prng, real32 a, real32 b ) {
    return lerpf( a, b, rngNext(prng) );
}



// Random point inside a cube.
static vector rngNextPointInUnitCube( PRNG* prng, real32 radius ) {
    return (vector) { rngNextRange( prng, -radius, +radius ),
                      rngNextRange( prng, -radius, +radius ),
                      rngNextRange( prng, -radius, +radius ) };
}



// Inverse of linear interpolation, clamped to [0:1]
static real32 boxStepf( real32 value, real32 low, real32 high ) {
    real32 range = high  - low;
    real32 delta = value - low;
    real32 ratio = delta / range;
    return max( 0.0f, min(ratio,1.0f) );
}



/// Dust mote.
/// All attributes are duplicated for technical GPU-particle reasons.
/// Also used as a vertex format.
typedef struct Mote {
    vector pos;    ///< World position
    real32 alpha;  ///< Brightness variation       [0:1]
    real32 vis;    ///< Visibility range variation [0:1]
    vector dpos;   ///< Duplicated
    real32 dalpha; ///< Duplicated
    real32 dvis;   ///< Duplicated
} Mote;

/// Dust volume which follows the camera.
typedef struct DustVolume {
    vector  centre;         ///< Centre of the cube
    real32  radius;         ///< Half-length of cube sides
    real32  fadeNear;       ///< Mote distance from camera where alpha reaches 1
    real32  fadeFar;        ///< Mote distance from camera where alpha reaches 0
    real32  camRefNear;     ///< Camerica distance from lookat poiint where nearness scaling factor is 0 (lerps to min values)
    real32  camRefFar;      ///< Camerica distance from lookat poiint where nearness scaling factor is 1 (lerps to max values)
    real32  closeNearMin;   ///< Mote distance to camera where alpha reaches 0 (min)
    real32  closeNearMax;   ///< Mote distance to camera where alpha reaches 0 (max)
    real32  closeFarMin;    ///< Mote distance to camera where alpha reaches 1 (min)
    real32  closeFarMax;    ///< Mote distance to camera where alpha reaches 1 (max)
    hmatrix matCurr;        ///< Camera * projection matrix, current frame
    hmatrix matPrev;        ///< Camera * projection matrix, previous frame
    Mote*   motes;          ///< Motes allocated memory (these are also the vertexes)
    udword  moteCount;      ///< Motes count in allocation
    udword  moteBytes;      ///< Motes total bytes
    GLuint  moteVBO;        ///< GPU vertex buffer
} DustVolume;



// Transform world-space position to screenspace: xyz in range [-1:+1].
static vector screenSpacePos( vector world ) {
    // Project into homogenous clip space
    hvector ws, cs, ss;
    vecMakeHVecFromVec( ws, world );
    hmatMultiplyHMatByHVec( &cs, &rndCameraMatrix,     &ws );
    hmatMultiplyHMatByHVec( &ss, &rndProjectionMatrix, &cs );

    // Perspective divide to get screen space
    const real32 pd = 1.0f / ss.w;
    return (vector) { ss.x*pd, ss.y*pd, ss.z*pd };
}



static void getCamProjMatrix( hmatrix* out ) {
    // Same result as the points first being multiplied by camera, then perspective
    hmatMultiplyHMatByHMat( out, &rndProjectionMatrix, &rndCameraMatrix );
}



static void spaceDustInit( DustVolume* vol, const Camera* cam ) {
    // Basic position/scale of the volume
    const vector centre = cam->eyeposition;
    const real32 radius = 6000.0f;

    // Long distance fadeout ranges
    const real32 fadeNear = radius * 0.80f;
    const real32 fadeFar  = radius * 1.00f;

    // Near-camera fading parameters. These relate to typical view distances of the camera in absolute terms
    const real32 camRefNear   =  3500.0f; // Camera distance where nearness factor is 0 (lerps to min values)
    const real32 camRefFar    = 12000.0f; // Camera distance where nearness factor is 1 (lerps to max values)
    const real32 closeNearMin =    50.0f; // Near-camera fade zero-point is right on top of the camera when camera is very close to the target object.
    const real32 closeNearMax =  3500.0f; // But gets quite far away when at a long distance, so the dust isn't distractingly flying right over the camera plane.
    const real32 closeFarMin  =   150.0f; // Fade start point is also very close to the camera initially so motes can flyby the camera and look cool.
    const real32 closeFarMax  =  5000.0f; // But it gets very wide as the camera goes further out so the fadeoff is more gradual.

    // Variation ranges. Limited to [0:1] range.
    const real32 visVarMin  = 0.00f; // Mote visibility offset min. Used as (distance*(1+vis)) in distance fadeout calc.
    const real32 visVarMax  = 0.30f; // Mote visibility offset max.
    const real32 alphaMin   = 0.65f; // Mote alpha min. Simple modulation.
    const real32 alphaMax   = 1.00f; // Mote alpha max.

    // Area and density
    const real32 area             = 6.0f * sqr(radius);
    const real32 motesPerUnitArea = 1.0f / 200'000.0f; // Average density, area-independent
    const real32 moteDensityScale = getLevelDensity();

    // Count and storage
    const udword moteCount = (udword)(area * motesPerUnitArea * moteDensityScale); 
    const udword moteBytes = moteCount * sizeof(Mote);

    // Allocate memory for motes
    Mote* const motes = memAlloc( moteBytes, "Dust motes", NonVolatile );
    memset( motes, 0x00, moteBytes );

    printf( "Motes count = %u\n", moteCount );

    // Get our PRNG ready
    PRNG prng;
    rngInit( &prng, getLevelSeed() );

    // Generate motes
    for (udword i=0; i<moteCount; i++) {
        Mote* mote  = &motes[ i ];

        mote->pos   = rngNextPointInUnitCube( &prng, radius );
        mote->vis   = rngNextRange( &prng, visVarMin, visVarMax );
        mote->alpha = rngNextRange( &prng, alphaMin,  alphaMax  );

        mote->dpos   = mote->pos;
        mote->dvis   = mote->vis;
        mote->dalpha = mote->alpha;
    }

    // Initial matrix
    hmatrix matCamProj;
    getCamProjMatrix( &matCamProj );

    // Write vars
    *vol = (DustVolume) {
        .centre       = centre,
        .radius       = radius,
        .fadeNear     = fadeNear,
        .fadeFar      = fadeFar,
        .camRefNear   = camRefNear,
        .camRefFar    = camRefFar,
        .closeNearMin = closeNearMin,
        .closeNearMax = closeNearMax,
        .closeFarMin  = closeFarMin,
        .closeFarMax  = closeFarMax,
        .motes        = motes,
        .moteCount    = moteCount,
        .moteBytes    = moteBytes,
        .matCurr      = matCamProj,
        .matPrev      = matCamProj
    };
}



static void spaceDustShutdown( DustVolume* vol ) {
    if (vol->moteVBO) glDeleteBuffers( 1, &vol->moteVBO );
    if (vol->motes)   memFree( vol->motes );

    vol->moteVBO = 0;
    vol->motes   = NULL;
}



static void spaceDustUpdateVolume( DustVolume* vol, Camera* camera ) {
    // Update the centre position
    vol->centre = camera->eyeposition;

    // Update our matrix pair
    vol->matPrev = vol->matCurr;
    getCamProjMatrix( &vol->matCurr );
}



// Show all the dust motes in worldspace as purple points ignoring everything but their position.
void spaceDustRenderDebug( DustVolume* vol ) {
    sdword add = rndAdditiveBlends( FALSE );
    sdword tex = rndTextureEnable( FALSE );
    sdword lit = rndLightingEnable( FALSE );
    sdword bln = glIsEnabled( GL_BLEND );
    glDisable( GL_BLEND );

    glBegin( GL_POINTS );
        glColor4f( 1,0,1,1 );
        for (udword i=0; i<vol->moteCount; i++)
            glVertex3fv( &vol->motes[i].pos.x );
    glEnd();

    rndAdditiveBlends( add );
    rndTextureEnable( tex );
    rndLightingEnable( lit );
}



// Draw lines from previous to current position in screenspace.
// Space dust is supposed to be tiny so the lines are always thin.
void spaceDustRender( DustVolume* vol, Camera* camera, real32 alpha ) {
    // Update volume
    spaceDustUpdateVolume( vol, camera );

    // Don't call this when no game is running, since it will render over menus and stuff lol
    if ( ! gameIsRunning)
        return;



    // Shader setup
    static GLuint* prog         = NULL;
    static GLint   locMatCurr   = -1;
    static GLint   locMatPrev   = -1;
    static GLint   locMode      = -1;
    static GLint   locCentre    = -1;
    static GLint   locRadius    = -1;
    static GLint   locRes       = -1;
    static GLint   locCol       = -1;
    static GLint   locAlpha     = -1;
    static GLint   locCloseNear = -1;
    static GLint   locCloseFar  = -1;
    static GLint   locFadeNear  = -1;
    static GLint   locFadeFar   = -1;
    static GLint   locResScale  = -1;
    static GLint   locMbScale   = -1;

    if ( ! prog)
        prog = shaderProgramLoad( "spacedust" );

    if (shaderProgramWasJustLoaded( prog )) {
        locMatCurr   = glGetUniformLocation( *prog, "uMatCurr"   );
        locMatPrev   = glGetUniformLocation( *prog, "uMatPrev"   );
        locMode      = glGetUniformLocation( *prog, "uMode"      );
        locCentre    = glGetUniformLocation( *prog, "uCentre"    );
        locRadius    = glGetUniformLocation( *prog, "uRadius"    );
        locRes       = glGetUniformLocation( *prog, "uRes"       );
        locCol       = glGetUniformLocation( *prog, "uCol"       );
        locAlpha     = glGetUniformLocation( *prog, "uAlpha"     );
        locCloseNear = glGetUniformLocation( *prog, "uCloseNear" );
        locCloseFar  = glGetUniformLocation( *prog, "uCloseFar"  );
        locFadeNear  = glGetUniformLocation( *prog, "uFadeNear"  );
        locFadeFar   = glGetUniformLocation( *prog, "uFadeFar"   );
        locResScale  = glGetUniformLocation( *prog, "uResScale"  );
        locMbScale   = glGetUniformLocation( *prog, "uMbScale"   );
    }

    // Calculate various uniform inputs.
    // uMode is set later when drawing since it varies per primitive type.
    const real32 res[2] = { (real32)MAIN_WindowWidth, (real32)MAIN_WindowHeight };
    const real32 col[3] = { 1.0f, 1.0f, 1.0f };

    const real32 resScale = 0.5f * sqrtf(getResDensityRelative());
    const real32 primSize = max( 1.0f, min(2.0f, resScale) );

    const real32 camDist     = sqrtf( vecDistanceSquared(camera->eyeposition, camera->lookatpoint) );
    const real32 closeFactor = boxStepf( camDist, vol->camRefNear, vol->camRefFar );
    const real32 closeNear   = lerpf( vol->closeNearMin, vol->closeNearMax, closeFactor );
    const real32 closeFar    = lerpf( vol->closeFarMin,  vol->closeFarMax,  closeFactor );

    // Use the shader and set uniforms
    glUseProgram( *prog );
    glUniformMatrix4fv( locMatCurr, 1, FALSE, &vol->matCurr.m11 );
    glUniformMatrix4fv( locMatPrev, 1, FALSE, &vol->matPrev.m11 );
    glUniform3fv      ( locCentre,  1,        &vol->centre.x    );
    glUniform1f       ( locRadius,             vol->radius      );
    glUniform2fv      ( locRes,     1,         res              );
    glUniform3fv      ( locCol,     1,         col              );
    glUniform1f       ( locAlpha,              alpha            );
    glUniform1f       ( locCloseNear,          closeNear        );
    glUniform1f       ( locCloseFar,           closeFar         );
    glUniform1f       ( locFadeNear,           vol->fadeNear    );
    glUniform1f       ( locFadeFar,            vol->fadeFar     );
    glUniform1f       ( locResScale,           primSize         );
    glUniform1f       ( locMbScale,            0.125f           );



    // Work out some vertex things
    // Lines have two vertexes per mote so they use the duplicated values
    // Points only have one vertex so they skip over the dupes
    const udword  lineStride     = sizeof(Mote)   / 2;
    const udword  lineVertCount  = vol->moteCount * 2;
    const udword  lineModulo     = 2;
    const udword  pointStride    = sizeof(Mote);
    const udword  pointVertCount = vol->moteCount;
    const udword  pointModulo    = 1;
    GLvoid* const posOffs        = (GLvoid*) offsetof( Mote, pos   );
    GLvoid* const colOffs        = (GLvoid*) offsetof( Mote, alpha );



    // Set render params
    const bool wasAdd   = rndAdditiveBlends( TRUE  );
    const bool wasTex   = rndTextureEnable ( FALSE );
    const bool wasLit   = rndLightingEnable( FALSE );
    const bool wasBlend = glIsEnabled( GL_BLEND );
    glEnable( GL_BLEND );
    glDepthMask( FALSE );
    glLineWidth( primSize );
    glPointSize( primSize );

    // Bind and create GPU vertex buffer if needed
    if ( ! vol->moteVBO) {
        glGenBuffers( 1, &vol->moteVBO );
        glBindBuffer( GL_ARRAY_BUFFER, vol->moteVBO );
        glBufferData( GL_ARRAY_BUFFER, vol->moteBytes, vol->motes, GL_STATIC_DRAW );
    } else {
        glBindBuffer( GL_ARRAY_BUFFER, vol->moteVBO );
    }

    // Enable position and colour attribs. (It's not really colour though!  They're per-mote alpha variances)
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY  );
    
    // Draw lines
    glUniform1i( locMode, lineModulo );
    glVertexPointer( 3, GL_FLOAT, lineStride, posOffs );
    glColorPointer ( 2, GL_FLOAT, lineStride, colOffs );
    glDrawArrays( GL_LINES, 0, lineVertCount );
    
    // Draw points
    glUniform1i( locMode, pointModulo );
    glVertexPointer( 3, GL_FLOAT, pointStride, posOffs );
    glColorPointer ( 2, GL_FLOAT, pointStride, colOffs );
    glDrawArrays( GL_POINTS, 0, pointVertCount );
    
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY  );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // Restore state
    glUseProgram( 0 );
    glPointSize( 1.0f );
    glLineWidth( 1.0f );
    glDepthMask( TRUE );

    if ( ! wasBlend) glDisable( GL_BLEND );
    rndLightingEnable( wasLit );
    rndTextureEnable ( wasTex );
    rndAdditiveBlends( wasAdd );
}



void spaceDustTest( Camera* camera ) {
    static DustVolume vol;
    static bool       inited = FALSE;

    if ( ! inited) {
        inited = TRUE;
        spaceDustInit( &vol, camera );
    }
    
    spaceDustRender( &vol, camera, 1.0f );
}


