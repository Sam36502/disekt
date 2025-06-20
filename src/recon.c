#include "../include/recon.h"
#include <math.h>


bool REC_Sector_HasData(FILE *f_disk, DSK_Position pos) {
	int err = DSK_File_SeekPosition(f_disk, pos);
	if (err != 0) return false;

	for (int i=0; i<BLOCK_SIZE; i++) {
		uint8_t byte;
		int err = fread(&byte, sizeof(uint8_t), 1, f_disk);
		if (err != 1) return false;
		if (byte != 0x00) {
			if (i != 1 || byte != 0xFF) return true;
		}
	}

	return false;
}

void __drawhexbyte(Font font, int x, int y, uint8_t byte, Color clr) {
	for (int i=0; i<2; i++) {
		uint8_t n = byte & 0x0F;

		int o = ((1 - i) * 14);
		if (n == 1) o += 6;

		char c = '0';
		if (n > 9) { n -= 10; c = 'A'; }
		c += n;

		DrawTextCodepoint(font, c,
			(Vector2){ x + o, y },
			20, clr
		);

		byte >>= 4;
	}
}

void REC_DrawData(int x, int y, void *buf, size_t bufsz, bool hex_mode, bool show_offset) {
	if (buf == NULL) return;

	Font font = GetFontDefault();
	int line_num = 0;

	int nx = x;
	if (show_offset) nx += 40;
	int cw = 32;
	if (hex_mode) cw = 32;

	for (int i=0; i<bufsz; i++) {
		int bi = i & 0b1111;

		int c = ((uint8_t *) buf)[i];
		Color clr = BLACK;

		if (isspace(c) || !isprint(c)) {
			clr = LIGHTGRAY;
			if (!hex_mode && isspace(c)) c = '.';
		}

		if (bi == 0) {
			if (show_offset) {
				__drawhexbyte(font, x, y + (line_num * 20), i, GRAY);
			}

			DrawTextCodepoint(font, '|',
				(Vector2){ nx, y + (line_num * 20) },
				20, BLACK
			);
		}

		if (hex_mode) {
			__drawhexbyte(font, nx + 10 + (bi * cw), y + (line_num * 20), c, clr);
		} else {
			DrawTextCodepoint(font, c,
				(Vector2){ nx + 10 + (bi * cw), y + (line_num * 20) },
				20, clr
			);
		}

		if (bi == 15) {
			DrawTextCodepoint(font, '|',
				(Vector2){ nx + 10 + (16 * cw), y + (line_num++ * 20) },
				20, BLACK
			);
		}
	}

	if ((bufsz & 0b111) != 0) {
		DrawTextCodepoint(font, '|',
			(Vector2){ nx + 10 + (16 * cw), y + (line_num++ * 20) },
			20, BLACK
		);
	}

}

