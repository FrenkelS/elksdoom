/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2023-2024 Frenkel Smeijers
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      ELKS implementation of i_system.h
 *
 *-----------------------------------------------------------------------------*/

#include <stdarg.h>
#include <sys/time.h>

#include "doomdef.h"
#include "doomtype.h"
#include "compiler.h"
#include "d_main.h"
#include "i_system.h"
#include "globdata.h"


void I_InitGraphicsHardwareSpecificCode(void);


static boolean isGraphicsModeSet = false;

//**************************************************************************************
//
// Functions that are available in DOS, but not in ELKS
//

int16_t abs(int16_t v)
{
	return v < 0 ? -v : v;
}


int32_t labs(int32_t v)
{
	return v < 0 ? -v : v;
}


void __far* _fmemchr(const void __far* str, int c, size_t n)
{
	uint8_t __far* s = (uint8_t __far*)str;
	uint8_t b = c;

	for (size_t i = 0; i < n; i++)
	{
		if (*s == b)
		{
			return s;
		}

		s++;
	}

	return NULL;
}


void __far* _fmemcpy(void __far* destination, const void __far* source, size_t num)
{
	uint8_t __far* s = (uint8_t __far*)source;
	uint8_t __far* d = (uint8_t __far*)destination;

	for (size_t i = 0; i < num; i++)
		*d++ = *s++;

	return NULL;
}


void __far* _fmemset(void __far* str, int c, size_t n)
{
	uint8_t __far* d = (uint8_t __far*)str;

	for (size_t i = 0; i < n; i++)
		*d++ = c;

	return NULL;
}


char __far* _fstrcpy(char __far* destination, const char __far* source)
{
	char __far* s = (char __far*)source;
	char c = *s++;

	while (!c)
	{
		*destination++ = c;
		c = *s++;
	}
	*destination = 0;

	return NULL;
}


size_t _fstrlen(const char __far* str)
{
	size_t l = 0;
	char __far* s = (char __far*)str;

	while (!s)
	{
		l++;
		s++;
	}

	return l;
}


//**************************************************************************************
//
// Screen code
//

void I_InitGraphics(void)
{
	I_InitGraphicsHardwareSpecificCode();
	isGraphicsModeSet = true;
}


//**************************************************************************************
//
// Keyboard code
//

#define KBDQUESIZE 32

static byte keyboardqueue[KBDQUESIZE];
static int16_t kbdtail, kbdhead;
static boolean isKeyboardIsrSet = false;


void I_InitScreen(void)
{
	I_SetScreenMode(3);

	isKeyboardIsrSet = true;
}


#define SC_ESCAPE			0x01
#define SC_MINUS			0x0c
#define SC_PLUS				0x0d
#define SC_TAB				0x0f
#define SC_BRACKET_LEFT		0x1a
#define SC_BRACKET_RIGHT	0x1b
#define SC_ENTER			0x1c
#define SC_CTRL				0x1d
#define SC_LSHIFT			0x2a
#define SC_RSHIFT			0x36
#define SC_COMMA			0x33
#define SC_PERIOD			0x34
#define SC_ALT				0x38
#define SC_SPACE			0x39
#define SC_F10				0x44
#define SC_UPARROW			0x48
#define SC_DOWNARROW		0x50
#define SC_LEFTARROW		0x4b
#define SC_RIGHTARROW		0x4d

#define SC_Q	0x10
#define SC_P	0x19
#define SC_A	0x1e
#define SC_L	0x26
#define SC_Z	0x2c
#define SC_M	0x32


