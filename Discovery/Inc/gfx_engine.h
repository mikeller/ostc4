///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/gfx_engine.h
/// \brief  DATA INPUT POSITION structure definition
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
#ifndef GFX_ENGINE_H
#define GFX_ENGINE_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "gfx.h"

/* Exported types ------------------------------------------------------------*/

/**
* @brief  DATA INPUT POSITION structure definition
*/

typedef struct
{
    uint32_t FBStartAdress;              /*!< Configures the color frame buffer address */
    uint16_t ImageWidth;                 /*!< Configures the color frame buffer line length.
                                             This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0x1FFF. */
    uint16_t ImageHeight;                /*!< Specifies the number of line in frame buffer.
                                              This parameter must be a number between Min_Data = 0x000 and Max_Data = 0x7FF. */
    uint8_t	LayerIndex;
} GFX_DrawCfgScreen;


typedef struct
{
    uint16_t WindowX0;                   /*!< Configures the Window vertical Start Position.
                                              This parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF. */
    uint16_t WindowX1;                   /*!< Configures the Window vertical Stop Position.
                                              This parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF. */
    uint16_t WindowY0;                   /*!< Configures the Window Horizontal Start Position.
                                              This parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF. */
    uint16_t WindowY1;                   /*!< Configures the Window Horizontal Stop Position.
                                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF. */

    uint16_t WindowTab;                  /*!< Configures the Window Horizontal Absolute Tab Position.
                                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF. */

    uint16_t WindowNumberOfTextLines;    /*!< Configures the Number of Text Lines.
                                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF. */

    uint16_t WindowLineSpacing;          /*!< Configures the Number of Text Lines.
                                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF. */
    GFX_DrawCfgScreen *Image;
} GFX_DrawCfgWindow;

typedef struct
{
        int left;
        int right;
        int top;
        int bottom;
} SWindowGimpStyle;

/* Exported variables --------------------------------------------------------*/

/**
    * \001	center
    * \002	right
    * \003	doubleSize
    * \004	NOP
    * \005	checkbox checked
    * \006	checkbox unchecked
    * \007	invert					\a
    * \010	use color				\b
    *	\011	tab							\t
    *	\012	line feed 			\n
    * \014	top align				\f
    * \015	carriage return	\r
    *	\016	O:o O.o style
    * \017	back to O font
    * \020	color 0
    *  ...
    * \027  color 8
    * \026	color 11
    * \033 UNUSED \c ESC
    * \034	monospaced space large size mode
    * \035	standard space size mode
    * \040	space == begin of text
    *  ...
    * \x7F = \177 del = space with size of next char
    * \x7F\x7F = \177\177 del del = space with size of next byte
    * \x80	begin of TXT_Substring with Language support
    */

/* Exported functions --------------------------------------------------------*/

void GFX_init(uint32_t * pDestinationOut);
void GFX_init1_no_DMA(uint32_t  * pDestinationOut, uint8_t blockFrames);

void GFX_helper_font_memory_list(const tFont *Font);

void GFX_start_VSYNC_IRQ(void);
void GFX_change_LTDC(void);

void GFX_SetFrameTop(uint32_t pDestination);
void GFX_SetFrameBottom(uint32_t pDestination, uint32_t x0, uint32_t y0, uint32_t width, uint32_t height);
void GFX_SetFramesTopBottom(uint32_t pTop, uint32_t pBottom, uint32_t heightBottom);

void GFX_SetWindowLayer0(uint32_t pDestination, int16_t XleftGimpStyle, int16_t XrightGimpStyle, int16_t YtopGimpStyle, int16_t YbottomGimpStyle);
void change_CLUT_entry(uint8_t entryToChange, uint8_t entryUsedForChange);

uint16_t GFX_return_offset(const tFont *Font, char *pText, uint8_t position);

void GFX_VGA_transform(uint32_t pSource, uint32_t pDestination);

HAL_StatusTypeDef GFX_SetBackgroundColor(uint32_t LayerIdx, uint8_t red, uint8_t green, uint8_t blue);

void GFX_LTDC_LayerTESTInit(uint16_t LayerIndex, uint32_t FB_Address);

void GFX_fill_buffer(uint32_t pDestination, uint8_t alpha, uint8_t color);

void GFX_clear_window_immediately(GFX_DrawCfgWindow* hgfx);

