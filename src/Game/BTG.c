// =============================================================================
//  BTG.c
//  - Background Tool of the Geeks; load and render the BTG backgrounds
// =============================================================================
//  Copyright Relic Entertainment, Inc. All rights reserved.
//  Created 4/27/1998 by khent
// =============================================================================

#include "BTG.h"

#include "Debug.h"
#include "FastMath.h"
#include "File.h"
#include "main.h"
#include "Memory.h"
#include "Options.h"
#include "texreg.h"
#include "Universe.h"
#include "Vector.h"
#include "rShaderProgram.h"
#include "rStateCache.h"

#ifdef HW_BUILD_FOR_DEBUGGING
    #define BTG_VERBOSE_LEVEL  3    // print extra info
#else
    #define BTG_VERBOSE_LEVEL  0
#endif


// -----
// data
// -----

#define M_PI_F 3.1415926535f
#define VERT_POS_ELEMS 3
#define VERT_TEX_ELEMS 2
#define VERT_COL_ELEMS 4



typedef struct btgTransVertex {
    vector  pos;
    real32  r,g,b,a;
} btgTransVertex;

typedef struct btgTransStarVertex {
    vector  pos;
    GLfloat tx, ty;
    GLubyte r,g,b,a;
} btgTransStarVertex;

typedef struct btgTransStar {
    btgTransStarVertex vert[4]; // Order: ll, lr, ul, ur
} btgTransStar;

typedef struct starTex {
    char filename[48];
    GLuint glhandle;
    sdword width, height;
    struct starTex* next;
} starTex;

// List of stars to render.
typedef struct btgStarList {
    GLuint texture; ///< Texture to use. (Never 0)
    udword first;   ///< Index of first star
    udword count;   ///< Number of stars
} btgStarList;



char btgLastBackground[128] = ""; // Used in savegames

static real32 btgThetaOffset;
static real32 btgPhiOffset;
static real32 btgFade = 1.0f;

static bool   btgGpuBuffersInited = FALSE;
static bool   btgGpuBuffersUpdate = FALSE;
static GLuint btgGpuBgVBO         = 0;
static GLuint btgGpuBgIBO         = 0;
static GLuint btgGpuStarVBO       = 0;

static btgHeader*      btgHead       = NULL;
static btgVertex*      btgVerts      = NULL;
static btgStar*        btgStars      = NULL;
static btgPolygon*     btgPolys      = NULL;
static btgTransVertex* btgTransVerts = NULL;
static btgTransStar*   btgTransStars = NULL;
static uword*          btgIndices    = NULL;
static btgStarList*    btgStarLists  = NULL;
static udword          btgStarListsCount;

static starTex* texList = NULL;



typedef struct tagTGAFileHeader
{
    unsigned char idLength;
    unsigned char colorMapType;
    unsigned char imageType;
    // Color Map information.
    unsigned short colorMapStartIndex;
    unsigned short colorMapNumEntries;
    unsigned char colorMapBitsPerEntry;
    // Image Map information.
    signed short imageOffsetX;
    signed short imageOffsetY;
    unsigned short imageWidth;
    unsigned short imageHeight;
    unsigned char pixelDepth;
    unsigned char imageDescriptor;
} TGAFileHeader;


// -----
// code
// -----

