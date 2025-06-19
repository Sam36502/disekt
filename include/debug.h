#ifndef DEBUG_H
#define DEBUG_H

//	Some basic variables that are easily edited to make debugging things in real time easier


#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define KEYCODE_SHIFT 340
#define KEYCODE_DEBUG_INT_INC 334
#define KEYCODE_DEBUG_INT_DEC 333
#define KEYCODE_DEBUG_VIEW_TOGGLE 294

extern int g_debug_int;
extern double g_debug_prog;
extern bool g_verbose_log;
extern bool g_show_debug_view;

//	Handles events that impact the debug variables
//
//	returns true if the screen should be redrawn
bool DEBUG_HandleEvents(int key);

//	Draws debug info to the screen
//
void DEBUG_DrawDevInfo();

#endif
