/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2024 Frenkel Smeijers
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

#include <fcntl.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <linuxmt/mem.h>
#include <sys/ioctl.h>

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
	uint8_t b = c;

	for (size_t i = 0; i < n; i++)
		*d++ = b;

	return NULL;
}


char __far* _fstrcpy(char __far* destination, const char __far* source)
{
	char __far* s = (char __far*)source;

	while (*s)
		*destination++ = *s++;

	*destination = '\0';

	return NULL;
}


size_t _fstrlen(const char __far* str)
{
	size_t l = 0;
	char __far* s = (char __far*)str;

	while (*s)
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

static struct termios oldt;
static boolean isKeyboardIsrSet = false;


static void I_cfmakeraw(struct termios *t)
{
	t->c_cc[VMIN]  = 0;
	t->c_cc[VTIME] = 0;

	t->c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	t->c_cflag |= (CS8 | CREAD);
	t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | INPCK | ISTRIP);
	t->c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	t->c_iflag |= (BRKINT | IGNPAR);
	t->c_oflag &= ~(OPOST | OLCUC | OCRNL | ONOCR | ONLRET | OFILL | OFDEL);
	t->c_oflag &= ~(NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY);
	t->c_oflag |= (ONLCR | NL0 | CR0 | TAB3 | BS0 | VT0 | FF0);
	t->c_lflag &= ~(ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHONL);
	t->c_lflag &= ~(NOFLSH | XCASE);
	t->c_lflag &= ~(ECHOPRT | ECHOCTL | ECHOKE);
}


void I_InitKeyboard(void)
{
	struct termios newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	I_cfmakeraw(&newt);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	isKeyboardIsrSet = true;
}


static void I_ShutdownKeyboard(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}


void I_StartTic(void)
{
	//
	// process keyboard events
	//

	static uint8_t prev_input_buffer[NUMKEYS];
	static uint8_t curr_input_buffer[NUMKEYS];

	memcpy(prev_input_buffer, curr_input_buffer, NUMKEYS);
	memset(curr_input_buffer, 0, NUMKEYS);

	event_t ev;
	ev.type = ev_keydown;

	byte k;
	ssize_t r = read(STDIN_FILENO, &k, 1);
	while (r > 0)
	{
		if (k == 17) // Ctrl + Q
			I_Quit();

		if ('a' <= k && k <= 'z')
		{
			ev.data1 = k;
			D_PostEvent(&ev);
		}
		else
		{
			switch (k)
			{
				case 27: // Escape
					curr_input_buffer[KEYD_START] = true;
					break;
				case 13: // Enter
				case ' ':
					curr_input_buffer[KEYD_A] = true;
					break;
				//case : // Shift
				//	curr_input_buffer[KEYD_SPEED] = true;
				//	break;
				case '8':
					curr_input_buffer[KEYD_UP] = true;
					break;
				case '2':
					curr_input_buffer[KEYD_DOWN] = true;
					break;
				case '4':
					curr_input_buffer[KEYD_LEFT] = true;
					break;
				case '6':
					curr_input_buffer[KEYD_RIGHT] = true;
					break;
				case 9: // Tab
					curr_input_buffer[KEYD_SELECT] = true;
					break;
				case '/':
					curr_input_buffer[KEYD_B] = true;
					break;
				//case : // Alt
				//	curr_input_buffer[KEYD_STRAFE] = true;
				//	break;
				case ',':
					curr_input_buffer[KEYD_L] = true;
					break;
				case '.':
					curr_input_buffer[KEYD_R] = true;
					break;
				case '-':
					curr_input_buffer[KEYD_MINUS] = true;
					break;
				case '=':
					curr_input_buffer[KEYD_PLUS] = true;
					break;
				case '[':
					curr_input_buffer[KEYD_BRACKET_LEFT] = true;
					break;
				case ']':
					curr_input_buffer[KEYD_BRACKET_RIGHT] = true;
					break;
			}
		}

		r = read(STDIN_FILENO, &k, 1);
	}

	for (int16_t i = 0; i < NUMKEYS; i++)
	{
		if (curr_input_buffer[i])
		{
			ev.data1 = i;
			D_PostEvent(&ev);
		}
	}

	ev.type = ev_keyup;
	for (int16_t i = 0; i < NUMKEYS; i++)
	{
		if (prev_input_buffer[i] && !curr_input_buffer[i])
		{
			ev.data1 = i;
			D_PostEvent(&ev);
		}
	}
}


//**************************************************************************************
//
// Returns time in 1/35th second tics.
//

static uint32_t __far* pjiffies;


int32_t I_GetTime(void)
{
	uint32_t tick10ms = *pjiffies; /* read kernel 10ms timer directly */
	//return tick10ms * TICRATE / 100;
	return (tick10ms * TICRATE * 10) / 1024;
}


void I_InitTimer(void)
{
	int16_t fd = open("/dev/kmem", O_RDONLY);
	if (fd < 0) {
		I_Error("No kmem");
	}

	uint16_t kernel_ds, offset;
	if (ioctl(fd, MEM_GETDS,       &kernel_ds) < 0
	 || ioctl(fd, MEM_GETJIFFADDR, &offset   ) < 0) {
		I_Error("No mem ioctl");
	}
	close(fd);

	pjiffies = D_MK_FP(kernel_ds, offset);
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

	if (isKeyboardIsrSet)
		I_ShutdownKeyboard();
}


void I_Quit(void)
{
	I_Shutdown();

	int16_t lumpnum = W_GetNumForName("ENDOOM");
	W_ReadLumpByNum(lumpnum, D_MK_FP(0xb800, __djgpp_conventional_base));

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