int REC_AnalyseDisk(FILE *f_disk, FILE *f_meta, DSK_Directory dir, REC_Analysis *analysis) {
	if (f_disk == NULL || analysis == NULL) return 1;

	// Initialise analysis struct
	for (int i=0; i<MAX_ANALYSIS_ENTRIES; i++) {
		analysis->entries[i] = (REC_Entry){
			.type = SECTYPE_INVALID,
			.status = SECSTAT_INVALID,
			.file_index = -1,
			.dir_entry = { SECTYPE_INVALID, { 0, 0 }, "", 0 },
			.dir_index = -1,
			.checksum = 0x0000,
		};
	}

	//	Traverse the known directory sectors on track 18
	DSK_Position pos;
	DSK_File_SeekPosition(f_disk, DSK_POSITION_BAM);
	fread(&pos, sizeof(DSK_Position), 1, f_disk);

	int index = DSK_PositionToIndex(pos);
	while (index >= 0) {
		analysis->entries[index].type = SECTYPE_DIR;
		analysis->entries[index].status = SECSTAT_GOOD;	// TODO: Check if dir block is actually good

		DSK_File_SeekPosition(f_disk, pos);
		fread(&pos, sizeof(DSK_Position), 1, f_disk);
		index = DSK_PositionToIndex(pos);
	}

	// Traverse each block for each directory file and assign entries to known sectors
	for (int i=0; i<dir.num_entries; i++) {
		DSK_DirEntry entry = dir.entries[i];

		DSK_Position pos = entry.pos;
		int index = DSK_PositionToIndex(pos);
		int num = 0;
		while (index >= 0 && num < entry.block_count) {
			//printf("---> Adding dir entry for sector [% 3i/% 3i]; File block %i/%i\n",
			//	pos.track, pos.sector, num+1, entry.block_count
			//);
			analysis->entries[index].dir_entry = entry;
			analysis->entries[index].dir_index = i;
			analysis->entries[index].file_index = num;
			analysis->entries[index].type = entry.filetype;
			analysis->entries[index].status = SECSTAT_UNKNOWN;

			DSK_File_SeekPosition(f_disk, pos);
			int n = fread(&pos, sizeof(DSK_Position), 1, f_disk);
			if (n != 1) {
				analysis->entries[index].status = SECSTAT_BAD;
				break;
			}

			// TODO: Properly analyse file blocks based on their type
			// For now: Count them as "good" so long as their next block pointer is valid
			if (num < entry.block_count-1) {
				if (DSK_IsPositionValid(pos)) {
					analysis->entries[index].status = SECSTAT_GOOD;
				} else {
					analysis->entries[index].status = SECSTAT_BAD;
				}
			} else {
				if (pos.track == 0x00 && pos.sector == 0xFF) analysis->entries[index].status = SECSTAT_GOOD;
				// TODO: Does it matter in the last block of a file?
				// Need to find out...
			}

			//printf("---> Next block in file: [% 3i/% 3i]\n", pos.track, pos.sector);
			index = DSK_PositionToIndex(pos);
			num++;
		}

	}

	// Check all other sectors
	for (int t=MIN_TRACKS; t<=MAX_TRACKS; t++) {
		for (int s=0; s<21; s++) {
			DSK_Position pos = { t, s };
			int index = DSK_PositionToIndex(pos);
			if (index < 0) continue;

			// Get the metadata entry if available
			analysis->entries[index].checksum = 0x0000;
			analysis->entries[index].disk_err = 0x00;
			if (f_meta != NULL) {
				NYB_DataBlock block;
				block.track_num = t;
				block.sector_index = s;
				int err = NYB_Meta_ReadBlock(f_meta, &block);
				if (err == 0 && block.block_status != 0x00) {
					analysis->entries[index].checksum = block.checksum;
					if (block.checksum != DSK_Checksum(block.data)) analysis->entries[index].status = SECSTAT_CORRUPTED;
					else analysis->entries[index].status = SECSTAT_CONFIRMED;

					analysis->entries[index].disk_err = block.err_code | 0x80;
				}
			}

			//if (analysis->entries[index].type != SECTYPE_INVALID) continue; // Skip entries we've already checked

			bool has_data = REC_Sector_HasData(f_disk, pos);
			bool is_free = (dir.bam.entries[pos.track - 1] >> (8 + pos.sector)) & 1;
			DSK_SectorType type = analysis->entries[index].type;
			REC_Status stat = analysis->entries[index].status;

			if (type == SECTYPE_INVALID) {
				if (t == 18 && s == 0) {
					type = SECTYPE_BAM;
					stat = SECSTAT_GOOD;	// TODO: Check if the BAM really matches the expected format on load
				} else {
					type = SECTYPE_NONE;
					stat = SECSTAT_UNKNOWN;
				}
			}

			if (stat == SECSTAT_INVALID || stat == SECSTAT_UNKNOWN) {
				if (is_free) {
					if (has_data) stat = SECSTAT_UNEXPECTED;
					else stat = SECSTAT_EMPTY;
				} else {
					if (has_data) stat = SECSTAT_PRESENT;
					else stat = SECSTAT_MISSING;
				}
			}

			analysis->entries[index].type = type;
			analysis->entries[index].status = stat;
		}
	}

	return 0;
}

int REC_GetInfo(REC_Analysis analysis, DSK_Position pos, REC_Entry *entry) {
	if (entry == NULL) return 1;

	int index = DSK_PositionToIndex(pos);
	if (index < 0) return 2;

	memcpy(entry, &(analysis.entries[index]), sizeof(REC_Entry));
	return 0;
}

const char *REC_GetStatusName(REC_Status status) {
	switch (status) {
		case SECSTAT_EMPTY: return "Empty";
		case SECSTAT_UNEXPECTED: return "Has unexpected Data";
		case SECSTAT_MISSING: return "Missing";
		case SECSTAT_PRESENT: return "Has Data";
		case SECSTAT_BAD: return "Data doesn't match expected format";
		case SECSTAT_GOOD: return "Sector is Good!";
		case SECSTAT_CORRUPTED: return "Invalid Checksum";
		case SECSTAT_CONFIRMED: return "Checksum Confirmed";

		case SECSTAT_INVALID: return "Invalid";
		case SECSTAT_UNKNOWN: return "Unknown";
	}

	return "";
}

