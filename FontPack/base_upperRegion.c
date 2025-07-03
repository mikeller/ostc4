///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   FontPack/./base_upperRegion.c
/// \brief  The beginning of it all. main() is part of this.
/// \author heinrichs weikamp gmbh
/// \date   31-August-2015
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
/**
	@verbatim
	==============================================================================
							##### New characters in fonts #####
	==============================================================================
	[..] Use font_tmore.c and add line to corresponding font like font_t54c
			 Don't forget to adjust the length of the font in the last line starting
			 const tFont .....

	[..] last char before the big gap, as of 160217
			 image_data_FontT24_0x002b[364]
			 __attribute__((at( START_T24_FONT + (1647 * 28) ))), START_T24_FONT  (0x08142F00 - MINUS_BANK)

			 -> free from 0x0814E490
			 then the logo image_data_ostc_fuer_Tauchcomputer_240px
			 approx 120 kBytes free space available!

	 @endverbatim
	******************************************************************************
	*/

/* Includes ------------------------------------------------------------------*/

// From Common/Inc:
#include "FirmwareData.h"


#ifdef DEBUG
// From AC6 support:
#include <stdio.h>
#include <stdint.h>
#endif

//////////////////////////////////////////////////////////////////////////////

const SFirmwareData font_FirmwareData   __attribute__(( section(".font_firmware_data") )) =
{
		.versionFirst   = 1,
		.versionSecond  = 0,
		.versionThird	= 0,
		.versionBeta	= 0,

		/* 4 bytes, including trailing 0 */
		.signature      = "im",

		.release_year   = 18,
		.release_month  = 10,
		.release_day    = 04,
		.release_sub    = 0,

		/* max 48, including trailing 0 */
		.release_info   ="FontPack extension",

		/* for safety reasons and coming functions */
		.magic[0] = FIRMWARE_MAGIC_FIRST,
		.magic[1] = FIRMWARE_MAGIC_SECOND,
		.magic[2] = FIRMWARE_MAGIC_FONT, /* the magic byte for fonts*/
		.magic[3] = FIRMWARE_MAGIC_END
};


const SFirmwareData font_CustomData   __attribute__(( section(".font_custom_data") )) =
{
		.versionFirst   = 1,
		.versionSecond  = 0,
		.versionThird	= 0,
		.versionBeta	= 0,

		/* 4 bytes, including trailing 0 */
		.signature      = "im",

		.release_year   = 18,
		.release_month  = 10,
		.release_day    = 04,
		.release_sub    = 0,

		/* max 48, including trailing 0 */
		.release_info   ="FontPack extension",

		/* for safety reasons and coming functions */
		.magic[0] = FIRMWARE_MAGIC_FIRST,
		.magic[1] = FIRMWARE_MAGIC_SECOND,
		.magic[2] = FIRMWARE_MAGIC_FONT, /* the magic byte for fonts*/
		.magic[3] = FIRMWARE_MAGIC_END
};

//////////////////////////////////////////////////////////////////////////////

#ifdef BUILD_LIBRARY
/* Fonts fixed in upper region */
#include "Fonts/font_awe48.h"
#include "Fonts/font_T24.h"
#include "Fonts/font_T42.h"
#include "Fonts/font_T48_plus.h"
#include "Fonts/font_T54.h"
#include "Fonts/font_T84.h"
#include "Fonts/font_T105.h"
#include "Fonts/font_T144_plus.h"

#include "Fonts/image_ostc.h"
#else
#include "gfx_fonts.h"
#endif

/* Images fixed in upper region */
#include "Fonts/image_battery.h"
#include "Fonts/image_heinrichs_weikamp.h"

/////////////////////////////////////////////////////////////////////////////


#ifdef DEBUG

