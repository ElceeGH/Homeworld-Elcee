/*=============================================================================
    Name    : video.c
    Purpose : LibAV-powered video playback.

    Created 07/09/2021 by Elcee
=============================================================================*/



#include "video.h"
#include "Debug.h"
#include "File.h"
#include "SDL.h"
#include "render.h"
#include "rStateCache.h"

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"



// Need to know the window dimensions
extern int MAIN_WindowWidth;
extern int MAIN_WindowHeight;



/// Parameters given to videoPlay().
typedef struct VideoParams {
    bool           animatic; ///< Whether video is an animatic
    VideoCallback* cbUpdate; ///< Invoked after a new frame is decoded, at the framerate of the video.
    VideoCallback* cbRender; ///< Invoked after a frame is rendered, at the game framerate.
} VideoParams;

/// LibAV context data.
typedef struct LavContext {
    AVFormatContext*   fctx;    ///< Container format context
    AVStream*          stream;  ///< Video stream in container
    AVCodecContext*    cctx;    ///< Codec context
    AVCodecParameters* cparams; ///< Codec params
    AVCodec*           codec;   ///< Codec instance
    AVPacket*          packet;  ///< Container packet
    AVFrame*           vframe;  ///< Decoded raw frame
    struct SwsContext* sctx;    ///< Scaler/converter context
    AVFrame*           sframe;  ///< Scaled frame in BGRA
} LavContext;

/// Video context struct
typedef struct Video {
    LavContext  lav;          ///< LibAV context
    VideoParams params;       ///< Input parameters
    VideoStatus status;       ///< User-visible status
    GLuint      texture;      ///< OpenGL texture handle
    GLenum      texGpuFormat; ///< OpenGL internal texture format
    GLenum      texMemFormat; ///< OpenGL upload texture layout
    GLenum      texMemType;   ///< OpenGL upload texture datatype
    udword      time;         ///< Time last frame was decoded
    bool        finished;     ///< Whether video is finished playing
} Video;



static bool   videoOpen        (       Video* vid, const char* file, VideoParams params );
static void   videoConstruct   (       Video* vid, LavContext lav, VideoParams params );
static void   videoClose       (       Video* vid );
static bool   videoShouldUpdate( const Video* vid );
static bool   videoUpdate      (       Video* vid );
static void   videoConvert     (       Video* vid );
static void   videoUpload      (       Video* vid );
static void   videoRender      ( const Video* vid );
static real32 videoScale       ( const Video* vid, const float aspect );
static void   videoUpdateStatus(       Video* vid );



/// Open the video. Returns whether it succeeded.
static bool videoOpen( Video* vid, const char* file, VideoParams params ) {
    // Clear struct
    memset( vid, 0x00, sizeof(*vid) );

    // Allocate format context
    AVFormatContext* fctx = avformat_alloc_context();

    // Open input file. If it can't be opened stop here.
    // The context is freed by LibAV if this fails, so there's nothing to clean up.
    if (avformat_open_input(&fctx, file, NULL, NULL) != 0) {
        dbgMessagef( "Failed to open video file: %s", file );
        return FALSE;
    }

    // Get the stream info from the file.
    avformat_find_stream_info( fctx, NULL );

    // Find a video stream and set up the decoder.
    for (unsigned int i=0; i<fctx->nb_streams; i++) {
        // Get the codec parameters for this stream
        AVStream*          stream  = fctx->streams[ i ];
        AVCodecParameters* cparams = stream->codecpar;
        
        // Is this what we're looking for?
        if (cparams->codec_type == AVMEDIA_TYPE_VIDEO) {
            // Create the video decoder and context
            AVCodec*        codec = avcodec_find_decoder( cparams->codec_id );
            AVCodecContext* cctx  = avcodec_alloc_context3( codec );
            avcodec_parameters_to_context( cctx, cparams );
            avcodec_open2( cctx, codec, NULL );

            // Create the context struct with what we have so far.
            LavContext lav = {
                .fctx    = fctx,
                .stream  = stream,
                .cctx    = cctx,
                .cparams = cparams,
                .codec   = codec,
            };

            // Construct the remaining parts of the context to produce the final structure.
            videoConstruct( vid, lav, params );
            return TRUE;
        }
    }

    // Did not find a video stream. Clean up and exit.
    videoClose( vid );
    return FALSE;
}



