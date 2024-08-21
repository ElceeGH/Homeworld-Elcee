/*=============================================================================
    Name    : prim3d.c
    Purpose : Draw 3d primitives.

    Created 7/1/1997 by lmoloney
    Copyright Relic Entertainment, Inc.  All rights reserved.
=============================================================================*/

#include "prim3d.h"

#include <math.h>
#include <stdlib.h>

#include "Debug.h"
#include "devstats.h"
#include "FastMath.h"
#include "LinkedList.h"
#include "Memory.h"
#include "render.h"
#include "rStateCache.h"

#define sizeofverticearray(x)   sizeof(vertice_array) + (sizeof(vector) * (x-1))

typedef struct
{
    Node   node;
    udword num_vertices;
    uword  axis;
    vector vertice[1];
} vertice_array;

//globals
LinkedList CircleList;

/*=============================================================================
    Functions:
=============================================================================*/

void primLine3Fade(vector *p1, vector *p2, color c, real32 fade)
{
    bool blendon = glIsEnabled(GL_BLEND);
    if (!blendon) glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    rndAdditiveBlends(FALSE);

    glBegin(GL_LINES);
    glColor4ub(colRed(c), colGreen(c), colBlue(c), (ubyte)(255.0f * fade));
    glVertex3fv((const GLfloat *)p1);
    glVertex3fv((const GLfloat *)p2);
    glEnd();

    if (!blendon) glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}

