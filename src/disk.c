#include "../include/disk.h"

int DSK_Track_GetSectorCount(int track_num) {
	if (track_num < MIN_TRACKS || track_num > MAX_TRACKS) return 0;

	if (track_num < 18) return 21;
	if (track_num < 25) return 19;
	if (track_num < 31) return 18;
	return 17;
}

bool DSK_IsPositionValid(DSK_Position pos) {
	if (pos.track < MIN_TRACKS || pos.track > MAX_TRACKS) return false;

	if (pos.sector >= DSK_Track_GetSectorCount(pos.track)) return false;

	return true;
}

void DSK_File_SeekPosition(FILE *f_disk, DSK_Position pos) {
	if (f_disk == NULL) return;
	if (!DSK_IsPositionValid(pos)) return;

	//printf("---> Postion: [% 3i/% 3i]\n", pos.track, pos.sector);

	// Calculate offset
	long offset = 0;
	int tr = pos.track - MIN_TRACKS;
	if (tr > 30) { offset += (tr-30) * 17; tr = 30; }
	if (tr > 24) { offset += (tr-24) * 18; tr = 24; }
	if (tr > 17) { offset += (tr-17) * 19; tr = 17; }
	offset += tr * 21;
	offset += pos.sector;
	offset *= 0x100;

	//printf("---> Offset = 0x%08lX; %li Bytes\n", offset, offset);
	
	fseek(f_disk, offset, SEEK_SET);

	return;
}

int DSK_File_ParseBAM(FILE *f_disk, DSK_BAM *bam) {
	if (f_disk == NULL || bam == NULL) return 1;

	// Seek to the position
	DSK_File_SeekPosition(f_disk, DSK_POSITION_BAM);

	// Read the BAM
	char *magic_a;
	fread(&bam->directory, sizeof(DSK_Position), 1, f_disk);
	fread(&magic_a, sizeof(char), 2, f_disk);
	fread(&bam->entries, sizeof(uint32_t), MAX_TRACKS, f_disk);

	// Read the rest into the header string
	fread(bam->dir_header, sizeof(char), DIR_HEADER_SIZE-1, f_disk);

	return 0;
}

char *DSK_GetDescription(DSK_BAM bam) {
	static char buffer[128];
	buffer[0] = '\0';

	int bi = 0;
	int last_space = 0;
	for (int i=0; i<DIR_HEADER_SIZE && bam.dir_header[i] != '\0'; i++) {
		char c = bam.dir_header[i];
		if ((uint8_t) c == 0xA0) c = 0x00;
		if (c == ' ') {
			if (bi == 0) continue;
			last_space = bi;
		}
		buffer[bi++] = c;
	}
	if (last_space > 0) buffer[last_space] = '\0';

	return buffer;
}

char *DSK_GetName(DSK_BAM bam) {
	static char buffer[18];
	buffer[0] = '\0';

	char *full = DSK_GetDescription(bam);
	int i = 0;
	int last_space = 0;
	while (i < 17 && full[i] != '\0') {
		buffer[i] = full[i];
		if (full[i] == ' ') last_space = i;
		i++;
	}
	if (last_space == 0) last_space = i;
	buffer[last_space] = '\0';

	return buffer;
}

void DSK_PrintBAM(DSK_BAM bam) {
	printf(" BAM Contents:\n");
	printf("---------------------------------\n");
	printf(" Directory Postion: [% 3i/% 3i]\n", bam.directory.track, bam.directory.sector);
	for (int i=0; i<MAX_TRACKS; i++) {
		uint8_t sec_free = bam.entries[i] & 0xFF;
		int sec_total = DSK_Track_GetSectorCount(i+1);
		printf(" Track % 3i: (% 3i/% 3i free) - ", i+1, sec_free, sec_total);

		for (int s=0; s<sec_total; s++) {
			int is_free = (bam.entries[i] >> (8 + s)) & 1;
			printf("%c", is_free ? '_' : '#');
		}

		putchar('\n');
	}
}

