//
//  pcboard.c
//  AnsiLove/C
//
//  Copyright (c) 2011-2018 Stefan Vogt, Brian Cassidy, and Frederic Cambus.
//  All rights reserved.
//
//  This source code is licensed under the BSD 2-Clause License.
//  See the LICENSE file for details.
//

#include "../ansilove.h"

// Character structure
struct pcbChar {
	int32_t column;
	int32_t row;
	int32_t background;
	int32_t foreground;
	int32_t current_character;
};

int ansilove_pcboard(struct ansilove_ctx *ctx, struct ansilove_options *options)
{
	// some type declarations
	struct fontStruct fontData;
	uint32_t loop, structIndex;

	// font selection
	alSelectFont(&fontData, options->font);

	// libgd image pointers
	gdImagePtr canvas;

	// process PCBoard
	uint32_t character, current_character, next_character;
	uint32_t background = 0, foreground = 7;
	uint32_t column = 0, row = 0, columnMax = 0, rowMax = 0;

	// PCB buffer structure array definition
	struct pcbChar *pcboard_buffer, *temp;

	// PCB buffer dynamic memory allocation
	pcboard_buffer = malloc(sizeof (struct pcbChar));

	// reset loop
	loop = 0;
	structIndex = 0;

	while (loop < ctx->length) {
		current_character = ctx->buffer[loop];
		next_character = ctx->buffer[loop+1];

		if (column == 80) {
			row++;
			column = 0;
		}

		// CR + LF
		if (current_character == 13 && next_character == 10) {
			row++;
			column = 0;
			loop++;
		}

		// LF
		if (current_character == 10) {
			row++;
			column = 0;
		}

		// Tab
		if (current_character == 9)
			column += 8;

		// Sub
		if (current_character == 26)
			break;

		// PCB sequence
		if (current_character == 64 && next_character == 88) {
			// set graphics rendition
			background = ctx->buffer[loop+2];
			foreground = ctx->buffer[loop+3];
			loop += 3;
		}
		else if (current_character == 64 && next_character == 67 &&
		    ctx->buffer[loop+2] == 'L' && ctx->buffer[loop+3] == 'S') {
			// erase display
			column = 0;
			row = 0;

			columnMax = 0;
			rowMax = 0;

			loop += 4;
		} else if (current_character == 64 && next_character == 80 && ctx->buffer[loop+2] == 'O'
		    && ctx->buffer[loop+3] == 'S' && ctx->buffer[loop+4] == ':') {
			// cursor position
			if (ctx->buffer[loop+6] == '@')
			{
				column = ((ctx->buffer[loop+5])-48)-1;
				loop += 5;
			}
			else
			{
				column = (10 * ((ctx->buffer[loop+5])-48) + (ctx->buffer[loop+6])-48)-1;
				loop += 6;
			}
		} else if (current_character != 10 && current_character != 13 && current_character != 9) {
			// record number of columns and lines used
			if (column > columnMax)
				columnMax = column;

			if (row > rowMax)
				rowMax = row;

			// reallocate structure array memory
			temp = realloc(pcboard_buffer, (structIndex + 1) * sizeof (struct pcbChar));
			pcboard_buffer = temp;

			// write current character in pcbChar structure
			pcboard_buffer[structIndex].column = column;
			pcboard_buffer[structIndex].row = row;
			pcboard_buffer[structIndex].background = pcb_colors[background];
			pcboard_buffer[structIndex].foreground = pcb_colors[foreground];
			pcboard_buffer[structIndex].current_character = current_character;

			column++;
			structIndex++;
		}
		loop++;
	}
	columnMax++;
	rowMax++;

	// allocate buffer image memory
	canvas = gdImageCreate(80 * options->bits, (rowMax)*fontData.height);

	if (!canvas) {
		ctx->error = GD_ERROR;
		return -1;
	}

	// allocate black color and create background canvas
	gdImageColorAllocate(canvas, 0, 0, 0);
	gdImageFill(canvas, 0, 0, 0);

	// allocate color palette
	int32_t colors[16];

	for (int i = 0; i < 16; i++) {
		colors[i] = gdImageColorAllocate(canvas, ansi_palette[i*3],
		    ansi_palette[i*3+1],
		    ansi_palette[i*3+2]);
	}

	// the last value of loop tells us how many items are stored in there
	uint32_t pcbBufferItems = structIndex;

	// render PCB
	for (loop = 0; loop < pcbBufferItems; loop++) {
		// grab our chars out of the structure
		column = pcboard_buffer[loop].column;
		row = pcboard_buffer[loop].row;
		background = pcboard_buffer[loop].background;
		foreground = pcboard_buffer[loop].foreground;
		character = pcboard_buffer[loop].current_character;

		drawchar(canvas, fontData.font_data, options->bits, fontData.height,
		    column, row, colors[background], colors[foreground], character);
	}

	// create output image
	output(ctx, canvas, options->retinaScaleFactor);

	// free memory
	free(pcboard_buffer);

	return 0;
}
