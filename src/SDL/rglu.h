/*=============================================================================
    Name    : rglu.h
    Purpose : glu* functions for Homeworld

    Created 7/6/1998 by khent
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#ifndef ___RGLU_H
#define ___RGLU_H

#include "glinc.h"
#include "Vector.h"

void rgluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);

void rgluLookAt( vector eye, vector centre, vector up);

#endif