//void GFX_draw_circle_with_MEMORY(uint8_t use_memory, GFX_DrawCfgScreen *hgfx, point_t center, uint8_t radius, int8_t color);
void GFX_draw_circle(GFX_DrawCfgScreen *hgfx, point_t center, uint8_t radius, int8_t color);
void GFX_draw_colorline(GFX_DrawCfgScreen *hgfx, point_t start, point_t stop, uint8_t color);
void GFX_draw_thick_line(uint8_t thickness, GFX_DrawCfgScreen *hgfx, point_t start, point_t stop, uint8_t color);
void GFX_draw_line(GFX_DrawCfgScreen *hgfx, point_t start, point_t stop, uint8_t color);
void GFX_draw_box2(GFX_DrawCfgScreen *hgfx, point_t start, point_t stop, uint8_t color, uint8_t roundCorners);
void GFX_draw_box(GFX_DrawCfgScreen *hgfx, point_t LeftLow, point_t WidthHeight, uint8_t Style, uint8_t color);
void GFX_draw_header(GFX_DrawCfgScreen *hgfx, uint8_t colorId);
void GFX_clean_line(GFX_DrawCfgWindow* hgfx, uint32_t line_number);
void GFX_clean_area(GFX_DrawCfgScreen *tMscreen, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, uint16_t YBottomGimpStyle);

uint32_t GFX_write_string(const tFont *Font, GFX_DrawCfgWindow* hgfx, const char *pText, uint32_t line_number);
uint32_t GFX_write_string_color(const tFont *Font, GFX_DrawCfgWindow* hgfx, const char *pText, uint32_t line_number, uint8_t color);
uint32_t GFX_write_label(const tFont *Font, GFX_DrawCfgWindow* hgfx, const char *pText, uint8_t color);
void Gfx_write_label_var(GFX_DrawCfgScreen *screenInput, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const uint8_t color, const char *text);
void GFX_LTDC_Init(void);
void GFX_VGA_LTDC_Init(void);
void GFX_LTDC_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
void GFX_VGA_LTDC_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
void GFX_LTDC_LayerMenuBackgroundInit(uint32_t FB_Address);
//void GFX_ResetLayer(uint32_t LayerIdx);

//void GFX_fill_full_image_ARGB8888_alternating_lines(GFX_DrawCfgWindow* hgfx, uint32_t color1, uint32_t color2);
//void GFX_fill_window_ARGB8888(GFX_DrawCfgWindow* hgfx, uint32_t color);

//void GFX_copy_full_image_ARGB8888_to_RGB888(GFX_DrawCfgWindow* hgfx, uint32_t FBdestination);

void  GFX_graph_print(GFX_DrawCfgScreen *hgfx, const  SWindowGimpStyle *window, int16_t drawVeilUntil, uint8_t Xdivide, uint16_t dataMin, uint16_t dataMax,  uint16_t *data, uint16_t datalength, uint8_t color, uint8_t *colour_data);
void GFX_draw_Grid(GFX_DrawCfgScreen *hgfx, SWindowGimpStyle window, int vlines, float vdeltaline, int hlines, float hdeltalines, uint8_t color);

uint32_t getFrame(uint8_t callerId);
uint32_t getFrameByNumber(uint8_t ZeroToMaxFrames);
uint8_t releaseFrame(uint8_t callerId, uint32_t frameStartAddress);
void GFX_forceReleaseFramesWithId(uint8_t callerId);
void releaseAllFramesExcept(uint8_t callerId, uint32_t frameStartAddress);
uint8_t housekeepingFrame(void);
uint16_t blockedFramesCount(void);
uint8_t getFrameCount(uint8_t frameId);

void write_content_simple(GFX_DrawCfgScreen *tMscreen, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t color);
void gfx_write_topline_simple(GFX_DrawCfgScreen *tMscreen, const char *text, uint8_t color);
void gfx_write_page_number(GFX_DrawCfgScreen *tMscreen, uint8_t page, uint8_t total, uint8_t color);

void GFX_draw_image_monochrome(GFX_DrawCfgScreen *hgfx, SWindowGimpStyle window, const tImage *image, uint8_t color);
void GFX_build_logo_frame(void);
void GFX_build_hw_background_frame(void);

void GFX_logoAutoOff(void);
uint8_t GFX_logoStatus(void);
void GFX_hwBackgroundOn(void);
void GFX_hwBackgroundOff(void);

uint8_t gfx_number_to_string(uint8_t max_digits, _Bool fill, char *pText, uint32_t number);

uint8_t GFX_is_colorschemeDiveStandard(void);
void GFX_use_colorscheme(uint8_t colorscheme);

void GFX_screenshot(void);

tFont* GFX_Check_Extra_Font(uint8_t character, tFont *Font);
uint32_t GFX_Character_Width(uint8_t character, tFont *Font);
void Gfx_colorsscheme_mod(char *text, uint8_t alternativeColor);
#endif // GFX_ENGINE_H
