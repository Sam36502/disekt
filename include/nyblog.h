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
#include "../include/arc.h"


//	
//	Struct Declarations
//	

typedef struct {
	uint8_t track_num;
	uint8_t sector_index;
	uint8_t err_code;
	uint16_t checksum;
	uint8_t data[256];
} NYB_DataBlock;


//	
//	Function Declarations
//	


//	Function to open and parse a transmission log
//
//	Returns 0 on success, otherwise:
//		1 = input filename, or buffer is null;
//		2 = buf_len is less than 1;
//		3 = input filename couldn't be read;
//		-1 = succeeded, but found more blocks than could fit in the provided buffer
int NYB_ParseLog(const char *filename, NYB_DataBlock *block_buf, int buf_len);

#endif
