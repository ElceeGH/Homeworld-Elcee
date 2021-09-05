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
uniform vec4 uDitherScale = vec4(63.0); // Derived from output bit depth.      Value = power( 2, bits ) - 1
uniform int  uDitherNoise = 0;          // Temporal variation in bayer dither. Value = frameindex & 0x3F

// 8x8 ordered dither pattern.
const float bayerPattern[] = float[8*8](
      0.0,  130.0,   32.0,  162.0,    8.0,  138.0,   40.0,  170.0,
    194.0,   65.0,  227.0,   97.0,  202.0,   73.0,  235.0,  105.0,
     49.0,  178.0,   16.0,  146.0,   57.0,  186.0,   24.0,  154.0,
    243.0,  113.0,  210.0,   81.0,  251.0,  121.0,  219.0,   89.0,
     12.0,  142.0,   45.0,  174.0,    4.0,  134.0,   36.0,  166.0,
    206.0,   77.0,  239.0,  109.0,  198.0,   69.0,  231.0,  101.0,
     61.0,  190.0,   28.0,  158.0,   53.0,  182.0,   20.0,  150.0,
    255.0,  125.0,  223.0,   93.0,  247.0,  117.0,  215.0,   85.0
);

// Get bayer value for this pixel
float bayerValue() {
    vec2  fp = gl_FragCoord.xy - 0.5;
    vec2  mp = mod( fp, 8.0 );
    float bi = int( mp.y*8.0 + mp.x );
    float ti = mod( bi + uDitherNoise, 64.0 );
    return bayerPattern[int(ti)] / 255.0;
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

