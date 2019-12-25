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

int write_png(byte * buffer, size_t w, size_t h, FILE * out_file) {
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
		png_byte * row = (png_byte *) png_malloc(png_ptr, sizeof (byte) * w * lp_COLOR_BYTES);
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
			byte b, g, r;
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
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return EXIT_SUCCESS;
}

int convert_and_write_to(byte * in_file_data, uint8_t convert_to, size_t w, size_t h, uint32_t param, file out_file) {
	string data(reinterpret_cast<char*> (in_file_data));
	unique_ptr<Animation> animation = Animation::loadFromData(data, "", "", false);
	if (!animation) {
		fputs("Unable to load animation\n", stderr);
		return EXIT_FAILURE;
	}
	size_t frame_count = animation->totalFrame();
	unique_ptr < uint32_t[] > buffer = unique_ptr < uint32_t[]>(new uint32_t[w * h]);
	if(buffer == nullptr || buffer.get() == nullptr){
		fputs("Unable to init frame buffer\n", stderr);
		return EXIT_FAILURE;
	}
	switch (convert_to) {
		case li_OUT_PNG:
		{
			size_t frame_to_extract = frame_count * param / 100;
			if (frame_to_extract > frame_count - 1) frame_to_extract = frame_count - 1;
			Surface surface(buffer.get(), w, h, w * lp_COLOR_BYTES);
			animation->renderSync(frame_to_extract, surface);
			return write_png(reinterpret_cast<byte*> (buffer.get()), w, h, out_file.file_pointer);
		}
		case li_OUT_PNGS:
		{
			float ratio = (float)animation->frameRate()/param;
			size_t frame_count_out = frame_count/ratio, file_template_len = strlen(out_file.path) + strlen(ls_OUT_PNGS_SUFFIX) + 1;
			size_t frame_count_out_t = frame_count_out;
			unsigned int digit_count = 1, frame_number = 0;
			while(frame_count_out_t > 9){
				frame_count_out_t /= 10;
				++digit_count;
			}
			char * file_name_template = (char *) calloc(file_template_len, sizeof (char)),
				* file_name = (char *) calloc(file_template_len + digit_count, sizeof (char));
			int result = EXIT_SUCCESS;
			if(file_name_template == NULL || file_name == NULL){
				if(file_name_template != NULL) free(file_name_template);
				if(file_name != NULL) free(file_name);
				perror("Unable to convert to png sequence, possibly prefix too long\n");
				return EXIT_FAILURE;
			}
			memset(file_name_template, '\0', file_template_len);
			strncpy(file_name_template, out_file.path, strlen(out_file.path));
			strncat(file_name_template, ls_OUT_PNGS_SUFFIX, file_template_len);
			for(float frame_current = 0.0f; frame_current < (float)frame_count; frame_current += ratio){
				FILE * fp = NULL;
				memset(file_name, '\0', file_template_len + digit_count);
				snprintf(file_name, file_template_len + digit_count, file_name_template, digit_count, frame_number++);
				if((fp = fopen(file_name, "wb")) == NULL){
					perror("Unable to write png frame\n");
					result = EXIT_FAILURE;
					break;
				}
				Surface surface(buffer.get(), w, h, w * lp_COLOR_BYTES);
				animation->renderSync((size_t)(frame_current + 0.5f), surface);
				result = write_png(reinterpret_cast<byte*> (buffer.get()), w, h, fp);
				fclose(fp);
				if(result != EXIT_SUCCESS){
					break;
				}
			}
			free(file_name_template);
			free(file_name);
			return result;
		}
		case li_OUT_GIF:
		{
			byte bg_r = (param >> 16) & 0xff, bg_g = (param >> 8) & 0xff, bg_b = param & 0xff;
			GifWriter writer = GIF_WRITER_INIT(out_file.file_pointer);
			int frame_to_extract = (int) (animation->frameRate() / 10);
			if (GifBegin(&writer, w, h, 1)) {
				for (int frame_current = 0; frame_current < frame_count; frame_current += frame_to_extract) {
					Surface surface(buffer.get(), w, h, w * lp_COLOR_BYTES);
					animation->renderSync(frame_current, surface);
					byte * byte_buffer_raw = reinterpret_cast<byte*> (buffer.get());
					byte * byte_buffer_exch = byte_buffer_raw, * byte_buffer_start = byte_buffer_raw;
					size_t pixel_count = w*h;
					for (int i = 0; i < pixel_count; ++i) {
						byte b, g, r, a;
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

int unzip(FILE * in_file, byte_buffer * out_data) {
	z_stream strm = {0};
	byte in[lz_CHUNK_SIZE];
	byte out[lz_CHUNK_SIZE];
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

		bytes_read = fread(in, sizeof (byte), sizeof (in), in_file);
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
							if (bb_append(out_data, in, bytes_read) == EXIT_FAILURE) {
								fputs("Unable to allocate memory\n", stderr);
								return EXIT_FAILURE;
							}
							bytes_read = fread(in, sizeof (byte), sizeof (in), in_file);
							if (ferror(in_file)) {
								perror("Unable to read file data\n");
								return EXIT_FAILURE;
							}
						} while (bytes_read > 0);
						return EXIT_SUCCESS;
					} else {
						fputs("zlib data error\n", stderr);
						return EXIT_FAILURE;
					}
				default:
					inflateEnd(& strm);
					fprintf(stderr, "zlib error %d.\n", zlib_status);
					return EXIT_FAILURE;
			}
			have = lz_CHUNK_SIZE - strm.avail_out;
			if (bb_append(out_data, out, have) == EXIT_FAILURE) {
				fputs("Unable to allocate memory\n", stderr);
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

	uint32_t param = 1;
	uint8_t convert_to = li_OUT_PNG;
	size_t w = 128, h = 128;
	FILE * in_file = stdin;
	file out_file = file_init(stdout, NULL);
	int argi = argc;
	switch (argc) {
		case 6:
			--argi;
			param = strtoul(argv[argi], NULL, 0);
		case 5:
		{
			--argi;
			size_t len = strlen(argv[argi]);
			size_t index_of_x = strcspn(argv[argi], "x");
			if (len > 3 && index_of_x > 0 && index_of_x < len - 1) {
				char * res = (char *) calloc(len, sizeof (char));
				if (res == NULL) {
					perror("Resolution error");
					return EXIT_FAILURE;
				}
				memset(res, '\0', len);
				strncpy(res, argv[argi], index_of_x);
				w = strtoul(argv[argi], NULL, 10);
				++index_of_x;
				memset(res, '\0', len);
				strncpy(res, argv[argi] + index_of_x, len - index_of_x);
				h = strtoul(argv[argi], NULL, 10);
				free(res);
				if(h == 0 || w == 0 || h > li_MAX_DIMENSION || w > li_MAX_DIMENSION){
					fputs("Invalid resolution\n", stderr);
					return EXIT_FAILURE;
				}
			} else {
				fputs("Invalid resolution\n", stderr);
				return EXIT_FAILURE;
			}
		}
		case 4:
		{
			--argi;
			if (!strcmp(argv[argi], ls_OUT_PNG)) {
				convert_to = li_OUT_PNG;
			} else if (!strcmp(argv[argi], ls_OUT_PNGS)) {
				convert_to = li_OUT_PNGS;
			} else if (!strcmp(argv[argi], ls_OUT_GIF)) {
				convert_to = li_OUT_GIF;
			} else {
				fputs("Unsupported out format\n", stderr);
				return EXIT_FAILURE;
			}
		}
		case 3:
		{
			--argi;
			size_t len = strlen(argv[argi]);
			bool write_to_file = len > 1 && argv[argi][0] != '-';
			if (convert_to == li_OUT_PNGS) {
				if (!write_to_file) {
					fputs("Unable to write image sequence to stdout, provide file prefix\n", stderr);
					return EXIT_FAILURE;
				}
				if(strlen(argv[argi]) > FILENAME_MAX){
					fputs("File name is too long", stderr);
				}
				out_file.path = argv[argi];
				out_file.file_pointer = NULL;
			} else {
				if (write_to_file) {
					out_file.file_pointer = fopen(argv[argi], "wb");
				}
				if (out_file.file_pointer == NULL) {
					perror(argv[argi]);
					return EXIT_FAILURE;
				}
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
				file_close(out_file);
				return EXIT_FAILURE;
			}
			break;
		}
		case 1:
			fputs("Usage: PROG input_file|- output_file|- png|pngs|gif [resolution(128x128)] [param]\n", stderr);
			return EXIT_FAILURE;
	}

	byte_buffer in_file_data = bb_init();
	in_file_data.buffer = (byte *) calloc(0, sizeof (byte));
	if (in_file_data.buffer == NULL) {
		perror("Unable to init byte buffer");
		fclose(in_file);
		file_close(out_file);
		return EXIT_FAILURE;
	}
	int result = unzip(in_file, &in_file_data);
	fclose(in_file);
	if (result == EXIT_SUCCESS) {
		byte eos[1] = {'\0'};
		bb_append(&in_file_data, eos, 1);
		result = convert_and_write_to(in_file_data.buffer, convert_to, w, h, param, out_file);
	}
	else
		fputs("zlib error\n", stderr);

	free(in_file_data.buffer);
	file_close(out_file);

	return result;
}

