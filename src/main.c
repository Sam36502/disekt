
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <raylib.h>

Color __hsv_to_rgb(double, double, double);

#include "../include/debug.h"
#include "../include/disk.h"
#include "../include/recon.h"
#include "../include/nyblog.h"

#define VERSION "0.3.0"
#define FRAMERATE 60		// FPS
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 1000
#define KEY_TOGGLE_HEX_MODE 290
#define KEY_TOGGLE_VIEW_MODE 291


// Function Declarations
void parse_args(int argc, char *argv[], char **log_filename, char **recon_filename, char **disk_filename);
void usage();
void version();


int main(int argc, char *argv[]) {

	// Read Input Parameters

	char *disk_filename = NULL;
	char *log_filename = NULL;
	char *recon_filename = NULL;

	parse_args(argc, argv, &log_filename, &recon_filename, &disk_filename);
	if (disk_filename == NULL) {
		printf("Error: you must specify a disk file argument\n\n");
		usage();
	}

	if (g_verbose_log) {
		printf("Arguments:\n");
		printf("   Disk File: %s\n", disk_filename);
		printf("    Log File: %s\n", (log_filename == NULL) ? "Not Specified" : log_filename);
		printf("  Recon File: %s\n\n", (recon_filename == NULL) ? "Not Specified" : recon_filename);
	} else {
		SetTraceLogLevel(LOG_WARNING);
	}

	// Read log file if specified
	if (log_filename != NULL) {

		// Parse log file into blocks
		NYB_DataBlock block_buf[256];
		long offset = 0;
		
		FILE *f_meta = NULL;
		if (recon_filename != NULL) {
			if (FileExists(recon_filename)) {
				f_meta = fopen(recon_filename, "r+b");
			} else {
				f_meta = fopen(recon_filename, "wb");
			}

			if (f_meta == NULL) {
				printf("Error: Failed to open recon file '%s' for writing\n", recon_filename);
				usage();
			}
		}

		int blocks_read = 256;
		while (blocks_read >= 256) {
			blocks_read = NYB_ParseLog(log_filename, block_buf, 256, &offset);
			if (blocks_read < 0) {
				printf("Error: Failed to read log file '%s'\n", log_filename);
				usage();
			}

			// Write blocks to output disk image
			int err = NYB_WriteToDiskImage(disk_filename, block_buf, blocks_read);
			if (err != 0) {
				printf("Error: Failed to write to disk image '%s'\n", disk_filename);
				usage();
			}

			// Write to recon file if set
			if (f_meta != NULL) {
				for (int i=0; i<blocks_read; i++) {
					NYB_Meta_WriteBlock(f_meta, &(block_buf[i]));
				}
			}
		}

	}

	// Read the disk file
	FILE *f_disk = fopen(disk_filename, "rb");
	if (f_disk == NULL) {
		printf("Error: Failed to read input file '%s'\n", disk_filename);
		usage();
	}

	// Read the meta file
	FILE *f_meta = NULL;
	if (recon_filename != NULL) {
		f_meta = fopen(recon_filename, "rb");
		if (f_meta == NULL) {
			printf("Error: Failed to read input metadata file '%s'\n", recon_filename);
			usage();
		}
	}

	DSK_Directory dir;
	int err = DSK_File_ParseDirectory(f_disk, &dir);
	if (err != 0) {
		printf("Failed to Parse BAM");
		usage();
	}

	if (g_verbose_log) {
		DSK_PrintBAM(dir.bam);
		DSK_PrintDirectory(dir);
	}
	fflush(stdout);

	//	Initialisation
	InitWindow(
		SCREEN_WIDTH, SCREEN_HEIGHT,
		"Disekt"
	);
	SetTargetFPS(FRAMERATE);

	// Perform reconciliation analysis
	DSK_Position curr_sector = DSK_POSITION_BAM;
	REC_Analysis analysis;
	err = REC_AnalyseDisk(f_disk, f_meta, dir, &analysis);
	if (err != 0) printf("Failed to analyse disk: Err-code %i\n", err);

	uint8_t curr_data[BLOCK_SIZE];
	uint16_t curr_checksum = 0x0000;
	DSK_File_GetData(f_disk, curr_sector, curr_data, BLOCK_SIZE);
	curr_checksum = DSK_Checksum(curr_data);
	if (f_meta != NULL) fclose(f_meta);
	fclose(f_disk);

	bool redraw = true;
	bool hex_mode = false;
	enum {
		VIEW_SECTYPE = 0, 
		VIEW_SECSTAT = 1, 
		VIEW_FILES = 2, 
	} view_mode = VIEW_SECTYPE;
	char *name = DSK_GetName(dir);


	// Main Loop
	while (!WindowShouldClose()) {

		// Handle inputs
		DSK_Position hov = DSK_GetHoveredSector();
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			curr_sector = hov;

			f_disk = fopen(disk_filename, "rb");
			DSK_File_GetData(f_disk, curr_sector, curr_data, BLOCK_SIZE);
			curr_checksum = DSK_Checksum(curr_data);
			fclose(f_disk);
		}

		int key = GetKeyPressed();
		if (true || key != 0) {
			if (key == KEY_TOGGLE_HEX_MODE) { hex_mode = !hex_mode; }
			if (key == KEY_TOGGLE_VIEW_MODE) { view_mode++; view_mode %= 3; }
			redraw = true;

			redraw |= DEBUG_HandleEvents(key);
		}
		
		if (redraw) {
			BeginDrawing();

			// Clear Background
			ClearBackground(RAYWHITE);

			REC_Entry curr_info;
			REC_GetInfo(analysis, curr_sector, &curr_info);
			REC_Entry hov_info;
			REC_GetInfo(analysis, hov, &hov_info);

			// Draw Disk-Sectors
			for (int t=MIN_TRACKS; t<=MAX_TRACKS; t++) {
				int sc = DSK_Track_GetSectorCount(t);
				for (int s=0; s<sc; s++) {
					DSK_Position pos = { t, s };
					DSK_DrawMode dm = DSK_DRAW_NORMAL;

					REC_Entry info;
					REC_GetInfo(analysis, pos, &info);

					if (pos.track == hov.track && pos.sector == hov.sector) {
						dm = DSK_DRAW_HIGHLIGHT;
					}
					if (pos.track == curr_sector.track && pos.sector == curr_sector.sector) {
						dm = DSK_DRAW_SELECTED;
					}

					Color clr = GRAY;
					switch (view_mode) {
						case VIEW_SECTYPE: clr = DSK_Sector_GetTypeColour(info.type); break;
						case VIEW_SECSTAT: clr = REC_GetStatusColour(info.status); break;
						case VIEW_FILES: clr = REC_GetFileColour(dir, info,
							(hov_info.dir_index == info.dir_index),
							(curr_info.dir_index == info.dir_index)
						); break;
					}

					DSK_Sector_Draw(dir, pos, dm, clr);
				}
			}

			const int info_x = DISK_CENTRE_X * 2;
			const int info_tab_x = info_x + (16 * 12);

			// Draw Title
			char buf[256];
			snprintf(buf, 256, "%s", name);
			DrawText(buf, 10, 10, 20, RED);
			snprintf(buf, 256, "\"%s\"", disk_filename);
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

			// Draw current view mode name
			DrawText("View Mode [F2]", info_x - 10 - (14 * 11), SCREEN_HEIGHT - 10 - 45, 20, BLACK);
			switch (view_mode) {
				case 0: DrawText("Sector Type",
					info_x - 10 - (12 * 12), SCREEN_HEIGHT - 10 - 20,
					20, BLUE
				); break;
				case 1: DrawText("Sector Status",
					info_x - 10 - (13 * 12), SCREEN_HEIGHT - 10 - 20,
					20, ORANGE
				); break;
				case 2: DrawText("File Blocks",
					info_x - 10 - (10 * 12), SCREEN_HEIGHT - 10 - 20,
					20, GRAY
				); break;
			}

			// Draw Sector Info
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
			snprintf(buf, 256, "%s", DSK_Sector_GetTypeName(curr_info.type));
			DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, DSK_Sector_GetTypeColour(curr_info.type));

			snprintf(buf, 256, "%16s", "Sector Status:");
			DrawText(buf, info_x + 10, 10 + (line_num * 20), 20, BLACK);
			snprintf(buf, 256, "%s", REC_GetStatusName(curr_info.status));
			DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, REC_GetStatusColour(curr_info.status));

			snprintf(buf, 256, "%16s", "Checksum:");
			DrawText(buf, info_x + 10, 10 + (line_num * 20), 20, BLACK);
			if (recon_filename != NULL && (curr_info.disk_err & 0x80) != 0x00) {
				snprintf(buf, 256, "0x%04X [%s]", curr_checksum, (curr_checksum == curr_info.checksum) ? "MATCH":"BREAK");
				DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, REC_GetStatusColour(curr_info.status));
			} else {
				snprintf(buf, 256, "0x%04X", curr_checksum);
				DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, DARKGRAY);
			}

			snprintf(buf, 256, "%16s", "Disk Error:");
			DrawText(buf, info_x + 10, 10 + (line_num * 20), 20, BLACK);
			if (recon_filename != NULL && (curr_info.disk_err & 0x80) != 0) {
				uint8_t e = curr_info.disk_err & ~0x80;
				snprintf(buf, 256, "%i [%s]", e, (e == 0x00) ? "OK":"ERR");
				DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, REC_GetStatusColour(curr_info.status));
			} else {
				snprintf(buf, 256, "%i", curr_info.disk_err);
				DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, DARKGRAY);
			}

			// Draw specific sector info
			line_num += 2;
			switch (curr_info.type) {

				case SECTYPE_BAM: {
					snprintf(buf, 256, "Full Header Text:");
					DrawText(buf, info_x + 20, 10 + (line_num++ * 20), 20, BLACK);
					line_num++;

					char *desc = DSK_GetDescription(dir);
					int desclen = strlen(desc);
					char *str = desc;
					int prev = 0;
					for (int i=0; i<desclen; i++) {
						if (i < desclen-1 && i-prev < 40 && desc[i] != '\n') continue;
						if (i >= desclen-1 || desc[i] == '\n') i++;
						int len = i - prev;

						snprintf(buf, 256, "%.*s", len, str);
						DrawText(buf, info_x + 20, 10 + (line_num++ * 20), 20, DARKGRAY);

						prev = i;
						str = desc + i;
					}
				} break;

				//case SECTYPE_DIR: {
				//	snprintf(buf, 256, "Directory Table:");
				//	DrawText(buf, info_x + 20, 10 + (line_num++ * 20), 20, ORANGE);
				//	line_num++;

				//	REC_DrawData(
				//		info_x + 20, 10 + (line_num * 20),
				//		dir.header, DIR_HEADER_SIZE,
				//		hex_mode, false
				//	);
				//} break;

				case SECTYPE_PRG:
				case SECTYPE_REL:
				case SECTYPE_SEQ:
				case SECTYPE_PRG_CORPSE:
				case SECTYPE_REL_CORPSE:
				case SECTYPE_SEQ_CORPSE: {

					snprintf(buf, 256, "%16s", "File:");
					DrawText(buf, info_x + 10, 10 + (line_num * 20), 20, BLACK);
					snprintf(buf, 256, "%s", curr_info.dir_entry.filename);
					DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, ORANGE);

					snprintf(buf, 256, "Block %i / %i", curr_info.file_index+1, curr_info.dir_entry.block_count);
					DrawText(buf, info_tab_x, 10 + (line_num++ * 20), 20, BLACK);
				} break;


				default: break;
			}

			// Display Sector contents
			line_num = 31;
			snprintf(buf, 256, "Sector Contents:");
			DrawText(buf, info_x + 20, 10 + (line_num * 20), 20, BLACK);
			snprintf(buf, 256, "%6s [F1]", hex_mode ? "HEX":"ASCII");
			DrawText(buf, SCREEN_WIDTH - 10 - (10 * 11), 10 + (line_num++ * 20), 20, BLUE);
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

