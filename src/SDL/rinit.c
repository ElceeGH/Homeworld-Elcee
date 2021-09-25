/*=============================================================================
    Name    : rinit.c
    Purpose : rGL / OpenGL enumeration initialization routines

    Created 1/5/1999 by khent
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/


/*#define WIN32_LEAN_AND_MEAN*/
#define STRICT

#include "SDL.h"
#include "Debug.h"
#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rinit.h"
#include "devstats.h"
#include "Types.h"



extern unsigned long strCurLanguage;

static rdevice* rDeviceList;
static int      nDevices;



static void* rinMemAlloc(int size)
{
    void* block = malloc(size);
    memset(block, 0, size);
    return block;
}

static void rinMemFree(void* memblock)
{
    free(memblock);
}



/*-----------------------------------------------------------------------------
    Name        : rinAddMode
    Description : add a display mode to a video driver
    Inputs      : dev - the device to add the mode to
                  width, height - dimensions
                  depth - bitdepth
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void rinAddMode(rdevice* dev, int width, int height, int depth)
{
    rmode* mode = rinMemAlloc(sizeof(rmode));
    mode->width  = width;
    mode->height = height;
    mode->depth  = depth;

    if (dev->modes == NULL)
    {
        //no modes yet
        dev->modes = mode;
        mode->next = NULL;
    }
    else
    {
        //add to tail
        rmode* cmode = dev->modes;
        while (cmode->next != NULL)
        {
            cmode = cmode->next;
        }
        cmode->next = mode;
        mode->next = NULL;
    }
}

/*-----------------------------------------------------------------------------
    Name        : rinModeAccepted
    Description : decides whether a given mode is supported
    Inputs      : dev - the device in question
                  width, height, depth - display mode characteristics
    Outputs     :
    Return      : true or false
----------------------------------------------------------------------------*/
static bool rinModeAccepted(rdevice* dev, int width, int height, int depth)
{
    return depth == 16
        || depth == 24
        || depth == 32;
}

/*-----------------------------------------------------------------------------
    Name        : rinSortModes
    Description : sorts a device's display modes the way I like them
                  (one bitdepth at a time), and remove duplicates
    Inputs      : dev - the device whose modes are to be sorted
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
static void rinSortModes(rdevice* dev)
{
    rmode* cmode;
    rmode* freeMode;
    rdevice dummy;
    int depths[] = { 32, 24, 16, 0 };
    int depth;

    if (dev->modes == NULL)
    {
        return;
    }

    memset(&dummy, 0, sizeof(rdevice));

    for (depth = 0; depths[depth] != 0; depth++)
    {
        cmode = dev->modes;
        do
        {
            if (cmode->depth == depths[depth])
            {
                if (rinModeAccepted(dev, cmode->width, cmode->height, cmode->depth) &&
                    (cmode->next==NULL || cmode->next->width!=cmode->width ||
                     cmode->next->height!=cmode->height || cmode->next->depth!=cmode->depth))
                {
                    rinAddMode(&dummy, cmode->width, cmode->height, cmode->depth);
                }
            }
            cmode = cmode->next;
        } while (cmode != NULL);
    }

    //free modes on dev
    cmode = dev->modes;
    while (cmode != NULL)
    {
        freeMode = cmode;    //save to free
        cmode = cmode->next; //next
        rinMemFree(freeMode);   //free
    }

    //attach new modes
    dev->modes = dummy.modes;
}

/*-----------------------------------------------------------------------------
    Name        : rinAddDevice
    Description : add a device to the head of the device list
    Inputs      : dev - the device to add
    Outputs     : rDeviceList is modified
    Return      :
----------------------------------------------------------------------------*/
static void rinAddDevice(rdevice* dev)
{
    rdevice* cdev;

    if (rDeviceList == NULL)
    {
        rDeviceList = dev;
        dev->next = NULL;
    }
    else
    {
        //add to head
        cdev = rDeviceList;
        dev->next = cdev;
        rDeviceList = dev;
    }
}

/*-----------------------------------------------------------------------------
    Name        : rinGetDeviceList
    Description : return pointer to device list
    Inputs      :
    Outputs     :
    Return      : rDeviceList
----------------------------------------------------------------------------*/
rdevice* rinGetDeviceList(void)
{
    return rDeviceList;
}

/*-----------------------------------------------------------------------------
    Name        : rinFreeDevices
    Description : clear the device list (free memory and such)
    Inputs      :
    Outputs     : rDeviceList and constituents are freed
    Return      :
----------------------------------------------------------------------------*/
void rinFreeDevices(void)
{
    rdevice* dev;
    rdevice* freeDev;
    rmode* mode;
    rmode* freeMode;

    //device loop
    dev = rDeviceList;
    while (dev != NULL)
    {
        //mode loop
        mode = dev->modes;
        while (mode != NULL)
        {
            freeMode = mode;      //save to free
            mode = mode->next;    //next
            rinMemFree(freeMode); //free
        }

        freeDev = dev;          //save to free
        dev = dev->next;        //next
        rinMemFree(freeDev);    //free
    }

    rDeviceList = NULL;
}

/*-----------------------------------------------------------------------------
    Name        : rinEnumeratePrimary
    Description : enumerate available display modes on the primary display
    Inputs      : dev - the device whose modes are to be filled
    Outputs     :
    Return      : true or false (could or couldn't enumerate)
----------------------------------------------------------------------------*/
static bool rinEnumeratePrimary(rdevice* dev)
{
	if (!dev)
		return FALSE;

	/* Make sure SDL video is initialized. */
	Uint32 flags = SDL_WasInit(SDL_INIT_EVERYTHING);
	if (!flags)
	{
		if (SDL_Init(SDL_INIT_VIDEO) == -1)
			return FALSE;
	}
	else if (!(flags & SDL_INIT_VIDEO))
	{
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
			return FALSE;
	}

    const int display_index = 0;
	for (int i=SDL_GetNumDisplayModes(display_index); i>0; i--){
        SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(display_index, i-1, &mode)) {
			dbgMessagef("Error in SDL_GetDisplayMode(): %d %s", i-1, SDL_GetError() );
			return FALSE;
		}
		if (mode.w >= 640 && mode.h >= 480)
		{
			rinAddMode(dev, mode.w, mode.h, SDL_BITSPERPIXEL(mode.format));
		}
	}
	
	return (dev->modes != 0);
}

/*-----------------------------------------------------------------------------
    Name        : rinEnumerateDevices
    Description : Enumerate available display modes.
    Inputs      :
    Outputs     : rDeviceList is filled.
    Return      :
----------------------------------------------------------------------------*/
void rinEnumerateDevices(void)
{
    rdevice* dev = rinMemAlloc(sizeof(rdevice));

    // Fixed as OpenGL.
    dev->type = RIN_TYPE_OPENGL;

    // Just use the basic name
    snprintf(dev->name, sizeof(dev->name), "%s", "OpenGL");

    // Enumerate video modes. If it fails, assume certain basic modes are supported
    if ( ! rinEnumeratePrimary(dev))
    {
        //assume these are supported
        rinAddMode(dev,  640,  480, 24);
        rinAddMode(dev,  800,  600, 24);
        rinAddMode(dev, 1024,  768, 24);
        rinAddMode(dev, 1280,  720, 24);
        rinAddMode(dev, 1920, 1080, 24);
    }

    rinSortModes(dev);
    rinAddDevice(dev);
    nDevices = 1;
}
