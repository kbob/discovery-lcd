Ways to render text.

Freetype or AGG.
Hinted or relaxed-hinted.
Pixel-aligned or unaligned.
Subpixel shaded or not.

That's 16 possibilities.

    F/A   Hint  Align Subpix

     F     H     P     N     - Freetype default.  Done.
     F     H     P     Y     - Don't know how.
     F     H     N     N     - 
     F     H     N     Y     - Don't know how.

     F     R     P     N     - 
     F     R     P     Y     - Don't know how.
     F     R     N     N     - 
     F     R     N     Y     - Don't know how.

     A     H     P     N     - 
     A     H     P     Y     - 
     A     H     N     N     - AGG default.  Done.
     A     H     N     Y     - 

     A     R     P     N     - 
     A     R     P     Y     - 
     A     R     N     N     - 
     A     R     N     Y     - 


So let's create a tool that accepts a bunch of parameters and spits
out a pixmap.  Well, a pixmap plus, because we also need to know where
the baseline is and how much the horizontal and vertical advances are.

    $ spitmap [options] identifier-root "text"

    --visual=argb8888|rgb888|rgb565|argb1555|argb4444|al88|l8|a8|al44|a4|l4
    --font=FONT.ttf
    --size=N
    --resolution=N
    --renderer=freetype|agg
    --hinting=yes|vertical|no
    --align=y|n
    --subpixel=y|n

creates this

    #include "pixmap.h"

    static const {visual} $(identifier-root}_bits[] = {
        ...
    };
    const text_pixmap ${identifier-root} = {
        ...
    };
  
where

    typedef struct text_pixmap { 
        pixmap pix;
        int baseline;
        float ascent;
        float descent;
        float advance_x;
        float advance_y;
    } text_pixmap;

