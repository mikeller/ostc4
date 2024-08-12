///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/gfx_fonts.h
/// \brief  Header file to control placement of font in STM32 Flash
/// \author heinrichs weikamp gmbh
/// \date   14-Aug-2014
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2018 Heinrichs Weikamp gmbh
///
///     This program is free software: you can redistribute it and/or modify
///     it under the terms of the GNU General Public License as published by
///     the Free Software Foundation, either version 3 of the License, or
///     (at your option) any later version.
///
///     This program is distributed in the hope that it will be useful,
///     but WITHOUT ANY WARRANTY; without even the implied warranty of
///     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///     GNU General Public License for more details.
///
///     You should have received a copy of the GNU General Public License
///     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef GFX_FONTS_H
#define GFX_FONTS_H

#include "gfx.h"

// From image_heinrichs_weikamp.c
enum { indexHWcolorSIZE = 256 };

// Forward declarations
//
// 2018-01-01 jDG:
//  - the exact address is fixed in linker script CPU1-F429.ld,
//    so that every font can be used even when not included in the actual
//    binary (boot-loader, fonts, or firmware).
//
//  - Actual font data is defined by the OSTC4/FontPack project, that shall
//    be loaded before the firmware is executed.
//
//  - Font directory starts at address 0x081E0000. Each header is 12 bytes.
//    A 0xFFFFFFFF (32 bits) marks the directory's end.
//    It is stored between __font_directory and __font_directory_end;
//
//  - Font data (chars) is stored after 0x08132040 (end of font's SFirmareData).
//    So there is space for 0x081E0000 - 0x08132040 = ADFC0 = 694 KB of fonts.
//    Actual data is stored between __upper_font_data and __upper_font_data_end,
//    and uses 0x081b9a3a - 0x08132040 = 879FA = 542 KB.
//

extern const tFont Awe48;
extern const tFont FontT24;
extern const tFont FontT42;
extern const tFont FontT48;
extern const tFont FontT54;
extern const tFont FontT84;
extern const tFont FontT105;
extern const tFont FontT144;

extern const tFont FontT84Spaced;

extern const tFont FontT54Extra;
extern const tFont FontT84Extra;
extern const tFont FontT105Extra;
extern const tFont Batt24;

extern const tImage ImgHWcolor;
extern const tImage ImgOSTC;

extern const uint32_t indexHWcolor[indexHWcolorSIZE];

// Macro to store in UPPER rom font sections
/* TODO: Looking at older map files the directories should be placed within the firmware memory */
/*       As well as the image data => to be confirmed */ 
#define UPPER_FONT_DIRECTORY __attribute__ (( used, section(".upper_font_directory") ))
#define LOWER_FONT_DIRECTORY __attribute__ (( used, section(".lower_font_directory") ))	/* store new fonts within firmware image to avoid fontlib update */

#define UPPER_IMAGE_DIRECTORY __attribute__(( section(".lower_image_directory") ))
#define UPPER_IMAGES          __attribute__(( section(".lower_images") ))

#endif // GFX_FONTS_H
