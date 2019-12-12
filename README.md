## LottieConverter
Simple lottie (telegram animated sticker) converter.

Animation can be converted to png (with transparency) or to gif.

Uses:

*  [rlottie library](https://github.com/Samsung/rlottie "Samsung/rlottie")

*  [gif.h simple library](https://github.com/WohlSoft/LunaLua/blob/master/LunaDll/libs/gif-h/gif.h)

*  libpng, zlib

## Usage
`lottieconverter input_file|- output_file|- type [resolution] [option]`

Parameters:

* input_file - path to lottie json or tgs (gzip-ed json), if set to `-` stdin is used
* output_file - file to be written, if set `-` stdout is used, **note, that for `pngs` type, file name prefix is required**
* type - output format, currently supported: `png`, `gif`, `pngs`
* resolution - size in pixels of out image, shoul be in `SIZExSIZE` format, default `128x128`
* option - depends of type, default 0 for all:
    * `png` - which frame in percents to extract, i.e. if set to `20`, and there's 200 frames in animation, 40th frame will be extracted
    * `pngs` - frame rate to try to extract images, i.e. if animation framerate is 30, and `option` set to 10 every 3rd frame will be extracted,
    if animation frame rate is 10, and `option` set to 30 every frame will be written 3 times
    * `gif` - background color (RGB) to be set unstead fully transparent pixels, format must be same as C constant representation (`12345`, `0x123abc`, `01267` etc.)
    
## Build
Just execute `make CONF=Release` for release build (with -O2 optimization), or `make CONF=Debug` for debug.
You can also set additional variables for c++ compiler (`CXXFLAGS_ADD`) and linker (`LDLIBSOPTIONS_ADD`), i.e.:
If rlottie headers and library not installed to system default search directories,
but you have downloaded it's [source](https://github.com/Samsung/rlottie) to `/home/user/rlottie` 
and build it inside rlottie's `"build"` subdir, just execute:
```bash
LDLIBSOPTIONS_ADD="-L/home/user/rlottie/build" CXXFLAGS_ADD="-I/home/user/rlottie/inc" make CONF=Release
```
to avoid `fatal error: rlottie.h No such file or directory` error.
