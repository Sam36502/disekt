#include "../include/analysis.h"


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

				.file_index = -1,
				.dir_index = -1,

				.checksum = 0x0000,
				.disk_err = 0xFF,
				.parse_err = 0xFF,
			};

			// Do basic type checks
			if (analysis->sectors[index].is_free) {
				analysis->sectors[index].type = SECTYPE_EMPTY;
			} else {
				if (t == 18) {
					if (s == 0) analysis->sectors[index].type = SECTYPE_BAM;
					else analysis->sectors[index].type = SECTYPE_DIR;
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

					// TODO: Replace
					if (block.checksum != DSK_Checksum(block.data)) analysis->sectors[index].status = SECSTAT_CORRUPTED;
					else analysis->sectors[index].status = SECSTAT_CONFIRMED;
					if (block.parse_error != 0x00) analysis->sectors[index].status = SECSTAT_CORRUPTED;

					memcpy(analysis->sectors[index].data, block.data, BLOCK_SIZE);
				}
			} else {
				int err = DSK_File_GetData(f_disk, pos, analysis->sectors[index].data, BLOCK_SIZE);
				if (err != 0) {
					for (int i=0; i<BLOCK_SIZE; i++) analysis->sectors[index].data[i] = 0x00;
				}
			}

			// Do basic status checks
			// TODO: Replace with proper tests

			// TODO: Replace with more permissive test (include known "empty" block patterns)
			uint8_t *d = analysis->sectors[index].data;
			for (int i=0; i<BLOCK_SIZE; i++) {
				if (d[i] != 0x00) { analysis->sectors[index].has_data = true; break; }
			}

			if (analysis->sectors[index].is_free) {
				if (analysis->sectors[index].has_data) analysis->sectors[index].status = SECSTAT_UNEXPECTED;
				else analysis->sectors[index].status = SECSTAT_EMPTY;
			} else {
				if (analysis->sectors[index].has_data) analysis->sectors[index].status = SECSTAT_PRESENT;
				else analysis->sectors[index].status = SECSTAT_MISSING;
			}

		}
	}

	//	Find the directory blocks on track 18
	DSK_Position pos;
	DSK_File_SeekPosition(f_disk, DSK_POSITION_BAM);
	fread(&pos, sizeof(DSK_Position), 1, f_disk);

	int index = DSK_PositionToIndex(pos);
	while (index >= 0) {
		analysis->sectors[index].type = SECTYPE_DIR;
		analysis->sectors[index].status = SECSTAT_GOOD;	// TODO: Check if dir block is actually good

		DSK_File_SeekPosition(f_disk, pos);
		fread(&pos, sizeof(DSK_Position), 1, f_disk);
		index = DSK_PositionToIndex(pos);
	}

	// Traverse each block for each directory file and assign entries to known sectors
	for (int i=0; i<dir.num_entries; i++) {
		DSK_DirEntry entry = dir.entries[i];

		DSK_Position pos = entry.head_pos;
		int index = DSK_PositionToIndex(pos);
		int num = 0;
		while (index >= 0 && num < entry.block_count) {
			analysis->sectors[index].has_directory_info = true;
			analysis->sectors[index].dir_entry = entry;
			analysis->sectors[index].dir_index = i;
			analysis->sectors[index].file_index = num;
			analysis->sectors[index].type = entry.type;
			analysis->sectors[index].status = SECSTAT_UNKNOWN;

			DSK_File_SeekPosition(f_disk, pos);
			int n = fread(&pos, sizeof(DSK_Position), 1, f_disk);
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
				if (pos.track == 0x00 && pos.sector == 0xFF) analysis->sectors[index].status = SECSTAT_GOOD;
				// TODO: Does it matter in the last block of a file?
				// Need to find out...
			}

			index = DSK_PositionToIndex(pos);
			num++;
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
		case SECSTAT_EMPTY: return LIGHTGRAY;
		case SECSTAT_UNEXPECTED: return PURPLE;
		case SECSTAT_MISSING: return ORANGE;
		case SECSTAT_PRESENT: return YELLOW;
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

const char *ANA_GetTestResultName(ANA_TestResult result) {
	switch (result) {
		case TEST_SKIPPED: return "SKIPPED";
		case TEST_FAILED: return "FAILED";
		case TEST_PASSED: return "PASSED";
		default: return "INVALID";
	}
}

Color ANA_GetTestResultColour(ANA_TestResult result) {
	switch (result) {
		case TEST_SKIPPED: return GRAY;
		case TEST_FAILED: return RED;
		case TEST_PASSED: return GREEN;
		default: return MAGENTA;
	}
}

