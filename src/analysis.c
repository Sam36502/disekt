#include "../include/analysis.h"


int ANA_AnalyseDisk(FILE *f_disk, FILE *f_meta, DSK_Directory dir, ANA_DiskInfo *analysis) {
	if (f_disk == NULL || analysis == NULL) return 1;

	analysis->dir = dir;

	// TODO: More initial checks (can we find the BAM? Is it valid? Is the header/directory valid? etc.)
	// For now we're just assuming that the provided parsed BAM and directories are healthy

	// Initialise analysis struct with basic information for each sector
	for (int t=MIN_TRACKS; t<=MAX_TRACKS; t++) {
		for (int s=0; s<21; s++) {
			DSK_Position pos = { t, s };
			int index = DSK_PositionToIndex(pos);
			if (index < 0) continue;

			analysis->sectors[index] = (ANA_SectorInfo){
				.pos = pos,
				.type = SECTYPE_UNKNOWN,
				.status = SECSTAT_UNKNOWN,
			
				.is_free = (dir.bam.entries[pos.track - 1] >> (8 + pos.sector)) & 0b1,
				.has_data = false,
				.has_transfer_info = false,
				.has_directory_info = false,
				.checksum_match = false,
				.is_blank = false,

				.file_index = -1,
				.dir_index = -1,

				.checksum = 0x0000,
				.disk_err = 0xFF,
				.parse_err = 0xFF,

				.prev_block_index = -1,
				.next_block_index = -1,
			};

			// Do basic type checks
			if (analysis->sectors[index].is_free) {
				analysis->sectors[index].type = SECTYPE_EMPTY;
			} else {
				if (t == 18) {
					if (s == 0) {
						analysis->sectors[index].type = SECTYPE_BAM;
						analysis->sectors[index].status = SECSTAT_GOOD;
					} else {
						analysis->sectors[index].type = SECTYPE_DIR;
					}
				}
			}

			// Get the metadata entry if available
			if (f_meta != NULL) {
				NYB_DataBlock block;
				block.track_num = t;
				block.sector_index = s;
				int err = NYB_Meta_ReadBlock(f_meta, &block);
				if (err == 0 && block.block_status != 0x00) {
					analysis->sectors[index].has_transfer_info = true;
					analysis->sectors[index].checksum = block.checksum;
					analysis->sectors[index].disk_err = block.err_code | 0x80;
					analysis->sectors[index].parse_err = block.parse_error;
					analysis->sectors[index].checksum_match = block.checksum == DSK_Checksum(block.data);

					memcpy(analysis->sectors[index].data, block.data, BLOCK_SIZE);
				}
			} else {
				int err = DSK_File_GetData(f_disk, pos, analysis->sectors[index].data, BLOCK_SIZE);
				if (err != 0) {
					for (int i=0; i<BLOCK_SIZE; i++) analysis->sectors[index].data[i] = 0x00;
				}
			}

			// Do basic status checks
			// TODO: Improve these

			for (int i=0; i<BLOCK_SIZE; i++) {
				if (analysis->sectors[index].data[i] != 0x00) {
					analysis->sectors[index].has_data = true;
					break;
				}
			}

			if (analysis->sectors[index].status == SECSTAT_UNKNOWN) {
				if (analysis->sectors[index].is_free) {
					if (analysis->sectors[index].has_data) analysis->sectors[index].status = SECSTAT_UNEXPECTED;
					else analysis->sectors[index].status = SECSTAT_EMPTY;
				} else {
					if (analysis->sectors[index].has_data) analysis->sectors[index].status = SECSTAT_PRESENT;
					else analysis->sectors[index].status = SECSTAT_MISSING;
				}

				if (analysis->sectors[index].has_transfer_info) {
					bool transfer_err = analysis->sectors[index].parse_err != 0x00;
					transfer_err |= analysis->sectors[index].disk_err != 0x80;
					transfer_err |= !analysis->sectors[index].checksum_match;

					if (transfer_err) analysis->sectors[index].status = SECSTAT_CORRUPTED;
				}
			}

		}
	}

	//	Find the directory blocks on track 18
	DSK_Position pos;
	DSK_File_SeekPosition(f_disk, DSK_POSITION_BAM);
	fread(&pos, sizeof(DSK_Position), 1, f_disk);

	int index = DSK_PositionToIndex(pos);
	int prev = -1;
	int dir_file_index = 0;
	while (index >= 0) {
		analysis->sectors[index].type = SECTYPE_DIR;
		analysis->sectors[index].dir_index = dir_file_index;

		if (analysis->sectors[index].status == SECSTAT_UNKNOWN || analysis->sectors[index].status == SECSTAT_PRESENT) {
			analysis->sectors[index].status = SECSTAT_GOOD;	// TODO: Check if dir block is actually good
		}

		analysis->sectors[index].prev_block_index = prev;
		analysis->sectors[index].next_block_index = -1;
		if (prev >= 0) {
			analysis->sectors[prev].next_block_index = index;
		}

		DSK_File_SeekPosition(f_disk, pos);
		fread(&pos, sizeof(DSK_Position), 1, f_disk);

		dir_file_index += 8;
		prev = index;
		index = DSK_PositionToIndex(pos);
	}

	// Traverse each block for each directory file and assign entries to known sectors
	for (int i=0; i<dir.num_entries; i++) {
		DSK_DirEntry entry = dir.entries[i];

		DSK_Position pos = entry.head_pos;
		int index = DSK_PositionToIndex(pos);
		ANA_SectorInfo curr = analysis->sectors[index];
		int num = 0;
		int prev = -1;
		while (index >= 0 && num < entry.block_count) {
			analysis->sectors[index].has_directory_info = true;
			analysis->sectors[index].dir_entry = entry;
			analysis->sectors[index].dir_index = i;
			analysis->sectors[index].file_index = num;
			analysis->sectors[index].type = entry.type;

			analysis->sectors[index].prev_block_index = prev;
			analysis->sectors[index].next_block_index = -1;
			if (prev >= 0) {
				analysis->sectors[prev].next_block_index = index;
			}

			DSK_File_SeekPosition(f_disk, pos);
			int n = fread(&pos, sizeof(DSK_Position), 1, f_disk);

			if (curr.status == SECSTAT_UNKNOWN || curr.status == SECSTAT_PRESENT || curr.status == SECSTAT_MISSING) {
				if (n != 1) {
					analysis->sectors[index].status = SECSTAT_BAD;
					break;
				}

				// TODO: Properly analyse file blocks based on their type
				// For now: Count them as "good" so long as their next block pointer is valid
				if (num < entry.block_count-1) {
					if (DSK_IsPositionValid(pos)) {
						analysis->sectors[index].status = SECSTAT_GOOD;
					} else {
						analysis->sectors[index].status = SECSTAT_BAD;
					}
				} else {
					analysis->sectors[index].status = SECSTAT_GOOD;
				}
			}

			prev = index;
			index = DSK_PositionToIndex(pos);
			num++;
		}

	}

	// Mark blocks with a known blank pattern as "empty" (not "unexpected")
	// TODO: Refactor into separate function
	uint8_t *blank_patterns[64];
	int blank_matches[64];
	int blank_pattern_count = 0;

	// ---> Find all the unique blocks which are marked as free, but still have non-zero data
	for (int i=0; i<MAX_ANALYSIS_ENTRIES; i++) {
		if (!analysis->sectors[i].is_free || !analysis->sectors[i].has_data) continue;

		bool found = false;
		for (int p=0; p<blank_pattern_count; p++) {
			if (memcmp(analysis->sectors[i].data, blank_patterns[p], BLOCK_SIZE) != 0) continue;
			found = true;
		}
		if (found) break;

		blank_matches[blank_pattern_count] = 0;
		blank_patterns[blank_pattern_count] = analysis->sectors[i].data;
		blank_pattern_count++;
	}

	// ---> Count how many sectors match each pattern
	for (int i=0; i<MAX_ANALYSIS_ENTRIES; i++) {
		if (!analysis->sectors[i].is_free || !analysis->sectors[i].has_data) continue;

		for (int p=0; p<blank_pattern_count; p++) {
			if (memcmp(analysis->sectors[i].data, blank_patterns[p], BLOCK_SIZE) != 0) continue;
			blank_matches[p]++; break;
		}
	}

	// ---> Find the pattern with the most matches; most likely to be the general disk blank pattern
	uint8_t *blank_pattern = NULL;
	int most = 0;
	for (int i=0; i<blank_pattern_count; i++) {
		if (blank_matches[i] < most) continue;

		most = blank_matches[i];
		blank_pattern = blank_patterns[i];
	}

	// ---> Mark all sectors that match that pattern
	if (blank_pattern != NULL) {
		for (int i=0; i<MAX_ANALYSIS_ENTRIES; i++) {
			if (!analysis->sectors[i].is_free || !analysis->sectors[i].has_data) continue;
			if (memcmp(analysis->sectors[i].data, blank_pattern, BLOCK_SIZE) != 0) continue;

			analysis->sectors[i].is_blank = true;

			if (analysis->sectors[i].status == SECSTAT_UNEXPECTED) analysis->sectors[i].status = SECSTAT_EMPTY;
		}
	}


	// Last pass to add confirmed checksums
	for (int i=0; i<MAX_ANALYSIS_ENTRIES; i++) {
		ANA_SectorInfo curr = analysis->sectors[i];
		if (!curr.has_transfer_info) continue;
		if (curr.status == SECSTAT_GOOD && curr.checksum_match) analysis->sectors[i].status = SECSTAT_CONFIRMED;
	}

	return 0;
}

