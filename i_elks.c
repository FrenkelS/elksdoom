/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2024-2025 Frenkel Smeijers
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

	static uint32_t gamekeytimestamps[NUMKEYS];

	event_t ev;
	ev.type = ev_keydown;

	uint8_t k;
	ssize_t r = read(STDIN_FILENO, &k, 1);
	while (r > 0)
	{
		ev.data1 = -1;

		if (k == 0x1b)
		{
			r = read(STDIN_FILENO, &k, 1);
			if (r <= 0)
			{
				// Escape
				ev.data1 = KEYD_START;
			}
			else
			{
				if (0x61 <= k && k <= 0x6c)
				{
					// F1 - F12 not supported
				}
				else if (k == 0x5b)
				{
					r = read(STDIN_FILENO, &k, 1);
					if (r <= 0)
					{
						// [
						// Escape followed by [ is not supported
					}
					else
					{
						if (k == 0x32 || k == 0x35 || k == 0x36)
						{
							// Insert, Page Up, Page Down not supported
							uint8_t temp;
							read(STDIN_FILENO, &temp, 1);
						}
						else
						{
							switch (k)
							{
								case 0x41: ev.data1 = KEYD_UP;    break;
								case 0x42: ev.data1 = KEYD_DOWN;  break;
								case 0x43: ev.data1 = KEYD_RIGHT; break;
								case 0x44: ev.data1 = KEYD_LEFT;  break;
								default: break; //not supported
							}
						}
					}
				}
				else
				{
					// Escape followed by an unknown key not supported
				}
			}
		}
		else if (k == 0x11) // Ctrl + Q
		{
			I_Quit();
		}
		else if ('a' <= k && k <= 'z')
		{
			ev.data1 = k;
		}
		else if ('A' <= k && k <= 'Z')
		{
			ev.data1 = k | 0x20; // tolower(k);
		}
		else
		{
			switch (k)
			{
				case 0x0d: // Enter
				case ' ':  // Spacebar
					ev.data1 = KEYD_A;
					break;
				//case : // Shift
				//	ev.data1 = KEYD_SPEED;
				//	break;
				case '8':
					ev.data1 = KEYD_UP;
					break;
				case '2':
					ev.data1 = KEYD_DOWN;
					break;
				case '4':
					ev.data1 = KEYD_LEFT;
					break;
				case '6':
					ev.data1 = KEYD_RIGHT;
					break;
				case 0x09: // Tab
					ev.data1 = KEYD_SELECT;
					break;
				case '/':
					ev.data1 = KEYD_B;
					break;
				//case : // Alt
				//	ev.data1 = KEYD_STRAFE;
				//	break;
				case ',':
					ev.data1 = KEYD_L;
					break;
				case '.':
					ev.data1 = KEYD_R;
					break;
				case '-':
					ev.data1 = KEYD_MINUS;
					break;
				case '=':
					ev.data1 = KEYD_PLUS;
					break;
				case '[':
					ev.data1 = KEYD_BRACKET_LEFT;
					break;
				case ']':
					ev.data1 = KEYD_BRACKET_RIGHT;
					break;
			}
		}

		if (ev.data1 != -1)
		{
			if (ev.data1 == 'a' || ev.data1 == 'h')
			{
				D_PostEvent(&ev);
				ev.data1 = KEYD_LEFT;
			}
			else if (ev.data1 == 's' || ev.data1 == 'j')
			{
				D_PostEvent(&ev);
				ev.data1 = KEYD_DOWN;
			}
			else if (ev.data1 == 'd' || ev.data1 == 'k')
			{
				D_PostEvent(&ev);
				ev.data1 = KEYD_UP;
			}
			else if (ev.data1 == 'f' || ev.data1 == 'l')
			{
				D_PostEvent(&ev);
				ev.data1 = KEYD_RIGHT;
			}

			if (ev.data1 < NUMKEYS)
				gamekeytimestamps[ev.data1] = *pjiffies;

			D_PostEvent(&ev);
		}

		r = read(STDIN_FILENO, &k, 1);
	}

	for (int16_t i = 0; i < NUMKEYS; i++)
	{
		if (gamekeytimestamps[i] != 0 && *pjiffies - gamekeytimestamps[i] > (42 * 100 + 999) / 1000)
		{
			gamekeytimestamps[i] = 0;
			event_t ev;
			ev.type  = ev_keyup;
			ev.data1 = i;
			D_PostEvent(&ev);
		}
	}
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