void I_StartTic(void)
{
	//
	// process keyboard events
	//

	while (kbdtail < kbdhead)
	{
		event_t ev;
		byte k = keyboardqueue[kbdtail & (KBDQUESIZE - 1)];
		kbdtail++;

		// extended keyboard shift key bullshit
		if ((k & 0x7f) == SC_LSHIFT || (k & 0x7f) == SC_RSHIFT)
		{
			if (keyboardqueue[(kbdtail - 2) & (KBDQUESIZE - 1)] == 0xe0)
				continue;
			k &= 0x80;
			k |= SC_RSHIFT;
		}

		if (k == 0xe0)
			continue;               // special / pause keys
		if (keyboardqueue[(kbdtail - 2) & (KBDQUESIZE - 1)] == 0xe1)
			continue;                               // pause key bullshit

		if (k == 0xc5 && keyboardqueue[(kbdtail - 2) & (KBDQUESIZE - 1)] == 0x9d)
		{
			//ev.type  = ev_keydown;
			//ev.data1 = KEY_PAUSE;
			//D_PostEvent(&ev);
			continue;
		}

		if (k & 0x80)
			ev.type = ev_keyup;
		else
			ev.type = ev_keydown;

		k &= 0x7f;
		switch (k)
		{
			case SC_ESCAPE:
				ev.data1 = KEYD_START;
				break;
			case SC_ENTER:
			case SC_SPACE:
				ev.data1 = KEYD_A;
				break;
			case SC_RSHIFT:
				ev.data1 = KEYD_SPEED;
				break;
			case SC_UPARROW:
				ev.data1 = KEYD_UP;
				break;
			case SC_DOWNARROW:
				ev.data1 = KEYD_DOWN;
				break;
			case SC_LEFTARROW:
				ev.data1 = KEYD_LEFT;
				break;
			case SC_RIGHTARROW:
				ev.data1 = KEYD_RIGHT;
				break;
			case SC_TAB:
				ev.data1 = KEYD_SELECT;
				break;
			case SC_CTRL:
				ev.data1 = KEYD_B;
				break;
			case SC_ALT:
				ev.data1 = KEYD_STRAFE;
				break;
			case SC_COMMA:
				ev.data1 = KEYD_L;
				break;
			case SC_PERIOD:
				ev.data1 = KEYD_R;
				break;
			case SC_MINUS:
				ev.data1 = KEYD_MINUS;
				break;
			case SC_PLUS:
				ev.data1 = KEYD_PLUS;
				break;
			case SC_BRACKET_LEFT:
				ev.data1 = KEYD_BRACKET_LEFT;
				break;
			case SC_BRACKET_RIGHT:
				ev.data1 = KEYD_BRACKET_RIGHT;
				break;

			case SC_F10:
				I_Quit();
			default:
				if (SC_Q <= k && k <= SC_P)
				{
					ev.data1 = "qwertyuiop"[k - SC_Q];
					break;
				}
				else if (SC_A <= k && k <= SC_L)
				{
					ev.data1 = "asdfghjkl"[k - SC_A];
					break;
				}
				else if (SC_Z <= k && k <= SC_M)
				{
					ev.data1 = "zxcvbnm"[k - SC_Z];
					break;
				}
				else
					continue;
		}
		D_PostEvent(&ev);
	}
}


//**************************************************************************************
//
// Returns time in 1/35th second tics.
//

static int32_t basetime;

static boolean isTimerSet;


static int32_t clock()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec * 1000L + now.tv_usec / 1000L;
}


int32_t I_GetTime(void)
{
	int32_t now = clock();
	return ((now - basetime) * TICRATE) / CLOCKS_PER_SEC;
}


void I_InitTimer(void)
{
	basetime = clock();

	isTimerSet = true;
}


static void I_ShutdownTimer(void)
{

}


//**************************************************************************************
//
// Exit code
//

static void I_Shutdown(void)
{
	if (isGraphicsModeSet)
		I_SetScreenMode(3);

	I_ShutdownSound();

	if (isTimerSet)
		I_ShutdownTimer();

	if (isKeyboardIsrSet)
	{
	}

	Z_Shutdown();
}


static void I_SetCursorPosition(void);
#pragma aux I_SetCursorPosition = \
	"push si",		\
	"push di",		\
	"push bp",		\
	"push es",		\
	"cli",			\
	"mov ah, 2",	\
	"mov bh, 0",	\
	"mov dl, 0",	\
	"mov dh, 23",	\
	"int 0x10",		\
	"sti",			\
	"pop es",		\
	"pop bp",		\
	"pop di",		\
	"pop si"		\
	modify [bh dx]

void I_Quit(void)
{
	I_Shutdown();

	int16_t lumpnum = W_GetNumForName("ENDOOM");
	W_ReadLumpByNum(lumpnum, D_MK_FP(0xb800, __djgpp_conventional_base));

	I_SetCursorPosition();

	printf("\n");
	exit(0);
}


void I_Error (const char *error, ...)
{
	va_list argptr;

	I_Shutdown();

	va_start(argptr, error);
	vfprintf(stdout, error, argptr);
	va_end(argptr);
	printf("\n");
	exit(1);
}
