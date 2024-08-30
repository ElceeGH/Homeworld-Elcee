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
#include "rStateCache.h"
#include <stdio.h>



// Anything not in this table gets density == 1.
static real32 levelDensity[] = {
    [MISSION_1_KHARAK_SYSTEM             ] = 0.3f, // Clean fresh space
    [MISSION_2_OUTSKIRTS_OF_KHARAK_SYSTEM] = 0.8f,
    [MISSION_3_RETURN_TO_KHARAK          ] = 2.3f, // Post-battle. TODO don't do dust when NIS is running since it's recounting events
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
static vector rngNextPointInCube( PRNG* prng, vector centre, real32 radius ) {
    return (vector) { rngNextRange( prng, -radius, +radius ),
                      rngNextRange( prng, -radius, +radius ),
                      rngNextRange( prng, -radius, +radius ) };
}



// Random point inside a sphere. Uniform distribution.
// Based on Karthik Karanth's great post: https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/
static vector rngNextPointInSphere( PRNG* prng, vector centre, real32 radius ) {
    // Create a random point on the unit cube and normalise it into an orientation vector
    vector vec = { rngNext(prng), rngNext(prng), rngNext(prng) };
    vecNormalize( &vec );

    // Scale by the cube root of another independent random number to create point inside unit sphere
    const real32 rcr = cbrtf( rngNext(prng) );
    vecMultiplyByScalar( vec, rcr );

    // Scale up radius, offset to centre
    vecMultiplyByScalar( vec, radius );
    vecAddTo( vec, centre );
    return vec;
}



// Motes don't move. Their motion is only apparent motion to the camera.
// They do get wrapped around the volume edges, but this fact is hidden 
// by fading the particles in and out at the edges.
typedef struct Mote {
    vector pos;   ///< World position
    vector lrss;  ///< Last rendered screenspace position
    real32 alpha; ///< Brightness variation       [0:1]
    real32 vis;   ///< Visibility range variation [0:1]
    bool   first; ///< Motes don't get rendered on the first frame or when they wrap around.
} Mote;

/// Mote vertex. Basic stuff
typedef struct MoteVert {
    vector pos;
    real32 r,g,b,a;
} MoteVert;

/// Dust volume which follows the camera.
typedef struct DustVolume {
    vector    centre;        ///< Centre of the cube
    real32    radius;        ///< Half length of cube sides
    real32    radiusVis;     ///< Visible range inside
    real32    radiusVisSqr;  ///< Visible range inside, squared
    Mote*     motes;         ///< Motes allocated memory
    udword    moteCount;     ///< Motes count in allocation
    MoteVert* verts;         ///< Mote vertexes
    udword    vertCount;     ///< Mote vertex count
    udword    vertBytes;     ///< Mote vertex bytes
    GLuint    vertVBO;       ///< GPU vertex buffer
    udword    vertPointBase; ///< Where points begin in the vertex array
    udword    vertLineBase;  ///< Where lines begin in the vertex array
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



static void spaceDustInit( DustVolume* vol, const Camera* cam ) {
    const vector centre           = cam->eyeposition;
    const real32 radiusVisSqr     = 6000*6000; // todo doesn't need to be that big...
    const real32 radiusVis        = sqrtf( radiusVisSqr );
    const real32 radius           = radiusVis;
    const real32 area             = 6.0f * radius * radius;
    const real32 motesPerUnitArea = 1.0f / 350'000.0f; // Average density TODO tune
    const real32 moteDensityScale = 8.0f;//getLevelDensity();

    const udword moteCount = (udword) (area * motesPerUnitArea * moteDensityScale);
    const udword vertCount = moteCount * (1 + 2); // Points and lines, worst case one of each per mote
    const udword moteBytes = moteCount * sizeof(Mote);
    const udword vertBytes = vertCount * sizeof(MoteVert);

    // Allocate memory for motes
    //Mote* const motes = memAlloc( moteBytes, "Dust motes", NonVolatile );
    Mote* const motes = malloc( moteBytes );
    memset( motes, 0x00, moteBytes );

    // Allocate memory for vertexes
    //MoteVert* const verts = memAlloc( vertBytes, "Dust verts", NonVolatile );
    MoteVert* const verts = malloc( vertBytes );
    memset( verts, 0x00, vertBytes );

    printf( "Motes count = %u\n", moteCount );

    // Get our PRNG ready
    PRNG prng;
    rngInit( &prng, getLevelSeed() );

    // Generate motes
    for (udword i=0; i<moteCount; i++) {
        Mote* mote  = &motes[ i ];
        mote->pos   = rngNextPointInCube( &prng, centre, radius );
        mote->vis   = rngNextRange( &prng, 1.00f, 1.50f );
        mote->alpha = rngNextRange( &prng, 0.25f, 1.00f );
        mote->first = TRUE;
        mote++;
    }

    // Colourise vertexes // TODO can introduce some random variation here, per-level colour, etc
    for (udword i=0; i<vertCount; i++) {
        verts[i].r = 1.0f;
        verts[i].g = 1.0f;
        verts[i].b = 1.0f;
        verts[i].a = 1.0f;
    }

    // Write vars
    *vol = (DustVolume) {
        .centre         = centre,
        .radius         = radius,
        .radiusVis      = radiusVis,
        .radiusVisSqr   = radiusVisSqr,
        .motes          = motes,
        .moteCount      = moteCount,
        .verts          = verts,
        .vertBytes      = vertBytes,
        .vertCount      = vertCount,
        .vertPointBase  = 0,
        .vertLineBase   = moteCount
    };
}



static void spaceDustShutdown( DustVolume* vol ) {
    if (vol->vertVBO) glDeleteBuffers( 1, &vol->vertVBO );
    if (vol->motes)   memFree( vol->motes );
    if (vol->verts)   memFree( vol->verts );

    vol->vertVBO = 0;
    vol->motes   = NULL;
    vol->verts   = NULL;
}



static void spaceDustUpdateVolume( DustVolume* vol, Camera* camera ) {
    // Update the centre position
    vol->centre = camera->eyeposition;

    // Generate cube extents
    vector cradius = { vol->radius, vol->radius, vol->radius };
    vector cmin, cmax;
    vecSub( cmin, vol->centre, cradius );
    vecAdd( cmax, vol->centre, cradius );

    // Wrap the motes on each axis so they repeat infinitely within the cube.
    // While loops just in case the camera moves incredibly fast.
    const real32 offs = vol->radius * 2.0f;

    for (udword i=0; i<vol->moteCount; i++) {
        Mote* mote  = &vol->motes[ i ];
        while (mote->pos.x > cmax.x) { mote->pos.x -= offs;  mote->first = TRUE; }
        while (mote->pos.x < cmin.x) { mote->pos.x += offs;  mote->first = TRUE; }
        while (mote->pos.y < cmin.y) { mote->pos.y += offs;  mote->first = TRUE; }
        while (mote->pos.y > cmax.y) { mote->pos.y -= offs;  mote->first = TRUE; }
        while (mote->pos.z < cmin.z) { mote->pos.z += offs;  mote->first = TRUE; }
        while (mote->pos.z > cmax.z) { mote->pos.z -= offs;  mote->first = TRUE; }
    }
}



// TODO this entire thing can be done in a shader IF:
// - Keep the previous camera / perspective matrix around
// - For lines, select camera matrix for vertices using gl_VertexID mod 2
// - Transform POS to both, do the length calcs based on that, set the alpha etc
// - For points, make the stride double to skip every second point and give modulo = 1
// - The positions DO need to be duplicated. Indexing would not work for this since we need the mod 2 on the index.
// - Colour doesn't need to go in the vertexes at all. Shader uniform. The vert colour can be used for the other attributes (vis, alpha, etc that are unique to each)
// - Teleport flag not needed. Just give a threshold where the distance is so large that the alpha is zero. (Happens anyway, don't even need to do anything)
// - This way ALL the calculations can be done in the vertex shader, and the code here need only wrap the world positions and upload the buffer. No need for separate points and lines vertexes either, just execute twice with different uniforms and ranges. Pretty damn sweet

// Generate the vertexes.
// Note: the centre of the volume is the camera eye position.
// TODO the line length fadeout should be normalised somehow, it would vary with resolution as it is...
static void spaceDustGenerateVertexes( DustVolume* vol, Camera* camera, real32 masterAlpha, udword* outPointVertCount, udword* outLineVertCount ) {
    const real32 hResW = 0.5f * (real32) MAIN_WindowWidth;
    const real32 hResH = 0.5f * (real32) MAIN_WindowHeight;

    const real32 threshAlpha   = 1.0f / 256.0f; // Can't see motes when they get this faded
    const real32 mbScale       = 0.125f;  // Rate of division proportional to length (should be resolution varying) OR DOES IT need to be based on the NDC coord length for this? IT's not for fading into a point!

    const real32 closeRange    = 3500.0f; // From camera eye. TODO adjust 
    const real32 closeRangeRcp = 1.0f / closeRange;

    const real32 fadeRangeHigh = vol->radiusVis;
    const real32 fadeRangeLow  = fadeRangeHigh * 0.75f;
    const real32 fadeRange     = fadeRangeHigh - fadeRangeLow;
    const real32 fadeRangeRcp  = 1.0f / fadeRange;

    MoteVert* const motePoints = &vol->verts[ vol->vertPointBase ];
    MoteVert* const moteLines  = &vol->verts[ vol->vertLineBase  ];
    MoteVert*       motePoint  = motePoints;
    MoteVert*       moteLine   = moteLines;

    for (udword i=0; i<vol->moteCount; i++) {
        Mote* const mote = &vol->motes[ i ];

        // Screenspace calc/update
        const vector curr = screenSpacePos( mote->pos );
        const vector prev = mote->lrss;
        mote->lrss = curr;

        // Don't render motes that just wrapped around.
        if (mote->first) {
            mote->first = FALSE;
            continue;
        }

        // Discard motes behind the camera. That's fully half of them on average!
        if (curr.z < 0.0f || prev.z < 0.0f)
            continue;

        // Don't render motes outside the visibility range.
        const real32 distSqr = vecDistanceSquared( vol->centre, mote->pos );
        if (distSqr > vol->radiusVisSqr)
            continue;

        // Fade in from max range. With per-mote variation.
        const real32 dist      = sqrtf( distSqr );
        const real32 fadeDist  = dist * mote->vis;
        const real32 fadeDelta = max( 0.0f, fadeDist - fadeRangeLow );
        const real32 fadeRatio = min( 1.0f, fadeDelta * fadeRangeRcp );
        const real32 fadeAlpha = 1.0f - fadeRatio;

        // Fade when too close to the camera.
        const real32 closeRatio = min( 1.0f, dist * closeRangeRcp );
        const real32 closeAlpha = 1.0f - closeRatio;

        // Merge all the alphas
        const real32 baseAlpha = mote->alpha * fadeAlpha * closeAlpha * masterAlpha;

        if (baseAlpha < threshAlpha)
            continue;

        // Compute apparent length in pixels in 2D.
        const real32 dx  = (prev.x - curr.x) * hResW;
        const real32 dy  = (prev.y - curr.y) * hResH;
        const real32 dot = sqr(dx) + sqr(dy);
        const real32 len = sqrtf( dot );
        
        // When there's no apparent motion, render a point. A zero length line has zero pixels.
        // Become a point gradually to avoid popping. Lines and points can exist simultaneously.
        if (len < 1.0f) {
            motePoint->pos = curr;
            motePoint->a   = baseAlpha * (1.0f - len);
            motePoint++;
        }

        // Lines fade out as they go from length 1 -> 0, while points fade in.
        // Lines fade out with length to simulate motion blur.
        if (len > 0.0f) {
            real32 alpha;
            if (len >= 1.0f)
                 alpha = baseAlpha / max( 1.0f, len*mbScale); // Motion blur fade
            else alpha = baseAlpha * len; // Fade out as point fades in

            moteLine[0].pos = curr;
            moteLine[1].pos = prev;
            moteLine[0].a   = alpha;
            moteLine[1].a   = alpha;
            moteLine += 2;
        }
    }

    // Write counts for drawing
    *outPointVertCount = motePoint - motePoints;
    *outLineVertCount  = moteLine  - moteLines;

    printf( "Rendering %u points, %u lines\n", *outPointVertCount, *outLineVertCount );
}



// Show all the dust motes in worldspace as green points ignoring all fading.
void spaceDustRenderDebug( DustVolume* vol ) {
    sdword add = rndAdditiveBlends( FALSE );
    sdword tex = rndTextureEnable( FALSE );
    sdword bln = glIsEnabled( GL_BLEND );
    glDisable( GL_BLEND );

    glBegin( GL_POINTS );
        for (udword i=0; i<vol->moteCount; i++) {
            glColor4f( 0,1,0,1 );
            glVertex3fv( &vol->motes[i].pos.x );
        }
    glEnd();

    glDisable( GL_BLEND );
    rndAdditiveBlends( add );
    rndTextureEnable( tex );
}



// Draw lines from previous to current position in screenspace.
// Space dust is supposed to be tiny so the lines are always thin.
void spaceDustRender( DustVolume* vol, Camera* camera, real32 alpha ) {
    // Update volume
    spaceDustUpdateVolume( vol, camera );

    //spaceDustRenderDebug( vol );
    //return;


    // Update vertexes
    udword pointVertCount, lineVertCount;
    spaceDustGenerateVertexes( vol, camera, alpha, &pointVertCount, &lineVertCount );

    // Save state
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();

    // Set render params
    const bool wasAdd   = rndAdditiveBlends( TRUE  );
    const bool wasTex   = rndTextureEnable ( FALSE );
    const bool wasLit   = rndLightingEnable( FALSE );
    const bool wasBlend = glIsEnabled( GL_BLEND );
    glEnable( GL_BLEND );
    glDepthMask( FALSE );

    const real32 primSize = max( 1.0f, 0.5f * sqrtf(getResDensityRelative()) );
    glLineWidth( primSize );
    glPointSize( primSize );

    // Bind and create/update GPU vertex buffers
    if ( ! vol->vertVBO) {
        glGenBuffers( 1, &vol->vertVBO );
        glBindBuffer( GL_ARRAY_BUFFER, vol->vertVBO );
        glBufferData( GL_ARRAY_BUFFER, vol->vertBytes, vol->verts, GL_STREAM_DRAW );
    } else {
        glBindBuffer   ( GL_ARRAY_BUFFER, vol->vertVBO );
        glBufferSubData( GL_ARRAY_BUFFER, 0, vol->vertBytes, vol->verts );
    }

    // Draw baby draw
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY  );
    glVertexPointer( 3, GL_FLOAT, sizeof(MoteVert), (GLvoid*) offsetof(MoteVert,pos) );
    glColorPointer ( 4, GL_FLOAT, sizeof(MoteVert), (GLvoid*) offsetof(MoteVert,r)   );

    glDrawArrays( GL_LINES,  vol->vertLineBase,  lineVertCount  );
    glDrawArrays( GL_POINTS, vol->vertPointBase, pointVertCount );

    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY  );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // Restore state
    glPointSize( 1.0f );
    glLineWidth( 1.0f );
    glDepthMask( TRUE );
    if (!wasBlend) glDisable( GL_BLEND );
    rndLightingEnable( wasLit );
    rndTextureEnable ( wasTex );
    rndAdditiveBlends( wasAdd );
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
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


