///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/gfx.h
/// \brief  Header file for common GFX files
/// \author heinrichs weikamp gmbh
/// \date   07-April-2014
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
#ifndef GFX_H
#define GFX_H

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

typedef struct {
		 const uint8_t *data;
		 uint16_t width;
		 uint16_t height;
		 } tImage;
typedef struct {
		 uint8_t *data;
		 const uint8_t *dataComp;
		 uint16_t sizeComp;
 		 uint16_t width;
 		 uint16_t height;
 		 } tImageComp;

typedef struct {
		 long int code;
		 const tImage *image;
		 } tChar;

typedef struct {
		long int code;
		const tImageComp *image;
		} tCharComp;


typedef struct {
		 uint32_t length;
		 const tChar *chars;
		 uint8_t spacesize;
		 uint8_t spacesize2Monospaced;
		 uint8_t height;
		 } tFont;

typedef struct {
 		 uint32_t length;
 		 const tCharComp *chars;
 		 uint8_t spacesize;
 		 uint8_t spacesize2Monospaced;
 		 uint8_t height;
 		 } tFontComp;


typedef struct {
		uint32_t x;
		uint32_t y;
} point_t;

typedef struct {
		int16_t x;
		int16_t y;
} int16_Point_t;

typedef union {
		uint32_t i[6];
		point_t p[3];
} bezier_t;

typedef struct {
		const bezier_t* data;
		uint8_t datalen;
} path_t;

#endif // GFX_H
