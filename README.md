## LottieConverter
Simple lottie (telegram animated sticker) converter.

Animation can be converted to png (with transparency) or to gif.

Uses:

* [rlottie library (MIT License)](https://github.com/Samsung/rlottie "Samsung/rlottie")

* [giflib (MIT Licence)](http://giflib.sourceforge.net)

* [libpng (zlib/libpng Licence)](http://www.libpng.org/pub/png/libpng.html)

* [zlib (zlib License)](https://zlib.net)

## Usage
`lottieconverter input_file|- output_file|- type [resolution] [option]`

Parameters:

* input_file - path to lottie json or tgs (gzip-ed json), if set to `-` stdin is used
* output_file - file to be written, if set `-` stdout is used, **note, that for `pngs` type, file name prefix is required**
* type - output format, currently supported: `png`, `gif`, `pngs`
* resolution - size in pixels of out image, shoul be in `SIZExSIZE` format, default `128x128`
* option - depends of type, default 1 for all:
    * `png` - which frame in percents to extract, i.e. if set to `20`, and there's 200 frames in animation, 40th frame will be extracted
    * `pngs` - frame rate to try to extract images, i.e. if animation framerate is 30, and `option` set to 10 every 3rd frame will be extracted,
    if animation frame rate is 10, and `option` set to 30 every frame will be written 3 times
    * `gif` - unused
    
## Build
1. Clone this repo or download sources from releases page
2. If cloned AND you need included `libpng`, `rlottie` or `giflib` fetch submodules
by executing `git submodule update --init`
3. Execute CMake build:
   1. `mkdir build && cd build`
   2. `cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .`
4. Copy or execute ready `lottieconverter` in `build` subdirectory.

## Additional build options
By default, project uses system's shared `libpng` library and header, 
static `rlottie` and `libgif`. 
You can change behaviour by providing cmake options 
`SYSTEM_PNG`, `SYSTEM_RL` and `SYSTEM_GL` to 0 or 1 respectively
(`-DSYSTEM_PNG=0 -DSYSTEM_RL=1 -DSYSTEM_GL=0` or any).

_NB: `libz` must be pre-installed in your system._

## Licencing notice

Up to v0.1.1 project has been licensed under GNU LGPL 2.1.
Actual revision is licenced under BSD-3-Clause.