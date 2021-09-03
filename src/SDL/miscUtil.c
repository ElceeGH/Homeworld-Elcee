/*=============================================================================
    Name    : miscUtil.c
    Purpose : Miscellaneous utility functions used in fixes/workarounds

    Created 03/09/2021 by ElceeGH
=============================================================================*/



#include "miscUtil.h"
#include <string.h>
#include <ctype.h>



/// Convert null terminated string to uppercase in-place.
/// Replaces numerous occurrences of a for-loop which causes pointless compiler warnings.
void strToLower( char* dst, const char* src ) {
    do *dst++ = tolower( *src );
    while (*src++ != '\0');
}

/// Convert null terminated string to uppercase in-place.
/// Replaces numerous occurrences of a for-loop which causes pointless compiler warnings.
void strToUpper( char* dst, const char* src ) {
    do *dst++ = toupper( *src );
    while (*src++ != '\0');
}