/*-----------------------------------------------------------------------------
    Name        : primLine3
    Description : Draw a line in 3D using having a thickness of 1 pixel
    Inputs      : p1, p2 - end points of line segment to draw
                  c - color to draw it in.
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void primLine3(vector *p1, vector *p2, color c)
{
    primLine3Fade( p1, p2, c, 1.0f );
}


// Draw a stippled line. Created for movement UI.
// You can animate the stipple along its length using the 'animate' param in [0:1] range.
void primLine3Stipple(vector *p1, vector *p2, color c, real32 step, real32 animate)
{
    bool blendon = glIsEnabled(GL_BLEND);
    if (!blendon) glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    rndAdditiveBlends(FALSE);

    const real32 stepHalf = step * 0.5f;
    const real32 distance = sqrtf(vecDistanceSquared(*p1,*p2));
    const real32 limit    = distance - stepHalf;
    
    vector delta;
    vecSub( delta, *p2, *p1 );
    vecNormalizeToLength( &delta, stepHalf );

    glBegin(GL_LINES);
        glColor3ub(colRed(c), colGreen(c), colBlue(c));

        // Offset initially to animate the stipple
        real32 cur = animate * step;
        vector pos = *p1;
        vecAddToScalarMultiply( pos, delta, animate*2.0f );

        // Render every other segment
        for (; cur<limit; cur+=step) {
            glVertex3fv( &pos.x );
            vecAddTo( pos, delta );
            glVertex3fv( &pos.x );
            vecAddTo( pos, delta );
        }

        // Complete the last partial segment
        if (cur < distance) {
            glVertex3fv( &pos.x );
            glVertex3fv( &p2->x );
        }
    glEnd();

    if (!blendon) glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}

/*-----------------------------------------------------------------------------
    Name        : primCircleSolid3
    Description : Draw a solid circle on the z = centre->z plane
    Inputs      : centre - centre point (in 3D coords) of the circle
                  radius - radius of circle (actually, distance
                    from centre to vertices)
                  nSlices - number of polygons to draw
                  c - color of circle
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void primCircleSolid3(vector *centre, real32 radius, sdword nSlices, color c)
{
    sdword index;
    GLfloat v[3];
    double theta;

    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    v[0] = centre->x;
    v[1] = centre->y;
    v[2] = centre->z;
    glBegin(GL_TRIANGLE_FAN);
    glVertex3fv(v);                                          //centre vertex
    for (index = 0, theta = 0.0; index < nSlices; index++)
    {
        v[0] = centre->x + (real32)(sin(theta)) * radius;
        v[1] = centre->y + (real32)(cos(theta)) * radius;
        theta += 2.0 * PI / (double)nSlices;
        glVertex3fv(v);                                      //vertex on outer rim
    }
    v[0] = centre->x;
    v[1] = centre->y + radius;
    glVertex3fv(v);                                          //final vertex on outer rim
    glEnd();
}

void primCircleSolid3Fade(vector *centre, real32 radius, sdword nSlices, color c, real32 fade)
{
    sdword index;
    GLfloat v[3];
    double theta;
    GLboolean blend = glIsEnabled(GL_BLEND);

    if (!blend)
    {
        glEnable(GL_BLEND);
        rndAdditiveBlends(FALSE);
    }

    glColor4ub(colRed(c), colGreen(c), colBlue(c), (ubyte)(fade * 255.0f));
    v[0] = centre->x;
    v[1] = centre->y;
    v[2] = centre->z;
    glBegin(GL_TRIANGLE_FAN);
    glVertex3fv(v);                                          //centre vertex
    for (index = 0, theta = 0.0; index < nSlices; index++)
    {
        v[0] = centre->x + sinf(theta) * radius;
        v[1] = centre->y + cosf(theta) * radius;
        theta += 2.0 * PI / (double)nSlices;
        glVertex3fv(v);                                      //vertex on outer rim
    }
    v[0] = centre->x;
    v[1] = centre->y + radius;
    glVertex3fv(v);                                          //final vertex on outer rim
    glEnd();

    if (!blend)
    {
        glDisable(GL_BLEND);
    }
}


/*-----------------------------------------------------------------------------
    Name        : primCreateNewCircleVerticeArray
    Description : Creates a set of vertices defining a unit circle around the axis given
    Inputs      : nSlices - the number of polygons in the unit circle,
                  axis - the axis around which the circle is drawn
    Outputs     : Allocates some memory and stuff
    Return      : the new vertice array
----------------------------------------------------------------------------*/
vertice_array *primCreateNewCircleVerticeArray(sdword nSlices, uword axis)
{
    udword i;
    double theta = 0.0;
    vertice_array *vertices = (vertice_array *)memAlloc(sizeofverticearray(nSlices+1), "circle_vertices", NonVolatile);

    vertices->num_vertices = nSlices + 1;
    vertices->axis         = axis;

    for (i = 0; i < vertices->num_vertices; i++)
    {
        switch (axis)
        {
            case X_AXIS:
                vertices->vertice[i].x = 0.0;
                vertices->vertice[i].y = cosf(theta);
                vertices->vertice[i].z = sinf(theta);
                break;
            case Y_AXIS:
                vertices->vertice[i].x = sin(theta);
                vertices->vertice[i].y = 0.0;
                vertices->vertice[i].z = cos(theta);
                break;
            case Z_AXIS:
                vertices->vertice[i].x = sin(theta);
                vertices->vertice[i].y = cos(theta);
                vertices->vertice[i].z = 0.0;
                break;
        }
        theta += 2.0 * PI / (real64)nSlices;
    }

    listAddNode(&CircleList, &vertices->node, vertices);

    return vertices;
}


