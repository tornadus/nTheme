/* Dark mode color math. Pure functions on 0x00RRGGBB values. */
#ifndef NTHEME_DARK_H
#define NTHEME_DARK_H

#define DARK_BACKGROUND 0x1E1E1E
#define DARK_FOREGROUND 0xE5E5E5

/* Invert the lightness of a near-gray color; saturated colors pass through unchanged. */
unsigned dark_flip(unsigned rgb);

/* Scale a light saturated background down so flipped black text stays readable on it. */
unsigned dark_dim(unsigned rgb);

/* Exact white/black become #FEFEFE/#010101: identical on the RGB565 panel, but distinguishable
 * from the hard-coded white/black that dark_remap() rewrites. */
unsigned dark_nudge(unsigned rgb);

/* Hard-coded OS colors: (near-)white becomes the dark background, (near-)black the light foreground. */
unsigned dark_remap(unsigned rgb);

#endif
