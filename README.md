## LottieConverter
Simple lottie (telegram animated sticker) converter.

Animation can be converted to png (with transparency) or to gif.

Uses:

*  [rlottie library](https://github.com/Samsung/rlottie "Samsung/rlottie")

*  [gif.h simple library](https://github.com/WohlSoft/LunaLua/blob/master/LunaDll/libs/gif-h/gif.h)

*  libpng, zlib

## Usage
`lottieconverter input_file|- output_file|- type [resolution] [out_frames] [bg_color]`

Parameters:

* input_file - path to lottie json or tgs (gzip-ed json), if set to `-` stdin is used
* output_file - file to be written, if set `-` stdout is used
* type - output format, currently supported: `png`, `gif`
* resolution - size in pixels of out image, shoul be in `SIZExSIZE` format, default `128x128`
* out_frames - only for `png`, which frame in percents to extract, i.e. if set to `20`, and there's 200 frames in animation, 40th frame will be extracted, default `1`
* bg_color - only for `gif`, background color (RGB) to be set unstead fully transparent pixels, format must be same as C constant representation, default `0xffffff` (white)

