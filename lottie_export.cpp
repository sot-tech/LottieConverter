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
 * File:   lottie_export.cpp
 * Author: sot
 *
 * Created on 26 sep 2019 y., 20:38
 */

#include "lottie_export.h"
#include "gif.h"

int write_png(uint8_t * buffer, size_t w, size_t h, FILE * out_file) {
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_byte ** row_pointers = NULL;


	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fputs("PNG export failed: unable to create structure\n", stderr);
		return EXIT_FAILURE;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fputs("PNG export failed: unable to create info data\n", stderr);
		return EXIT_FAILURE;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fputs("PNG export failed: longjump failed\n", stderr);
		return EXIT_FAILURE;
	}

	png_set_IHDR(png_ptr, info_ptr, w, h, lp_COLOR_DEPTH, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);

	row_pointers = (png_byte **) png_malloc(png_ptr, h * sizeof (png_byte *));
	if (row_pointers == NULL) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		perror("PNG export failed\n");
		return EXIT_FAILURE;
	}
	for (int y = 0; y < h; ++y) {
		png_byte * row = (png_byte *) png_malloc(png_ptr, sizeof (uint8_t) * w * lp_COLOR_BYTES);
		if (row == NULL) {
			perror("PNG export failed\n");
			for (int yy = 0; yy < y; ++yy) {
				png_free(png_ptr, row_pointers[yy]);
			}
			png_free(png_ptr, row_pointers);
			png_destroy_write_struct(&png_ptr, &info_ptr);
			return EXIT_FAILURE;
		}
		row_pointers[y] = row;
		for (int x = 0; x < w; x++) {
			uint8_t b, g, r;
			b = *buffer++;
			g = *buffer++;
			r = *buffer++;
			*row++ = r;
			*row++ = g;
			*row++ = b;
			*row++ = *buffer++;
		}
	}
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_init_io(png_ptr, out_file);

	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	for (int y = 0; y < h; ++y) {
		png_free(png_ptr, row_pointers[y]);
	}
	png_free(png_ptr, row_pointers);
	return EXIT_SUCCESS;
}

int convert_and_write_to(uint8_t * in_file_data, uint8_t convert_to, size_t w, size_t h, uint8_t frame, uint32_t bg_color, FILE * out_file) {
	string data(reinterpret_cast<char*> (in_file_data));
	unique_ptr<Animation> animation = Animation::loadFromData(data, "", "", false);
	if (!animation) {
		fputs("Unable to load animation\n", stderr);
		return EXIT_FAILURE;
	}
	size_t frame_count = animation->totalFrame();
	int frame_to_extract = frame_count * frame / 100;
	unique_ptr < uint32_t[] > buffer = unique_ptr < uint32_t[]>(new uint32_t[w * h]);
	switch (convert_to) {
		case li_OUT_PNG:
		{
			if (frame_to_extract < 1) frame_to_extract = 1;
			Surface surface(buffer.get(), w, h, w * lp_COLOR_BYTES);
			animation->renderSync(frame_to_extract, surface);
			return write_png(reinterpret_cast<uint8_t*> (buffer.get()), w, h, out_file);
		}
		case li_OUT_GIF:
		{
			uint8_t bg_r = (bg_color >> 16) & 0xff, bg_g = (bg_color >> 8) & 0xff, bg_b = bg_color & 0xff;
			GifWriter writer = GifWriter_init(out_file);
			frame_to_extract = (int) (animation->frameRate() / 10);
			if (GifBegin(&writer, w, h, 1)) {
				for (int frame_current = frame_to_extract; frame_current < frame_count; frame_current += frame_to_extract) {
					Surface surface(buffer.get(), w, h, w * lp_COLOR_BYTES);
					animation->renderSync(frame_current, surface);
					uint8_t * byte_buffer_raw = reinterpret_cast<uint8_t*> (buffer.get());
					uint8_t * byte_buffer_exch = byte_buffer_raw, * byte_buffer_start = byte_buffer_raw;
					size_t pixel_count = w*h;
					for (int i = 0; i < pixel_count; ++i) {
						uint8_t b, g, r, a;
						b = *byte_buffer_raw++;
						g = *byte_buffer_raw++;
						r = *byte_buffer_raw++;
						a = *byte_buffer_raw++;
						if (!a) {
							b = bg_b;
							g = bg_g;
							r = bg_r;
						}
						*byte_buffer_exch++ = r;
						*byte_buffer_exch++ = g;
						*byte_buffer_exch++ = b;
						*byte_buffer_exch++ = a;
					}
					if (!GifWriteFrame(&writer, byte_buffer_start, w, h, 1, lp_COLOR_DEPTH)) {
						fputs("Something went wrong while appending gif frame\n", stderr);
						break;
					}
				}
			} else {
				fputs("Something went wrong while creating gif\n", stderr);
			}
			GifEnd(&writer);
			break;
		}
	}
	return EXIT_SUCCESS;
}