/// Fill in the video structure and allocate the various bits of storage needed.
static void videoConstruct( Video* vid, LavContext lav, VideoParams params ) {
    // Clear struct
    memset( vid, 0x00, sizeof(*vid) );

    // Get our input/output parameters
    const int w    = lav.cparams->width;
    const int h    = lav.cparams->height;
    const int srcf = lav.cparams->format;
    const int dstf = AV_PIX_FMT_BGRA;

    // Allocate decoding memory
    AVPacket* packet = av_packet_alloc();
    AVFrame*  vframe = av_frame_alloc();

    // Allocate colourspace conversion memory
    AVFrame* sframe = av_frame_alloc();
    sframe->format  = dstf;
    sframe->width   = w;
    sframe->height  = h;
    av_frame_get_buffer( sframe, 0 );
    
    // Initialise the colourspace conversion context.
    struct SwsContext* sctx = sws_getContext( w,h, srcf, w,h, dstf, SWS_BICUBIC, NULL, NULL, NULL );

    // Populate the struct
    vid->lav.fctx     = lav.fctx;
    vid->lav.stream   = lav.stream;
    vid->lav.cctx     = lav.cctx;
    vid->lav.cparams  = lav.cparams;
    vid->lav.codec    = lav.codec;
    vid->lav.packet   = packet;
    vid->lav.vframe   = vframe;
    vid->lav.sctx     = sctx;
    vid->lav.sframe   = sframe;
    vid->params       = params;
    vid->texGpuFormat = GL_RGBA;
    vid->texMemFormat = GL_BGRA;
    vid->texMemType   = GL_UNSIGNED_BYTE;
}



/// Close the video and deallocate resources.
static void videoClose( Video* vid ) {
    // Deallocate LibAV objects
    LavContext* lav = &vid->lav;
    if (lav->sctx)   sws_freeContext     (  lav->sctx   );
    if (lav->sframe) av_frame_free       ( &lav->sframe );
    if (lav->vframe) av_frame_free       ( &lav->vframe );
    if (lav->packet) av_packet_free      ( &lav->packet );
    if (lav->cctx)   avcodec_free_context( &lav->cctx   );
    if (lav->fctx)   avformat_close_input( &lav->fctx   );

    // Destroy OpenGL texture
    if (vid->texture)
        glDeleteTextures( 1, &vid->texture );

    // Clear struct
    memset( vid, 0x00, sizeof(*vid) );
}



/// Check whether it's time to generate a new frame.
static bool videoShouldUpdate( const Video* vid ) {
    const real64 frameN = vid->lav.stream->time_base.num;
    const real64 frameD = vid->lav.stream->time_base.den;
    const real64 frame  = 1.0 / (frameD / frameN) * 1000.0;

    const real64 now   = SDL_GetTicks();
    const real64 delta = now - vid->time;
    return delta >= frame;
}



/// Update video state. Retreives the next frame and updates the current time.
/// Returns whether a new frame was generated.
static bool videoUpdate( Video* vid ) {
    // Get LAV pointer for convenience
    LavContext* lav = &vid->lav;

    // Read next frame. If there isn't one the video is finished (or broken).
    if (av_read_frame(lav->fctx, lav->packet) < 0) {
        vid->finished = TRUE;
        return FALSE;
    }

    // Give packet to the decoder and read a frame from it.
    avcodec_send_packet  ( lav->cctx, lav->packet );
    avcodec_receive_frame( lav->cctx, lav->vframe );

    // Dealloc the packet data now that it's in the frame.
    av_packet_unref( lav->packet );

    // Create converted frame and upload it to the GPU.
    videoConvert( vid );
    videoUpload ( vid );

    // Update time reference.
    vid->time = SDL_GetTicks();

    // Update status for the callback
    videoUpdateStatus( vid );

    // Invoke callback if set
    if (vid->params.cbUpdate)
        vid->params.cbUpdate( vid->status );

    // Let them know there's a new frame to show.
    return TRUE;
}



/// Convert video frame to BGRA format for texture upload.
static void videoConvert( Video* vid ) {
    // Help with readability...
    const AVFrame* in  = vid->lav.vframe;
          AVFrame* out = vid->lav.sframe;
    
    // Convert
    sws_scale( vid->lav.sctx, in->data, in->linesize, 0, in->height, out->data, out->linesize );
}



/// Upload the frame to the texture.
/// Also generates the texture if required.
static void videoUpload( Video* vid ) {
    // Use the converted frame as input
    const int    w         = vid->lav.cparams->width;
    const int    h         = vid->lav.cparams->height;
    const void*  data      = vid->lav.sframe->data[0];
    const GLenum gpuFormat = vid->texGpuFormat;
    const GLenum memFormat = vid->texMemFormat;
    const GLenum memType   = vid->texMemType;

    // Initialise the texture if needed, otherwise directly upload.
    if ( ! vid->texture) {
        glGenTextures  ( 1, &vid->texture );
        glBindTexture  ( GL_TEXTURE_2D, vid->texture );
        glTexImage2D   ( GL_TEXTURE_2D, 0, gpuFormat, w, h, 0, memFormat, memType, data );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    } else {
        glBindTexture  ( GL_TEXTURE_2D, vid->texture );
        glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, memFormat, memType, data );
    }
}



