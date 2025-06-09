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
