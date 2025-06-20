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
#include "../include/nyblog.h"

#define MAX_ANALYSIS_ENTRIES 1024

//	
//	Type Definitions
//	

typedef enum {
	SECSTAT_INVALID = 0x00,
	SECSTAT_EMPTY = 0x10,
	SECSTAT_UNEXPECTED = 0x11,
	SECSTAT_MISSING = 0x20,
	SECSTAT_PRESENT = 0x21,
	SECSTAT_CORRUPTED = 0x30,	// Checksum mismatch
	SECSTAT_CONFIRMED = 0x31,	// Checksum matches
	SECSTAT_BAD = 0x40,
	SECSTAT_GOOD = 0x41,
	SECSTAT_UNKNOWN = 0xFF,
} REC_Status;

//	A single reconciliation entry in a disk-recon file
typedef struct {
	int file_index;			// Which sector of the file this one is
	DSK_DirEntry dir_entry;	// Which directory file this sector belongs to (if any; undefined if type is not a file type)
	int dir_index;			// Entry number of this block's file in the directory
	DSK_SectorType type;	// What type of sector this is (
	REC_Status status;
	uint16_t checksum;
	uint8_t disk_err;		// Error code from the disk if available (OR'd with 0x80 to distinguish from not found)
} REC_Entry;

//	Contains the results of analysing the disk;
//	A REC_Entry for each sector
typedef struct {
	REC_Entry entries[MAX_ANALYSIS_ENTRIES];
} REC_Analysis;

//
//	
//	Function Declarations
//	

//	Checks if a sector has data (non-zero bytes)
//
//	Returns false if `f_disk` is NULL
bool REC_Sector_HasData(FILE *f_disk, DSK_Position pos);

//	Analyses a disk and stores the results in a REC_Analysis struct
//
int REC_AnalyseDisk(FILE *f_disk, FILE *f_meta, DSK_Directory dir, REC_Analysis *analysis);

//	Get the full recon analysis entry for a given sector
//
//	Returns 0 on success
int REC_GetInfo(REC_Analysis analysis, DSK_Position pos, REC_Entry *entry);

//	Gets a constant char pointer to the name of a sector status
//	
const char *REC_GetStatusName(REC_Status status);

//	Get a colour for a sector based on its state
//
Color REC_GetStatusColour(REC_Status status);

//	Get a colour for a sector based on which file it belongs to
//
Color REC_GetFileColour(DSK_Directory dir, REC_Entry entry, bool is_hovered, bool is_selected);

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
