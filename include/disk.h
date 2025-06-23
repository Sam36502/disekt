#ifndef DISK_H
#define DISK_H

//	Helper functions for reading data from a C64 disk image

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <raylib.h>

#include "../include/debug.h"
#include "../include/arc.h"


#define MIN_TRACKS 1		// Track numbers range from 1 - 35
#define MAX_TRACKS 35
#define BLOCK_SIZE 0x100	// A single disk sector is 256 Bytes
#define TRACK_GAPS 2		// How many pixels between each track
#define SECTOR_GAPS 8.0f	// How much of a gap to leave between each sector
#define SPINDLE_RADIUS 50	// in px
#define DISK_RADIUS 450		// in px
#define DISK_CENTRE_X 500	// Disk centre pos
#define DISK_CENTRE_Y 500
#define DIR_HEADER_SIZE 113	// From 144 to 256 plus null-terminator
#define MAX_DIR_ENTRIES 144	// 18 directory sectors; each with 8 entries; rounded up


//	
//	Type Definitions
//	

typedef enum {
	DSK_DRAW_NORMAL,
	DSK_DRAW_HIGHLIGHT,
	DSK_DRAW_SELECTED,
} DSK_DrawMode;

typedef struct {
	uint8_t track;
	uint8_t sector;
} DSK_Position;

typedef struct {
	uint32_t entries[MAX_TRACKS];
} DSK_BAM;

typedef struct {
	uint8_t is_corpse : 1;	// Was this file closed correctly last time?
	uint8_t is_meta : 1;	// Is this my custom type or a 1541 file type?
	uint8_t __flg_pad : 2;
	uint8_t type : 4;		// Type number

	DSK_Position head_pos;	// Position of the first block of this file
	char filename[17];		// Name of the file
	uint16_t block_count;	// Length of the file in blocks
} DSK_DirEntry;

typedef struct {
	DSK_BAM bam;
	char header[DIR_HEADER_SIZE];
	DSK_DirEntry entries[MAX_DIR_ENTRIES];
	int num_entries;
} DSK_Directory;

typedef enum {
	SECTYPE_DEL = 0x00,
	SECTYPE_SEQ = 0x01,
	SECTYPE_PRG = 0x02,
	SECTYPE_USR = 0x03,
	SECTYPE_REL = 0x04,
	SECTYPE_EMPTY = 0x40,
	SECTYPE_BAM = 0x41,
	SECTYPE_DIR = 0x42,
	SECTYPE_UNKNOWN = 0x43,
	SECTYPE_INVALID = 0x4F,
} DSK_SectorType;

#define DSK_POSITION_BAM (DSK_Position){ 0x12, 0x00 }


//	
//	Function Declarations
//	

//	---- Sector Utilities

//	Calculates the fletcher-checksum of a 256-Byte block of data
//
uint16_t DSK_Checksum(void *ptr);

//	Gets the number of sectors in a track
//
//	Returns 0 for invalid track numbers (t<1 or t>35)
int DSK_Track_GetSectorCount(int track_num);

//	Checks if a position is valid on a disk
//
bool DSK_IsPositionValid(DSK_Position pos);

//	Converts a disk position to an index of that position in blocks
//
//	Returns -1 if the position is invalid
int DSK_PositionToIndex(DSK_Position pos);

//	Seeks to the start of a given sector in a disk image file pointer
//
//	Returns 0 on success or
//		1 - if f_disk is NULL
//		2 - if pos is invalid
int DSK_File_SeekPosition(FILE *f_disk, DSK_Position pos);

//	Gets the disk position of the sector the mouse is hovering over
//
DSK_Position DSK_GetHoveredSector();

//	---- Retrieving Data

//	Fetches an arbitrary block of data from a given sector
//
int DSK_File_GetData(FILE *f_disk, DSK_Position pos, void *buf, size_t bufsz);

//	Takes a disk file pointer and parses the Directory
//
//	This function fseeks to track 18 and will leave the file pointer
//	wherever the end of the last directory block is
//
//	Returns 0 on success, otherwise:
//		1 = Received NULL argument pointer
int DSK_File_ParseDirectory(FILE *f_disk, DSK_Directory *dir);

//	---- Debug Printing

//	Prints out the contents of the BAM
//
void DSK_PrintBAM(DSK_BAM bam);

//	Prints out the contents of the Directory
//
void DSK_PrintDirectory(DSK_Directory dir);

//	---- Getting Strings & Colours

//	Gets the full directory header text
//
// Internal buffer; do not `free` !
// Replaces "shifted" spaces and trims leading/trailing spaces
char *DSK_GetDescription(DSK_Directory dir);

//	Gets a disk name from the directory header
//
// Internal buffer; do not `free` !
// Limits the name to 17 chars trims and replaces shifted spaces
char *DSK_GetName(DSK_Directory dir);

//	Get a constant string for the name of a sector-type
//
const char *DSK_Sector_GetTypeName(DSK_SectorType type);

//	Get a colour for a sector based on its type
//
Color DSK_Sector_GetTypeColour(DSK_SectorType type);

//	---- Drawing Functions

//	Draws a sector to the screen
//
void DSK_Sector_Draw(DSK_Directory dir, DSK_Position pos, DSK_DrawMode mode, Color clr);

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
void DSK_DrawData(int x, int y, void *buf, size_t bufsz, bool hex_mode, bool show_offset);


#endif