/// Get height adjusted for scaling purposes.
/// Homeworld's animatic videos are 640x480 but their video framing is widescreen 16:10.
/// For animatics we'll crop out the black bars, for anything else we make no assumptions.
static real32 videoScale( const Video* vid, float aspectW ) {
    if (vid->params.animatic)
         return min( aspectW, 1.6f );
    else return 1.0f;
}



/// Update the video status. These values are passed to the callbacks.
static void videoUpdateStatus( Video* vid ) {
    const real64 frameN = vid->lav.stream->time_base.num;
    const real64 frameD = vid->lav.stream->time_base.den;

    vid->status = (VideoStatus) {
        .frameRate   = (real32) (frameD / frameN),
        .frameIndex  = vid->lav.cctx->frame_number,
        .isLastFrame = vid->finished,
    };
}



/// Draw the current video frame with aspect-correct scaling.
static void videoRender( const Video* vid ) {
    // Inputs
    const real32 winW = (real32) MAIN_WindowWidth;
    const real32 winH = (real32) MAIN_WindowHeight;
    const real32 vidW = (real32) vid->lav.cparams->width;
    const real32 vidH = (real32) vid->lav.cparams->height;

    // Aspect-correct inclusive scale
    const real32 aspect = winW / winH;
    const real32 scaleX = winW / vidW;
    const real32 scaleY = winH / videoScale(vid,aspect);
    const real32 scale  = min(scaleX, scaleY);

    // Translation to centre projection in window
    const real32 transX   = (winW - vidW) / 2.0f;
    const real32 transY   = (winH - vidH) / 2.0f;
    const real32 winHalfW = winW / 2.0f;
    const real32 winHalfH = winH / 2.0f;

    // Save matrices
    glccMatrixMode( GL_MODELVIEW  ); glccPushMatrix();
    glccMatrixMode( GL_PROJECTION ); glccPushMatrix();

    // Set transform.
    glccLoadIdentity();
    glccOrtho     ( 0.0f, winW, winH, 0.0f, -1.0f, +1.0f );
    glccTranslatef(  winHalfW,  winHalfH, 0.0f );
    glccScalef    (  scale,     scale,    1.0f );
    glccTranslatef( -winHalfW, -winHalfH, 0.0f );
    glccTranslatef(  transX,    transY,   0.0f );
    
    // Configure GL state
    glccEnable ( GL_TEXTURE_2D );
    glccDisable( GL_BLEND      );
    glccDisable( GL_DEPTH_TEST );
    glBindTexture( GL_TEXTURE_2D, vid->texture );
    
    // Draw the textured video rectangle.
    glBegin( GL_TRIANGLE_STRIP );
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glTexCoord2f( 1, 1 );  glVertex2f( vidW, vidH );
        glTexCoord2f( 1, 0 );  glVertex2f( vidW, 0    );
        glTexCoord2f( 0, 1 );  glVertex2f( 0,    vidH );
        glTexCoord2f( 0, 0 );  glVertex2f( 0,    0    );
    glEnd();

    // Restore matrices
    glccMatrixMode( GL_PROJECTION ); glccPopMatrix();
    glccMatrixMode( GL_MODELVIEW  ); glccPopMatrix();

    // Invoke callback if set
    if (vid->params.cbRender)
        vid->params.cbRender( vid->status );
}



/// Handle events during video playback.
/// It only allows skipping the video using ESC or SPACE.
/// Note: Allowing any key to skip isn't a good idea. Don't skip a video just because I adjust the volume.
static void videoHandleEvents( bool* more ) {
    SDL_Event ev;
    while (SDL_PollEvent( &ev )) {
        if (ev.type != SDL_KEYDOWN)
            continue;

        switch (ev.key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_RETURN:
            case SDLK_SPACE:
                *more = FALSE;
        }
    }
}



/// Play a video. Blocks until complete/skipped.
/// Callbacks allow external code to take action after each update and render.
void videoPlay( char* filename, VideoCallback* cbUpdate, VideoCallback* cbRender, bool isAnimatic ) {
    // Create the full file path
    char path[ MAX_PATH ];
    snprintf( path, sizeof(path), "%s%s", fileHomeworldDataPath, filename );

    // Video param struct
    VideoParams params = { 0 };
    params.cbUpdate = cbUpdate;
    params.cbRender = cbRender;
    params.animatic = isAnimatic;

    // Video working data
    Video video = { 0 }; // Video state
    bool  more  = TRUE;  // Whether to continue playing

    // Open the video
    if ( ! videoOpen( &video, path, params )) {
        dbgMessagef( "Couldn't open video '%s'", path );
        return;
    }

    // Play the video.
    while (more) {
        if (videoShouldUpdate( &video ))
            more = videoUpdate( &video );

        videoRender( &video );
        rndFlush();

        videoHandleEvents( &more );
    }

    // Clean up.
    videoClose( &video );
}

