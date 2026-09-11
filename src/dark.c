#include "dark.h"

static unsigned pack(int r, int g, int b)
{
	return ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b;
}

static int clamp255(int v)
{
	return v > 255 ? 255 : v;
}

unsigned dark_flip(unsigned rgb)
{
	int r = (rgb >> 16) & 255, g = (rgb >> 8) & 255, b = rgb & 255;
	int max = r > g ? (r > b ? r : b) : (g > b ? g : b);
	int min = r < g ? (r < b ? r : b) : (g < b ? g : b);
	int lightness = (max + min) / 2;
	int denominator = max + min <= 255 ? max + min : 510 - max - min;
	if (denominator == 0)
		denominator = 1;
	int saturation = (max - min) * 255 / denominator;
	if (saturation > 40)
		return rgb;
	int flipped = 229 - (199 * lightness) / 255;
	if (lightness == 0)
		return pack(flipped, flipped, flipped);
	return pack(clamp255(r * flipped / lightness),
	            clamp255(g * flipped / lightness),
	            clamp255(b * flipped / lightness));
}

unsigned dark_dim(unsigned rgb)
{
	return pack(((rgb >> 16) & 255) * 90 / 255, ((rgb >> 8) & 255) * 90 / 255, (rgb & 255) * 90 / 255);
}

unsigned dark_nudge(unsigned rgb)
{
	if (rgb == 0xFFFFFF)
		return 0xFEFEFE;
	if (rgb == 0x000000)
		return 0x010101;
	return rgb;
}

unsigned dark_remap(unsigned rgb)
{
	switch (rgb) {
	case 0xFFFFFF:
	case 0xFFFEFF:   /* Lists & Spreadsheet cell */
	case 0xFEFFFF:   /* graph axis-label box */
		return DARK_BACKGROUND;
	case 0x000000:
	case 0x000001:   /* 2D editor black */
		return DARK_FOREGROUND;
	default:
		return rgb;
	}
}
