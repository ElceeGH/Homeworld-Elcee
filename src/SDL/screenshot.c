// =============================================================================
//  screenshot.c
//  - take screenshots via OpenGL
// =============================================================================
//  Copyright Relic Entertainment, Inc. All rights reserved.
//  Created 7/15/1998 by khent
// =============================================================================

#include "screenshot.h"
#include "Debug.h"
#include "glinc.h"
#include "interfce.h"
#include "main.h"
#include <stdbool.h>



static void ssGenScreenshotFilename( char* filepath, const size_t filepathSize );
static bool ssVerticallyFlipImage( ubyte* buffer, const size_t width, const size_t height, const size_t bpp );
static void ssSaveScreenshot( ubyte* buffer, const size_t width, const size_t height );



void ssTakeScreenshot(void)
{
    // Determine params
    const size_t width  = MAIN_WindowWidth;
    const size_t height = MAIN_WindowHeight;
    const size_t bpp    = 3; // RGB8
    const size_t bytes  = width * height * bpp;

    // Allocate the buffer
    ubyte* const buffer = malloc( bytes );
    if (buffer == NULL)
        return;

    // Read data from GL
    glPixelStorei( GL_PACK_ALIGNMENT, 1 );
    glReadPixels( 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer );

    // Vertically flip image
    if ( ! ssVerticallyFlipImage( buffer, width, height, bpp))
        return;

    // Save it and free memory
    ssSaveScreenshot( buffer, width, height );
    free( buffer );
}



/// OpenGL stores images with origin in the lower left.
/// Pretty much everything else places it at the top left.
/// Returns whether it succeeded.
static bool ssVerticallyFlipImage( ubyte* data, const size_t width, const size_t height, const size_t bpp )
{
    const size_t rows       = height;
    const size_t rowBytes   = width * bpp;
    const size_t imageBytes = rowBytes * rows;

    ubyte* const rowStart = data;
    ubyte* const rowEnd   = rowStart + imageBytes - rowBytes;
    ubyte* const rowT     = malloc( rowBytes );

    if (rowT == NULL)
        return FALSE;

    for (size_t rowOffset=0; rowOffset<rows/2; rowOffset++) {
        ubyte* const rowA = rowStart + (rowOffset * rowBytes);
        ubyte* const rowB = rowEnd   - (rowOffset * rowBytes);

        memcpy( rowT, rowA, rowBytes ); // A -> T
        memcpy( rowA, rowB, rowBytes ); // B -> A
        memcpy( rowB, rowT, rowBytes ); // T -> B
    }

    free( rowT );
    return TRUE;
}



static void ssGenScreenshotFilename( char* filepath, const size_t filepathSize ) 
{
    time_t now = 0;
    time(&now);

    char filename[ PATH_MAX ] = { '\0' };
    strftime( filename, sizeof(filename), "shot_%Y%m%d_%H%M%S.jpg", localtime(&now) );
    snprintf( filepath, filepathSize, "%s%s/%s", fileUserSettingsPath, "ScreenShots/", filename );
}



static void ssSaveScreenshot( ubyte* const buffer, const size_t width, const size_t height )
{
    // Generate the name
    char filepath[ MAX_PATH ] = { '\0' };
    ssGenScreenshotFilename( filepath, sizeof(filepath) );

    // Create directory structure if required
    if ( ! fileMakeDestinationDirectory(filepath)) {
        dbgMessagef( "Couldn't create screenshot directory! Path: %s", filepath );
        return;
    }
    
    // Open the file
    FILE* out = fopen( filepath, "wb" );
    if (out == NULL) {    
        dbgMessagef( "Couldn't open screenshot file! Path: %s", filepath );
        return;
    }

    // Show what's happening
    dbgMessagef( "Saving %dx%d screenshot to '%s'", width, height, filepath );

    // Fill out the JPG lib info structure
    JPEGDATA jp = {
        .ptr         = buffer,
        .width       = width,
        .height      = height,
        .output_file = out,
        .aritcoding  = 0,
        .quality     = 97,
    };

    // Write and close.
    JpegWrite( &jp );
    fclose( out );
}