int gunzip(FILE * in_file, byte_buffer * out_data) {
	z_stream strm = {0};
	uint8_t in[lz_CHUNK_SIZE];
	uint8_t out[lz_CHUNK_SIZE];
	bool first_read = true;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = in;
	strm.avail_in = 0;


	if (inflateInit2(& strm, lz_WINDOWN_BITS | lz_ENABLE_ZLIB_GZIP) < 0) {
		fputs("Unable to init zlib\n", stderr);
		return EXIT_FAILURE;
	}

	while (true) {
		size_t bytes_read;
		int zlib_status;

		bytes_read = fread(in, sizeof (uint8_t), sizeof (in), in_file);
		if (ferror(in_file)) {
			inflateEnd(& strm);
			perror("Unable to read file data\n");
			return EXIT_FAILURE;
		}
		strm.avail_in = bytes_read;
		strm.next_in = in;
		do {
			size_t have;
			strm.avail_out = lz_CHUNK_SIZE;
			strm.next_out = out;
			zlib_status = inflate(& strm, Z_NO_FLUSH);
			switch (zlib_status) {
				case Z_OK:
				case Z_STREAM_END:
				case Z_BUF_ERROR:
					break;

				case Z_DATA_ERROR:
					inflateEnd(&strm);
					if (first_read) {
						do {
							if (!bb_append(out_data, in, bytes_read)) {
								return EXIT_FAILURE;
							}
							bytes_read = fread(in, sizeof (uint8_t), sizeof (in), in_file);
							if (ferror(in_file)) {
								perror("Unable to read file data\n");
								return EXIT_FAILURE;
							}
						} while (bytes_read > 0);
						return EXIT_SUCCESS;
					} else {
						return EXIT_FAILURE;
					}
				default:
					inflateEnd(& strm);
					fprintf(stderr, "Gzip error %d.\n", zlib_status);
					return EXIT_FAILURE;
			}
			have = lz_CHUNK_SIZE - strm.avail_out;
			if (!bb_append(out_data, out, have)) {
				return EXIT_FAILURE;
			}
		} while (strm.avail_out == 0);
		if (feof(in_file)) {
			inflateEnd(& strm);
			break;
		}
		if (first_read) first_read = false;
	}
	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {

	uint32_t bg_color = 0xffffff;
	uint8_t get_frame = 1, convert_to = li_OUT_PNG;
	size_t w = 128, h = 128;
	FILE *in_file = stdin, *out_file = stdout;
	int argi = argc;
	switch (argc) {
		case 7:
			--argi;
			bg_color = strtoul(argv[argi], NULL, 0);
		case 6:
			--argi;
			get_frame = (uint8_t) atoi(argv[argi]);
			if (get_frame > 100) {
				get_frame = 100;
			}
		case 5:
		{
			--argi;
			size_t len = strlen(argv[argi]);
			size_t index_of_x = strcspn(argv[argi], "x");
			if (len > 3 && index_of_x > 0 && index_of_x < len - 1) {
				char res[10] = {'\0'};
				strncpy(res, argv[argi], index_of_x);
				w = atoi(res);
				memset(res, '\0', 10);
				++index_of_x;
				strncpy(res, argv[argi] + index_of_x, len - index_of_x);
				h = atoi(res);
			} else {
				fputs("Invalid resolution", stderr);
				return EXIT_FAILURE;
			}
		}
		case 4:
		{
			--argi;
			if (!strcmp(argv[argi], ls_OUT_PNG)) {
				convert_to = li_OUT_PNG;
			} else if (!strcmp(argv[argi], ls_OUT_GIF)) {
				convert_to = li_OUT_GIF;
			} else {
				fputs("Unsupported out format", stderr);
				return EXIT_FAILURE;
			}
		}
		case 3:
		{
			--argi;
			size_t len = strlen(argv[argi]);
			if (len > 1 && argv[argi][0] != '-') {
				out_file = fopen(argv[2], "wb");
			}
			if (!out_file) {
				perror(argv[argi]);
				return EXIT_FAILURE;
			}
		}
		case 2:
		{
			--argi;
			size_t len = strlen(argv[argi]);
			if (len > 1 && argv[argi][0] != '-') {
				in_file = fopen(argv[argi], "rb");
			}
			if (!in_file) {
				perror(argv[argi]);
				fclose(out_file);
				return EXIT_FAILURE;
			}
			break;
		}
		case 1:
			fputs("Usage: PROG input_file|- output_file|- png|gif [resolution(128x128)] [out_frames(1)] [bg_color(0xffffff)]\n", stderr);
			return EXIT_FAILURE;
	}

	byte_buffer in_file_data = bb_init();
	in_file_data.buffer = (uint8_t *) calloc(0, sizeof (uint8_t));
	if (in_file_data.buffer == NULL) {
		perror("Unable to init byte buffer");
		fclose(in_file);
		fclose(out_file);
		return EXIT_FAILURE;
	}

	int result = gunzip(in_file, &in_file_data);
	fclose(in_file);
	if (result == EXIT_SUCCESS) {
		uint8_t eof[1] = {'\0'};
		bb_append(&in_file_data, eof, 1);
		result = convert_and_write_to(in_file_data.buffer, convert_to, w, h, get_frame, bg_color, out_file);
	}

	free(in_file_data.buffer);
	fclose(out_file);

	return result;
}

