## LottieConverter
Simple lottie (telegram animated sticker) converter.

Animation can be converted to png (with transparency) or to gif.

Uses:

* [rlottie (MIT License)](https://github.com/Samsung/rlottie)

* [giflib (MIT Licence)](http://giflib.sourceforge.net)

* [libpng (zlib/libpng Licence)](http://www.libpng.org/pub/png/libpng.html)

* [zlib (zlib License)](https://zlib.net)

## Usage
`lottieconverter input_file|- output_file|- type [resolution] [option]`

Parameters:

* input_file - path to lottie json or tgs (gzip-ed json), if set to `-` stdin is used;
* output_file - file to be written, if set `-` stdout is used, **note, that for `pngs` type, file name prefix is required**;
* type - output format, currently supported: `png`, `gif`, `pngs`;
* resolution - **desired** size in pixels of out image, should be in `SIZExSIZE` format, default `128x128`. Resulting resolution depends on input;
* option - depends on type, default **10** for all:
    * `png` - which frame in percents to extract;
    * `pngs` and `gif` - desired frame rate.

### Notes
#### PNG Single frame (`png`)
Extract single PNG image, `option` value should be set in percents, 
If set to `20`, and there are 200 frames in animation, 40th frame will be extracted.

#### PNG Series (`pngs`)
Extract series of separate PNG files with prefix `output_file` and specified in `option` framerate.

If animation's framerate is the same as provided `option`, converter will extract every frame one time.

Otherwise:
* If animation framerate (i.e.) is 30, and `option` set to 10 (reduce FPS), 
every 3rd frame will be extracted;
* If animation frame rate is 10, and `option` set to 30 (increase FPS), 
every frame will be written 3 times with the same content.

#### GIF animation (`gif`)
Convert lottie animation to GIF animation.

GIF's minimum delay between frames is 1/100 second, so maximum framerate (value of `option`) limited to 100.

For the same reason, it is better to choose framerate value which is common factor of 100: 1, 2, 5, 10, 20, 25, 50 (100 is not recommended).

If `option` set to 30, delay will be rounded to 3 'ticks' (3/100 sec.), but GIF speed will be a little faster than original (round from 3.333... to 3),
and if 25 - delay will be exactly 4 'ticks' (5/100 sec.).

Another GIF's problem is transparency. GIF does not support alpha channel, 
but one of colors from frame's image palette can be marked as transparent.

Now, transparent color is hardcoded to 0'th color in palette, so if image contains non-background
elements with this color, they will be marked as transparent and some artifacts will appear.

## Build
### Basics
1. Clone this repo or download sources from releases page;
2. If cloned AND you need included `libpng`, `rlottie` or `giflib`, fetch submodules
by executing `git submodule update --init`;
3. Execute CMake build:
   1. `mkdir build && cd build`;
   2. `cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .`;
4. (optional) Perform test conversions with `ctest` (results will appear in `test/out` directory);
5. Copy or execute ready `lottieconverter` in `build` subdirectory.

### External libraries
By default, project uses system's shared `libpng` library, 
static `rlottie` and `libgif`. 
You can change behaviour by providing cmake options 
`SYSTEM_PNG`, `SYSTEM_RL` and `SYSTEM_GL` to 0 or 1 respectively
(`-DSYSTEM_PNG=0 -DSYSTEM_RL=1 -DSYSTEM_GL=0` or any).

Linux's development packages are usually named:
* `libpng-dev`;
* `libgif-dev` or `giflib-dev`;
* `librlottie-dev`.

and regular library packages:
* `libpng` or `libpng16`;
* `libgif` or `libgif7` or `giflib`;
* `rlottie` or `librlottie`.

_NB: `zlib` (`zlib1g`) or `zlib-dev` (`zlib1g-dev`) must be pre-installed in your system._

## Licencing notice

Versions v0.1.1 and earlier have been licensed under GNU LGPL 2.1.
Actual revision is licenced under BSD-3-Clause.