//
//  icedraw.c
//  AnsiLove/C
//
//  Copyright (c) 2011-2018 Stefan Vogt, Brian Cassidy, and Frederic Cambus.
//  All rights reserved.
//
//  This source code is licensed under the BSD 2-Clause License.
//  See the LICENSE file for details.
//

#include "../ansilove.h"

int ansilove_icedraw(struct ansilove_ctx *ctx, struct ansilove_options *options)
{
	// extract relevant part of the IDF header, 16-bit endian unsigned short
	int32_t x2 = (ctx->buffer[9] << 8) + ctx->buffer[8];

	// libgd image pointers
	gdImagePtr canvas;

	uint32_t loop = 12;
	int32_t index;
	int32_t colors[16];

	// process IDF
	uint32_t idf_sequence_length, idf_sequence_loop, i = 0;

	// dynamically allocated memory buffer for IDF data
	unsigned char *idf_buffer, *temp;
	idf_buffer = malloc(2);

	int16_t idf_data, idf_data_length;

	while (loop < ctx->length - 4096 - 48) {
		memcpy(&idf_data, ctx->buffer+loop, 2);

		// RLE compressed data
		if (idf_data == 1) {
			memcpy(&idf_data_length, ctx->buffer+loop+2, 2);

			idf_sequence_length = idf_data_length & 255;

			for (idf_sequence_loop = 0; idf_sequence_loop < idf_sequence_length; idf_sequence_loop++)
			{
				// reallocate IDF buffer memory
				temp = realloc(idf_buffer, i + 2);
				if (idf_buffer != NULL) {
					idf_buffer = temp;
				} else {
					ctx->error = MEMORY_ERROR;
					return -1;
				}

				idf_buffer[i] = ctx->buffer[loop + 4];
				idf_buffer[i+1] = ctx->buffer[loop + 5];
				i += 2;
			}
			loop += 4;
		} else {
			// reallocate IDF buffer memory
			temp = realloc(idf_buffer, i + 2);
			if (idf_buffer != NULL) {
				idf_buffer = temp;
			} else {
				ctx->error = MEMORY_ERROR;
				return -1;
			}

			// normal character
			idf_buffer[i] = ctx->buffer[loop];
			idf_buffer[i+1] = ctx->buffer[loop + 1];
			i += 2;
		}
		loop += 2;
	}

	// create IDF instance
	canvas = gdImageCreate((x2 + 1) * 8, i / 2 / 80 * 16);

	// error output
	if (!canvas) {
		ctx->error = GD_ERROR;
		return -1;
	}
	gdImageColorAllocate(canvas, 0, 0, 0);

	// process IDF palette
	for (loop = 0; loop < 16; loop++) {
		index = (loop * 3) + ctx->length - 48;
		colors[loop] = gdImageColorAllocate(canvas, (ctx->buffer[index] << 2 | ctx->buffer[index] >> 4),
		    (ctx->buffer[index + 1] << 2 | ctx->buffer[index + 1] >> 4),
		    (ctx->buffer[index + 2] << 2 | ctx->buffer[index + 2] >> 4));
	}

	// render IDF
	int32_t column = 0, row = 0;
	int32_t character, attribute, foreground, background;

	for (loop = 0; loop < i; loop += 2) {
		if (column == x2 + 1) {
			column = 0;
			row++;
		}

		character = idf_buffer[loop];
		attribute = idf_buffer[loop+1];

		background = (attribute & 240) >> 4;
		foreground = attribute & 15;

		drawchar(canvas, ctx->buffer+(ctx->length - 48 - 4096), 8, 16, column, row, colors[background], colors[foreground], character);

		column++;
	}

	// create output file
	output(ctx, canvas, options->retinaScaleFactor);

	// free memory
	free(idf_buffer);

	return 0;
}
