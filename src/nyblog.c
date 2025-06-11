#include "../include/nyblog.h"

int NYB_ParseLog(const char *filename, NYB_DataBlock *block_buf, int buf_len) {
	if (filename == NULL || block_buf == NULL) return 1;
	if (buf_len < 1) return 2;

	// Read input file
	FILE *f = fopen(filename, "r");
	if (f == NULL) return 3;

	char line_buf[128];
	enum { NO_BLOCK, START_BLOCK, IN_BLOCK, END_BLOCK } parse_state = NO_BLOCK;
	int line_num = 0;
	int curr_block_index = 0;
	int data_index = 0;
	while (!feof(f)) {
		line_num++;
		fgets(line_buf, 127, f);

		char *line = &line_buf[0];

		// Trim spaces in beginning
		while (isspace(*line)) line++;
		if (*line == '\0') continue; // Ignore empty lines

		switch (parse_state) {

			case NO_BLOCK: {
				if (strncmp(line, ">>> BLOCK-START: T=", 19 * sizeof(char)) != 0) continue;

				line += 19;
				int semi = 0;
				while (line[semi] != ';' && line[semi] != '\0') semi++;
				line[semi] = '\0';
				block_buf[curr_block_index].track_num = strtol(line, NULL, 10);
				line = &line[semi+4];
				block_buf[curr_block_index].sector_index = strtol(line, NULL, 10);
				//printf("---> Started block for track %i, sector %i\n",
				//	block_buf[curr_block_index].track_num,
				//	block_buf[curr_block_index].sector_index
				//);
				parse_state = START_BLOCK;
			} continue;

			case START_BLOCK: {
				if (line[0] != '[') continue;
				data_index = 0;
				parse_state = IN_BLOCK;
			}

			case IN_BLOCK: {
				while (*line != '\n' && *line != '\0') {
					if (*line != ' ') { line++; continue; }
					line++;

					int byte = strtoul(line, NULL, 16);
					block_buf[curr_block_index].data[data_index++] = byte;

					if (data_index >= 256) {
						parse_state = END_BLOCK;
						break;
					}
				}
			} continue;

			case END_BLOCK: {
				if (strncmp(line, ">>> BLOCK-END; C=0x", 19 * sizeof(char)) != 0) continue;

				line += 19;
				int semi = 0;
				while (line[semi] != ';' && line[semi] != '\0') semi++;
				line[semi] = '\0';
				block_buf[curr_block_index].checksum = strtoul(line, NULL, 16);
				line = &line[semi+4];
				block_buf[curr_block_index].err_code = strtoul(line, NULL, 10);
				//printf("---> ended block; CHK: 0x%04X, ERR#%i\n",
				//	block_buf[curr_block_index].checksum,
				//	block_buf[curr_block_index].err_code
				//);

				curr_block_index++;
				parse_state = NO_BLOCK;
			} break;

		}

		// DEBUG:
		//printf("---> [% 5i] '%s'\n", line_num, line);
		//if (line_num > 20) return 0;

	}

	return 0;
}

int NYB_Meta_ReadBlock(FILE *f_meta, NYB_DataBlock *block) {
	if (f_meta == NULL || block == NULL) return 1;

	//	Parse File Header
	fseek(f_meta, 0l, SEEK_SET);
	uint32_t header[4];
	size_t r = fread(header, sizeof(uint32_t), 4, f_meta);
	if (r < sizeof(uint32_t) * 4) return 2;
	if (header[0] != NYBLOG_BIN_MAGIC) return 2;

	//long offs_data = header[1];

	//	Read blocks until we find 
	return 0;
}

int NYB_Meta_WriteBlock(FILE *f_meta, NYB_DataBlock *block) {
	return 99;
}

int NYB_WriteToDiskImage(char *filename, NYB_DataBlock *block_buf, int buf_len) {
	FILE * f_disk = fopen(filename, "ab");
	if (f_disk == NULL) return 1;

	for (int i=0; i<buf_len; i++) {
		NYB_DataBlock block = block_buf[i];
		if (block.track_num < 1 || block.track_num > 35) continue;

		//printf("[% 5i] Writing Block (% 3i/% 3i); 0x%04X; Err#%i\n", i,
		//	block.track_num, block.sector_index,
		//	block.checksum, block.err_code
		//);
		//uint16_t chk = REC_Checksum(block.data);
		//printf("       Recalculated checksum: 0x%04X [%s]\n", chk, (chk == block.checksum) ? "MATCH" : "FAIL");

		DSK_File_SeekPosition(f_disk, (DSK_Position){ block.track_num, block.sector_index });
		fwrite(block.data, sizeof(uint8_t), BLOCK_SIZE, f_disk);
	}
	DSK_File_SeekPosition(f_disk, (DSK_Position){ MAX_TRACKS, 17 });

	fclose(f_disk);

	return 0;
}

