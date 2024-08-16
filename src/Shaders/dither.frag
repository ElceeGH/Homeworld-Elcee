/*=============================================================================
    Name    : dither.frag
    Purpose : Temporal dithering to remove banding artifacts.
    
    The interpolated colours are very high precision. This gets lost on the way to
    the display and the result is ugly banding which is visually distracting.
    
    The input colour may be 20+ bits per channel. The framebuffer is only 8-bit, and 
    most monitors are only 6-bit and use dithering themselves to simulate 8-bit.
    
    The default values target these 6-bit monitors. Those fortunate enough to have
    better monitors will not notice the difference. :)

    Created 04/09/2021 by Elcee
=============================================================================*/



#version 120

// Uniforms
uniform float uDitherScale       = 63.0;       // Derived from output bit depth.      Value = power( 2, bits ) - 1
uniform int   uDitherTemporal    = 0;          // Temporal variation in bayer dither. Value = frameindex & 0x3F
uniform float uDitherPattern[64] = float[64](  // Ordered dither pattern. 8x8 bayer is the default.
      0.0/255.0,  130.0/255.0,   32.0/255.0,  162.0/255.0,    8.0/255.0,  138.0/255.0,   40.0/255.0,  170.0/255.0,
    194.0/255.0,   65.0/255.0,  227.0/255.0,   97.0/255.0,  202.0/255.0,   73.0/255.0,  235.0/255.0,  105.0/255.0,
     49.0/255.0,  178.0/255.0,   16.0/255.0,  146.0/255.0,   57.0/255.0,  186.0/255.0,   24.0/255.0,  154.0/255.0,
    243.0/255.0,  113.0/255.0,  210.0/255.0,   81.0/255.0,  251.0/255.0,  121.0/255.0,  219.0/255.0,   89.0/255.0,
     12.0/255.0,  142.0/255.0,   45.0/255.0,  174.0/255.0,    4.0/255.0,  134.0/255.0,   36.0/255.0,  166.0/255.0,
    206.0/255.0,   77.0/255.0,  239.0/255.0,  109.0/255.0,  198.0/255.0,   69.0/255.0,  231.0/255.0,  101.0/255.0,
     61.0/255.0,  190.0/255.0,   28.0/255.0,  158.0/255.0,   53.0/255.0,  182.0/255.0,   20.0/255.0,  150.0/255.0,
    255.0/255.0,  125.0/255.0,  223.0/255.0,   93.0/255.0,  247.0/255.0,  117.0/255.0,  215.0/255.0,   85.0/255.0
);

// Get bayer threshold value for this pixel
float bayerValue() {
    vec2  fp = gl_FragCoord.xy - 0.5 + vec2(float(uDitherTemporal));
    vec2  mp = mod( fp, 8.0 );
    float bi = mp.y*8.0 + mp.x;
    return uDitherPattern[int(bi)] ;
}

// Dither the colour using the bayer pattern
vec4 bayerDither( vec4 col ) {
    vec4 raw   = col * uDitherScale;
    vec4 low   = floor( raw );
    vec4 frac  = fract( raw );
    vec4 bayer = vec4( bayerValue() );
    vec4 sel   = low + step( bayer, frac );
    return sel / uDitherScale;
}

void main() {
    gl_FragColor = bayerDither( gl_Color );
}

