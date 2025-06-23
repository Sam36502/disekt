#include "../include/nyblog.h"
#include <raylib.h>

int NYB_ParseLogLine(char *line, NYB_LogLineType *type, char data[26][32]) {
	if (line == NULL) return 1;

	// Reset data strings
	for (char c='A'; c<='Z'; c++) {
		data[c - 'A'][0] = '\0';
	}

	// Get text within >>> angle brackets <<<
	char *start = strstr(line, ">>>");
	if (start == NULL) return 3;
	start += 3;

	char *end = strstr(start, "<<<");
	if (end == NULL) return 3;
	end -= 1;

	char str[128];
	int len = (end-start);
	strncpy(str, start,  128 * sizeof(char));
	str[len] = '\0';

	char *tok = strtok(str, " :;");
	if (tok == NULL) return 4;

	*type = NYBLOG_INVALID;
	if (strncmp(tok, "INFO", 128 * sizeof(char)) == 0) *type = NYBLOG_INFO;
	else if (strncmp(tok, "BLOCK-START", 128 * sizeof(char)) == 0) *type = NYBLOG_BLOCK_START;
	else if (strncmp(tok, "BLOCK-END", 128 * sizeof(char)) == 0) *type = NYBLOG_BLOCK_END;
	else if (strncmp(tok, "WARNING", 128 * sizeof(char)) == 0) *type = NYBLOG_WARNING;
	else if (strncmp(tok, "ERROR", 128 * sizeof(char)) == 0) *type = NYBLOG_ERROR;

	tok = strtok(NULL, "=;");
	while (tok != NULL) {
		while(isspace(tok[0])) tok++;

		char key = tok[0];
		if (key < 'A' || key > 'Z') continue;

		tok = strtok(NULL, "=;");
		if (tok == NULL) continue;
		while(isspace(tok[0])) tok++;
		int len = strlen(tok);
		strncpy(data[key - 'A'], tok, 32 * sizeof(char));
		data[key - 'A'][len] = '\0';

		tok = strtok(NULL, "=;");
	}

	return 0;
}

int NYB_ParseDataLine(char *line, uint8_t *offset, uint8_t data[16]) {
	if (line == NULL) return 1;

	char str[128];
	int len = strlen(line);
	strncpy(str, line,  128 * sizeof(char));
	str[len] = '\0';

	char *tok = strtok(str, " ");
	if (tok == NULL) return 2;
	if (strlen(tok) != 6 || tok[0] != '[' || tok[5] != ']') return 2;
	*offset = strtol(tok+1, NULL, 16);

	int i=0;
	tok = strtok(NULL, " ");
	while (tok != NULL) {
		data[i] = strtol(tok, NULL, 16);
		tok = strtok(NULL, " ");
		i++;
	}

	return 0;
}

int NYB_ParseLogBlock(FILE *f_log, NYB_DataBlock *block) {
	if (f_log == NULL || block == NULL) return 1;
	block->parse_error = 0x00;

	char line[128];
	bool in_block = false;
	char *lp = fgets(line, 128, f_log);
	while (lp != NULL) {

		// Try parsing as log-line
		NYB_LogLineType type;

		char data[26][32];
		int err = NYB_ParseLogLine(line, &type, data);
		if (err == 0 && type != NYBLOG_INVALID) {
			switch (type) {
				case NYBLOG_INFO: {
					if (g_verbose_log) printf(" - Info Message in log file: %s\n", data['M' - 'A']);
				}; break;

				case NYBLOG_BLOCK_START: {
					block->track_num = strtol(data['T' - 'A'], NULL, 10);
					block->sector_index = strtol(data['S' - 'A'], NULL, 10);
					in_block = true;
				}; break;

				case NYBLOG_BLOCK_END: {
					block->err_code = strtol(data['E' - 'A'], NULL, 10);
					block->checksum = strtol(data['C' - 'A'], NULL, 16);
					return 0;
				}; break;

				case NYBLOG_WARNING: {
					block->parse_error = strtol(data['E' - 'A'], NULL, 10);
					return 0;
				}; break;

				case NYBLOG_ERROR: {
					block->parse_error = strtol(data['E' - 'A'], NULL, 10);
					return 0;
				}; break;

				case NYBLOG_INVALID: { printf("IMPOSSIBLE STATE ACHIEVED! CONGLATURATIONS YOU WIN!!\n"); };
			}
		}

		// Try parsing data-line
		if (in_block) {
			uint8_t offset = 0x00;
			uint8_t data[16];
			err = NYB_ParseDataLine(line, &offset, data);
			if (err == 0) {
				memcpy(block->data + offset, data, 16);
			}
		}

		lp = fgets(line, 128, f_log);
	}

	return 0;
}

