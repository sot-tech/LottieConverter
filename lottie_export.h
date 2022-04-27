/*
 * File:   lottie_export.h
 * Author: sot
 *
 * Created on 26 sep 2019 y., 20:40
 */

#ifndef LOTTIE_EXPORT_H
#define LOTTIE_EXPORT_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <zlib.h>
#include <png.h>
#include <rlottie.h>
#include <gif_lib.h>

//include from getarg.h
extern "C" int GifQuantizeBuffer(unsigned int Width, unsigned int Height,
                                 int *ColorMapSize, GifByteType *RedInput,
                                 GifByteType *GreenInput, GifByteType *BlueInput,
                                 GifByteType *OutputBuffer,
                                 GifColorType *OutputColorMap);

using namespace std;
using namespace rlottie;

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

#define li_MAX_DIMENSION 16384

#define li_OUT_PNG 0
#define li_OUT_PNGS 1
#define li_OUT_GIF 2
#define ls_OUT_PNG "png"
#define ls_OUT_PNGS "pngs"
#define ls_OUT_GIF "gif"
#define ls_OUT_PNGS_SUFFIX "%0*u.png"

#define lz_CHUNK_SIZE 0x4000
#define lz_WINDOW_BITS 15
#define lz_ENABLE_ZLIB_GZIP 32

#define lp_COLOR_DEPTH 8
#define lp_COLOR_BYTES 4
#define lp_COLOR_BG 0

typedef uint8_t byte;

struct byte_buffer {
	byte_buffer() = default;

	byte *buffer = nullptr;
	size_t size = 0;
};

struct file {
	FILE *file_pointer = nullptr;
	char *path = nullptr;

	file(FILE *fd, char *p) {
		file_pointer = fd;
		path = p;
	}

	void close() const {
		if (file_pointer != nullptr) fclose(file_pointer);
	}
};

int bb_append(byte_buffer *bb, byte *data, size_t data_size) {

	bb->buffer = (byte *) realloc(bb->buffer, (bb->size + data_size) * sizeof(byte));
	if (bb->buffer == nullptr) {
		return EXIT_FAILURE;
	}
	memset(bb->buffer + bb->size, 0, data_size);
	memcpy(bb->buffer + bb->size, data, data_size);
	bb->size += data_size;
	return EXIT_SUCCESS;
}

#endif /* LOTTIE_EXPORT_H */

