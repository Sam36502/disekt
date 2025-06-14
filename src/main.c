
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <raylib.h>

#include "../include/debug.h"
#include "../include/disk.h"
#include "../include/recon.h"

#define FRAMERATE 30		// FPS

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 1000

#define KEY_TOGGLE_HEX_MODE 290


void usage();


int main(int argc, char *argv[]) {

	// Read input file
	if (argc != 2) {
		printf("No input file provided!\n");
		usage();
		return 1;
	}

	const char *disk_filename = argv[1];
	FILE *f_disk = fopen(disk_filename, "rb");
	if (f_disk == NULL) {
		printf("Failed to read input file '%s'\n", argv[1]);
		usage();
		return 1;
	}


	// TODO: parse options and handle parsing nyblog to disk image
	// ---- Read nyblog
	//NYB_DataBlock blockbuf[1024]; 
	//for (int i=0; i<1024; i++) blockbuf[i].track_num = 0;
	//NYB_ParseLog(argv[1], blockbuf, 1024);
	//NYB_WriteToDiskImage( ... );


	DSK_Directory dir;
	int err = DSK_File_ParseDirectory(f_disk, &dir);
	if (err != 0) {
		printf("Failed to Parse BAM");
		usage();
	}

	DSK_PrintBAM(dir.bam);
	DSK_PrintDirectory(dir);
	fflush(stdout);

	//	Initialise Raylib
	InitWindow(
		SCREEN_WIDTH, SCREEN_HEIGHT,
		"Disekt"
	);
	SetTargetFPS(FRAMERATE);

	// Main Loop
	bool redraw = true;
	bool hex_mode = false;
	DSK_Position curr_sector = DSK_POSITION_BAM;
	char *name = DSK_GetName(dir);
	REC_Analysis analysis;
	REC_AnalyseDisk(f_disk, dir, &analysis);

	uint8_t curr_data[BLOCK_SIZE];
	DSK_File_GetData(f_disk, curr_sector, curr_data, BLOCK_SIZE);
	fclose(f_disk);

	while (!WindowShouldClose()) {

		// Handle inputs
		DSK_Position hov = DSK_GetHoveredSector();
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			curr_sector = hov;

			f_disk = fopen(disk_filename, "rb");
			DSK_File_GetData(f_disk, curr_sector, curr_data, BLOCK_SIZE);
			fclose(f_disk);
		}

		int key = GetKeyPressed();
		if (true || key != 0) {
			if (key == KEY_TOGGLE_HEX_MODE) { hex_mode = !hex_mode; }
			redraw = true;

			redraw |= DEBUG_HandleEvents(key);
		}
		
		if (redraw) {
			BeginDrawing();

			// Clear Background
			ClearBackground(RAYWHITE);

			// Draw Disk-Sectors
			for (int t=1; t<=35; t++) {
				for (int s=0; s<21; s++) {
					DSK_Position pos = { t, s };
					DSK_DrawMode dm = DSK_DRAW_NORMAL;

					if (pos.track == hov.track && pos.sector == hov.sector) dm = DSK_DRAW_HIGHLIGHT;
					if (pos.track == curr_sector.track && pos.sector == curr_sector.sector) dm = DSK_DRAW_SELECTED;
					
					REC_Status stat = REC_GetStatus(analysis, pos);
					DSK_Sector_Draw(dir, pos, dm, REC_GetStatusColour(stat));
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
			if (DSK_IsPositionValid(hov)) {
				snprintf(buf, 256, "[% 3i/% 3i]", hov.track, hov.sector);
			} else {
				snprintf(buf, 256, "[---/---]");
			}
			DrawText(buf, 10 + (9 * 12), SCREEN_HEIGHT - 10 - 20, 20, BLACK);


			// Draw Sector Info
			//DSK_SectorInfo info = DSK_Sector_GetFullInfo(dir, f_disk, curr_sector); 
			//REC_Entry info = REC_Get(dir, f_disk, curr_sector); 
			REC_Status stat = REC_GetStatus(analysis, curr_sector); 
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

			snprintf(buf, 256, "%16s", "Sector Type:");
			DrawText(buf, info_x + 10, 10 + (line_num * 20), 20, BLACK);
			//snprintf(buf, 256, "%s", DSK_Sector_GetTypeName(info.type));
			//DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, ORANGE);

			snprintf(buf, 256, "%16s", "Sector Status:");
			DrawText(buf, info_x + 10, 10 + (line_num * 20), 20, BLACK);
			snprintf(buf, 256, "%s", REC_GetStatusName(stat));
			DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, REC_GetStatusColour(stat));

			// Draw specific sector info
			line_num = 10;
			if (curr_sector.track == 18 && curr_sector.sector == 0) {
				//case SECTYPE_BAM: {
					snprintf(buf, 256, "Full Directory Header:");
					DrawText(buf, info_x + 20, 10 + (line_num++ * 20), 20, ORANGE);
					line_num++;

					REC_DrawData(
						info_x + 20, 10 + (line_num * 20),
						dir.header, DIR_HEADER_SIZE,
						hex_mode, false
					);
				//} break;
				//default: break;
			}

			// Display Sector contents
			line_num = 31;
			snprintf(buf, 256, "Sector Contents:");
			DrawText(buf, info_x + 20, 10 + (line_num++ * 20), 20, ORANGE);
			line_num++;

			REC_DrawData(
				info_x + 20, 10 + (line_num * 20),
				curr_data, BLOCK_SIZE,
				hex_mode, true
			);

			DEBUG_DrawDevInfo();

			EndDrawing();
			redraw = false;
		}
	}

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
