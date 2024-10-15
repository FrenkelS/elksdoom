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
 *      Render sky
 *
 *-----------------------------------------------------------------------------*/

#include "r_defs.h"
#include "r_main.h"
#include "w_wad.h"


#define FLAT_SKY_COLOR 100

#define ANGLETOSKYSHIFT         22


const int16_t skyflatnum = -2;
static int16_t skypatchnum;
static uint16_t skywidthmask;


static const patch_t __far* skypatch;


void R_LoadSkyPatch(void)
{
	skypatch = W_TryGetLumpByNum(skypatchnum);
}


void R_FreeSkyPatch(void)
{
	if (skypatch)
	{
		Z_ChangeTagToCache(skypatch);
		skypatch = NULL;
	}
}


void R_DrawSky(draw_column_vars_t *dcvars)
{
	if (skypatch == NULL)
		R_DrawColumnFlat(FLAT_SKY_COLOR, dcvars);
	else
	{
		dcvars->texturemid = (SCREENHEIGHT_VGA / 2) * FRACUNIT;

		if (!(dcvars->colormap = fixedcolormap))
			dcvars->colormap = fullcolormap;

		dcvars->iscale = (FRACUNIT * SCREENHEIGHT_VGA) / (VIEWWINDOWHEIGHT + 16);

		int16_t xc = viewangle >> FRACBITS;
		xc += xtoviewangleTable[dcvars->x];
		xc >>= ANGLETOSKYSHIFT - FRACBITS;
		xc &= skywidthmask;

		const column_t __far* column = (const column_t __far*) ((const byte __far*)skypatch + (uint16_t)skypatch->columnofs[xc]);

		dcvars->source = (const byte __far*)column + 3;
		R_DrawColumn(dcvars);
	}
}


// Set the sky map.
void R_InitSky(void)
{
	int16_t skytexture = R_CheckTextureNumForName("SKY1");
	const texture_t __far* tex = R_GetTexture(skytexture);
	skypatchnum  = tex->patches[0].patch_num;
	skywidthmask = tex->widthmask;
}
