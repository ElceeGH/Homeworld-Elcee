/*=============================================================================
    Name    : capitalglow.vert
    Purpose : Renders capital ship engine glow.
    
    Calling pow() per-vertex in software? Kiiiinda slow.
    Doing it on the GPU is much better.

    Created 14/08/2024 by Elcee
=============================================================================*/



#version 120

// Uniforms
uniform vec3  uLightVec = vec3( 0.0, 0.0, -1.0 ); // Light direction
uniform float uSpecExp  = 1.0;                    // Specular exponent
uniform float uFade     = 1.0;                    // Alpha multiplier

void main() {
    vec3  normal = normalize( gl_NormalMatrix * gl_Normal );
    float cosine = dot( normal, uLightVec );
          cosine = abs( cosine );
    float alpha  = pow( cosine, uSpecExp );

    vec4  col    = gl_Color;
          col.g *= clamp( alpha, 0.0, 0.92 );
          col.a *= clamp( alpha, 0.0, 1.00 ) * uFade;
    
    gl_FrontColor = col;
    gl_BackColor  = gl_FrontColor;
    gl_Position   = gl_ModelViewProjectionMatrix * gl_Vertex;
}

