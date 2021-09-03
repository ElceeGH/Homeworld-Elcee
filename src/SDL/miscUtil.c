/*=============================================================================
    Name    : miscUtil.c
    Purpose : Miscellaneous utility functions used in fixes/workarounds

    Created 03/09/2021 by ElceeGH
=============================================================================*/



#include "miscUtil.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



/// Alternate memmove implementation which won't be optimised to memcpy.
/// This works around a problem in the VS2019 address sanitizer caused by the optimiser replacing memmove with memcpy.
/// It's important that ASAN is available since it has already caught important memory corruption bugs.
/// Yeah I know you can do this without allocating memory. It's not performance critical.
void memmoveAlt( void* dst, const void* src, size_t size ) {
    void* temp = malloc(size);
    memcpy( temp, src, size );
    memcpy( dst, temp, size );
    free(temp);
}



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
