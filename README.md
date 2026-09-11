# nTheme

A modern theme manager for TI-Nspire calculators running ndless

Alpha release, currently only supports the CX II-T on 6.4.0.74 with ndless r2022 or later

_Screenshots Here_

## Features

- Dark mode (experimental, not perfect)
- Accent colors
- Home screen wallpaper from PNG or JPEG, with crop, fit and fill scaling
- Glass (translucent) home screen text background
- Custom or blank home screen title (via `ndless/theme.cfg.tns`)
- Native settings dialog under Home > 5 > Style

## Installation and use

1. Copy `nTheme.tns` anywhere on your calculator
2. Run the program, the settings dialog will open automatically
3. To edit settings again, look under Home > 5 > Style

Note: Transfer `nTheme.tns` to `ndless/startup` if you'd like it to install itself alongside ndless automatically

### Wallpaper support

Put PNG or JPEG files into `ndless/wallpapers/`. The file name must end in `.tns`, for example `wallpaper.png.tns`. Pictures up to 4 megapixels can be decoded, and the home screen area is 320x217 pixels.

Tips: Try to keep images small and crop to ~3:2 for best results

## Known limitations

Dark mode isn't perfect. There may be bugs, including unreadable UI elements or issues involving Python and Lua apps. Open documents will retain some old colors until reopened.

## Please add support for [insert CX variant here] on [insert OS here]!!!

Take a snapshot (RAM state) of your variant and OS version under firebird while on the home screen, then run `utils/hookfinder.py` against it.
After that, open a GitHub issue with the .txt file it spits out, and I'll see what I can do from there.

## Build
1. install ndless SDK (ndless-sdk/bin on PATH)
2. run `make` at the project root

## Roadmap

- Theme bundles (cfg, wallpapers, icons, and colors all in a single file for easy sharing)
- Custom home screen icons (I have a working PoC for this)
- Custom status bar backgrounds
- Advanced theme editor (manual editing of the color table)
- Theme gallery webpage