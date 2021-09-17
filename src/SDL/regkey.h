/*=============================================================================
    REGKEY.H: Registry key definitions for Homeworld.

    Created Sept 1999 by Janik Joire
=============================================================================*/

#ifndef ___REGKEY_H
#define ___REGKEY_H

/*
    This used to define the Windows registry keys and such.
    The game no longer uses the registry, so now it defines the environment
    variable to query for storing user data and the config directory names
    for each platform.
*/

#if defined(HW_GAME_DEMO)
    #ifdef _MACOSX
        #define CONFIGDIR "Library/Application Support/HomeworldDownloadable"
    #elif defined(_WINDOWS)
        #define CONFIGDIR "HomeworldDownloadable"
    #else
        #define CONFIGDIR ".homeworldDownloadable"
    #endif

#else

    #ifdef _MACOSX
        #define CONFIGDIR "Library/Application Support/Homeworld"
    #elif defined(_WINDOWS)
        #define CONFIGDIR "Homeworld"
    #else
        #define CONFIGDIR ".homeworld"
    #endif

#endif



#ifdef _WINDOWS
    #define APPDATADIR "APPDATA"
#else
    #define APPDATADIR "HOME"
#endif



#endif



