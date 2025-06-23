
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <raylib.h>

#include "../include/debug.h"
#include "../include/disk.h"
#include "../include/analysis.h"
#include "../include/nyblog.h"


#define VERSION "0.4.2"
#define FRAMERATE 60		// FPS
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 1000

#define KEY_TOGGLE_HEX_MODE 290
#define KEY_TOGGLE_VIEW_MODE 291
#define KEY_ARROW_RIGHT 262
#define KEY_ARROW_LEFT 263
#define KEY_ARROW_DOWN 264
#define KEY_ARROW_UP 265

#define CLR_ACCENT BLUE


// Function Declarations
void draw_text(const char *text, int x, int y, int align, Color clr);
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


	// Perform Disk Analysis
	ANA_DiskInfo analysis;
	err = ANA_AnalyseDisk(f_disk, f_meta, dir, &analysis);
	if (err != 0) {
		printf("Failed to analyse disk: Err-code %i\n", err);
		return EXIT_FAILURE;
	}
	if (f_meta != NULL) fclose(f_meta);
	fclose(f_disk);


	//	Main Drawing Loop

	DSK_Position curr_pos = DSK_POSITION_BAM;
	ANA_SectorInfo curr_sector;
	uint16_t curr_checksum = 0x0000;
	err = ANA_GetInfo(analysis, curr_pos, &curr_sector);
	if (err != 0) {
		printf("Failed to get info for current sector (% 3i/% 3i): Err-code %i\n", curr_pos.track, curr_pos.sector, err);
		return EXIT_FAILURE;
	} else {
		curr_checksum = DSK_Checksum(curr_sector.data);
	}
	char *name = DSK_GetName(dir);

	const int info_x = DISK_CENTRE_X * 2;
	const int info_tab_x = info_x + (16 * 12);

	enum {
		VIEW_SECSTAT, 
		VIEW_BAM, 
		VIEW_TRANSFER, 
		VIEW_SECTYPE, 
		VIEW_FILES, 
		VIEW_INVALID,
	} view_mode = VIEW_SECSTAT;
	bool redraw = true;
	bool hex_mode = false;
	while (!WindowShouldClose()) {

		// Handle inputs
		DSK_Position hov = DSK_GetHoveredSector();
		bool sector_changed = false;

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			if (DSK_IsPositionValid(hov)) {
				curr_pos = hov;
				sector_changed = true;
			}

			switch (curr_sector.type) {
				case SECTYPE_DIR: {
					for (int i=0; i<dir.num_entries; i++) {
						Rectangle rect = {
							info_x + 30, 10 + ((15+i) * 20),
							50 + MeasureText(dir.entries[i].filename, 20), 20,
						};
						//DrawRectangleRec(rect, (Color){ 0xFF, 0x80, 0x40, 0x80 });
						if (CheckCollisionPointRec(GetMousePosition(), rect)) {
							curr_pos = dir.entries[i].head_pos;
							sector_changed = true;
						}
					}
				} break;

				case SECTYPE_PRG:
				case SECTYPE_SEQ:
				case SECTYPE_REL:
				case SECTYPE_DEL:
				case SECTYPE_USR: {

					// Next Button
					if (curr_sector.file_index < curr_sector.dir_entry.block_count - 1) {
						Rectangle rect = {
							info_x + 10, 10 + (20 * 20),
							40 + MeasureText("Next Block", 20), 40,
						};
						if (CheckCollisionPointRec(GetMousePosition(), rect)) {
							// TODO: Map file blocks previous/next positions
							for (int i=0; i<MAX_ANALYSIS_ENTRIES; i++) {
								if (analysis.sectors[i].dir_index != curr_sector.dir_index) continue;
								if (analysis.sectors[i].file_index != curr_sector.file_index+1) continue;

								curr_pos = analysis.sectors[i].pos;
								sector_changed = true;
								break;
							}
						}

					}

				} break;

				default: break;
			}

		}


		int key = GetKeyPressed();
		if (true || key != 0) {
			switch (key) {
				case KEY_TOGGLE_HEX_MODE: { hex_mode = !hex_mode; } break;
				case KEY_TOGGLE_VIEW_MODE: { view_mode++; if (view_mode >= VIEW_INVALID) view_mode = 0; } break;
				case KEY_ARROW_UP: { if (curr_pos.track < 35) curr_pos.track++; sector_changed = true; } break;
				case KEY_ARROW_DOWN: { if (curr_pos.track > 1) curr_pos.track--; sector_changed = true; } break;
				case KEY_ARROW_LEFT: { curr_pos.sector--; sector_changed = true; } break;
				case KEY_ARROW_RIGHT: { curr_pos.sector++; sector_changed = true; } break;
			}
			int secs = DSK_Track_GetSectorCount(curr_pos.track);
			if (curr_pos.sector > 0x80) curr_pos.sector = secs - 1;
			if (curr_pos.sector >= secs) curr_pos.sector = 0;

			redraw = true;

			redraw |= DEBUG_HandleEvents(key);
		}

		if (sector_changed) {
			err = ANA_GetInfo(analysis, curr_pos, &curr_sector);
			if (err != 0) {
				printf("Failed to get info for current sector (% 3i/% 3i): Err-code %i\n", curr_pos.track, curr_pos.sector, err);
				return EXIT_FAILURE;
			} else {
				curr_checksum = DSK_Checksum(curr_sector.data);
			}
		}

		if (redraw) {
			BeginDrawing();

			// Clear Background
			ClearBackground(RAYWHITE);

			int hov_dir_index = -1;
			ANA_SectorInfo hov_info;
			err = ANA_GetInfo(analysis, hov, &hov_info);
			if (err == 0) hov_dir_index = hov_info.dir_index;

			// Draw Disk-Sectors
			for (int t=MIN_TRACKS; t<=MAX_TRACKS; t++) {
				int sc = DSK_Track_GetSectorCount(t);
				for (int s=0; s<sc; s++) {
					DSK_Position pos = { t, s };
					DSK_DrawMode dm = DSK_DRAW_NORMAL;

					ANA_SectorInfo info;
					err = ANA_GetInfo(analysis, pos, &info);
					if (err != 0) {
						DSK_Sector_Draw(dir, pos, dm, MAGENTA);
						continue;
					}

					if (pos.track == hov.track && pos.sector == hov.sector) {
						dm = DSK_DRAW_HIGHLIGHT;
					}
					if (pos.track == curr_pos.track && pos.sector == curr_pos.sector) {
						dm = DSK_DRAW_SELECTED;
					}

					Color clr = GRAY;
					switch (view_mode) {
						case VIEW_INVALID: view_mode = VIEW_SECSTAT; // Fall-through
						case VIEW_SECSTAT: clr = REC_GetStatusColour(info.status); break;

						case VIEW_BAM: {
							if (!info.is_free) {
								if (info.has_data && !info.is_blank) clr = GREEN;
								else clr = RED;
							} else {
								if (!info.has_data || info.is_blank) clr = LIGHTGRAY;
								else clr = BLACK;
							}
						} break;

						case VIEW_TRANSFER: {
							if (info.has_transfer_info && info.has_data) {
								if (info.parse_err == 0x00) {
									if (info.disk_err == 0x80 && info.checksum_match) clr = GREEN;
									else clr = RED;
								} else {
									clr = RED;
								}
							} else {
								clr = LIGHTGRAY;
							}
						} break;

						case VIEW_SECTYPE: clr = DSK_Sector_GetTypeColour(info.type); break;
						case VIEW_FILES: clr = ANA_GetFileColour(dir, info,
							(hov_dir_index == info.dir_index),
							(curr_sector.dir_index == info.dir_index)
						); break;
					}

					DSK_Sector_Draw(dir, pos, dm, clr);
				}
			}

			// Draw Title
			draw_text(name, 10, 10, -1, CLR_ACCENT);
			draw_text(TextFormat("\"%s\"", disk_filename),
				10, 30, -1, BLACK
			);

			// Draw Currently selected sector pos & hovered sector pos
			if (DSK_IsPositionValid(hov)) {
				draw_text(TextFormat("[% 3i/% 3i]", hov.track, hov.sector),
					10, SCREEN_HEIGHT - 50, -1, BLACK
				);
			} else {
				draw_text("[---/---]",
					10, SCREEN_HEIGHT - 50, -1, LIGHTGRAY
				);
			}
			draw_text(TextFormat("[% 3i/% 3i]", curr_pos.track, curr_pos.sector),
				10, SCREEN_HEIGHT - 30, -1, CLR_ACCENT
			);

			// Draw current view mode name
			draw_text("View Mode [F2]",
				info_x - 10, SCREEN_HEIGHT - 50, 1, BLACK
			);
			char *mode_name = "";
			switch (view_mode) {
				case VIEW_INVALID: view_mode = VIEW_SECSTAT; // Fall-through
				case VIEW_SECSTAT: mode_name = "Sector Status"; break;
				case VIEW_BAM: mode_name = "Missing Sectors"; break;
				case VIEW_TRANSFER: mode_name = "Transfer Errors"; break;
				case VIEW_SECTYPE: mode_name = "Sector Type"; break;
				case VIEW_FILES: mode_name = "File Blocks"; break;
			}
			draw_text(mode_name,
				info_x - 10, SCREEN_HEIGHT - 30, 1, CLR_ACCENT
			);

			// TODO: Draw full disk usage & analysis stats in top-right corner


			// Draw Sector Info

			DrawLineEx(
				(Vector2){ info_x, 10 },
				(Vector2){ info_x, SCREEN_HEIGHT - 10 },
				2.0f, BLACK
			);

			int line_num = 0;
			draw_text("Current Sector Info:",
					info_x + 10, 10 + (line_num++ * 20), -1, BLACK
			);

			DrawLineEx(
				(Vector2){ info_x + 10, 18 + (line_num * 20) },
				(Vector2){ SCREEN_WIDTH - 10, 18 + (line_num * 20) },
				2.0f, BLACK
			); line_num++;

			draw_text("Sector Type:",
					info_tab_x - 5, 10 + (line_num * 20), 1, BLACK
			);
			draw_text(DSK_Sector_GetTypeName(curr_sector.type),
					info_tab_x + 5, 10 + (line_num++ * 20), -1, DSK_Sector_GetTypeColour(curr_sector.type)
			);

			draw_text("Checksum:",
					info_tab_x - 5, 10 + (line_num * 20), 1, BLACK
			);
			if (curr_sector.has_transfer_info) {
				draw_text(TextFormat("0x%04X - 0x%04X [%s]",
						curr_checksum, curr_sector.checksum,
						(curr_checksum == curr_sector.checksum) ? "MATCH":"BREAK"
					), info_tab_x + 5, 10 + (line_num++ * 20), -1,
					(curr_checksum == curr_sector.checksum) ? GREEN:RED
				);
			} else if (curr_sector.has_data) {
				draw_text(TextFormat("0x%04X", curr_checksum),
					info_tab_x + 5, 10 + (line_num++ * 20), -1,
					GRAY
				);
			} else {
				draw_text("-",
					info_tab_x + 5, 10 + (line_num++ * 20), -1,
					GRAY
				);
			}
			line_num++;

			if (curr_sector.has_transfer_info) {
				draw_text("Disk Error:",
						info_tab_x - 5, 10 + (line_num * 20), 1, BLACK
				);

				uint8_t e = curr_sector.disk_err & ~0x80;
				draw_text(TextFormat("0x%02X (%i)", e, e),
					info_tab_x + 5, 10 + (line_num++ * 20), -1,
					ANA_GetDiskErrorColour(curr_sector.disk_err)
				);
				draw_text(ANA_GetDiskErrorName(curr_sector.disk_err),
					info_tab_x + 5, 10 + (line_num++ * 20), -1,
					ANA_GetDiskErrorColour(curr_sector.disk_err)
				);
				line_num++;

				e = curr_sector.parse_err;
				draw_text("Parse Error:",
						info_tab_x - 5, 10 + (line_num * 20), 1, BLACK
				);
				draw_text(TextFormat("0x%02X (%i)", e, e),
					info_tab_x + 5, 10 + (line_num++ * 20), -1,
					ANA_GetParseErrorColour(e)
				);
				draw_text(ANA_GetParseErrorName(e),
					info_tab_x + 5, 10 + (line_num++ * 20), -1,
					ANA_GetParseErrorColour(e)
				);
				line_num++;
			}

			draw_text("Sector Status:",
				info_tab_x - 5, 10 + (line_num * 20), 1, BLACK
			);
			draw_text(REC_GetStatusName(curr_sector.status),
				info_tab_x + 5, 10 + (line_num++ * 20), -1,
				REC_GetStatusColour(curr_sector.status)
			);

			// Draw specific sector info
			line_num = 12;
			DrawLineEx(
				(Vector2){ info_x + 10, 19 + (line_num * 20) },
				(Vector2){ SCREEN_WIDTH - 10, 19 + (line_num * 20) },
				2.0f, BLACK
			); line_num++;
			switch (curr_sector.type) {

				case SECTYPE_BAM: {
					draw_text("Full Header Text:",
						info_x + 10, 10 + (line_num++ * 20), -1,
						BLACK
					);
					line_num++;

					char *desc = DSK_GetDescription(dir);
					int desclen = strlen(desc);
					char *str = desc;
					int prev = 0;
					for (int i=0; i<desclen; i++) {
						if (i < desclen-1 && i-prev < 40 && desc[i] != '\n') continue;
						if (i >= desclen-1 || desc[i] == '\n') i++;
						int len = i - prev;

						draw_text(TextFormat("%.*s", len, str),
							info_x + 20, 10 + (line_num++ * 20), -1,
							GRAY
						);

						prev = i;
						str = desc + i;
					}
				} break;

				case SECTYPE_DIR: {
					draw_text("Directory Files:",
						info_x + 10, 10 + (line_num++ * 20), -1,
						BLACK
					);
					line_num++;

					for (int i=0; i<dir.num_entries; i++) {
						Color clr = GRAY;
						Rectangle rect = {
							info_x + 30, 10 + (line_num * 20),
							50 + MeasureText(dir.entries[i].filename, 20), 20,
						};

						if (CheckCollisionPointRec(GetMousePosition(), rect)) clr = CLR_ACCENT;

						DrawPoly((Vector2){ info_x + 40, 19 + (line_num * 20) },
							4, 5, 0, clr
						);
						draw_text(dir.entries[i].filename,
							info_x + 60, 10 + (line_num++ * 20), -1,
							clr
						);
					}
				} break;

				case SECTYPE_PRG:
				case SECTYPE_REL:
				case SECTYPE_USR:
				case SECTYPE_SEQ: {
					draw_text("File Information:",
						info_x + 10, 10 + (line_num++ * 20), -1,
						BLACK
					);
					line_num++;

					draw_text(curr_sector.dir_entry.filename,
						info_x + 20, 10 + (line_num++ * 20), -1,
						DSK_Sector_GetTypeColour(curr_sector.dir_entry.type)
					);
					draw_text(TextFormat("Block %i / %i", curr_sector.file_index+1, curr_sector.dir_entry.block_count),
						info_x + 20, 10 + (line_num++ * 20), -1,
						BLACK
					);

					line_num = 20;
					if (curr_sector.file_index < curr_sector.dir_entry.block_count - 1) {
						Rectangle rect = {
							info_x + 10, 10 + (20 * 20),
							40 + MeasureText("Next Block", 20), 40,
						};
						Color clr = GRAY;
						if (CheckCollisionPointRec(GetMousePosition(), rect)) clr = CLR_ACCENT;
						draw_text(TextFormat("Next Block"),
							info_x + 30, 20 + (line_num++ * 20), -1, clr
						);
						DrawRectangleLinesEx(rect, 2, clr);
					}

				} break;

				default: break;
			}

			// Display Sector contents
			line_num = 30;
			DrawLineEx(
				(Vector2){ info_x + 10, 20 + (line_num * 20) },
				(Vector2){ SCREEN_WIDTH - 10, 20 + (line_num * 20) },
				2.0f, BLACK
			); line_num++;

			draw_text("Sector Contents:",
				info_x + 10, 10 + (line_num * 20), -1,
				BLACK
			);
			draw_text(hex_mode ? "HEX [F1]":"ASCII [F1]",
				SCREEN_WIDTH - 10, 10 + (line_num++ * 20), 1,
				CLR_ACCENT
			);

			DrawLineEx(
				(Vector2){ info_x + 10, 18 + (line_num * 20) },
				(Vector2){ SCREEN_WIDTH - 10, 18 + (line_num * 20) },
				2.0f, BLACK
			); line_num++;

			DSK_DrawData(
				info_x + 20, 10 + (line_num * 20),
				curr_sector.data, BLOCK_SIZE,
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


void draw_text(const char *text, int x, int y, int align, Color clr) {
	int len = MeasureText(text, 20);

	if (align != 0) align = align / abs(align);
	switch (align) {
		case -1: DrawText(text, x, y, 20, clr); return;
		case  0: DrawText(text, x - (len/2), y, 20, clr); return;
		case +1: DrawText(text, x - len, y, 20, clr); return;
	}

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
	printf("  disk path			The path to a Commodore 64 disk file to read\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help		Print this usage text and exit\n");
	printf("  -v, --version		Print the current version number and exit\n");
	printf("  -d, --debug		Enable more verbose logging for debugging\n");
	printf("  -l <filename>		Parse the text log file provided and write its\n");
	printf("					contents to the given disk file\n");
	printf("  -e <filename>		Include information from an external reconciliation\n");
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