/*-----------------------------------------------------------------------------
    Name        : primCircleOutline3
    Description : Draw a solid circle on the z = centre->z plane
    Inputs      : centre - centre point (in 3D coords) of the circle
                  radius - radius of circle (actually, distance
                    from centre to vertices)
                  nSlices - number of polygons to draw
                  nSpokes - number of 'spokes' which actually get drawn.
                    nSlices should integer divide by this with no remainder.
                  color - color of circle
                  axis - axis on which to draw the circle
    Outputs     : ..
    Return      : void
----------------------------------------------------------------------------*/
void primCircleOutline3(vector *centre, real32 radius, sdword nSlices,
                        sdword nSpokes, color color, uword axis)
{
    Node *node;
    vertice_array *vertices;
    register vector *vec_ptr;
    sdword index;
    GLfloat c[3], rim[3];

    // find the vertice list containing the unit circle that has the same
    // number of slices and is aligned along the same axis as the circle to be drawn
    node = CircleList.head;
    while (node != NULL)
    {
        vertices = (vertice_array *)listGetStructOfNode(node);
        if ((vertices->num_vertices == (nSlices + 1)) && (vertices->axis == axis))
        {
            //found the correct unit circle
            break;
        }

        node = node->next;
    }

    //if the unit circle isn't found, generate a new one
    if (node == NULL)
    {
        vertices = primCreateNewCircleVerticeArray(nSlices, axis);
    }

    if (nSpokes != 0)
    {
        nSpokes = nSlices / nSpokes;
    }

    glColor3ub(colRed(color), colGreen(color), colBlue(color));
    c[0] = centre->x;                                       //compute centre point
    c[1] = centre->y;
    c[2] = centre->z;

    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    rndAdditiveBlends(FALSE);

    vec_ptr = &vertices->vertice[0];

    switch (axis)
    {
        case X_AXIS:
            rim[0] = centre->x + vec_ptr->x * radius;

            //draw the circle
            glBegin(GL_LINE_LOOP);
            for (index = 0; index <= nSlices; index++, vec_ptr++)
            {
                rim[1] = centre->y + vec_ptr->y * radius;
                rim[2] = centre->z + vec_ptr->z * radius;
                glVertex3fv(rim);                                   //vertex on rim
            }
            glEnd();

            //now draw the spokes
            if (nSpokes)
            {
                vec_ptr = &vertices->vertice[0];
                glBegin(GL_LINES);
                for (index = 0; index <= nSlices; index += nSpokes, vec_ptr += nSpokes)
                {
                    rim[1] = centre->y + vec_ptr->y * radius;
                    rim[2] = centre->z + vec_ptr->z * radius;
                    glVertex3fv(c);
                    glVertex3fv(rim);
                }
                glEnd();
            }
            break;

        case Y_AXIS:
            rim[1] = centre->y + vec_ptr->y * radius;

            //draw the circle
            glBegin(GL_LINE_LOOP);
            for (index = 0; index <= nSlices; index++, vec_ptr++)
            {
                rim[0] = centre->x + vec_ptr->x * radius;
                rim[2] = centre->z + vec_ptr->z * radius;
                glVertex3fv(rim);                                   //vertex on rim
            }
            glEnd();

            //draw the spokes
            if (nSpokes)
            {
                vec_ptr = &vertices->vertice[0];
                glBegin(GL_LINES);
                for (index = 0; index <= nSlices; index += nSpokes, vec_ptr += nSpokes)
                {
                    rim[0] = centre->x + vec_ptr->x * radius;
                    rim[2] = centre->z + vec_ptr->z * radius;
                    glVertex3fv(c);
                    glVertex3fv(rim);
                }
                glEnd();
            }
            break;

        case Z_AXIS:
            rim[2] = centre->z + vec_ptr->z * radius;

            //draw the circle
            glBegin(GL_LINE_LOOP);
            for (index = 0; index <= nSlices; index++, vec_ptr++)
            {
                rim[0] = centre->x + vec_ptr->x * radius;
                rim[1] = centre->y + vec_ptr->y * radius;
                glVertex3fv(rim);                                   //vertex on rim
            }
            glEnd();

            //draw the spokes
            if (nSpokes)
            {
                vec_ptr = &vertices->vertice[0];
                glBegin(GL_LINES);
                for (index = 0; index <= nSlices; index += nSpokes, vec_ptr += nSpokes)
                {
                    rim[0] = centre->x + vec_ptr->x * radius;
                    rim[1] = centre->y + vec_ptr->y * radius;
                    glVertex3fv(c);
                    glVertex3fv(rim);
                }
                glEnd();
            }
            break;

        default:
            break;
    }

    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}

