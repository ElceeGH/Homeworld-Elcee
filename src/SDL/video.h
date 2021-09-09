


#pragma once
#include "Types.h"



/// Type of callback the video system uses.
typedef void VideoCallback( sdword frame );



void videoPlay( char* filename, VideoCallback* cbUpdate, VideoCallback* cbRender, bool isAnimatic );