void parse_args(int argc, char *argv[], char **log_filename, char **recon_filename, char **disk_filename) {
	if (argc < 2) {
		printf("Error: at least one argument (disk filename) is required\n\n");
		usage();
		exit(EXIT_FAILURE);
	}

	for (int i=1; i<argc; i++) {
		char *curr_arg = argv[i];

		// End of options; this is the disk filename
		if (curr_arg[0] != '-') {
			*disk_filename = curr_arg;
			return;
		}
		curr_arg++;

		// Long option; search full word
		int len = strlen(curr_arg);
		if (len <= 0) continue;
		if (curr_arg[0] == '-') {
			curr_arg++;
			len--;
			if (len <= 0) continue;
			
			if (len >= 4 && strncmp(curr_arg, "help", len * sizeof(char)) == 0) usage();
			if (len >= 7 && strncmp(curr_arg, "version", len * sizeof(char)) == 0) version();
			if (len >= 5 && strncmp(curr_arg, "debug", len * sizeof(char)) == 0) { g_verbose_log = true; continue; };

			printf("Error: Unrecognised option '%s'; Skipping\n", curr_arg);

			continue;
		}

		// Short options
		char o = curr_arg[0];
		switch (o) {
			case 'h': usage(); break;
			case 'v': version(); break;
			case 'd': { g_verbose_log = true; } continue;

			case 'l': {
				if (len > 1) {
					*log_filename = curr_arg + 1;
				} else {
					if (i >= argc-1) {
						printf("Error: Log-File option (-l) requires a file argument\n\n");
						usage();
						exit(EXIT_FAILURE);
					}

					*log_filename = argv[i+1];
					i++;
				}
			} continue;

			case 'r': {
				if (len > 1) {
					*recon_filename = curr_arg + 1;
				} else {
					if (i >= argc-1) {
						printf("Error: Recon-File option (-r) requires a file argument\n\n");
						usage();
						exit(EXIT_FAILURE);
					}

					*recon_filename = argv[i+1];
					i++;
				}
			} continue;

			default: printf("Error: Unrecognised option '%c'; Skipping\n", o); continue;
		}

	}
}

void version() {
	printf("disekt version %s\n", VERSION);

	exit(EXIT_SUCCESS);
}

void usage() {
	printf("Usage: disekt [OPTIONS] <disk path>\n");
	printf("\n");
	printf("Args:\n");
	printf("  disk path			The path to a C64 disk file to read the BAM of\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help		Print this usage text and exit\n");
	printf("  -v, --version		Print the current version number and exit\n");
	printf("  -d, --debug		Enable more verbose logging for debugging\n");
	printf("  -l <filename>		Parse the text log file provided and write its\n");
	printf("					contents to the given disk file\n");
	printf("  -r <filename>		Include information from an external reconciliation\n");
	printf("					file. If provided with -l, the log writes the recon\n");
	printf("					data to this file before loading\n");
	printf("\n");
	printf("NOTE: All write operations will completely overwrite the provided file!\n");
	printf("\n");
	printf("Examples:\n");
	printf("  disekt test_disk.d64\n");
	printf("  disekt -l dump_log.txt -r test_disk.r64 test_disk.d64\n");
	printf("  disekt -r test_disk.r64 test_disk.d64\n");

	exit(EXIT_SUCCESS);
}