int ANA_GatherStats(ANA_DiskInfo *analysis) {
	if (analysis == NULL) return 1;

	for (int i=0; i<MAX_ANALYSIS_ENTRIES; i++) {
		ANA_SectorInfo entry = analysis->sectors[i];

		if (entry.is_free) analysis->count_free++;

		if (entry.status == SECSTAT_GOOD
			|| entry.status == SECSTAT_PRESENT
			|| entry.status == SECSTAT_CONFIRMED
			|| entry.status == SECSTAT_EMPTY
		) {
			analysis->count_healthy++;
		}
	}

	return 0;
}

int ANA_GetInfo(ANA_DiskInfo analysis, DSK_Position pos, ANA_SectorInfo *entry) {
	if (entry == NULL) return 1;

	int index = DSK_PositionToIndex(pos);
	if (index < 0) return 2;

	*entry = analysis.sectors[index];
	return 0;
}

const char *ANA_GetStatusName(ANA_Status status) {
	switch (status) {
		case SECSTAT_EMPTY: return "Empty";
		case SECSTAT_UNEXPECTED: return "Unexpected Data";
		case SECSTAT_MISSING: return "Missing!";
		case SECSTAT_PRESENT: return "Orphaned Data";
		case SECSTAT_BAD: return "Invalid Format";
		case SECSTAT_GOOD: return "Valid Format";
		case SECSTAT_CORRUPTED: return "Transfer Error";
		case SECSTAT_CONFIRMED: return "Checksum Confirmed";

		case SECSTAT_INVALID: return "Invalid";
		case SECSTAT_UNKNOWN: return "Unknown";
	}

	return "";
}

