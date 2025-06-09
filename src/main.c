
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <raylib.h>

#include "../include/debug.h"
#include "../include/disk.h"

#define FRAMERATE 30		// FPS

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 1000


void usage();


int main(int argc, char *argv[]) {

	// Read input file
	if (argc != 2) {
		printf("No input file provided!\n");
		usage();
		return 1;
	}
	FILE *f_disk = fopen(argv[1], "rb");
	if (f_disk == NULL) {
		printf("Failed to read input file '%s'\n", argv[1]);
		usage();
		return 1;
	}

	// ---- Read nyblog
	//NYB_DataBlock blockbuf[1024]; 
	//for (int i=0; i<1024; i++) blockbuf[i].track_num = 0;
	//NYB_ParseLog(argv[1], blockbuf, 1024);

	//// --- Write to test disk image
	//FILE *f_disk = fopen("disks/testout.d64", "wb");

	//for (int i=0; i<1024; i++) {
	//	NYB_DataBlock block = blockbuf[i];
	//	if (block.track_num < 1 || block.track_num > 35) continue;

	//	printf("[% 5i] Writing Block (% 3i/% 3i); 0x%04X; Err#%i\n", i, block.track_num, block.sector_index, block.checksum, block.err_code);
	//	uint16_t chk = REC_Checksum(block.data);
	//	printf("       Recalculated checksum: 0x%04X [%s]\n", chk, (chk == block.checksum) ? "MATCH" : "FAIL");

	//	DSK_File_SeekPosition(f_disk, (DSK_Position){ block.track_num, block.sector_index });
	//	fwrite(block.data, sizeof(uint8_t), BLOCK_SIZE, f_disk);
	//}

	//fclose(f_disk);

	//return 0;

	DSK_BAM bam;
	DSK_File_ParseBAM(f_disk, &bam);
	//DSK_PrintBAM(bam);
	fflush(stdout);

	g_debug_int += 5;

	//	Initialise Raylib
	InitWindow(
		SCREEN_WIDTH, SCREEN_HEIGHT,
		"Disekt"
	);
	SetTargetFPS(FRAMERATE);

	// Main Loop
	bool redraw = true;
	DSK_Position curr_sector = DSK_POSITION_BAM;
	char *name = DSK_GetName(bam);

	while (!WindowShouldClose()) {

		redraw |= DEBUG_HandleEvents();
		redraw = true;
		
		if (redraw) {
			BeginDrawing();

			// Clear Background
			ClearBackground(RAYWHITE);
			DSK_Position hov = DSK_GetHoveredSector();
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) curr_sector = hov;

			// Draw Disk-Sectors
			for (int t=1; t<=35; t++) {
				for (int s=0; s<21; s++) {
					DSK_Position pos = { t, s };
					DSK_DrawMode dm = DSK_DRAW_NORMAL;

					if (pos.track == hov.track && pos.sector == hov.sector) dm = DSK_DRAW_HIGHLIGHT;
					if (pos.track == curr_sector.track && pos.sector == curr_sector.sector) dm = DSK_DRAW_SELECTED;

					DSK_Sector_Draw(bam, pos, dm);
				}
			}
			// Draw Title
			char buf[256];
			snprintf(buf, 256, "%s", name);
			DrawText(buf, 10, 10, 20, RED);
			snprintf(buf, 256, "\"%s\"", argv[1]);
			DrawText(buf, 10, 30, 20, BLACK);

			// Draw Currently selected sector pos & hovered sector pos
			snprintf(buf, 256, "[% 3i/% 3i]", curr_sector.track, curr_sector.sector);
			DrawText(buf, 10, SCREEN_HEIGHT - 10 - 20, 20, ORANGE);
			snprintf(buf, 256, "[% 3i/% 3i]", hov.track, hov.sector);
			DrawText(buf, 10 + (9 * 12), SCREEN_HEIGHT - 10 - 20, 20, BLACK);


			// Draw Sector Info
			DSK_SectorInfo info = DSK_Sector_GetFullInfo(bam, f_disk, curr_sector); 
			const int info_x = DISK_CENTRE_X * 2;
			const int info_tab_x = info_x + (16 * 12);
			DrawLineEx(
				(Vector2){ info_x, 10 },
				(Vector2){ info_x, SCREEN_HEIGHT - 10 },
				2.0f, BLACK
			);
			DrawLineEx(
				(Vector2){ info_x + 10, 10 + 26 },
				(Vector2){ SCREEN_WIDTH - 10, 10 + 26 },
				2.0f, BLACK
			);

			int line_num = 0;
			snprintf(buf, 256, "Current Sector Info:");
			DrawText(buf, info_x + 10, 10 + (line_num++ * 20), 20, BLACK);
			line_num++;

			snprintf(buf, 256, "% 16s", "Sector Type:");
			DrawText(buf, info_x + 10, 10 + (line_num * 20), 20, BLACK);
			snprintf(buf, 256, "%s", DSK_Sector_GetTypeName(info.type));
			DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, ORANGE);

			snprintf(buf, 256, "% 16s", "Sector Status:");
			DrawText(buf, info_x + 10, 10 + (line_num * 20), 20, BLACK);
			snprintf(buf, 256, "%s", DSK_Sector_GetStatusName(info.status));
			DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, DSK_Sector_GetStatusColour(info.status));

			// Draw specific sector info
			line_num = 10;
			switch (info.type) {
				case SECTYPE_BAM: {
					snprintf(buf, 256, "Full Directory Header:");
					DrawText(buf, info_x + 10, 10 + (line_num++ * 20), 20, ORANGE);
					line_num++;

					for (int i=0; i<DIR_HEADER_SIZE; i++) {
						int bi = i & 0b1111;

						int c = bam.dir_header[i];
						Color clr = BLACK;
						if (isspace(c) || !isprint(c)) {
							clr = LIGHTGRAY;
							if (isspace(c)) c = '.';
						}

						if (bi == 0) {
							DrawTextCodepoint(GetFontDefault(), '|',
								(Vector2){ info_x + 10, 10 + (line_num * 20) },
								20, BLACK
							);
						}

						DrawTextCodepoint(GetFontDefault(), c,
							(Vector2){ info_x + 20 + (bi * 20), 10 + (line_num * 20) },
							20, clr
						);

						if (bi == 15) {
							DrawTextCodepoint(GetFontDefault(), '|',
								(Vector2){ info_x + 20 + (16 * 20), 10 + (line_num++ * 20) },
								20, BLACK
							);
						}
					}

					DrawTextCodepoint(GetFontDefault(), '|',
						(Vector2){ info_x + 20 + (16 * 20), 10 + (line_num++ * 20) },
						20, BLACK
					);

				} break;
				default: break;
			}


			DEBUG_DrawDevInfo();

			EndDrawing();
			redraw = false;
		}
	}

	// Close the disk file
	fclose(f_disk);

	// Terminate Raylib
	CloseWindow();
	return 0;
}

void usage() {
	printf("Usage: disekt <disk path>\n");
	printf("\n");
	printf("Args:\n");
	printf("  disk path		The path to a C64 disk file to read the BAM of\n");
	printf("\n");
	printf("Example:\n");
	printf("  disekt test_disk.d64\n");
}
