#include "../include/recon.h"


uint16_t REC_Checksum(void *ptr) {
	uint8_t lo = 0x00;
	uint8_t hi = 0x00;

	uint8_t *bp = ptr;
	for (int i=0; i<256; i++) {
		lo += bp[i];
		hi += lo;
	}

	return (uint16_t)(hi) << 8 | (uint16_t)(lo);
}


REC_File *REC_ReadFile(char *filename) {
	if (filename == NULL) return NULL;

	REC_File *rec = malloc(sizeof(REC_File));
	rec->path = NULL;
	rec->disk_path = NULL;

	// Open the file
	size_t fnlen = strlen(filename);
	rec->path = malloc(fnlen + 1);
	strlcpy(rec->path, filename, fnlen);
	FILE *f = fopen(filename, "rb");

	// Check for the 4 magic bytes of the rec file header
	uint8_t magic[4];
	fread(magic, sizeof(uint8_t), 4, f);
	for (int i=0; i<4; i++) {
		if (magic[i] != REC_MAGIC_BYTES[i]) {
			fclose(f);
			REC_CloseFile(rec);
			return NULL;
		}
	}

	// Read region offsets
	fread(&rec->offs_entries, sizeof(uint32_t), 1, f);
	fread(&rec->offs_data, sizeof(uint32_t), 1, f);
	fread(&rec->offs_strings, sizeof(uint32_t), 1, f);

	// Try to read the disk file path
	

	return 0;
}

void REC_CloseFile(REC_File *rec) {
	if (rec == NULL) return;

	//if (rec->disk_path
	
}

int REC_WriteFile(REC_File *rec) {
	if (rec == NULL) return 1;

	return 0;
}

REC_Entry REC_FindEntry(REC_File *rec, REC_RecordType type, int index) {
	if (rec == NULL) return (REC_Entry){};

	return (REC_Entry){};
}

