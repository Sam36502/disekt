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
	SECSTAT_INVALID = 0x00,
	SECSTAT_EMPTY = 0x10,
	SECSTAT_UNEXPECTED = 0x11,
	SECSTAT_MISSING = 0x20,
	SECSTAT_PRESENT = 0x21,
	SECSTAT_CORRUPTED = 0x30,
	SECSTAT_CONFIRMED = 0x31,
	SECSTAT_BAD = 0x40,
	SECSTAT_GOOD = 0x41,
	SECSTAT_UNKNOWN = 0xFF,
} REC_Status;

//	A single reconciliation entry in a disk-recon file
typedef struct {
	DSK_SectorType type;
	REC_Status status;
	uint16_t checksum;
} REC_Entry;

//	Contains the results of analysing the disk;
//	A REC_Entry for each sector
typedef struct {
	REC_Entry entries[1024];
} REC_Analysis;

//
//	
//	Function Declarations
//	

//	Calculates the fletcher-checksum of a 256-Byte block of data
//
uint16_t REC_Checksum(void *ptr);

//	Checks if a sector has data (non-zero bytes)
//
//	Returns false if `f_disk` is NULL
bool REC_Sector_HasData(FILE *f_disk, DSK_Position pos);

//	Analyses a disk and stores the results in a REC_Analysis struct
//
int REC_AnalyseDisk(FILE *f_disk, DSK_Directory dir, REC_Analysis *analysis);

//	Get the state of a sector according to the Directory
//
//	Returns SECTOR_INVALID if pos is invalid
REC_Status REC_GetStatus(REC_Analysis analysis, DSK_Position pos);

//	Gets a constant char pointer to the name of a sector status
//	
const char *REC_GetStatusName(REC_Status status);

//	Get a colour for a sector based on its state
//
Color REC_GetStatusColour(REC_Status status);

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
