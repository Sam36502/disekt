#include "../include/debug.h"
#include <raylib.h>

int g_debug_int = 0;
bool g_show_debug_view = 0;

bool DEBUG_HandleEvents() {
	int pressed = GetKeyPressed();
	//printf("---> Pressed char: '%c'\n", pressed);

	if (pressed == KEYCODE_DEBUG_INT_INC && g_debug_int < 100) { g_debug_int++; return true; }
	if (pressed == KEYCODE_DEBUG_INT_DEC && g_debug_int > 0) { g_debug_int--; return true; }
	if (pressed == KEYCODE_DEBUG_VIEW_TOGGLE) { g_show_debug_view = !g_show_debug_view; return true; }

	return false;
}

#define MIN_TRACKS 1		// Track numbers range from 1 - 35
#define MAX_TRACKS 35
#define SECTOR_SIZE 0x100	// A single disk sector is 256 Bytes
#define TRACK_GAPS 2		// How many pixels between each track
#define SECTOR_GAPS 8.0f	// How much of a gap to leave between each sector
#define SPINDLE_RADIUS 50	// in px
#define DISK_RADIUS 450		// in px
#define DISK_CENTRE_X 500	// Disk centre pos
#define DISK_CENTRE_Y 500

void DEBUG_DrawDevInfo() {
	if (!g_show_debug_view) return;

	int mouse_x = GetMouseX();
	int mouse_y = GetMouseY();
	double len_x = mouse_x - DISK_CENTRE_Y;
	double len_y = mouse_y - DISK_CENTRE_X;

	double hyp = sqrt((len_x * len_x) + (len_y * len_y));

	//	Draw line to cursor
	DrawLineEx(
		(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y }, 
		(Vector2){ mouse_x, DISK_CENTRE_Y },
		4, RED
	);
	DrawLineEx(
		(Vector2){ mouse_x, DISK_CENTRE_Y }, 
		(Vector2){ mouse_x, mouse_y },
		4, GREEN
	);
	DrawLineEx(
		(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y }, 
		(Vector2){ mouse_x, mouse_y },
		4, BLUE
	);

	// Calculate angle from mouse pos
	float oa = len_y / len_x;
	float mouse_angle;
	if (len_x < 0) {
		mouse_angle = 180.0f + atanf(oa) * (360.0f / (2*M_PI));
	} else {
		if (len_y < 0) {
			mouse_angle = 360.0f + atanf(oa) * (360.0f / (2*M_PI));
		} else {
			mouse_angle = 0.0f + atanf(oa) * (360.0f / (2*M_PI));
		}
	}

	// Draw mouse angle & radius
	DrawRing(
		(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y }, 
		0.0f, 20.0f,
		0.0f, mouse_angle,
		64, RED
	);
	DrawRing(
		(Vector2){ DISK_CENTRE_X, DISK_CENTRE_Y }, 
		hyp, hyp + 10.0f,
		0.0f, 360.0f,
		64, PURPLE
	);

	// Draw cursor info
	char buf[256];
	sprintf(buf, "Mouse Position: %i/%i\n", mouse_x, mouse_y);
	DrawText(buf, 10, 10,  20, BLACK);
	sprintf(buf, "Relative Position: %4.2f/%4.2f\n", len_x, len_y);
	DrawText(buf, 10, 30,  20, BLACK);
	sprintf(buf, "Cursor Angle: %4.4f°\n", mouse_angle);
	DrawText(buf, 10, 50,  20, BLACK);
	sprintf(buf, "Cursor Radius: %4.4f\n", hyp);
	DrawText(buf, 10, 70,  20, BLACK);

	// Print last pressed key
	static int last_key = 0;
	int kc = GetKeyPressed();
	if (kc != 0) last_key = kc;
	sprintf(buf, "Last Keycode: %i\n", last_key);
	DrawText(buf, 10, 100,  20, MAROON);

	sprintf(buf, "Debug int value: %i\n", g_debug_int);
	DrawText(buf, 10, 120,  20, MAROON);

}