void DSK_Sector_Draw(DSK_BAM bam, DSK_Position pos, DSK_DrawMode mode) {
	if (!DSK_IsPositionValid(pos)) return;

	int track_index = MAX_TRACKS - pos.track;
	int track_width = (DISK_RADIUS - SPINDLE_RADIUS)/MAX_TRACKS;
	float r_inner = SPINDLE_RADIUS + track_width * track_index;
	float r_outer = r_inner + track_width - TRACK_GAPS;
	double sector_angle = 360.0f / (double)(DSK_Track_GetSectorCount(pos.track));
	double start_angle = sector_angle * pos.sector - 90.0f;
	double end_angle = start_angle + sector_angle - (1.0f/(float)(track_index+5) * SECTOR_GAPS);

	switch (mode) {
		case DSK_DRAW_HIGHLIGHT:
		case DSK_DRAW_NORMAL: {
			DrawRing(
				(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y },
				r_inner, r_outer,
				start_angle, end_angle,
				ARC_RESOLUTION,
				DSK_Sector_GetStatusColour(DSK_Sector_GetStatus(bam, pos))
			);

			if (mode == DSK_DRAW_HIGHLIGHT) {
				DrawRing(
					(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y },
					r_inner, r_outer,
					start_angle, end_angle,
					ARC_RESOLUTION,
					(Color){ 0xFF, 0xFF, 0xFF, 0x80 }
				);
			}
		} return;

		case DSK_DRAW_SELECTED: {
			DrawRing(
				(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y },
				r_inner, r_outer,
				start_angle, end_angle,
				ARC_RESOLUTION,
				DSK_Sector_GetStatusColour(DSK_Sector_GetStatus(bam, pos))

			);
			DrawRing(
				(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y },
				r_inner + 2.0f, r_outer - 2.0f,
				start_angle + 0.5f, end_angle - 0.5f,
				ARC_RESOLUTION,
				WHITE
			);
		} return;
	}
}

DSK_SectorStatus DSK_Sector_GetStatus(DSK_BAM bam, DSK_Position pos) {
	if (!DSK_IsPositionValid(pos)) return SECTOR_INVALID;
	
	int is_free = (bam.entries[pos.track - 1] >> (8 + pos.sector)) & 1;
	if (is_free) return SECTOR_FREE;
	else return SECTOR_IN_USE;
}

const char *DSK_Sector_GetStatusName(DSK_SectorStatus status) {
	switch (status) {
		case SECTOR_FREE: return "Sector Free";
		case SECTOR_IN_USE: return "Sector in use";
		default: return "Invalid Sector";
	}
}


Color DSK_Sector_GetStatusColour(DSK_SectorStatus status) {
	switch (status) {
		case SECTOR_INVALID: return MAGENTA;
		case SECTOR_FREE: return BLACK;
		case SECTOR_IN_USE: return GREEN;
		default: return (Color){ 0x00, 0x00, 0x00, 0x00 };
	}
}

DSK_SectorInfo DSK_Sector_GetFullInfo(DSK_BAM bam, FILE *f_disk, DSK_Position pos) {
	DSK_SectorInfo info = {
		.type = SECTYPE_INVALID,
		.status = SECTOR_INVALID,
		.checksum_original = 0x0000,
		.checksum_calculated = 0xFFFF
	};
	if (f_disk == NULL) return info;

	info.status = DSK_Sector_GetStatus(bam, pos);

	//	TODO: Parse the Directory and sort out the disk structure
	//	For now:
	if (pos.track == 18) {
		if (pos.sector == 0) info.type = SECTYPE_BAM;
		else info.type = SECTYPE_DIR;
	} else {
		if (info.status == SECTOR_IN_USE) info.type = SECTYPE_USR;
		if (info.status == SECTOR_FREE) info.type = SECTYPE_NONE;
	}

	//DSK_File_SeekPosition(f_disk, pos);

	return info;
}

const char *DSK_Sector_GetTypeName(DSK_SectorType type) {
	switch(type) {
		case SECTYPE_INVALID: return "Invalid Sector Type";
		case SECTYPE_NONE: return "-";
		case SECTYPE_BAM: return "Block-Availability Map";
		case SECTYPE_DIR: return "Directory Table";
		case SECTYPE_DEL: return "Deleted Block";
		case SECTYPE_SEQ: return "Sequential Data Block";
		case SECTYPE_PRG: return "Program Block";
		case SECTYPE_USR: return "User Data Block";
		case SECTYPE_REL: return "Relative Data Block";
	}
	return "";
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

