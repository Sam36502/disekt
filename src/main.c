
#include <stdio.h>
#include <stdbool.h>
#include <raylib.h>

#include "../include/debug.h"
//#include "../include/arc.h"

#include "../include/disk.h"
//#include "../include/recon.h"
//#include "../include/nyblog.h"

#define EVENT_TIMEOUT 10	// ms
#define FRAMERATE 30		// FPS

#define BLOCK_SIZE 0x100	// Size of a disk sector (block) in bytes
#define BAM_ADDRESS 0x16500	// Offset in bytes from the start of a disk to the BAM

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 1000

#define CLR_BACKGROUND	0xFF, 0xFF, 0xFF		// White
#define CLR_SECT_FREE	0x00, 0x00, 0x00		// Black
#define CLR_SECT_FULL	0xFF, 0x20, 0x10		// Red

//void err_msg(const char *msg);
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
	DSK_Position selected_sector = POSITION_BAM;

	while (!WindowShouldClose()) {

		redraw |= DEBUG_HandleEvents();
		redraw = true;
		
		if (redraw) {
			BeginDrawing();

			// Clear Background
			ClearBackground(WHITE);
			DSK_Position hov = DSK_GetHoveredSector();

			for (int t=1; t<=35; t++) {
				for (int s=0; s<21; s++) {
					DSK_Position pos = { t, s };

					if (pos.track == hov.track && pos.sector == hov.sector) {
						DSK_Sector_Highlight(bam, hov);
					} else {
						DSK_Sector_Draw(bam, pos);
					}

				}
			}


			//DrawRing((Vector2){ 500.0f, 400.0f },
			//		20.0f, 220.0f,
			//		0.0f, (360.0f / 100.0f) * g_debug_int,
			//		ARC_RESOLUTION, RED
			//);

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