Color REC_GetStatusColour(REC_Status status) {
	switch (status) {
		case SECSTAT_EMPTY: return GRAY;
		case SECSTAT_UNEXPECTED: return PURPLE;
		case SECSTAT_MISSING: return ORANGE;
		case SECSTAT_PRESENT: return GOLD;
		case SECSTAT_BAD: return RED;
		case SECSTAT_GOOD: return GREEN;
		case SECSTAT_CORRUPTED: return MAROON;
		case SECSTAT_CONFIRMED: return DARKGREEN;

		case SECSTAT_INVALID: return MAGENTA;
		case SECSTAT_UNKNOWN: return DARKPURPLE;
	}

	return BLACK;
}

double __normalise(double x) {
	if (x < 0.0) x = fabs(x);
	if (x > 1.0) x -= (int) x;
	return x;
}

double __clamp(double x) {
	if (x < 0.0) return 0.0;
	if (x > 1.0) return 1.0;
	return x;
}

double __trapezoid_wave(double x) {
	x = __normalise(x);

	double m = x * 6;
	int seg = m;
	switch (seg) {
		case 0: return m;
		case 1: return 1.0;
		case 2: return 1.0;
		case 3: return 1.0 - (m - 3.0);
		case 4: return 0.0;
		case 5: return 0.0;
	}
	return x;
}

Color __hsv_to_rgb(double h, double s, double v) {
	h = __normalise(h);
	s = __normalise(s);
	v = __normalise(v);

	double c = v * s;
	double m = v - c;

	double r = c * __trapezoid_wave(h + 0.6666);
	double g = c * __trapezoid_wave(h + 0.0001);
	double b = c * __trapezoid_wave(h + 0.3333);

	return (Color){
		0xFF * (r + m),
		0xFF * (g + m),
		0xFF * (b + m),
		0xFF
	};
}

Color REC_GetFileColour(DSK_Directory dir, REC_Entry entry, bool is_hovered, bool is_selected) {
	if (entry.dir_index < 0) return LIGHTGRAY;

	Color clr = __hsv_to_rgb(
		(double) entry.dir_index / (double) dir.num_entries,
		//(double) entry.dir_entry.pos.track / (double) 15,
		0.5 + (is_selected ? 0.5 : 0.0),
		0.7 + (!is_selected && is_hovered ? 0.1 : 0.0) + (is_selected ? 0.3 : 0.0)
	);

	return clr;
}




//REC_File *REC_ReadFile(char *filename) {
//	if (filename == NULL) return NULL;
//
//	REC_File *rec = malloc(sizeof(REC_File));
//	rec->path = NULL;
//	rec->disk_path = NULL;
//
//	// Open the file
//	size_t fnlen = strlen(filename);
//	rec->path = malloc(fnlen + 1);
//	strlcpy(rec->path, filename, fnlen);
//	FILE *f = fopen(filename, "rb");
//
//	// Check for the 4 magic bytes of the rec file header
//	uint8_t magic[4];
//	fread(magic, sizeof(uint8_t), 4, f);
//	for (int i=0; i<4; i++) {
//		if (magic[i] != REC_MAGIC_BYTES[i]) {
//			fclose(f);
//			REC_CloseFile(rec);
//			return NULL;
//		}
//	}
//
//	// Read region offsets
//	fread(&rec->offs_entries, sizeof(uint32_t), 1, f);
//	fread(&rec->offs_data, sizeof(uint32_t), 1, f);
//	fread(&rec->offs_strings, sizeof(uint32_t), 1, f);
//
//	// Try to read the disk file path
//	
//
//	return 0;
//}
//
//void REC_CloseFile(REC_File *rec) {
//	if (rec == NULL) return;
//
//	//if (rec->disk_path
//	
//}
//
//int REC_WriteFile(REC_File *rec) {
//	if (rec == NULL) return 1;
//
//	return 0;
//}
//
//REC_Entry REC_FindEntry(REC_File *rec, REC_RecordType type, int index) {
//	if (rec == NULL) return (REC_Entry){};
//
//	return (REC_Entry){};
//}