/*-----------------------------------------------------------------------------
    Name        : primCircleOutlineZ
    Description : Similar to the previous function, except that it has no spokes
                    and is optimized for circles aligned to the Z-axis.
    Inputs      : centre - where to draw it.
                  radius - obvious
                  nSegments - number of line segments to make up the circle
                  c - color to draw in
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void primCircleOutlineZ(vector *centre, real32 radius, sdword nSegments, color c)
{
    //now that primCircleOutline3 has been optimized, it is faster than this one
    //this one can probably still be tweaked even further, so I'll leave it here
    primCircleOutline3(centre, radius, nSegments, 0, c, Z_AXIS);
}



// Draw a point that fades out from the centre to the edge.
// Used to replace primBlurryPoint22() and friends with a resolution-scalable alternative.
void primBlurryPoint3Fade( vector* centre, real32 size, color col, real32 fade ) {
    // This is for tiny objects, so just use a small fixed number of vertexes. YAGNI!
    udword steps = 16;

    // Circle edge position and rotational transform info
    real32 ex   = 0;
    real32 ey   = size;
    real32 roto = DEG_TO_RAD(360.0f / (real32)steps);
    real32 rotx = cosf( roto );
    real32 roty = sinf( roto );

    glEnable( GL_BLEND );
    glDepthMask( GL_FALSE );
    glShadeModel( GL_SMOOTH );
    rndBillboardEnable( centre );

    // Render fan with the centre at given alpha and edges transparent
    glBegin( GL_TRIANGLE_FAN );
        
        // Colour components
        ubyte red   = colRed(col);
        ubyte green = colBlue(col);
        ubyte blue  = colGreen(col);
    
        // Origin in centre
        glColor4ub( red, green, blue, (ubyte) (fade*255.0f) );
        glVertex2f( 0.0f, 0.0f );

        // All points at the edge have the same colour
        glColor4ub( red, green, blue, 0 );

        // Rotate around the circle's edge
        for (udword i=0; i<steps; i++) {
            glVertex2f( ex, ey );
            
            // Rotate around Z
            real32 nx = ex*rotx + ey*-roty;
            real32 ny = ex*roty + ey*+rotx;
            ex = nx;
            ey = ny;
        }

        // Complete last triangle
        glVertex2f( ex, ey );
    
    // Donezo, go back to normal
    glEnd();

    rndBillboardDisable();
    glShadeModel( GL_FLAT );
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
}



/*-----------------------------------------------------------------------------
    Name        : primEllipseOutlineZ
    Description : Draw an axis-aligned ellipse centred about a specified point on the Z plane
    Inputs      : centre - centre of the ellipse
                  rx - x - axis radius
                  ry - y - axis radius
                  nSegments - number of segments
                  c - color of ellipse
    Outputs     :
    Return      :
----------------------------------------------------------------------------*/
void primEllipseOutlineZ(vector *centre, real32 rx, real32 ry, sdword nSegments, color c)
{
    GLfloat rim[3];
    real32 theta, thetaDelta;
    real32 x, y;

    theta = 0.0f;
    thetaDelta = 2.0f * PI / (real32)nSegments;
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    x = centre->x;
    y = centre->y;
    rim[2] = centre->z;
    glBegin(GL_LINE_STRIP);
    for (; nSegments >= 0; nSegments--)
    {
        rim[0] = x + sinf(theta) * rx;
        rim[1] = y + cosf(theta) * ry;
        glVertex3fv(rim);
        theta += thetaDelta;
    }
    glEnd();
}

