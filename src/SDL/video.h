


#pragma once
#include "Types.h"


/// Video callback parameters.
typedef struct VideoStatus {
    sdword frameIndex;  ///< Current frame, starts from 1.
    real32 frameRate;   ///< Framerate in Hertz
    bool   isLastFrame; ///< Whether this is the last frame.
} VideoStatus;

/// Type of callback the video system uses.
typedef void VideoCallback( VideoStatus status );



void videoPlay( char* filename, VideoCallback* cbUpdate, VideoCallback* cbRender, bool isAnimatic );

