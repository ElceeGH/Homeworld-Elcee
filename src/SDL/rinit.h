/*=============================================================================
    Name    : rinit.h
    Purpose : rGL / OpenGL enumeration initialization routines

    Created 1/5/1999 by khent
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#ifndef ___RINIT_H
#define ___RINIT_H

typedef struct rmode
{
    int width, height, depth;
    struct rmode* next;
} rmode;

#define RIN_TYPE_OPENGL   1
#define RIN_TYPE_DIRECT3D 2
#define RIN_TYPE_SOFTWARE 4

typedef struct rdevice
{
    int type;
    char data[64];
    char name[64];
    struct rmode* modes;
    struct rdevice* next;
} rdevice;

void     rinEnumerateDevices(void);
void     rinFreeDevices(void);
rdevice* rinGetDeviceList(void);

#endif

