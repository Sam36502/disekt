#ifndef DISK_H
#define DISK_H

//	Helper functions for reading data from a C64 disk image
//	Also reads my custom reconciliation format that includes metadata for each block

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <raylib.h>

#include "../include/debug.h"
#include "../include/arc.h"


#define MIN_TRACKS 1		// Track numbers range from 1 - 35
#define MAX_TRACKS 35
#define SECTOR_SIZE 0x100	// A single disk sector is 256 Bytes
#define TRACK_GAPS 2		// How many pixels between each track
#define SECTOR_GAPS 8.0f	// How much of a gap to leave between each sector
#define SPINDLE_RADIUS 50	// in px
#define DISK_RADIUS 450		// in px
#define DISK_CENTRE_X 500	// Disk centre pos
#define DISK_CENTRE_Y 500


//	
//	Struct Declarations
//	

typedef struct {
	uint8_t track;
	uint8_t sector;
} DSK_Position;

typedef struct {
	DSK_Position directory;
	uint32_t entries[MAX_TRACKS];
} DSK_BAM;
	
typedef enum {
	SECTOR_INVALID,
	SECTOR_FREE,
	SECTOR_IN_USE,
} DSK_SectorStatus;

#define DSK_POSITION_BAM (DSK_Position){ 0x12, 0x00 }

//	
//	Function Declarations
//	

//	Gets the number of sectors in a track
//
//	Returns 0 for invalid track numbers (t<1 or t>35)
int DSK_Track_GetSectorCount(int track_num);

//	Checks if a position is valid on a disk
//
bool DSK_IsPositionValid(DSK_Position pos);

//	Seeks to the start of a given sector in a disk image file pointer
//
void DSK_File_SeekPosition(FILE *f_disk, DSK_Position pos);

//	Takes a disk file pointer and parses out the BAM
//
//	This function fseeks to the BAM
//	Returns 0 on success, otherwise:
//		1 = Received NULL argument pointer
int DSK_File_ParseBAM(FILE *f_disk, DSK_BAM *bam);

//	Prints out the contents of the BAM
//
void DSK_PrintBAM(DSK_BAM bam);

//	Draws a sector to the screen with its colour determined by its status
//
void DSK_Sector_Draw(DSK_BAM bam, DSK_Position pos);

//	Draws bright lines around the outline of a sector
//
void DSK_Sector_Highlight(DSK_BAM bam, DSK_Position pos);

//	Get the state of a sector according to the BAM
//
//	Returns SECTOR_INVALID if pos is invalid
DSK_SectorStatus DSK_Sector_GetStatus(DSK_BAM bam, DSK_Position pos);

//	Gets a constant char pointer to the name of a sector status
//	
const char *DSK_Sector_GetStatusName(DSK_SectorStatus status);

//	Get a colour for a sector based on its state
//
Color DSK_Sector_GetStatusColour(DSK_SectorStatus status);

//	Gets the disk position of the sector the mouse is hovering over
//
DSK_Position DSK_GetHoveredSector();

#endif
