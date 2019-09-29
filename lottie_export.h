/*
 * Copyright (C) 2019 sot.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 */

/* 
 * File:   lottie_export.h
 * Author: sot
 *
 * Created on 26 sep 2019 y., 20:40
 */

#ifndef LOTTIE_EXPORT_H
#define LOTTIE_EXPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <zlib.h>
#include <png.h>
#include <rlottie.h>

using namespace std;
using namespace rlottie;

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

#define li_OUT_PNG 0
#define li_OUT_GIF 1
#define ls_OUT_PNG "png"
#define ls_OUT_GIF "gif"

#define lz_CHUNK_SIZE 0x4000
#define lz_WINDOWN_BITS 15
#define lz_ENABLE_ZLIB_GZIP 32

#define lp_COLOR_DEPTH 8
#define lp_COLOR_BYTES 4

struct byte_buffer {
	uint8_t * buffer = NULL;
	size_t size = 0;
};

#define bb_init() {.buffer = NULL, .size = 0}

int bb_append(byte_buffer * bb, uint8_t * data, size_t data_size) {
	
	bb->buffer = (uint8_t *) realloc(bb->buffer, (bb->size + data_size) * sizeof (uint8_t));
	if (bb->buffer == NULL) {
		perror("Unable to extend byte buffer\n");
		return EXIT_FAILURE;
	}
	memset(bb->buffer + bb->size, 0, data_size);
	memcpy(bb->buffer + bb->size, data, data_size);
	bb->size += data_size;
}

#endif /* LOTTIE_EXPORT_H */

