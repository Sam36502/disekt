#ifndef NYBLOG_H
#define NYBLOG_H

//	Helper functions for reading data from the text-log my
//	arduino program creates, when interpreting data received from the commodore

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "../include/debug.h"
#include "../include/disk.h"


#define NYBLOG_BIN_MAGIC (uint32_t)(*(uint32_t *)"NYBB")


//	
//	Struct Declarations
//	

typedef enum {
	NYBLOG_INVALID,
	NYBLOG_INFO,
	NYBLOG_BLOCK_START,
	NYBLOG_BLOCK_END,
} NYB_LogLineType;

typedef struct {
	uint8_t block_status;	// Determines whether this block is readable in the file or not
	uint8_t track_num;
	uint8_t sector_index;
	uint8_t err_code;
	uint16_t checksum;
	uint8_t __padding1[10];
	uint8_t data[BLOCK_SIZE];
} NYB_DataBlock;


//	
//	Function Declarations
//	

//	Parses a single line of a file containing a NybLog info entry
//
//	Intended to be used internally
int NYB_ParseLogLine(char *line, NYB_LogLineType *type, char data[26][32]);

//	Parses a single line of hex data from a NybLog file
//
//	Intended to be used internally
int NYB_ParseDataLine(char *line, uint8_t *offset, uint8_t data[16]);

//	Reads text from a file pointer until it's read a complete block
//
//	Intended to be used internally
int NYB_ParseLogBlock(FILE *f_log, NYB_DataBlock *block);

//	Function to open and parse a transmission log
//
//	`data_offset` is a pointer to a file offset index
//	if it's not NULL, the file will first go to the provided offset before beginning to read
//	and once it's filled the provided buffer, it'll write the current file position to it.
//	This lets you easily pick up the parsing of a file part of the way through,
//	so you don't have to parse large logs all at once.
//
//	Returns the number of blocks successfully read or -1 if it fails
int NYB_ParseLog(const char *filename, NYB_DataBlock *block_buf, int buf_len, long *data_offset);

//	Function to read a binary block from a meta-disk file
//
//	Returns 0 on success
int NYB_Meta_ReadBlock(FILE *f_meta, NYB_DataBlock *block);

//	Function to write a block to an output meta-disk file
//
//	Returns 0 on success
int NYB_Meta_WriteBlock(FILE *f_meta, NYB_DataBlock *block);

//	Function to write disk data to a `.d64` disk image
//
//	WARNING! Overwrites the provided block location
//
//	Returns 0 on success
//		1 = Input was NULL
int NYB_WriteToDiskImage(char *filename, NYB_DataBlock *block_buf, int buf_len);


#endif