Color ANA_GetStatusColour(ANA_Status status) {
	switch (status) {
		case SECSTAT_EMPTY: return LIGHTGRAY;
		case SECSTAT_UNEXPECTED: return SKYBLUE;

		case SECSTAT_MISSING: return BLACK;
		case SECSTAT_BAD: return RED;
		case SECSTAT_CORRUPTED: return MAROON;

		case SECSTAT_PRESENT: return GOLD;

		case SECSTAT_GOOD: return GREEN;
		case SECSTAT_CONFIRMED: return LIME;

		case SECSTAT_UNKNOWN: return DARKGRAY;

		case SECSTAT_INVALID: return MAGENTA;
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

Color ANA_GetFileColour(DSK_Directory dir, ANA_SectorInfo entry, bool is_hovered, bool is_selected) {
	if (entry.dir_index < 0) return LIGHTGRAY;

	Color clr = __hsv_to_rgb(
		(double) entry.dir_index / (double) dir.num_entries,
		//(double) entry.dir_entry.pos.track / (double) 15,
		0.5 + (is_selected ? 0.5 : 0.0),
		0.7 + (!is_selected && is_hovered ? 0.1 : 0.0) + (is_selected ? 0.3 : 0.0)
	);

	return clr;
}

const char *ANA_GetParseErrorName(int err_code) {
	switch (err_code) {
		case 0: return "No Error";
		case 1: return "Checksum failed on first byte (chk_lo)";
		case 2: return "Checksum failed on second byte (chk_hi)";
		case 3: return "Disk failed to read; See disk error code";
		case 4: return "Final NULL missing; Format not quite as expected, but not necessarily fatal";
		default: return TextFormat("Unrecognised Nyb-Log error code: %i", err_code);
	}
	return "";
}

Color ANA_GetParseErrorColour(int err_code) {
	if (err_code == 0) return GREEN;
	return RED;
}

const char *ANA_GetDiskErrorName(uint8_t err_code) {
	switch (err_code & ~0x80) {
		case 0: return "OK";
		case 1:	return "Files scratched response. Not an error condition.";
		case 20: return "Block header not found on disk.";
		case 21: return "Sync character not found.";
		case 22: return "Data block not present.";
		case 23: return "Checksum error in data.";
		case 24: return "Byte decoding error in data.";
		case 25: return "Write-verify error.";
		case 26: return "Attempt to write with write protect on.";
		case 27: return "Checksum error in header.";
		case 28: return "Data extends into next block.";
		case 29: return "Disk id mismatch.";
		case 30: return "General syntax error";
		case 32: return "Invalid command.";
		case 31: return "Long line.";
		case 33: return "Invalid filename.";
		case 34: return "No file given.";
		case 39: return "Command file not found.";
		case 50: return "Record not present.";
		case 51: return "Overflow in record.";
		case 52: return "File too large.";
		case 60: return "File open for write.";
		case 61: return "File not open.";
		case 62: return "File not found.";
		case 63: return "File exists.";
		case 64: return "File type mismatch.";
		case 65: return "No block.";
		case 66: return "Illegal track or sector.";
		case 67: return "Illegal system track or sector.";
		case 70: return "No channels available.";
		case 71: return "Directory error.";
		case 72: return "Disk full or directory full.";
		case 73: return "Power up message, or write attempt with DOS Mismatch";
		case 74: return "Drive not ready.";
	}
	return "Invalid Disk Error";
}

Color ANA_GetDiskErrorColour(uint8_t  err_code) {
	uint8_t e = err_code & ~0x80;
	if (e == 0) return GREEN;
	if (e == 1) return BLUE;
	if (e >= 20 && e <= 74) return RED;
	return MAGENTA;
}

