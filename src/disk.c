#include "../include/disk.h"
#include <raylib.h>


//	---- Sector Utilities

uint16_t DSK_Checksum(void *ptr) {
	uint8_t lo = 0x00;
	uint8_t hi = 0x00;

	uint8_t *bp = ptr;
	for (int i=0; i<256; i++) {
		lo += bp[i];
		hi += lo;
	}

	return (uint16_t)(hi) << 8 | (uint16_t)(lo);
}

int DSK_Track_GetSectorCount(int track_num) {
	if (track_num < MIN_TRACKS || track_num > MAX_TRACKS) return 0;

	if (track_num < 18) return 21;
	if (track_num < 25) return 19;
	if (track_num < 31) return 18;
	return 17;
}

bool DSK_PositionsEqual(DSK_Position a, DSK_Position b) {
	return a.track == b.track && a.sector == b.sector;
}

bool DSK_IsPositionValid(DSK_Position pos) {
	if (pos.track < MIN_TRACKS || pos.track > MAX_TRACKS) return false;

	if (pos.sector >= DSK_Track_GetSectorCount(pos.track)) return false;

	return true;
}

int DSK_PositionToIndex(DSK_Position pos) {
	if (!DSK_IsPositionValid(pos)) return -1;

	int index = 0;
	int tr = pos.track - MIN_TRACKS;
	if (tr > 30) { index += (tr-30) * 17; tr = 30; }
	if (tr > 24) { index += (tr-24) * 18; tr = 24; }
	if (tr > 17) { index += (tr-17) * 19; tr = 17; }
	index += tr * 21;
	index += pos.sector;

	return index;
}

int DSK_File_SeekPosition(FILE *f_disk, DSK_Position pos) {
	if (f_disk == NULL) return 1;

	long offset = DSK_PositionToIndex(pos);
	if (offset < 0) return 2;
	offset *= 0x100;
	
	return fseek(f_disk, offset, SEEK_SET);
}

DSK_Position DSK_GetHoveredSector() {
	int mouse_x = GetMouseX();
	int mouse_y = GetMouseY();
	double len_x = (double) mouse_x - DISK_CENTRE_X;
	double len_y = (double) mouse_y - DISK_CENTRE_Y;

	// Calculate track-number from mouse pos
	double hyp = sqrt((len_x * len_x) + (len_y * len_y));
	double track_width = (((double) DISK_RADIUS - SPINDLE_RADIUS) / (MAX_TRACKS + MIN_TRACKS));
	int tracknum = MIN_TRACKS + MAX_TRACKS - (hyp - SPINDLE_RADIUS) / track_width;

	// Calculate angle from mouse pos
	float oa = len_y / len_x;
	float mouse_angle;
	if (mouse_x - DISK_CENTRE_X < 0) {
		mouse_angle = 180.0f + atanf(oa) * (360.0f / TAU);
	} else {
		if (mouse_y - DISK_CENTRE_Y < 0) {
			mouse_angle = 360.0f + atanf(oa) * (360.0f / TAU);
		} else {
			mouse_angle = 0.0f + atanf(oa) * (360.0f / TAU);
		}
	}
	mouse_angle += 90.0f;
	if (mouse_angle > 360.0f) mouse_angle -= 360.0f;

	int trklen = DSK_Track_GetSectorCount(tracknum);
	int sec = mouse_angle / (360.0f / trklen);

	return (DSK_Position){
		.track = tracknum,
		.sector = sec,
	};
}


//	---- Retrieving Data

int DSK_File_GetData(FILE *f_disk, DSK_Position pos, void *buf, size_t bufsz) {
	int err = DSK_File_SeekPosition(f_disk, pos);
	if (err > 0) return err;

	for (int i=0; i<BLOCK_SIZE && i<bufsz; i++) {
		fread(buf + i, sizeof(uint8_t), 1, f_disk);
	}

	return 0;
}