/*-----------------------------------------------------------------------------
    Name        : btgStartup
    Description : initialize the btg subsystem
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgStartup(void)
{
    btgReset();
}

// Helper to reduce crap
static void btgMemFree( void** buffer ) {
    if (*buffer != NULL)
    {
        memFree(*buffer);
        *buffer = NULL;
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgReset
    Description : reset the btg subsystem
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgReset(void)
{
    btgFade        = 1.0f;
    btgThetaOffset = 0.0f;
    btgPhiOffset   = 0.0f;

    if (btgGpuBuffersInited) {
        btgGpuBuffersInited = FALSE;

        const udword count = 1;
        glDeleteBuffers( count, &btgGpuBgVBO   );
        glDeleteBuffers( count, &btgGpuBgIBO   );
        glDeleteBuffers( count, &btgGpuStarVBO );

        btgGpuBgVBO   = 0;
        btgGpuBgIBO   = 0;
        btgGpuStarVBO = 0;
    }
    
    universe.backgroundColor = 0;
    rndSetClearColor(colRGBA(0,0,0,255));

    if (texList != NULL)
    {
        starTex* ptex = texList;
        starTex* temptex;

        while (ptex != NULL)
        {
            if (ptex->glhandle != 0)
            {
                glDeleteTextures(1, &ptex->glhandle);
            }
            temptex = ptex;
            ptex = ptex->next;
            memFree(temptex);
        }

        texList = NULL;
    }

    btgMemFree( &btgHead       );
    btgMemFree( &btgVerts      );
    btgMemFree( &btgStars      );
    btgMemFree( &btgStarLists  );
    btgMemFree( &btgPolys      );
    btgMemFree( &btgTransVerts );
    btgMemFree( &btgTransStars );
    btgMemFree( &btgIndices    );
}

/*-----------------------------------------------------------------------------
    Name        : btgShutdown
    Description : shut down the btg subsystem
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgShutdown(void)
{
    btgReset();
}

/*-----------------------------------------------------------------------------
    Name        : btgSetTheta/btgGetTheta/
    Description : set/get the x theta offset of the entire background
    Inputs      : theta - amount, in degrees, to offset
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgSetTheta(real32 theta)
{
    btgThetaOffset = -theta;
}
real32 btgGetTheta(void)
{
    return(-btgThetaOffset);
}

/*-----------------------------------------------------------------------------
    Name        : btgSetPhi/btgGetPhi
    Description : set/get the y phi offset of the entire background
    Inputs      : phi - amount, in degrees, to offset
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgSetPhi(real32 phi)
{
    btgPhiOffset = -phi;
}
real32 btgGetPhi(void)
{
    return(-btgPhiOffset);
}

/*-----------------------------------------------------------------------------
    Name        : btgGetTexture
    Description : creates a GL texture object with the contents of the given .TGA file
    Inputs      : filename - the name of the bitmap, in btg\bitmaps
                  thandle - place where the texture name goes
                  width, height - dimensions of the bitmap
    Outputs     : thandle, width, height are modified
    Return      :
----------------------------------------------------------------------------*/
static void btgGetTexture(char* filename, udword* thandle, sdword* width, sdword* height)
{
    char   fullname[64];
    ubyte* data;
    ubyte* pdata;
    ubyte* bp;
    unsigned short* psdata;
    TGAFileHeader head;
    sdword i;

    strcpy(fullname, "btg/bitmaps/");
    strcat(fullname, filename);

    if (fileExists(fullname, 0))
    {
        fileLoadAlloc(fullname, (void**)&data, 0);

        //this pointer wackiness gets around alignment strangeness
        pdata = data;
        head.idLength = *pdata++;
        head.colorMapType = *pdata++;
        head.imageType = *pdata++;
        psdata = (unsigned short*)pdata;

#if FIX_ENDIAN
        head.colorMapStartIndex = FIX_ENDIAN_INT_16( *psdata );
        pdata += 2;
        psdata = (unsigned short*)pdata;
        head.colorMapNumEntries = FIX_ENDIAN_INT_16( *psdata );
        pdata += 2;
        head.colorMapBitsPerEntry = *pdata++;
        psdata = (unsigned short*)pdata;
        head.imageOffsetX = (signed short)FIX_ENDIAN_INT_16( *psdata );
        pdata += 2;
        psdata = (unsigned short*)pdata;
        head.imageOffsetY = (signed short)FIX_ENDIAN_INT_16( *psdata );
        pdata += 2;
        psdata = (unsigned short*)pdata;
        head.imageWidth = FIX_ENDIAN_INT_16( *psdata );
        pdata += 2;
        psdata = (unsigned short*)pdata;
        head.imageHeight = FIX_ENDIAN_INT_16( *psdata );
#else
        head.colorMapStartIndex = *psdata;
        pdata += 2;
        psdata = (unsigned short*)pdata;
        head.colorMapNumEntries = *psdata;
        pdata += 2;
        head.colorMapBitsPerEntry = *pdata++;
        psdata = (unsigned short*)pdata;
        head.imageOffsetX = (signed short)*psdata;
        pdata += 2;
        psdata = (unsigned short*)pdata;
        head.imageOffsetY = (signed short)*psdata;
        pdata += 2;
        psdata = (unsigned short*)pdata;
        head.imageWidth = *psdata;
        pdata += 2;
        psdata = (unsigned short*)pdata;
        head.imageHeight = *psdata;
#endif // FIX_ENDIAN

        pdata += 2;
        head.pixelDepth = *pdata++;
        head.imageDescriptor = *pdata++;

        //only 32bit TGAs
        dbgAssertOrIgnore(head.pixelDepth == 32);

        pdata += head.idLength;

        *width  = (sdword)head.imageWidth;
        *height = (sdword)head.imageHeight;

        //convert to GL_RGBA
        for (i = 0, bp = pdata; i < 4*(*width)*(*height); i += 4, bp += 4)
        {
            ubyte r, b;
            r = bp[0];
            b = bp[2];
            bp[0] = b;
            bp[2] = r;
            bp[3] = (ubyte)(255 - (int)bp[3]);
        }

        //create the GL texture object
        glGenTextures(1, thandle);
        glBindTexture(GL_TEXTURE_2D, *thandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pdata);

        memFree(data);
    }
    else
    {
        *thandle = 0;
        *width   = 0;
        *height  = 0;
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgTexInList
    Description : determines whether a given bitmap has already been loaded
    Inputs      : filename - the name of the bitmap
    Outputs     :
    Return      : TRUE or FALSE (in or not in list)
----------------------------------------------------------------------------*/
static bool btgTexInList(char* filename)
{
    starTex* ptex;

    ptex = texList;
    while (ptex != NULL)
    {
        if (!strcmp(filename, ptex->filename))
        {
            return TRUE;
        }
        ptex = ptex->next;
    }

    return FALSE;
}

/*-----------------------------------------------------------------------------
    Name        : btgFindTexture
    Description : searches the star bitmap list to find a GL texture object
    Inputs      : filename - name of the bitmap
                  star - btg star structure to fill in
    Outputs     : star is modified accordingly
    Return      :
----------------------------------------------------------------------------*/
static void btgFindTexture(char* filename, btgStar* star)
{
    starTex* ptex;

    ptex = texList;
    while (ptex != NULL)
    {
        if (!strcmp(filename, ptex->filename))
        {
            star->glhandle = ptex->glhandle;
            star->width = ptex->width;
            star->height = ptex->height;
            return;
        }
        ptex = ptex->next;
    }

    star->glhandle = 0;
    star->width = 0;
    star->height = 0;
}

/*-----------------------------------------------------------------------------
    Name        : btgAddTexToList
    Description : adds a starTex struct for a supplied gl texture name
    Inputs      : filename - name of the bitmap
                  glhandle - handle of the GL's texture object
                  width, height - dimensions of the bitmap
    Outputs     : texList is readjusted to contain a newly allocated starTex struct
    Return      :
----------------------------------------------------------------------------*/
static void btgAddTexToList(char *filename, udword glhandle, sdword width, sdword height)
{
    starTex* newtex;

    newtex = (starTex*)memAlloc(sizeof(starTex), "btg star bitmap", 0);
    newtex->glhandle = glhandle;
    newtex->width = width;
    newtex->height = height;
    strcpy(newtex->filename, filename);
    newtex->next = NULL;

    if (texList == NULL)
    {
        //first bitmap loaded
        texList = newtex;
    }
    else
    {
        //other bitmaps exist
        newtex->next = texList;
        texList = newtex;
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgCloseTextures
    Description : free texture handles
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgCloseTextures(void)
{
    udword i;
    btgStar* pStar;

    if (btgStars != NULL)
    {
        for (i = 0, pStar = btgStars; i < btgHead->numStars; i++, pStar++)
        {
            if (pStar->glhandle != 0)
            {
                glDeleteTextures(1, &pStar->glhandle);
            }
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgLoadTextures
    Description : reload the background
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgLoadTextures(void)
{
    if (btgLastBackground[0] != '\0')
    {
        btgLoad(btgLastBackground);
        btgFade             = 1.0f;
        btgGpuBuffersUpdate = TRUE;
    }
}

// Compare stars by their texture
static int btgStarListCompareTexture(const btgStar* a, const btgStar* b) {
    return (int)a->glhandle - (int)b->glhandle;
}

// Many stars use the same texture. And they're additively blended which means the drawing order doesn't matter at all.
// So sort them and divide them into ranges that can all be farted out to the GPU in a tiny amount of calls.
// Exclude any with no texture, since they wouldn't get drawn anyway.
static void btgSortStarsAndGenerateLists() {
    // Sort the star data
    const udword count = btgHead->numStars;
    qsort( btgStars, count, sizeof(btgStars[0]), btgStarListCompareTexture );

    // Clear the star lists.
    const udword starListBytes = count * sizeof(btgStarList);
    memset( btgStarLists, 0x00, starListBytes );
    btgStarListsCount = 0;

    // Skip to first one with non-zero texture, if any
    udword base = 0;
    for (udword i=0; i<count; i++) {
        if ( ! btgStars[base].glhandle)
             base++;
        else break;
    }

    // Generate lists
    btgStarList* list = btgStarLists;
    for (udword i=base; i<count; i++) {
        GLuint tex = btgStars[i].glhandle;
        list->first   = i;
        list->count   = 1;
        list->texture = tex;

        for (udword z=i+1; z<count; z++) {
            if (tex == btgStars[z].glhandle) {
                list->count++;
                i++;
            }
        }

        btgStarListsCount++;
        list++;
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgLoad
    Description : load a btg scene into our structures
    Inputs      : filename - the name of the btg file to load
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgLoad(char* filename)
{
    ubyte* btgData       = NULL;
    ubyte* btgDataOffset = NULL;
    ubyte* instarp       = NULL;
    udword headSize;
    udword vertSize;
    udword vertSizeFile;
    udword starSize, fileStarSize;
    udword polySize;
    real32 thetaSave, phiSave;
    udword i;
#if FIX_ENDIAN
    Uint64 *swap;
#endif

#if BTG_VERBOSE_LEVEL >= 2
    dbgMessagef("filename= %s", filename);
#endif


    fileLoadAlloc(filename, (void**)&btgData, 0);
    btgDataOffset = btgData;

    memStrncpy(btgLastBackground, filename, 127);

    //reset / free any previous structures
    thetaSave = btgThetaOffset;
    phiSave = btgPhiOffset;
    btgReset();
    btgThetaOffset = thetaSave;
    btgPhiOffset = phiSave;

    //header.  trivial copy
    headSize = sizeof(btgHeader);
    btgHead = (btgHeader*)memAlloc(headSize, "btg header", 0);

#if BTG_VERBOSE_LEVEL >=3
 dbgMessagef("btgData= %x", btgData);
 dbgMessagef("btgHead= %x", btgHead);
#endif


// Hard coding sizeof values. 
// This is because they may change later on in the world but static in the file.
// This allows us to align variables. It replaces 
//  memcpy(btgHead, btgData, headSize);


    memset(btgHead, 0, sizeof(*btgHead));

    #define READ_HEADER(field,bytes) { memcpy( (ubyte*)btgHead+offsetof(btgHeader,field), btgDataOffset, bytes );  btgDataOffset += bytes; }
    READ_HEADER(btgFileVersion, 4 );
    READ_HEADER(numVerts      , 4 );
    READ_HEADER(numStars      , 4 );
    READ_HEADER(numPolys      , 4 );
    READ_HEADER(xScroll       , 4 );
    READ_HEADER(yScroll       , 4 );
    READ_HEADER(zoomVal       , 4 );
    READ_HEADER(pageWidth     , 4 );
    READ_HEADER(pageHeight    , 4 );
    READ_HEADER(mRed          , 4 );
    READ_HEADER(mGreen        , 4 );
    READ_HEADER(mBlue         , 4 );
    READ_HEADER(mBGRed        , 4 );
    READ_HEADER(mBGGreen      , 4 );
    READ_HEADER(mBGBlue       , 4 );
    READ_HEADER(bVerts        , 4 );
    READ_HEADER(bPolys        , 4 );
    READ_HEADER(bStars        , 4 );
    READ_HEADER(bOutlines     , 4 );
    READ_HEADER(bBlends       , 4 );
    READ_HEADER(renderMode    , 4 );
    #undef READ_HEADER
        
//    memcpy(btgHead, btgData, headSize);  //See above.

#if FIX_ENDIAN
	btgHead->btgFileVersion = FIX_ENDIAN_INT_32( btgHead->btgFileVersion );
	btgHead->numVerts       = FIX_ENDIAN_INT_32( btgHead->numVerts );
	btgHead->numStars       = FIX_ENDIAN_INT_32( btgHead->numStars );
	btgHead->numPolys       = FIX_ENDIAN_INT_32( btgHead->numPolys );
	btgHead->xScroll        = FIX_ENDIAN_INT_32( btgHead->xScroll );
	btgHead->yScroll        = FIX_ENDIAN_INT_32( btgHead->yScroll );
	btgHead->zoomVal        = FIX_ENDIAN_INT_32( btgHead->zoomVal );
	btgHead->pageWidth      = FIX_ENDIAN_INT_32( btgHead->pageWidth );
	btgHead->pageHeight     = FIX_ENDIAN_INT_32( btgHead->pageHeight );
	btgHead->mRed           = FIX_ENDIAN_INT_32( btgHead->mRed );
	btgHead->mGreen         = FIX_ENDIAN_INT_32( btgHead->mGreen );
	btgHead->mBlue          = FIX_ENDIAN_INT_32( btgHead->mBlue );
	btgHead->mBGRed         = FIX_ENDIAN_INT_32( btgHead->mBGRed );
	btgHead->mBGGreen       = FIX_ENDIAN_INT_32( btgHead->mBGGreen );
	btgHead->mBGBlue        = FIX_ENDIAN_INT_32( btgHead->mBGBlue );
	btgHead->bVerts         = FIX_ENDIAN_INT_32( btgHead->bVerts );
	btgHead->bPolys         = FIX_ENDIAN_INT_32( btgHead->bPolys );
	btgHead->bStars         = FIX_ENDIAN_INT_32( btgHead->bStars );
	btgHead->bOutlines      = FIX_ENDIAN_INT_32( btgHead->bOutlines );
	btgHead->bBlends        = FIX_ENDIAN_INT_32( btgHead->bBlends );
	btgHead->renderMode     = FIX_ENDIAN_INT_32( btgHead->renderMode );
#endif

    //set background colour
    universe.backgroundColor = colRGB(btgHead->mBGRed, btgHead->mBGGreen, btgHead->mBGBlue);

    //version check
    dbgAssertOrIgnore(btgHead->btgFileVersion == BTG_FILE_VERSION);

    vertSize     = btgHead->numVerts * sizeof(btgVertex);  //Machine Specific size
    vertSizeFile = btgHead->numVerts * (4 + (2* 8) + (5*4));  //Actual size from file. (No Alignment)

    if (vertSize)
    {

        btgVerts = (btgVertex*)memAlloc(vertSize, "btg verts", 0);

        

        for( i=0; i<btgHead->numVerts; i++ )
        {
            #define READ_VERTEX(field,bytes) { memcpy( (ubyte*)btgVerts+ ( i * sizeof(btgVertex)) +offsetof(btgVertex,field), btgDataOffset, bytes );  btgDataOffset += bytes; }
            READ_VERTEX( flags     , 4 );
            READ_VERTEX( x         , 8 );
            READ_VERTEX( y         , 8 );
            READ_VERTEX( red       , 4 );
            READ_VERTEX( green     , 4 );
            READ_VERTEX( blue      , 4 );
            READ_VERTEX( alpha     , 4 );
            READ_VERTEX( brightness, 4 );
            #undef READ_VERTEX
        }

//        memcpy(btgVerts, btgData + headSize, vertSize);  //Replaced by above.

#if FIX_ENDIAN
        for( i=0; i<btgHead->numVerts; i++ )
        {
            btgVerts[i].flags = FIX_ENDIAN_INT_32( btgVerts[i].flags );
            
            swap  = ( Uint64 *)&btgVerts[i].x;
            *swap = SDL_SwapLE64( *swap );
            
            swap  = ( Uint64 *)&btgVerts[i].y;
            *swap = SDL_SwapLE64( *swap );
            
            btgVerts[i].red        = FIX_ENDIAN_INT_32( btgVerts[i].red );
            btgVerts[i].green      = FIX_ENDIAN_INT_32( btgVerts[i].green );
            btgVerts[i].blue       = FIX_ENDIAN_INT_32( btgVerts[i].blue );
            btgVerts[i].alpha      = FIX_ENDIAN_INT_32( btgVerts[i].alpha );
            btgVerts[i].brightness = FIX_ENDIAN_INT_32( btgVerts[i].brightness );
        }
#endif
    }

#if BTG_VERBOSE_LEVEL >= 2
 dbgMessagef("numStars= %d", btgHead->numStars);
#endif

    //stars.  non-trivial munging around
    starSize = btgHead->numStars * sizeof(btgStar);
    fileStarSize = 0;
    if (starSize != 0)
    {
        btgStar* outstarp;
        btgStar* inp;
        udword*  udp;
        sdword   j, tempSize, count, length;
        char     filename[48];

        btgStars = (btgStar*)memAlloc(starSize, "btg stars", 0);
        instarp  = btgDataOffset;
#if BTG_VERBOSE_LEVEL >= 3
 dbgMessagef("instarp= %x",instarp);
 dbgMessagef("Offset= %x",instarp - btgData);
#endif
        outstarp = btgStars;
        inp = ( btgStar *)instarp;

        for (i = 0; i < btgHead->numStars; i++, outstarp++)
        {
            //extract constant-sized header
//            tempSize = sizeof(udword) + 2*sizeof(real64) + 4*sizeof(sdword);
            tempSize = 4 + 2*8 + 4*4;

            #define READ_STAR(field,bytes) { memcpy( (ubyte*)outstarp+offsetof(btgStar,field), instarp, bytes);  instarp+=bytes; }
            READ_STAR(flags, 4);
            READ_STAR(x    , 8);
            READ_STAR(y    , 8);
            READ_STAR(red  , 4);
            READ_STAR(green, 4);
            READ_STAR(blue , 4);
            READ_STAR(alpha, 4);
            #undef READ_STAR

//            memcpy(outstarp, instarp, tempSize); //Replaced by Above.
#if BTG_VERBOSE_LEVEL >= 3
 dbgMessagef("tempSize= %x", tempSize);
 dbgMessagef("instarp= %x", instarp);
#endif
            fileStarSize += tempSize;

#if FIX_ENDIAN
            swap = ( Uint64 *)&outstarp->x;
            *swap = SDL_SwapLE64( *swap );
            swap = ( Uint64 *)&outstarp->y;
            *swap = SDL_SwapLE64( *swap );
            outstarp->flags = FIX_ENDIAN_INT_32( outstarp->flags );
            outstarp->red   = FIX_ENDIAN_INT_32( outstarp->red );
            outstarp->green = FIX_ENDIAN_INT_32( outstarp->green );
            outstarp->blue  = FIX_ENDIAN_INT_32( outstarp->blue );
            outstarp->alpha = FIX_ENDIAN_INT_32( outstarp->alpha );
#endif

            //extract variable-sized filename
            count = 0;
            memset(filename, 0, 48);
            udp = (udword*)instarp;

#if FIX_ENDIAN
            length = FIX_ENDIAN_INT_32( (sdword)*udp );
#else
            length = (sdword)*udp;
#endif

#if BTG_VERBOSE_LEVEL >=4
 dbgMessagef("instarp= %x", instarp);
 dbgMessagef("udp= %x", udp);
 dbgMessagef("length= %d", length);
 dbgMessagef("filename= %s", filename);
#endif

            instarp += 4;
            fileStarSize += 4;
            for (j = 0; j < length; j++)
            {
                filename[count++] = *instarp++;
                fileStarSize++;
            }
            memcpy(outstarp->filename, filename, 48);

            //create a GL texture object
            if (!btgTexInList(filename))
            {
                btgGetTexture(filename, &outstarp->glhandle, &outstarp->width, &outstarp->height);
                btgAddTexToList(filename, outstarp->glhandle, outstarp->width, outstarp->height);
            }
            else
            {
                btgFindTexture(filename, outstarp);
            }
        }
    }

    //reset the game's current texture, which now differs from the GL's
    trClearCurrent();

    btgPolygon* polyOut;

    //polys.  trivial copy
    polySize = btgHead->numPolys * sizeof(btgPolygon);
    if (polySize != 0)
    {
        btgPolys = (btgPolygon*)memAlloc(polySize, "btg polys", 0);

        polyOut= btgPolys;
// HERE FIX IT HERE
//        memcpy(btgPolys, btgData + headSize + vertSize + fileStarSize, polySize);

	for( i=0; i<btgHead->numPolys; i++, polyOut++ )
	{
            memcpy((ubyte*)polyOut+offsetof(btgPolygon,flags), instarp, 4); instarp += 4;
            memcpy((ubyte*)polyOut+offsetof(btgPolygon,v0   ), instarp, 4); instarp += 4;
            memcpy((ubyte*)polyOut+offsetof(btgPolygon,v1   ), instarp, 4); instarp += 4;
            memcpy((ubyte*)polyOut+offsetof(btgPolygon,v2   ), instarp, 4); instarp += 4;

	}
		
#if FIX_ENDIAN
		for( i=0; i<btgHead->numPolys; i++ )
		{
			btgPolys[i].flags = FIX_ENDIAN_INT_32( btgPolys[i].flags );
			btgPolys[i].v0    = FIX_ENDIAN_INT_32( btgPolys[i].v0 );
			btgPolys[i].v1    = FIX_ENDIAN_INT_32( btgPolys[i].v1 );
			btgPolys[i].v2    = FIX_ENDIAN_INT_32( btgPolys[i].v2 );
		}
#endif
    }

    memFree(btgData);

    btgIndices = (uword*)memAlloc(3 * btgHead->numPolys * sizeof(uword), "btg indices", NonVolatile);

    btgStarLists = (btgStarList*)memAlloc(btgHead->numStars * sizeof(btgStarList), "btg starlist", NonVolatile); // As much as could ever be needed

    // Order stars for efficient rendering
    btgSortStarsAndGenerateLists();

    //spherically project things, blend colours, &c
    btgConvertVerts();
}

/*-----------------------------------------------------------------------------
    Name        : btgSortCompare
    Description : callback from qsort to sort btg vertices by y coordinate
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static int btgSortCompare(const void* p0, const void* p1)
{
    btgVertex* va;
    btgVertex* vb;

    va = btgVerts + *((sdword*)p0);
    vb = btgVerts + *((sdword*)p1);

    if (va->y > vb->y)
    {
        return 1;
    }
    else if (va->y < vb->y)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgSortByY
    Description : sorts a list of vertex indices by y coordinate
    Inputs      : listToSort - the list of indices to sort
                  n - number of elements
    Outputs     : listToSort is sorted
    Return      :
----------------------------------------------------------------------------*/
static void btgSortByY(udword* listToSort, sdword n)
{
    qsort(listToSort, n, sizeof(sdword), btgSortCompare);
}

/*-----------------------------------------------------------------------------
    Name        : btgZip
    Description : fixup seaming
    Inputs      :
    Outputs     : btgPolys are adjusted to eliminate seaming
    Return      :
----------------------------------------------------------------------------*/
static void btgZip(void)
{
    udword i, iPoly;
    udword numLeft, numRight;
    udword lefts[64], rights[64];
    btgVertex* vert;
    btgPolygon* poly;

    //make lists of left, right verts
    numLeft = numRight = 0;
#if BTG_VERBOSE_LEVEL >=3
 dbgMessagef("numVerts= %d", btgHead->numVerts);
#endif

    for (i = 0, vert = btgVerts; i < btgHead->numVerts; i++, vert++)
    {
        if (vert->x < 0.5)
        {
            lefts[numLeft++] = i;
        }
        else if (vert->x > (btgHead->pageWidth - 0.5))
        {
            rights[numRight++] = i;
        }
    }

#if BTG_VERBOSE_LEVEL >=3
 dbgMessagef("numLeft= %d, numRight= %d\n", numLeft, numRight);
#endif

    //continue only if approximately equal number of edge verts
    if ((numLeft > numRight ? numLeft - numRight : numRight - numLeft) > 1)
    {
        return;
    }

    //sort lists
    btgSortByY(lefts, numLeft);
    btgSortByY(rights, numRight);

    //minimum of edge verts
    if (numLeft < numRight)
    {
        numRight = numLeft;
    }
    else if (numRight < numLeft)
    {
        numLeft = numRight;
    }

    //remove right refs
    for (i = 0; i < numLeft; i++)
    {
        for (iPoly = 0, poly = btgPolys; iPoly < btgHead->numPolys; iPoly++, poly++)
        {
            if (poly->v0 == rights[i])
            {
                poly->v0 = lefts[i];
            }
            if (poly->v1 == rights[i])
            {
                poly->v1 = lefts[i];
            }
            if (poly->v2 == rights[i])
            {
                poly->v2 = lefts[i];
            }
        }
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgConvertAVert
    Description : spherically unprojects a vert
    Inputs      : out - target output vector
                  x, y - 2D position
    Outputs     : out is modified
    Return      :
----------------------------------------------------------------------------*/
static void btgUnprojectVert(vector* out, real32 x, real32 y)
{
    real32 pageWidth  = (real32)btgHead->pageWidth;
    real32 pageHeight = (real32)btgHead->pageHeight;

    x = (pageWidth - 1.0f) - x;

    real32 xFrac = x / pageWidth;
    real32 yFrac = y / pageHeight;

    real32 theta = 2.0f * M_PI_F * xFrac;
    real32 phi   = 1.0f * M_PI_F * yFrac;

    theta += DEG_TO_RAD(btgThetaOffset) + DEG_TO_RAD(90.0f);
    phi   += DEG_TO_RAD(btgPhiOffset);

    real32 sinTheta = sinf(theta);
    real32 cosTheta = cosf(theta);
    real32 sinPhi   = sinf(phi);
    real32 cosPhi   = cosf(phi);

    real32 radius = CAMERA_CLIP_FAR - 500.0f;
    out->x = radius * cosTheta * sinPhi;
    out->y = radius * sinTheta * sinPhi;
    out->z = radius * cosPhi;
}

/*-----------------------------------------------------------------------------
    Name        : btgConvertVert
    Description : do appropriate things to spherically unproject a vert
    Inputs      : out - target output btgTransVert
                  nVert - index of 2D btg vertex
    Outputs     : out will contain the appropriate thing
    Return      :
----------------------------------------------------------------------------*/
static void btgConvertVert(btgTransVertex* out, udword nVert)
{
    btgVertex* in = &btgVerts[ nVert ];

    btgUnprojectVert( &out->pos, (real32) in->x, (real32) in->y );

    real32 r = (real32)in->red        / 255.0f;
    real32 g = (real32)in->green      / 255.0f;
    real32 b = (real32)in->blue       / 255.0f;
    real32 a = (real32)in->alpha      / 255.0f;
    real32 s = (real32)in->brightness / 255.0f;

    out->r = r * a * s;
    out->g = g * a * s;
    out->b = b * a * s;
    out->a = 1.0f;
}

/*-----------------------------------------------------------------------------
    Name        : btgConvertStar
    Description : do appropriate things to spherically unproject a star
    Inputs      : out - target output btgTransVert
                  nVert - index of 2D btg star
    Outputs     : out will contain the appropriate thing
    Return      :
----------------------------------------------------------------------------*/
static void btgConvertStar(btgTransStar* out, udword nVert)
{
    // Texture coord corners
    static const real32 tx[4] = {0.0f, 1.0f, 1.0f, 0.0f};
    static const real32 ty[4] = {0.0f, 0.0f, 1.0f, 1.0f};

    // Pos offset signs
    static const real32 sx[4] = {-1.0f, +1.0f, +1.0f, -1.0f};
    static const real32 sy[4] = {+1.0f, +1.0f, -1.0f, -1.0f};

    btgStar* in         = btgStars + nVert;
    real32   halfWidth  = (real32)in->width  / 2.0f;
    real32   halfHeight = (real32)in->height / 2.0f;
    
    for (udword v=0; v<4; v++) {
        btgTransStarVertex* vert = &out->vert[ v ];

        real32 x = (real32)in->x + halfWidth  * sx[ v ];
        real32 y = (real32)in->y + halfHeight * sy[ v ];
        btgUnprojectVert( &vert->pos, x, y );

        vert->tx = tx[ v ];
        vert->ty = ty[ v ];
        vert->r  = (GLubyte) in->red;
        vert->g  = (GLubyte) in->green;
        vert->b  = (GLubyte) in->blue;
        vert->a  = (GLubyte) in->alpha;
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgConvertVerts
    Description : transforms 2D BTG vertices into 3D
    Inputs      :
    Outputs     : btgTransVerts, btgTransStars are allocated and initialized
    Return      :
----------------------------------------------------------------------------*/
static void btgConvertVerts(void)
{
    udword          nVert, nPoly, index;
    btgTransVertex* pTransVert;
    btgTransStar*   pTransStar;
    btgPolygon*     pPoly;

    btgZip();

    btgTransVerts = memAlloc(btgHead->numVerts * sizeof(btgTransVertex), "btg trans verts", NonVolatile);

    if (btgHead->numStars > 0)
    {
        btgTransStars = memAlloc(btgHead->numStars * sizeof(btgTransStar), "btg trans stars", NonVolatile);
    }
    else
    {
        btgTransStars = NULL;
    }

    for (nVert = 0, pTransVert = btgTransVerts;
         nVert < btgHead->numVerts;
         nVert++, pTransVert++)
    {
        btgConvertVert(pTransVert, nVert);
    }

    for (nVert = 0, pTransStar = btgTransStars; nVert < btgHead->numStars; nVert++, pTransStar++)
    {
        btgConvertStar(pTransStar, nVert);
    }

    for (nPoly = 0, index = 0, pPoly = btgPolys;
         nPoly < btgHead->numPolys;
         nPoly++, pPoly++, index += 3)
    {
        btgIndices[index + 0] = pPoly->v0;
        btgIndices[index + 1] = pPoly->v1;
        btgIndices[index + 2] = pPoly->v2;
    }
}

/*-----------------------------------------------------------------------------
    Name        : btgSetColourMultiplier
    Description : set colour multiplier to allow fading of the entire background
    Inputs      : t - [0..1], the parameter of fade-ness, default 1.0
    Outputs     : btgFade global == t * 255
    Return      :
----------------------------------------------------------------------------*/
void btgSetColourMultiplier(real32 t)
{
//    dbgAssertOrIgnore(t >= 0.0f && t <= 1.0f);
    btgFade = t;
}



/*-----------------------------------------------------------------------------
    Name        : btgRender
    Description : renders the background.  assumes modelview already set
    Inputs      :
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void btgRender(void)
{
    if (btgHead == NULL)
        btgLoad("BTG/default.btg");

#if MR_KEYBOARD_CHEATS
    extern bool gMosaic;
    glShadeModel(gMosaic ? GL_FLAT : GL_SMOOTH);
#else
    glShadeModel(GL_SMOOTH);
#endif

    const sdword    lightOn = rndLightingEnable(FALSE);
    const sdword    addOn   = rndAdditiveBlends(FALSE);
    const GLboolean depthOn = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean texOn   = glIsEnabled(GL_TEXTURE_2D);
    const GLboolean blendOn = glIsEnabled(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND); // Shader does the blending

    // Get the background colour for blending into
    real32 bgCol[4] = { (real32) btgHead->mBGRed   / 255.0f, 
                        (real32) btgHead->mBGGreen / 255.0f, 
                        (real32) btgHead->mBGBlue  / 255.0f,
                        1.0f };

    // Clear the screen. (Not all BTGs have full coverage of the sphere)
    glClearColor( bgCol[0], bgCol[1], bgCol[2], bgCol[3] );
    glClear( GL_COLOR_BUFFER_BIT );



    // Set up the shader for drawing the main background triangles.
    // The shader handles the BTG colour blending, fading and dithering.
    static GLuint* btgProg              = NULL;
    static GLint   btgLocBackground     = -1;
    static GLint   btgLocFade           = -1;
    static GLint   btgLocDitherEnable   = -1;
    static GLint   btgLocDitherScale    = -1;
    static GLint   btgLocDitherTemporal = -1;
    static udword  btgDitherCounter     =  0;

    if ( ! btgProg) {
        btgProg              = loadShaderProgram( "btg" );
        btgLocFade           = glGetUniformLocation( *btgProg, "uFade"           );
        btgLocBackground     = glGetUniformLocation( *btgProg, "uBackground"     );
        btgLocDitherEnable   = glGetUniformLocation( *btgProg, "uDitherEnable"   );
        btgLocDitherScale    = glGetUniformLocation( *btgProg, "uDitherScale"    );
        btgLocDitherTemporal = glGetUniformLocation( *btgProg, "uDitherTemporal" );
    }

    glUseProgram( *btgProg );
    glUniform1f ( btgLocFade,           btgFade );
    glUniform4fv( btgLocBackground, 1,  bgCol );
    glUniform1i ( btgLocDitherEnable,   opRenderBtgDitherEnable );
    glUniform1f ( btgLocDitherScale,    (real32)((1 << opRenderBtgDitherBits) - 1) );
    glUniform1i ( btgLocDitherTemporal, btgDitherCounter & 0x3F );

    if (opRenderBtgDitherTemporal)
        btgDitherCounter++;
    


    // Get our buffer sizes in order
    const udword vertsPerTri   = 3; // Obviously, but better to actually write it lol
    const udword bgTriCount    = btgHead->numPolys;
    const udword bgVertCount   = vertsPerTri * bgTriCount;
    const udword bgIndCount    = bgVertCount;
    const udword bgVertStride  = sizeof(btgTransVertex);
    const udword bgVertexBytes = bgVertCount * sizeof(btgTransVertex);
    const udword bgIndexBytes  = bgIndCount  * sizeof(uword);

    const udword vertsPerStar    = 4; // Quads
    const udword starVertCount   = vertsPerStar * btgHead->numStars;
    const udword starVertStride  = sizeof(btgTransStarVertex);
    const udword starVertexBytes = starVertCount * starVertStride;



    // Create the buffer handles if we didn't already
    if ( ! btgGpuBuffersInited) {
        const udword count = 1;
        glGenBuffers( count, &btgGpuBgVBO   );
        glGenBuffers( count, &btgGpuBgIBO   );
        glGenBuffers( count, &btgGpuStarVBO );

        btgGpuBuffersInited = TRUE;
        btgGpuBuffersUpdate = TRUE;
    }

    // Upload to GPU if data changed
    if (btgGpuBuffersUpdate) {
        glBindBuffer( GL_ARRAY_BUFFER, btgGpuStarVBO );
        glBufferData( GL_ARRAY_BUFFER, starVertexBytes, btgTransStars, GL_STATIC_DRAW );
        
        glBindBuffer( GL_ARRAY_BUFFER,         btgGpuBgVBO );
        glBufferData( GL_ARRAY_BUFFER,         bgVertexBytes, btgTransVerts, GL_STATIC_DRAW );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, btgGpuBgIBO );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, bgIndexBytes,  btgIndices,    GL_STATIC_DRAW );

        btgGpuBuffersUpdate = FALSE;
    }



    // Start drawing.  First the background, typically >1000 tris
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY  );
    glBindBuffer( GL_ARRAY_BUFFER,         btgGpuBgVBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, btgGpuBgIBO ); 
    glVertexPointer( VERT_POS_ELEMS, GL_FLOAT, bgVertStride, (GLvoid*) offsetof(btgTransVertex,pos) );
    glColorPointer ( VERT_COL_ELEMS, GL_FLOAT, bgVertStride, (GLvoid*) offsetof(btgTransVertex,r)   );

    glDrawElements( GL_TRIANGLES, bgIndCount, GL_UNSIGNED_SHORT, NULL );

    glBindBuffer( GL_ARRAY_BUFFER,         0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );

    glUseProgram( 0 );



    // Draw the stars. Typically >50 textured quads
    trClearCurrent();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    rndAdditiveBlends(TRUE);
    rndTextureEnvironment(RTE_Modulate);

    glEnableClientState( GL_COLOR_ARRAY );
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    glBindBuffer( GL_ARRAY_BUFFER, btgGpuStarVBO );
    glVertexPointer  ( VERT_POS_ELEMS, GL_FLOAT,         starVertStride, (GLvoid*) offsetof(btgTransStarVertex,pos) );
    glTexCoordPointer( VERT_TEX_ELEMS, GL_FLOAT,         starVertStride, (GLvoid*) offsetof(btgTransStarVertex,tx)  );
    glColorPointer   ( VERT_COL_ELEMS, GL_UNSIGNED_BYTE, starVertStride, (GLvoid*) offsetof(btgTransStarVertex,r)   );
    
    // Render batches of stars
    for (udword i=0; i<btgStarListsCount; i++) {
        btgStarList* list      = &btgStarLists[ i ];
        GLuint       vertFirst = list->first * vertsPerStar;
        GLuint       vertCount = list->count * vertsPerStar;
        glBindTexture( GL_TEXTURE_2D, list->texture );
        glDrawArrays( GL_QUADS, vertFirst, vertCount );
    }

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );



    glEnable(GL_CULL_FACE);
    rndAdditiveBlends(addOn);
    rndLightingEnable(lightOn);

    if (!texOn)  glDisable(GL_TEXTURE_2D);
    if (blendOn) glEnable(GL_BLEND);
    if (depthOn) glEnable(GL_DEPTH_TEST);
}
