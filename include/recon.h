#ifndef RECON_H
#define RECON_H

//	Helper functions for reading data from my custom disk
//	reconciliation file format

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <raylib.h>
#include "../include/debug.h"
#include "../include/arc.h"
#include "../include/disk.h"


#define REC_MAGIC_BYTES ((uint8_t *)("D64R"))


//	
//	Type Definitions
//	

typedef enum {
	RECTYPE_MISSING = 0x00,
	RECTYPE_EMPTY = 0x01,
	RECTYPE_CORRUPT = 0x10,
	RECTYPE_UNCONFIRMED = 0x11,
	RECTYPE_CONFIRMED = 0x20,
	RECTYPE_HEALTHY = 0x21,
	RECTYPE_META_NAME = 0xF0,
	RECTYPE_META_AUTHOR = 0xF1,
	RECTYPE_META_DATE = 0xF2,
	RECTYPE_META_DESCRIPTION = 0xF3,
	RECTYPE_META_ARCHIVE_DATE = 0xF4,
	RECTYPE_META_DISKFILE = 0xFF,	// Metadata entry which contains the filepath of the disk data
} REC_RecordType;

//	A single reconciliation entry in a disk-recon file
typedef struct {
	uint8_t type;
	uint8_t track_num;
	uint8_t sector_index;
	uint8_t _unused_01;
	uint16_t checksum;
	uint8_t _unused_02[10];
} REC_Entry;

//	A single metadata entry
typedef struct {
	uint8_t type;
	uint8_t flags;
	uint16_t offset;
	uint16_t length;
	uint8_t key_string[10];
} REC_MetadataEntry;

//	A full recon file's contents
typedef struct {
	char* path;
	char* disk_path;
	uint32_t offs_entries;
	uint32_t offs_data;
	uint32_t offs_strings;
} REC_File;


//	
//	Function Declarations
//	

//	Calculates the fletcher-checksum of a 256-Byte block of data
//
uint16_t REC_Checksum(void *ptr);

//	Reads a disk recon file into the provided struct
//
//	Copies the filename to an internal pointer
//
//	Returns NULL if it fails
REC_File *REC_ReadFile(char *filename);

//	Cleans up and terminates a rec_file pointer
//	
void REC_CloseFile(REC_File *rec);

//	Writes a recon struct to a file
//
//	Returns 0 on success
int REC_WriteFile(REC_File *rec);

//	Find an entry with a certain type
//
//	Returns the `index`-th record entry with type `type`
REC_Entry REC_FindEntry(REC_File *rec, REC_RecordType type, int index);


#endif