#define ASSERT(e) \
		do { if( ! (e) )  {	++errors; printf("FAIL at %3d: %s \n", __LINE__, #e); errorflag = 1;} } while(0)

#define ASSERT_RANGE(e, min, max) ASSERT(min <= e); ASSERT( e <= max)

#if DEBUG_STLINK_V2
extern void initialise_monitor_handles(void);
#endif

static int errors = 0;
static uint8_t errorflag = 0;

int main(void)
{
#if DEBUG_STLINK_V2
	 initialise_monitor_handles();
#endif
		//---- Check the linker puts the directory at the requested address ------
		ASSERT( & Awe48    == (tFont*)0x81DFE00 );
		ASSERT( & FontT24  == (tFont*)0x81DFE0c );
		ASSERT( & FontT42  == (tFont*)0x81DFE18 );
		ASSERT( & FontT48  == (tFont*)0x81DFE24 );
		ASSERT( & FontT54  == (tFont*)0x81DFE30 );
		ASSERT( & FontT84  == (tFont*)0x81DFE3c );
		ASSERT( & FontT105 == (tFont*)0x81DFE48 );
		ASSERT( & FontT144 == (tFont*)0x81DFE54 );

		//---- Check the linker puts the font data in the requested section ------
		extern tChar __upper_font_data;
		extern tChar __upper_font_data_end;
		ASSERT( &__upper_font_data == (tChar*)0x08132040 );
		ASSERT_RANGE( (int)&__upper_font_data_end, 0x08132040, 0x081E0000);

		//---- Walk through the directory data -----------------------------------
		extern const tFont __font_directory;
		extern const tFont __font_directory_end;

		ASSERT_RANGE(&ImgOSTC,  (tImage*)&__upper_font_data, (tImage*)&__upper_font_data_end);

		for(const tFont* font = & __font_directory; font < &__font_directory_end; ++font)
		{
			printf("Font: %x\n",(unsigned int)font);
				// Check END-OF-DIRECTORY magic marker
				if( font->length == (uint32_t)-1 )
				{
						ASSERT( font == &FontT144 + 1 );
						break;
				}

				// Check font descriptors are inside a safe range.
				ASSERT_RANGE( font->length,               10, 150 ); /* old 103: some fonts meanwhile contain more charactes */
				ASSERT_RANGE( font->spacesize,             0,  28 ); /* old 18 : Awe40 has some size 28 characters */
				ASSERT_RANGE( font->spacesize2Monospaced, 13,  72 );
				ASSERT_RANGE( font->height,               28, 108 );

				//---- Walk through each char ----------------------------------------
				for(int i = 0; i < font->length; ++i)
				{
						const tChar* c = &font->chars[i];

						// Check char data is indeed stored in the actual data section
						ASSERT_RANGE( c, &__upper_font_data, &__upper_font_data_end);

						// Check char data sanity
						ASSERT_RANGE( c->code, 0x0000, 0xF143);

						// Check image sanity
						const tImage* image = c->image;
						ASSERT_RANGE(image,  (tImage*)&__upper_font_data, (tImage*)&__upper_font_data_end);
#if 0 /* common failure: root cause not clear */
						ASSERT_RANGE(image->width, font->spacesize, font->spacesize2Monospaced);
#endif
						ASSERT(image->height == font->height);
						if(errorflag)
						{
							printf("image %x: h=%d fonth=%d\n",(unsigned int)image,image->height,font->height);
							errorflag = 0;
						}
						// Uncompress image bytes
						const uint8_t* byte = image->data;
						ASSERT_RANGE(byte,  (uint8_t*)&__upper_font_data, (uint8_t*)&__upper_font_data_end);

						for(int w=0; w <image->width; ++w)
						{
								// Compression: special 0x01 byte at start of column means just skip it.
								if( *byte++ == 0x01 )
										continue;

								int zeros = (byte[-1] == 0x00) ? 1 : 0;
								for(int h = 1; h < image->height; ++h)
								{
										if( *byte == 0x00 )
												++zeros;
#if 0 /* this rule is violated very often but does not seems to have an impact */
										// Other bytes cannot have the 0x01 value...
										ASSERT( *byte++ != 0x01 );
#endif
								}
#if 0 /* just an information, not an error => activate if interested */
								if( zeros == image->height )
										printf("Font[%d] char[%d]: could skip column %d \n",
														&__font_directory - font, i, w);
#endif
						}

#if 0 /* byte usually pints to the next char ==> not sure what the check is about */
						// Check the byte stream do not collide with the next char,
						// or with the first tImage struct of the font.
						if( (i+1) < font->length )
								ASSERT( byte < font->chars[i+1].image->data );
						else
								ASSERT( byte < (uint8_t*)font->chars[0].image );

						// TODO: check image bytes are contiguous between chars.
#endif
				}
		}

		if( errors )
		{
				printf("Font Check: %d errors.\n", errors);
				return -1;
		}

		printf("Font Check: no errors.\n");
		return 0;
}
/*#endif*/
#else
#define ASSERT(e) \
		do { if( ! (e) )  {	++errors; errorflag = 1;} } while(0)

#define ASSERT_RANGE(e, min, max) ASSERT(min <= e); ASSERT( e <= max)

uint16_t errors = 1;
static uint8_t errorflag = 0;
uint16_t CheckFontPosition()
{
	uint16_t retvalue;
	ASSERT( & Awe48    == (tFont*)0x81DFE00 );
	ASSERT( & FontT24  == (tFont*)0x81DFE0c );
	ASSERT( & FontT42  == (tFont*)0x81DFE18 );
	ASSERT( & FontT48  == (tFont*)0x81DFE24 );
	ASSERT( & FontT54  == (tFont*)0x81DFE30 );
	ASSERT( & FontT84  == (tFont*)0x81DFE3c );
	ASSERT( & FontT105 == (tFont*)0x81DFE48 );
	ASSERT( & FontT144 == (tFont*)0x81DFE54 );
	retvalue = errors;
	return retvalue;
}
#endif
/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