int DSK_File_ParseDirectory(FILE *f_disk, DSK_Directory *dir, bool ignore_bam) {
	if (f_disk == NULL || dir == NULL) return 1;

	// Seek to track 18
	DSK_File_SeekPosition(f_disk, DSK_POSITION_BAM);

	// Read the BAM
	DSK_Position next_pos;
	if (ignore_bam) {
		next_pos = (DSK_Position){ 18, 1 };
		for (int t=MIN_TRACKS; t<=MAX_TRACKS; t++) {
			if (t == 18) {
				dir->bam[t-1] = (uint32_t) 0x07FFFC11;
				continue;
			}

			dir->bam[t-1] = (uint32_t) 0x1FFFFF00 | DSK_Track_GetSectorCount(t);
		}
	} else {
		char magic_a[2];
		fread(&next_pos, sizeof(DSK_Position), 1, f_disk);
		fread(&magic_a, sizeof(char), 2, f_disk);
		
		// Check format
		if (!DSK_IsPositionValid(next_pos)) return 2;
		if (magic_a[0] != 'A' || magic_a[1] != 0x00) return 3;

		fread(&dir->bam, sizeof(uint32_t), MAX_TRACKS, f_disk);
	}

	// Read the rest into the header string
	fread(dir->header, sizeof(char), DIR_HEADER_SIZE-1, f_disk);

	// Parse the directory blocks
	dir->num_entries = 0;
	while (DSK_IsPositionValid(next_pos)) {
		DSK_File_SeekPosition(f_disk, next_pos);
		fread(&next_pos, sizeof(DSK_Position), 1, f_disk);

		uint8_t type;
		int index = sizeof(DSK_Position);
		while (index < BLOCK_SIZE) {
			fread(&type, sizeof(uint8_t), 1, f_disk);
			if (type == 0x00) break;
			index += sizeof(uint8_t);

			DSK_Position pos;
			fread(&pos, sizeof(DSK_Position), 1, f_disk);
			index += sizeof(DSK_Position);
			if (!DSK_IsPositionValid(pos)) break;

			uint8_t namebuf[16];
			fread(&namebuf, sizeof(uint8_t), 16, f_disk);
			index += sizeof(uint8_t) * 16;

			// TODO: Handle rel-specific directory data?
			//fread(NULL, sizeof(uint8_t), 9, f_disk);
			fseek(f_disk, 9, SEEK_CUR);
			index += sizeof(uint8_t) * 9;

			uint16_t num_blocks;
			fread(&num_blocks, sizeof(uint16_t), 1, f_disk);
			index += sizeof(uint16_t);

			dir->entries[dir->num_entries] = (DSK_DirEntry){
				.type = type,
				.head_pos = pos,
				.block_count = num_blocks,
			};

			// Clean & Trim filename
			int ni = 0;
			int end = 0;
			for (int i=0; i<16; i++) {
				int c = namebuf[i];
				if (!isprint(c)) continue;
				if (isspace(c)) {
					if (ni == 0) continue;
					end = ni;
				} else {
					end = 0;
				}
				dir->entries[dir->num_entries].filename[ni++] = c;
			}
			if (end == 0) end = ni;
			dir->entries[dir->num_entries].filename[end] = '\0';

			dir->num_entries++;
			fseek(f_disk, 2, SEEK_CUR); // Skip two bytes after each entry
			index += 2;
		}

	}

	return 0;
}


//	---- Debug Printing

void DSK_PrintBAM(uint32_t bam[MAX_TRACKS]) {
	printf("\n BAM Contents:\n");
	printf("---------------------------------\n");
	for (int i=0; i<MAX_TRACKS; i++) {
		uint8_t sec_free = bam[i] & 0xFF;
		int sec_total = DSK_Track_GetSectorCount(i+1);
		printf(" Track % 3i: (% 3i/% 3i free) - ", i+1, sec_free, sec_total);

		for (int s=0; s<sec_total; s++) {
			int is_free = (bam[i] >> (8 + s)) & 1;
			printf("%c", is_free ? '_' : '#');
		}

		putchar('\n');
	}
}

void DSK_PrintDirectory(DSK_Directory dir) {
	printf("\n Disk directory contents:\n--------------------------\n");
	for (int i=0; i<dir.num_entries; i++) {
		DSK_DirEntry e = dir.entries[i];
		printf(" % 4i: [% 3i/% 3i] (%s) \"%s\" contains %i blocks\n", i,
			e.head_pos.track, e.head_pos.sector,
			DSK_Sector_GetTypeName((DSK_SectorType) e.type), e.filename,
			e.block_count
		);
	}
}