int NYB_ParseLog(const char *filename, NYB_DataBlock *block_buf, int buf_len, long *data_offset) {
	if (filename == NULL || block_buf == NULL) return -1;
	if (buf_len < 1) return -1;

	FILE *f_log = fopen(filename, "r+b");
	if (f_log == NULL) return -1;

	if (data_offset != NULL) fseek(f_log, *data_offset, SEEK_SET);
	fread(NULL, 0, 0, f_log);
	if (feof(f_log)) return 0;

	int count = 0;
	for (int i=0; i<buf_len; i++) {
		int err = NYB_ParseLogBlock(f_log, &block_buf[i]);
		if (err != 0) return count;
		if (feof(f_log)) return count;
		count++;
	}

	if (data_offset != NULL) *data_offset = ftell(f_log);

	fclose(f_log);
	return count;
}

int NYB_Meta_ReadBlock(FILE *f_meta, NYB_DataBlock *block) {
	if (f_meta == NULL || block == NULL) return 1;

	//	Parse File Header
	fseek(f_meta, 0l, SEEK_SET);
	uint32_t header[4];
	size_t r = fread(header, sizeof(uint32_t), 4, f_meta);
	if (r < 4) return 2;
	if (header[0] != NYBLOG_BIN_MAGIC) return 2;

	long offs_data = header[1];

	//	Find and read selected block
	DSK_Position blockpos = { block->track_num, block->sector_index };
	int block_index = DSK_PositionToIndex(blockpos);
	if (block_index < 0) {
		if (g_verbose_log) printf("Error: Failed to read sector metadata from file; Invalid position [% i/% i]\n", blockpos.track, blockpos.sector);
		return 2;
	}

	fseek(f_meta, offs_data + (block_index * sizeof(NYB_DataBlock)), SEEK_SET);
	int read = fread(block, sizeof(NYB_DataBlock), 1, f_meta);
	if (read != 1) {
		if (g_verbose_log) printf("Error: Failed to read sector metadata from file; Past end of file; %i\n", read);
		return 3;
	}

	return 0;
}

int NYB_Meta_WriteBlock(FILE *f_meta, NYB_DataBlock *block) {
	if (f_meta == NULL || block == NULL) return 1;

	//	Parse File Header
	uint32_t header[4];
	fseek(f_meta, 0l, SEEK_SET);
	size_t r = fread(header, sizeof(uint32_t), 4, f_meta);
	if (r < 4 || header[0] != NYBLOG_BIN_MAGIC) {
		if (g_verbose_log) printf("Warn: No header in metadata file; creating one...\n");

		// Create Header
		header[0] = NYBLOG_BIN_MAGIC;
		header[1] = 16 * 4;	// Leave a few lines blank just in case
		header[2] = 0x00000000; // Unused for now
		header[3] = 0x00000000;
		fseek(f_meta, 0l, SEEK_SET);
		fwrite(header, sizeof(uint32_t), 4, f_meta);
	}

	long offs_data = header[1];

	//	Find and read selected block
	DSK_Position blockpos = { block->track_num, block->sector_index };
	int block_index = DSK_PositionToIndex(blockpos);
	if (block_index < 0) {
		printf("Error: Failed to write block metadata to file; Invalid position [% i/% i]\n", blockpos.track, blockpos.sector);
		return 2;
	}

	// Make sure type byte is nonzero
	if (block->block_status == 0x00) block->block_status = 0x01;
	
	fseek(f_meta, offs_data + (block_index * sizeof(NYB_DataBlock)), SEEK_SET);
	fwrite(block, sizeof(NYB_DataBlock), 1, f_meta);

	return 0;
}

int NYB_WriteToDiskImage(char *filename, NYB_DataBlock *block_buf, int buf_len) {
	if (block_buf == NULL) return 1;

	FILE * f_disk;
	if (FileExists(filename)) f_disk = fopen(filename, "r+b");
	else f_disk = fopen(filename, "w+b");
	 
	if (f_disk == NULL) return 2;

	if (g_verbose_log) printf("\nWriting parsed data to '%s'...\n", filename);
	for (int i=0; i<buf_len; i++) {
		NYB_DataBlock block = block_buf[i];
		uint16_t chk = DSK_Checksum(block.data);

		if (g_verbose_log) {
			printf("  [% 3i/% 3i] ", block.track_num, block.sector_index);
			if (block.err_code != 0) {
				printf("Skipping due to disk-read error (code %i)\n", block.err_code);
				continue;
			}
			if (block.checksum != chk) {
				printf("Skipping due to checksum mismatch (0x%04X =/= 0x%04X)\n", block.checksum, chk);
				continue;
			}

			printf("Written to disk!\n");
		}

		DSK_File_SeekPosition(f_disk, (DSK_Position){ block.track_num, block.sector_index });
		fwrite(block.data, sizeof(uint8_t), BLOCK_SIZE, f_disk);
	}

	// Seek to end of disk to create blank sectors
	DSK_File_SeekPosition(f_disk, (DSK_Position){ MAX_TRACKS, 16 });
	fseek(f_disk, 256, SEEK_CUR);

	uint8_t lastbyte;
	fread(&lastbyte, sizeof(uint8_t), 1, f_disk);
	fseek(f_disk, -1, SEEK_CUR);
	fwrite(&lastbyte, sizeof(uint8_t), 1, f_disk);

	fclose(f_disk);

	return 0;
}
