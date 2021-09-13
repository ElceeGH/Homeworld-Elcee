/*=============================================================================
    Name    : hyperspace.frag
    Purpose : Improved hyperspace effect.
    
    Homeworld normally uses a clipping plane, but it's hideous when viewed
    from the front since you can see into the hollow ship interiors. This
    changes the effect so that there is an opaque cross section.
    
    Backface culling must be disabled for this to work.

    Created 10/09/2021 by Elcee
=============================================================================*/



#version 120

uniform sampler2D uTex;       // Texture unit
uniform vec4      uViewport;  // glViewport params
uniform mat4      uProjInv;   // Projection matrix inverse
uniform vec4      uClipPlane; // Clip plane equation params
uniform float     uGlowDist;  // Radius of hyperspace glow
uniform vec4      uGlowCol;   // Colour of hyperspace glow
uniform vec4      uCrossCol;  // Colour of hyperspace intersect



// Position of the fragment in eye space.
// Same result as having a vertex shader output modelview * vertex.
vec4 eyeSpacePosition() {
    vec4  view  = uViewport;
    vec4  coord = gl_FragCoord;
    float dn    = gl_DepthRange.near;
    float df    = gl_DepthRange.far;

    vec4 ndcSpace;
    ndcSpace.xy = ((2.0 * coord.xy) - (2.0 * view.xy)) / (view.zw) - 1.0;
    ndcSpace.z  = (2.0 * coord.z - dn - df) / (df - dn);
    ndcSpace.w  = 1.0;

    vec4 clipSpace = ndcSpace / coord.w;
    return uProjInv * clipSpace;
}



// Signed distance to the clipping plane.
// Result is negative when behind the plane.
float planeSignedDistance( vec3 point ) {
    return dot( point, uClipPlane.xyz ) + uClipPlane.w;
}



// Gradient which fades linearly with distance.
float nearPlaneGlow( float dist ) {
    if (dist > uGlowDist)
        return 0.0;
    
    float linear = 1.0 - (dist / uGlowDist);
    return pow( linear, 2.2 ); // Linear to gamma space
}



void main() {
    // Get the distance to the clip plane.
    vec4  pos  = eyeSpacePosition();
    float dist = planeSignedDistance( pos.xyz );
    
    // Clip when on/behind the plane.
    if (dist <= 0.0)
        discard;
    
    // Basic colour
    float glow = nearPlaneGlow( dist );
    vec4  tex  = texture2D( uTex, gl_TexCoord[0].xy );
    vec4  col  = tex * gl_Color + glow * uGlowCol;
    
    // Back faces become the cross section colour.
    if ( ! gl_FrontFacing)
        col = uCrossCol;

    // Final output colour
    gl_FragColor = col;
}