//	---- Getting Strings & Colours

char *DSK_GetDescription(DSK_Directory dir) {
	static char buffer[128];
	buffer[0] = '\0';

	int bi = 0;
	int last_space = -1;
	for (int i=0; i<DIR_HEADER_SIZE && dir.header[i] != '\0'; i++) {
		char c = dir.header[i];
		if ((uint8_t) c == 0xA0) c = 0x00;
		if (isspace(c)) {
			if (bi == 0) continue;
			last_space = bi;
		}
		last_space = -1;
		buffer[bi++] = c;
	}
	if (last_space > 0) buffer[last_space] = '\0';
	else buffer[bi] = '\0';

	return buffer;
}

char *DSK_GetName(DSK_Directory dir) {
	static char buffer[18];
	buffer[0] = '\0';

	char *full = DSK_GetDescription(dir);
	int i = 0;
	int last_space = 0;
	while (i < 17 && full[i] != '\0') {
		buffer[i] = full[i];
		if (isspace(full[i])) last_space = i;
		else last_space = 0;
		i++;
	}
	if (last_space == 0) last_space = i;
	buffer[last_space] = '\0';

	return buffer;
}

const char *DSK_Sector_GetTypeName(DSK_SectorType type) {
	switch(type) {
		case SECTYPE_DEL: return "Deleted Block";
		case SECTYPE_SEQ: return "Sequential Data Block";
		case SECTYPE_PRG: return "Program Block";
		case SECTYPE_USR: return "User Data Block";
		case SECTYPE_REL: return "Relative Data Block";
		case SECTYPE_EMPTY: return "Empty";
		case SECTYPE_BAM: return "Block-Availability Map";
		case SECTYPE_DIR: return "Directory Table";
		case SECTYPE_UNKNOWN: return "???";
		case SECTYPE_INVALID: return "Invalid Sector Type";
	}
	return "";
}

Color DSK_Sector_GetTypeColour(DSK_SectorType type) {
	switch(type) {
		case SECTYPE_DEL: return BLACK;
		case SECTYPE_SEQ: return GREEN;
		case SECTYPE_PRG: return BLUE;
		case SECTYPE_USR: return RED;
		case SECTYPE_REL: return PURPLE;
		case SECTYPE_EMPTY: return LIGHTGRAY;
		case SECTYPE_BAM: return GOLD;
		case SECTYPE_DIR: return GOLD;
		case SECTYPE_UNKNOWN: return DARKGRAY;
		case SECTYPE_INVALID: return MAGENTA;
	}
	return GRAY;
}


//	---- Drawing Functions

void DSK_Sector_Draw(DSK_Directory dir, DSK_Position pos, DSK_DrawMode mode, Color clr) {
	if (!DSK_IsPositionValid(pos)) return;

	int track_index = MAX_TRACKS - pos.track;
	int track_width = (DISK_RADIUS - SPINDLE_RADIUS)/MAX_TRACKS;
	float r_inner = SPINDLE_RADIUS + track_width * track_index;
	float r_outer = r_inner + track_width - TRACK_GAPS;
	double sector_angle = 360.0f / (double)(DSK_Track_GetSectorCount(pos.track));
	double start_angle = sector_angle * pos.sector - 90.0f;
	double end_angle = start_angle + sector_angle - (1.0f/(float)(track_index+5) * SECTOR_GAPS);

	DrawRing(
		(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y },
		r_inner, r_outer,
		start_angle, end_angle,
		ARC_RESOLUTION, clr
	);

	switch (mode) {
		case DSK_DRAW_NORMAL: break;

		case DSK_DRAW_HIGHLIGHT: {
			DrawRing(
				(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y },
				r_inner, r_outer,
				start_angle, end_angle,
				ARC_RESOLUTION,
				(Color){ 0xFF, 0xFF, 0xFF, 0x80 }
			);
		} return;

		case DSK_DRAW_SELECTED: {
			DrawRing(
				(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y },
				r_inner + 3.0f, r_outer - 3.0f,
				start_angle + 0.8f, end_angle - 0.8f,
				ARC_RESOLUTION,
				WHITE
			);
		} return;

	}
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

void DSK_DrawData(int x, int y, void *buf, size_t bufsz, bool hex_mode, bool show_offset) {
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

