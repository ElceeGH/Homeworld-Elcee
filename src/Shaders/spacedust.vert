/*=============================================================================
    Name    : spacedust.vert
    Purpose : Renders space dust.

    May as well do it on the GPU since we can. That thing is doing nothing!

    How this works takes a bit of explanation.
    - Dust motes are worldspace points that don't move*.
    - Dust should always be visible.
    - Lines with zero length have zero pixels, so we need points too.
    - Since we need both lines and points, the shader is executed twice
      once for each primitive type.
    - Vertexes are duplicated for each mote so that a mote can be
      emitted as a line with no additional CPU overhead.
    - Lines select the curr/prev transformed pos by mod 2 of vertex index.
    - Points select curr by mod 1 and skip every second vertex using stride.
    
    * Mostly. The vertex positions are constant, but their position is moved
      into the cube centred at the camera and wrapped around at its edges.
      You can't see it, even without alpha fading, because the worldspace
      position is conjured using the curr/prev camproj matrixes as if it were
      continuous motion.
    
    Created 30/08/2024 by Elcee
=============================================================================*/



#version 130 // Needed for gl_VertexID

// Uniforms
uniform mat4  uMatCurr;   // CamProj matrix, current frame
uniform mat4  uMatPrev;   // CamProj matrix, previous frame
uniform int   uMode;      // Primitive/modulo. 1 = points, 2 = lines

uniform vec3  uCentre;    // Camera position / centre of volume
uniform float uRadius;    // Cube side half-length
uniform vec2  uRes;       // Screen dimensions

uniform vec3  uCol;       // Dust mote colour
uniform float uAlpha;     // Dust mote overall alpha
uniform float uCloseNear; // Fade in when this close to camera (begin)
uniform float uCloseFar;  // Fade in when this close to camera (end)
uniform float uFadeNear;  // Fade out when this far from camera (begin)
uniform float uFadeFar;   // Fade out when this far from camera (end)
uniform float uResScale;  // Thickness in [1:2] range
uniform float uMbScale;   // Relative length for line motion blurring



// Inverse linear interpolation, clamped to [0:1]
float boxStep( float value, float low, float high ) {
    float range = high  - low;
    float delta = value - low;
    float ratio = delta / range;
    return clamp( ratio, 0.0, 1.0 );
}



// Move mote to the [camera eye / volume centre] accounting for wrapping.
// Pos is a position in the cube in [-radius, +radius].
// Cen is the camera eye.
vec4 wrapPosition( vec3 pos, vec3 cen ) {
    float hstep   = uRadius;
    float step    = hstep * 2.0;
    vec3  wrapped = floor((pos + cen + step) / step) * step - pos - hstep;
    return vec4( wrapped, 1.0 );
}



void main() {
    // Vertex attribs
    vec4  motePos   = wrapPosition( gl_Vertex.xyz, uCentre );
    float moteAlpha = gl_Color.r;
    float moteVis   = gl_Color.g;

    // Distance from camera
    float dist = distance( uCentre, motePos.xyz );    

    // Fade in from max range. With per-mote variation.  
    float fadeDist  = dist * (1.0 + moteVis);
    float fadeAlpha = 1.0 - boxStep( fadeDist, uFadeNear, uFadeFar );

    // Fade when too close to the camera.
    float closeAlpha = boxStep( dist, uCloseNear, uCloseFar );

    // Merge all the basic alphas
    float baseAlpha = moteAlpha * fadeAlpha * closeAlpha * uAlpha;

    // Transform to current and previous positions in clip space
    vec4 csCurr = uMatCurr * motePos;
    vec4 csPrev = uMatPrev * motePos;
    
    // Get line length in 2D screenspace (perspective division, viewport scale)
    vec2  ssCurr  = csCurr.xy / csCurr.w;
    vec2  ssPrev  = csPrev.xy / csPrev.w;
    vec2  ssDelta = (ssCurr - ssPrev) * uRes * 0.5;
    float ssLen   = length( ssDelta );

    // Lines fade in as length goes from 0 -> thickness.
    float lineRatio = boxStep( ssLen, 0.0, uResScale );
    float lineAlpha = baseAlpha * lineRatio;

    // Points fade in as length goes from thickness -> 0
    float pointRatio = 1.0 - lineRatio;
    float pointAlpha = baseAlpha * pointRatio;

    // Line motion blur. TODO room for improvement to reduce popping.
    lineAlpha /= max( 1.0, ssLen * uMbScale );

    // Select the point/line alpha
    float alpha = (uMode == 1) ? pointAlpha : lineAlpha;
    
    // Set colour
    gl_FrontColor = vec4( uCol, alpha );
    gl_BackColor  = gl_FrontColor;

    // Select the line end to use. For points this always selects csCurr.
    int vertexSelect = gl_VertexID % uMode;
    gl_Position = (vertexSelect == 0) ? csCurr : csPrev;
}


