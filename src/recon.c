#include "../include/recon.h"
#include <raylib.h>


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
	int cw = 20;
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

