#ifndef ANALYSIS_H
#define ANALYSIS_H

//	Helper functions for analysing the contents of a C64 disk image

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <raylib.h>
#include <sys/types.h>
#include "../include/debug.h"
#include "../include/arc.h"
#include "../include/disk.h"
#include "../include/nyblog.h"

#define MAX_ANALYSIS_ENTRIES 683	// Total number of sectors on a disk

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
} ANA_Status;

//	The full analysis of all collected information about a single disk sector
typedef struct {
	DSK_Position pos;
	DSK_SectorType type;			// What type of sector this is (
	ANA_Status status;				// ! DEPRECATED !
	uint8_t data[BLOCK_SIZE];		// The full block data
	
	// General info flags
	uint8_t is_free : 1;			// Is this block marked as free in the BAM
	uint8_t has_data : 1;			// Does the data for this block contain useful, non-zero bytes?
	uint8_t has_transfer_info : 1;	// Do we have valid transfer info for this sector
	uint8_t has_directory_info : 1;	// Do we have valid directory info for this sector
	uint8_t checksum_match : 1;		// Does the provided checksum match the data?
	uint8_t is_blank : 1;			// The block is non-zero, but matches a known "empty" format

	// Directory Info
	DSK_DirEntry dir_entry;			// Which directory file this sector belongs to
	int file_index;					// Which block of a file this sector holds data for
	int dir_index;					// Entry number of this block's file in the directory
									// OR (if this is a directory block) which directory index the first file has

	// Transfer Info
	uint16_t checksum;				// The checksum calculated before transfer from the C64
	uint8_t disk_err;				// Error code from the disk if available (OR'd with 0x80 to distinguish from not found)
	uint8_t parse_err;				// Error code from the nybbler transfer

	// Links
	int prev_block_index;			// Link to the previous block (if applicable)
	int next_block_index;			// Link to the next block (if applicable)
} ANA_SectorInfo;

//	Contains the results of analysing the disk;
typedef struct {
	DSK_Directory dir;
	ANA_SectorInfo sectors[MAX_ANALYSIS_ENTRIES];
} ANA_DiskInfo;

//
//	
//	Function Declarations
//	

//	Analyses a disk and stores the results in the provided DiskInfo struct
//
//	Tries to find as much information about every sector as it can
int ANA_AnalyseDisk(FILE *f_disk, FILE *f_meta, DSK_Directory dir, ANA_DiskInfo *analysis);

//	Get the full analysis entry for a given sector
//
//	Returns 0 on success
int ANA_GetInfo(ANA_DiskInfo analysis, DSK_Position pos, ANA_SectorInfo *entry);

//	Gets a constant char pointer to the name of a sector status
//	
//	*Deprecated*
const char *REC_GetStatusName(ANA_Status status);

//	Get a colour for a sector based on its state
//
//	*Deprecated*
Color REC_GetStatusColour(ANA_Status status);

//	Get a colour for a sector based on which file it belongs to
//
Color ANA_GetFileColour(DSK_Directory dir, ANA_SectorInfo entry, bool is_hovered, bool is_selected);

//	Get a constant char pointer to the name of a nyb-log parse error
//
const char *ANA_GetParseErrorName(int err_code);

//	Get a colour for a nyb-log parsing error code
//
Color ANA_GetParseErrorColour(int err_code);

//	Get a constant char pointer to the name of a DOS disk error
//
const char *ANA_GetDiskErrorName(uint8_t err_code);

//	Get a colour for a DOS disk error code
//
Color ANA_GetDiskErrorColour(uint8_t  err_code);


#endif
