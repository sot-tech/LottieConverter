/*
 * File:   lottie_export.cpp
 * Author: sot
 *
 * Created on 26 sep 2019 y., 20:38
 */

#include "lottie_export.h"

int write_png(byte *buffer, size_t w, size_t h, FILE *out_file) {
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	png_byte **row_pointers;


	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr) {
		fputs("PNG export failed: unable to create structure\n", stderr);
		return EXIT_FAILURE;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fputs("PNG export failed: unable to create info data\n", stderr);
		return EXIT_FAILURE;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fputs("PNG export failed: longjump failed\n", stderr);
		return EXIT_FAILURE;
	}

	png_set_IHDR(png_ptr, info_ptr, w, h, lp_COLOR_DEPTH, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_DEFAULT,
	             PNG_FILTER_TYPE_DEFAULT);

	row_pointers = (png_byte **) png_malloc(png_ptr, h * sizeof(png_byte *));
	if (row_pointers == nullptr) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		perror("PNG export failed\n");
		return EXIT_FAILURE;
	}
	for (unsigned int y = 0; y < h; ++y) {
		auto *row = (png_byte *) png_malloc(png_ptr, sizeof(byte) * w * lp_COLOR_BYTES);
		if (row == nullptr) {
			perror("PNG export failed\n");
			for (unsigned int yy = 0; yy < y; ++yy) {
				png_free(png_ptr, row_pointers[yy]);
			}
			png_free(png_ptr, row_pointers);
			png_destroy_write_struct(&png_ptr, &info_ptr);
			return EXIT_FAILURE;
		}
		row_pointers[y] = row;
		for (unsigned int x = 0; x < w; x++) {
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

	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

	for (unsigned int y = 0; y < h; ++y) {
		png_free(png_ptr, row_pointers[y]);
	}
	png_free(png_ptr, row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return EXIT_SUCCESS;
}

int convert_and_write_to(byte *in_file_data, uint8_t convert_to, int w, int h, uint32_t param, file *out_file) {
	int status = EXIT_SUCCESS;
	string data(reinterpret_cast<char *> (in_file_data));
	unique_ptr<Animation> animation = Animation::loadFromData(data, "", "", false);
	if (!animation) {
		fputs("Unable to load animation\n", stderr);
		return EXIT_FAILURE;
	}
	size_t frame_count = animation->totalFrame();
	auto frame_inc = (float)animation->frameRate() / (float)param;
	unique_ptr<uint32_t[]> buffer = unique_ptr<uint32_t[]>(new uint32_t[w * h]);
	if (buffer == nullptr) {
		fputs("Unable to init frame buffer\n", stderr);
		return EXIT_FAILURE;
	}
	switch (convert_to) {
		case li_OUT_PNG: {
			size_t frame_to_extract = frame_count * param / 100;
			if (frame_to_extract > frame_count - 1) frame_to_extract = frame_count - 1;
			Surface surface(buffer.get(), w, h, w * lp_COLOR_BYTES);
			animation->renderSync(frame_to_extract, surface);
			return write_png(reinterpret_cast<byte *> (buffer.get()), w, h, out_file->file_pointer);
		}
		case li_OUT_PNGS: {
			float frame_current = 0.0f;
			auto frame_count_out = (size_t)((float)frame_count / frame_inc);
			size_t frame_count_out_t = frame_count_out;
			unsigned int digit_count = 1, frame_number = 0;
			while (frame_count_out_t > 9) {
				frame_count_out_t /= 10;
				++digit_count;
			}
			char *file_name_template = (char *) calloc(FILENAME_MAX, sizeof(char)),
				*file_name = (char *) calloc(FILENAME_MAX, sizeof(char));
			int result = EXIT_SUCCESS;
			if (file_name_template == nullptr || file_name == nullptr) {
				if (file_name_template != nullptr) free(file_name_template);
				if (file_name != nullptr) free(file_name);
				perror("Unable to convert to png sequence, possibly prefix too long\n");
				return EXIT_FAILURE;
			}
			memset(file_name_template, '\0', FILENAME_MAX);
			strncpy(file_name_template, out_file->path, FILENAME_MAX);
			strncat(file_name_template, ls_OUT_PNGS_SUFFIX, FILENAME_MAX);
			while (frame_current < (float) frame_count) {
				FILE *fp;
				memset(file_name, '\0', FILENAME_MAX);
				snprintf(file_name, FILENAME_MAX, file_name_template, digit_count, frame_number++);
				if ((fp = fopen(file_name, "wb")) == nullptr) {
					perror("Unable to write png frame\n");
					result = EXIT_FAILURE;
					break;
				}
				Surface surface(buffer.get(), w, h, w * lp_COLOR_BYTES);
				animation->renderSync(lround(frame_current), surface);
				result = write_png(reinterpret_cast<byte *> (buffer.get()), w, h, fp);
				fclose(fp);
				if (result != EXIT_SUCCESS) {
					break;
				}
				frame_current += frame_inc;
			}
			free(file_name_template);
			free(file_name);
			return result;
		}
		case li_OUT_GIF: {
			if(param == 0 || param > 100){
				fputs("GIF framerate must be between 1 and 100", stderr);
				return EXIT_FAILURE;
			}
			float frame_current = 0.0f;
			int error_code = 0;
			GifFileType *writer = EGifOpenFileHandle(fileno(out_file->file_pointer), &error_code);
			if (writer == nullptr || error_code != 0) {
				fprintf(stderr, "Unable to initialize GIF writer, error code: %d\n", error_code);
				return EXIT_FAILURE;
			}
			int color_map_size = 1 << lp_COLOR_DEPTH;
			byte loop[]{1, 0, 0}; //infinite gif loop
			static byte delay[4] = {0x0D,   // Bit 0 - flag if transparent index given (checked),
												// Bit 1 - User Input Flag (unchecked),
												// Bits 2-4 - Disposal Method:
												//      0 - unspecified,
												//      1 - don't dispose,
												//      2 - restore to background color,
												//      3 - restore to previous (checked)
                                            10,    // >Hundredths of
		                                     0,                     // seconds to wait<
				                             0};    // Transparent color index //TODO: try to calculate
			ColorMapObject *output_palette = GifMakeMapObject(color_map_size, nullptr);
			if(output_palette == nullptr){
				status = EXIT_FAILURE;
				goto CLOSE_FILE;
			}
			EGifSetGifVersion(writer, true);
			if (EGifPutScreenDesc(
				writer,
				w, h, lp_COLOR_DEPTH, 0,
				output_palette
			) == GIF_ERROR) {
				fputs("Something went wrong while creating gif\n", stderr);
				status = EXIT_FAILURE;
				goto FREE_MAP;
			}
			if (EGifPutExtensionLeader(writer, APPLICATION_EXT_FUNC_CODE) == GIF_ERROR) {
				fputs("Unable to write gif extension\n", stderr);
				status = EXIT_FAILURE;
				goto FREE_MAP;
			}
			if (EGifPutExtensionBlock(writer, 11, "NETSCAPE2.0") == GIF_ERROR) {
				fputs("Unable to write gif extension\n", stderr);
				status = EXIT_FAILURE;
				goto FREE_MAP;
			}
			if (EGifPutExtensionBlock(writer, 3, loop) == GIF_ERROR) {
				fputs("Unable to write gif extension\n", stderr);
				status = EXIT_FAILURE;
				goto FREE_MAP;
			}
			if (EGifPutExtensionTrailer(writer) == GIF_ERROR) {
				fputs("Unable to write gif extension\n", stderr);
				status = EXIT_FAILURE;
			}
			while (frame_current < (float)frame_count && status == EXIT_SUCCESS) {
				Surface surface(buffer.get(), w, h, w * lp_COLOR_BYTES);
				animation->renderSync(lround(frame_current), surface);
				auto *byte_buffer_raw = reinterpret_cast<byte *> (buffer.get());
				size_t pixel_count = w * h;
				byte *rb = (byte *)calloc(pixel_count, sizeof(byte)),
					*gb = (byte *)calloc(pixel_count, sizeof(byte)),
					*bb = (byte *)calloc(pixel_count, sizeof(byte)),
					*out = (byte *)calloc(pixel_count, sizeof(byte));
				if (rb == nullptr || gb == nullptr || bb == nullptr || out == nullptr) {
					perror("Unable to init byte buffer");
					if(rb != nullptr) free(rb);
					if(gb != nullptr) free(gb);
					if(bb != nullptr) free(bb);
					if(out != nullptr) free(out);
					status = EXIT_FAILURE;
					break;
				}
				for (size_t i = 0; i < pixel_count; ++i) {
					byte b, g, r, a;
					b = *byte_buffer_raw++;
					g = *byte_buffer_raw++;
					r = *byte_buffer_raw++;
					a = *byte_buffer_raw++;
					if (a) {
						if (a != 0xFF) {
							r += (byte) (((float) r) * (float) (0xFF - a) / 255.0f);
							g += (byte) (((float) g) * (float) (0xFF - a) / 255.0f);
							b += (byte) (((float) b) * (float) (0xFF - a) / 255.0f);
						}
					} else {
						r = g = b = 0; //background
					}
					rb[i] = r;
					gb[i] = g;
					bb[i] = b;
				}
				error_code = GifQuantizeBuffer(w, h, &output_palette->ColorCount,
				                               rb, gb, bb,
				                               out, output_palette->Colors);
				free(rb);
				free(gb);
				free(bb);
				if(error_code != GIF_OK){
					free(out);
					fputs("Unable quantize gif colors\n", stderr);
					status = EXIT_FAILURE;
					break;
				}
				byte *line_buffer = out;
				if(EGifPutExtension(writer, GRAPHICS_EXT_FUNC_CODE, 4, delay) == GIF_OK) {
					if (EGifPutImageDesc(writer, 0, 0, w, h, false, output_palette) == GIF_OK) {
						for (int line = 0; line < h; ++line) {
							if (EGifPutLine(writer, line_buffer, w) != GIF_OK) {
								fputs("Unable to write GIF line\n", stderr);
								status = EXIT_FAILURE;
								break;
							}
							line_buffer += w;
						}
					} else{
						fputs("Unable to write gif image info\n", stderr);
						status = EXIT_FAILURE;
					}
				} else {
					fputs("Unable to write gif extension\n", stderr);
					status = EXIT_FAILURE;
				}
			    free(out);
				frame_current += frame_inc;
			}
FREE_MAP:
			GifFreeMapObject(output_palette);
CLOSE_FILE:
			if (EGifCloseFile(writer, &error_code) != GIF_OK) {
				fprintf(stderr, "Unable to finalize GIF writer, error code: %d\n", error_code);
				status = EXIT_FAILURE;
			}
			//already closed by EGifCloseFile
			out_file->file_pointer = nullptr;
			break;
		}
		default:
			break;
	}
	return status;
}

int unzip(FILE *in_file, byte_buffer *out_data) {
	z_stream strm = {nullptr};
	byte in[lz_CHUNK_SIZE];
	byte out[lz_CHUNK_SIZE];
	bool first_read = true;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = in;
	strm.avail_in = 0;


	if (inflateInit2(&strm, lz_WINDOWN_BITS | lz_ENABLE_ZLIB_GZIP) < 0) {
		fputs("Unable to init zlib\n", stderr);
		return EXIT_FAILURE;
	}

	while (true) {
		size_t bytes_read;
		int zlib_status;

		bytes_read = fread(in, sizeof(byte), sizeof(in), in_file);
		if (ferror(in_file)) {
			inflateEnd(&strm);
			perror("Unable to read file data\n");
			return EXIT_FAILURE;
		}
		strm.avail_in = bytes_read;
		strm.next_in = in;
		do {
			size_t have;
			strm.avail_out = lz_CHUNK_SIZE;
			strm.next_out = out;
			zlib_status = inflate(&strm, Z_NO_FLUSH);
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
							bytes_read = fread(in, sizeof(byte), sizeof(in), in_file);
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
					inflateEnd(&strm);
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
			inflateEnd(&strm);
			break;
		}
		if (first_read) first_read = false;
	}
	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {

	uint32_t param = 1;
	uint8_t convert_to = li_OUT_PNG;
	unsigned long w = 128, h = 128;
	FILE *in_file = stdin;
	file out_file = file(stdout, nullptr);
	int argi = argc;
	switch (argc) {
		case 6:
			--argi;
			param = strtoul(argv[argi], nullptr, 0);
		case 5: {
			--argi;
			size_t len = strlen(argv[argi]);
			size_t index_of_x = strcspn(argv[argi], "x");
			if (len > 3 && index_of_x > 0 && index_of_x < len - 1) {
				char *res = (char *) calloc(len, sizeof(char));
				if (res == nullptr) {
					perror("Resolution error");
					return EXIT_FAILURE;
				}
				memset(res, '\0', len);
				strncpy(res, argv[argi], index_of_x);
				w = strtoul(argv[argi], nullptr, 10);
				++index_of_x;
				memset(res, '\0', len);
				strncpy(res, argv[argi] + index_of_x, len - index_of_x);
				h = strtoul(argv[argi], nullptr, 10);
				free(res);
				if (h == 0 || w == 0 || h > li_MAX_DIMENSION || w > li_MAX_DIMENSION) {
					fputs("Invalid resolution\n", stderr);
					return EXIT_FAILURE;
				}
			} else {
				fputs("Invalid resolution\n", stderr);
				return EXIT_FAILURE;
			}
		}
		case 4: {
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
		case 3: {
			--argi;
			size_t len = strlen(argv[argi]);
			bool write_to_file = len > 1 && argv[argi][0] != '-';
			if (convert_to == li_OUT_PNGS) {
				if (!write_to_file) {
					fputs("Unable to write image sequence to stdout, provide file prefix\n", stderr);
					return EXIT_FAILURE;
				}
				if (strlen(argv[argi]) > FILENAME_MAX) {
					fputs("File name is too long", stderr);
				}
				out_file.path = argv[argi];
				out_file.file_pointer = nullptr;
			} else {
				if (write_to_file) {
					out_file.file_pointer = fopen(argv[argi], "wb");
				}
				if (out_file.file_pointer == nullptr) {
					perror(argv[argi]);
					return EXIT_FAILURE;
				}
			}
		}
		case 2: {
			--argi;
			size_t len = strlen(argv[argi]);
			if (len > 1 && argv[argi][0] != '-') {
				in_file = fopen(argv[argi], "rb");
			}
			if (!in_file) {
				perror(argv[argi]);
				out_file.close();
				return EXIT_FAILURE;
			}
			break;
		}
		default:
			fputs("Usage: PROG input_file|- output_file|- png|pngs|gif [resolution(128x128)] [param]\n", stderr);
			return EXIT_FAILURE;
	}

	byte_buffer in_file_data = byte_buffer();
	in_file_data.buffer = (byte *) calloc(0, sizeof(byte));
	if (in_file_data.buffer == nullptr) {
		perror("Unable to init byte buffer");
		fclose(in_file);
		out_file.close();
		return EXIT_FAILURE;
	}
	int result = unzip(in_file, &in_file_data);
	fclose(in_file);
	if (result == EXIT_SUCCESS) {
		byte eos[1] = {'\0'};
		bb_append(&in_file_data, eos, 1);
		result = convert_and_write_to(in_file_data.buffer, convert_to, (int)w, (int)h, param, &out_file);
	} else
		fputs("zlib error\n", stderr);

	free(in_file_data.buffer);
	out_file.close();

	return result;
}