/*-----------------------------------------------------------------------------
    Name        : primPoint3
    Description : Draw a 3D point
    Inputs      : p1 - location of point
                  c - color of point
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void primPoint3(vector *p1, color c)
{
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glBegin(GL_POINTS);
    glVertex3f(p1->x, p1->y, p1->z);                        //!!! no size
    glEnd();
}

/*-----------------------------------------------------------------------------
    Name        : primPointSize3
    Description : Draw a 3D point with size
    Inputs      : p1 - location of point
                  size - physical size of point
                  c - color of point
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void primPointSize3(vector *p1, real32 size, color c)
{
    glPointSize(size);
    glColor3ub(colRed(c), colGreen(c), colBlue(c));
    glBegin(GL_POINTS);
    glVertex3f(p1->x, p1->y, p1->z);                        //!!! no size
    glEnd();
    glPointSize(1.0f);
}

void primPointSize3Fade(vector *p1, real32 size, color c, real32 fade)
{
    GLboolean blend = glIsEnabled(GL_BLEND);

    if (!blend)
    {
        glEnable(GL_BLEND);
    }

    glPointSize(size);
    glColor4ub(colRed(c), colGreen(c), colBlue(c), (ubyte)(fade * 255.0f));
    glBegin(GL_POINTS);
    glVertex3f(p1->x, p1->y, p1->z);                        //!!! no size
    glEnd();
    glPointSize(1.0f);

    if (!blend)
    {
        glDisable(GL_BLEND);
    }
}

/*-----------------------------------------------------------------------------
    Name        : primSolidTexture3
    Description : Draw a 3D point with size
    Inputs      : p1 - location of point
                  size - physical size of point
                  c - color of point
    Outputs     :
    Return      : void
----------------------------------------------------------------------------*/
void primSolidTexture3(vector *p1, real32 size, color c, trhandle tex)
{
    real32 halfsize;
    real32 biasRed, biasGreen, biasBlue;
    texreg* reg;

    halfsize = 0.5f * size;

    rndTextureEnable(TRUE);
//    glDepthMask(GL_FALSE);

    trMakeCurrent(tex);
    reg = trStructureGet(tex);
    if (bitTest(reg->flags, TRF_Alpha))
    {
        glEnable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
        rndAdditiveBlends(TRUE);
    }

    biasRed = colReal32(colRed(c));
    biasGreen = colReal32(colGreen(c));
    biasBlue = colReal32(colBlue(c));

    glColor3f(biasRed, biasGreen, biasBlue);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(p1->x-halfsize, p1->y-halfsize, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(p1->x+halfsize, p1->y-halfsize, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(p1->x+halfsize, p1->y+halfsize, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(p1->x-halfsize, p1->y+halfsize, 0.0f);
    glEnd();

    glDisable(GL_BLEND);
//    glDepthMask(GL_TRUE);
    rndAdditiveBlends(FALSE);
}

void primSolidTexture3Fade(vector *p1, real32 size, color c, trhandle tex, real32 fade)
{
   real32 halfsize = size / 2;
   real32 biasRed, biasGreen, biasBlue;
   texreg* reg;

   rndTextureEnable(TRUE);

   trMakeCurrent(tex);
   reg = trStructureGet(tex);
   if (bitTest(reg->flags, TRF_Alpha))
   {
      glEnable(GL_BLEND);
      glDisable(GL_ALPHA_TEST);
      rndAdditiveBlends(TRUE);
   }

   biasRed = colReal32(colRed(c));
   biasGreen = colReal32(colGreen(c));
   biasBlue = colReal32(colBlue(c));

   glColor4f(biasRed, biasGreen, biasBlue, fade);

   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 0.0f);
   glVertex3f(p1->x-halfsize, p1->y-halfsize, 0.0f);
   glTexCoord2f(1.0f, 0.0f);
   glVertex3f(p1->x+halfsize, p1->y-halfsize, 0.0f);
   glTexCoord2f(1.0f, 1.0f);
   glVertex3f(p1->x+halfsize, p1->y+halfsize, 0.0f);
   glTexCoord2f(0.0f, 1.0f);
   glVertex3f(p1->x-halfsize, p1->y+halfsize, 0.0f);
   glEnd();

   glDisable(GL_BLEND);
   rndAdditiveBlends(FALSE);
}


/*-----------------------------------------------------------------------------
    Name        : prim3dStartup
    Description : Initializes some 3d drawing structures
    Inputs      :
    Outputs     : Initializes the circle vertice list
    Return      : void
----------------------------------------------------------------------------*/
void prim3dStartup(void)
{
    listInit(&CircleList);
}


/*-----------------------------------------------------------------------------
    Name        : prim3dShutdown
    Description : Performs shutdown on 3d drawing structures
    Inputs      :
    Outputs     : Deallocates the circle vertice list
    Return      : void
----------------------------------------------------------------------------*/
void prim3dShutdown(void)
{
    listDeleteAll(&CircleList);
}


