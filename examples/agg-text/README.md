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


