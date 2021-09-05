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
uniform vec4 uDitherScale = vec4(63.0); // Derived from output bit depth.     Scale = power( 2, bits ) - 1
uniform int  uDitherNoise = 0;          // Temporal variation in bayer dither. Noise = frameindex & 0x3F



// Component-wise IHOR
bvec4 inHalfOpenRange( vec4 f, float low, float high ) {
    bvec4 gt = greaterThanEqual( f, vec4(low)  );
    bvec4 lt = lessThan        ( f, vec4(high) );
    return bvec4(
        lt.x && gt.x,
        lt.y && gt.y,
        lt.z && gt.z,
        lt.w && gt.w
    );
}

// Mix using bvec4
vec4 select( vec4 a, vec4 b, bvec4 f ) {
    return vec4(
        f.x ? b.x : a.x,
        f.y ? b.y : a.y,
        f.z ? b.z : a.z,
        f.w ? b.w : a.w
    );
}

// 2x2 order dither.  A bit hacky.
//  00  10  10  01  11
//  00  00  01  11  11
vec4 dither( vec4 col ) {
    vec2 fp = gl_FragCoord.xy - 0.5; // Shift from pixel centre to integer
    vec2 mp = mod( fp, 2.0 );        // 0 or 1
    
    float f25 = (mp.x  + mp.y == 0.0) ? 1.0 : 0.0; // 12.5 - 37.5 %
    float f50 = (mp.x == mp.y)        ? 1.0 : 0.0; // 37.5 - 62.5 %
    float f75 = 1.0 - f25;                         // 62.5 - 87.5 %
    
    vec4 scaled = col * uDitherScale;
    vec4 low    = floor( scaled );
    vec4 frac   = fract( scaled );
    vec4 high   = ceil ( scaled );
    
    bvec4 isF25  = inHalfOpenRange ( frac, 0.125, 0.375 );
    bvec4 isF50  = inHalfOpenRange ( frac, 0.375, 0.625 );
    bvec4 isF75  = inHalfOpenRange ( frac, 0.625, 0.875 );
    bvec4 isF100 = greaterThanEqual( frac, vec4(0.875)  );
    
    vec4 ditherSelect = vec4( 0.0 );
         ditherSelect = select( ditherSelect, vec4(f25), isF25  );
         ditherSelect = select( ditherSelect, vec4(f50), isF50  );
         ditherSelect = select( ditherSelect, vec4(f75), isF75  );
         ditherSelect = select( ditherSelect, vec4(1.0), isF100 );
    
    vec4 select = mix( low, high, ditherSelect );
    vec4 renorm = select / uDitherScale;
    
    return renorm;
}



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
    float i  = mod( bi + uDitherNoise, 64.0 );
    return bayerPattern[int(i)] / 255.0;
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
    vec4 a = dither( gl_Color );
    vec4 b = bayerDither( gl_Color );
    
    if (gl_FragCoord.x < 2560/2)
         gl_FragColor = a;
    else gl_FragColor = b;
    
    if (gl_FragCoord.x - 0.5 == 2560/2)
        gl_FragColor = vec4(1.0);
}





