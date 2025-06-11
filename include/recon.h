#ifndef RECON_H
#define RECON_H

//	Helper functions for reconciling the contents of a C64 disk image

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <raylib.h>
#include "../include/debug.h"
#include "../include/arc.h"
#include "../include/disk.h"

//	
//	Type Definitions
//	

typedef enum {
	SECSTAT_MISSING = 0x00,
	SECSTAT_EMPTY = 0x01,
	SECSTAT_CORRUPT = 0x10,
	SECSTAT_UNCONFIRMED = 0x11,
	SECSTAT_CONFIRMED = 0x20,
	SECSTAT_HEALTHY = 0x21,
} REC_Status;

//	A single reconciliation entry in a disk-recon file
typedef struct {
	uint8_t type;
	uint8_t track_num;
	uint8_t sector_index;
	uint8_t _unused_01;
	uint16_t checksum;
	uint8_t _unused_02[10];
} REC_Entry;

//
//	
//	Function Declarations
//	

//	Calculates the fletcher-checksum of a 256-Byte block of data
//
uint16_t REC_Checksum(void *ptr);

//	Draw a block of sector-data to the screen in fixed-width ASCII columns
//
//	The argument `hex_mode` determines whether to print the hexadecimal values or their ASCII characters
//	The last argument `show_offset` determines whether to include the byte count
//	before each row:
//
//		Off:				On:
//		| A B C D |			0x00 | A B C D |	
//		| E F G H |			0x04 | E F G H |	
//		| I J K L |			0x08 | I J K L |	
//
void REC_DrawData(int x, int y, void *buf, size_t bufsz, bool hex_mode, bool show_offset);

#endif
