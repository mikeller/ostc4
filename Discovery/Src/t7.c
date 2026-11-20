///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/t7.c
/// \brief  Main Template file for dive mode 7x
/// \author Heinrichs Weikamp gmbh
/// \date   23-April-2014
///
/// \details
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

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>

#include "t7.h"
#include "t7_customview.h"
#include "t3.h"
#include "settings.h"
#include "data_exchange_main.h"
#include "data_central.h"
#include "decom.h"
#include "gfx_fonts.h"
#include "device_time_hooks.h"   /* sim_pin_update_necessary_off (Phase 0 overlay) */
#include "logbook_miniLive.h"
#include "math.h"
#include "tComm.h"
#include "tHome.h"
#include "simulation.h"
#include "timer.h"
#include "unit.h"
#include "motion.h"
#include "configuration.h"
#include "ostc.h"
#include "base.h"
#include "tMenuEditSetpoint.h"
#include "vpm.h"
#include "cavemode.h"
#include "compass_rose.h"

#define TIMER_ACTION_DELAY_S 10


/* Bv6 iOS-style compass rose configuration for t7 surface view.
   Center derived from CUSTOMBOX_LINE_LEFT (250) + CUSTOMBOX_INSIDE_OFFSET (2)
   + CUSTOMBOX_SPACE_INSIDE/2 (148) = 400 in x, and 250 in y (matches the dial
   center the legacy t7_compass_circle() table used). */
static const RoseConfig t7_rose_cfg = {
    .center                  = {.x = 400, .y = 220},
    .r_out                   = 112,
    .r_ring_inner            = 110,  /* r_out - 2 */
    .r_tick_minor_tip        = 105,   /* 5px per design spec; 109 gave a 1px nub the toggle couldn't reveal */
    .r_tick_mid_tip          = 104,
    .r_tick_major_tip        = 95,
    .r_tick_cardinal_tip     = 95,   /* same as major: no change to t7 */
    .r_label_num             = 125,  /* pushed out from 121: ~3px margin clear of the ring + bearing-marker caps */
    .r_label_card            = 128,
    .r_tri_apex              = 120,
    .tick_step_deg           = 10,  /* Medium = 10/30/90; the 10deg minors gate on compassMinorTicks */
    .tick_thickness_minor    = 1,
    .tick_thickness_mid      = 2,
    .tick_thickness_major    = 3,
    .tick_thickness_cardinal = 3,
    .cardinal_pencil         = 0,
    .color_tick_minor        = CLUT_Font031,
    .color_tick_mid          = CLUT_Font031,
    .color_tick_major        = CLUT_Font030,
    .color_ring              = CLUT_pluginboxSurface,  /* dark grey: lets the white ticks/letters stand out */
    .color_label_num         = CLUT_Font030,  /* clean white (was grey Font031), matches cardinals */
    .color_label_card        = CLUT_Font030,
    .color_label_card_north  = CLUT_Font030,  /* monochrome: same as other cardinals */
    .color_north_tri         = CLUT_CompassUserBackHeadingTick,
    .r_lubber_out            = 75,   /* arrowhead apex (r_lubber_out+9 = 84) ~4px clear of the bearing-marker stem inner end (r_out-24 = 88) */
    .r_lubber_in             = 40,   /* tail stops short of the central FontT54 heading readout (overlapped it when tilted) */
    .color_lubber            = CLUT_Font030,
    .lubber_thickness        = 5,
    .lubber_pointer          = 1,
    .flip_display            = false,
    .font_num                = &FontT24,
    .font_card               = &FontT42,
    .marker_style            = 0,    /* T/arc-cap */
    .r_lubber_tail           = 0,    /* no tail (t7) */
    .ring_thickness          = 2,
};

/* Private function prototypes -----------------------------------------------*/

void t7_refresh_surface(void);
void t7_refresh_surface_debugmode(void);
static uint8_t t7_getGnssPowerSaveState(void);
#ifdef HAVE_DEBUG_VIEW
void t7_refresh_gnss_debug(void);
void t7_refresh_gnss_hardware(void);
#endif
void t7_refresh_divemode(void);
void t7_refresh_sleep_design_fun(void);
void t7_refresh_divemode_userselected_left_lower_corner(void);
void t7_refresh_customview(void);

void draw_frame(_Bool PluginBoxHeader, _Bool LinesOnTheSides, uint8_t colorBox, uint8_t colorLinesOnTheSide);

void t7_tissues(const SDiveState * pState);
void t7_compass(uint16_t ActualHeading, uint16_t UserSetHeading);
void t7_SummaryOfLeftCorner(void);

void t7_miniLiveLogProfile(void);;
void t7_ChargerView(void);

uint8_t t7_test_customview_warnings(void);
void t7_show_customview_warnings(void);

uint8_t t7_test_customview_warnings_surface_mode(void);
void t7_show_customview_warnings_surface_mode(void);

static void t7_drawAcentGraph(uint8_t color);
static uint8_t t7_drawSlowExitGraph(void);
static void t7_showPosition(void);

/* Imported function prototypes ---------------------------------------------*/
extern uint8_t write_gas(char *text, uint8_t oxygen, uint8_t helium);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

GFX_DrawCfgScreen	t7screen;

/* left 3 fields
 * right 3 fields
 * centered one field on top of customview, one below
 * customview header + customview + warning
 */
GFX_DrawCfgWindow	t7l1, t7l2, t7l3;
GFX_DrawCfgWindow	t7r1, t7r2, t7r3;
GFX_DrawCfgWindow	t7c1, t7batt, t7c2, t7charge, t7voltage;
GFX_DrawCfgWindow	t7cH, t7cC, t7cW, t7cY0free;
GFX_DrawCfgWindow	t7surfaceL, t7surfaceR;

uint8_t selection_customview = LLC_Temperature;
#ifdef HAVE_DEBUG_VIEW
static uint8_t gnssDebugPage = 0;			/* the debug page of the position view is shown instead of the home screen: 0 none, 1 receiver values, 2 receiver hardware */
#endif

uint8_t updateNecessary = 0;

typedef struct{
uint32_t pointer;
uint32_t x0;
uint32_t y0;
uint32_t width;
uint32_t height;
} SBackground;

SBackground background =
{
    .pointer = 0,
};


/* Private types -------------------------------------------------------------*/


const uint8_t customviewsSurfaceStandard[] =
{
//  CVIEW_CompassDebug,
    CVIEW_Hello,
    CVIEW_sensors,
    CVIEW_Compass,
    CVIEW_Tissues,
    CVIEW_Gaslist,
    CVIEW_sensors_mV,
	CVIEW_Charger,
    CVIEW_CcrSummary,
    CVIEW_Timer,
#if defined ENABLE_GNSS_INTERN || defined ENABLE_GNSS_EXTERN
	CVIEW_Position,
#endif
    CVIEW_END
};


static uint8_t selection_custom_field = LLC_Temperature;

const uint8_t *customviewsDive		= cv_changelist;
const uint8_t *customviewsSurface	= customviewsSurfaceStandard;

#define TEXTSIZE 30
/* offset includes line: 2 = line +1
 * box (line) is 300 px
 * inside is 296 px
 * left of box are 249 px ( 0..248)
 * right of box are 249 px (551 .. 799)
 */

#define CUSTOMBOX_LINE_LEFT (250)
#define CUSTOMBOX_LINE_RIGHT (549)
#define CUSTOMBOX_LINE_TOP	  (0)
#define CUSTOMBOX_LINE_MIDDLE  (142)
#define CUSTOMBOX_LINE_BOTTOM  (318)
#define CUSTOMBOX_INSIDE_OFFSET (2)
#define CUSTOMBOX_OUTSIDE_OFFSET (2)
#define CUSTOMBOX_SPACE_INSIDE (CUSTOMBOX_LINE_RIGHT + 1 - (CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET + CUSTOMBOX_INSIDE_OFFSET))
#define TOP_LINE_HIGHT (25)

#define SHOW_AMBIENTE_SURFACE_DELTA		(0.02f)
#define SHOW_AMBIENTE_DEBOUNCE			(0.003f)
#define SHOW_TEMPERATURE_DEBOUNCE		(0.5f)

#define MAX_NUM_SUMMARY_LINES 6

/* Exported functions --------------------------------------------------------*/

void t7_init(void)
{

	SSettings* pSettings;
	pSettings = settingsGetPointer();

    selection_custom_field = LLC_Temperature;
    selection_customview = customviewsSurface[0];

    t7screen.FBStartAdress = 0;
    t7screen.ImageHeight = 480;
    t7screen.ImageWidth = 800;
    t7screen.LayerIndex = 1;

    if(!pSettings->FlipDisplay)
    {
		t7l1.Image = &t7screen;
		t7l1.WindowNumberOfTextLines = 2;
		t7l1.WindowLineSpacing = 19; // Abstand von Y0
		t7l1.WindowTab = 100; // vermtl. ohne Verwendung in diesem Fenster
		t7l1.WindowX0 = 0;
		t7l1.WindowX1 = CUSTOMBOX_LINE_LEFT - CUSTOMBOX_OUTSIDE_OFFSET;
		t7l1.WindowY0 = 318;
		t7l1.WindowY1 = 479;

		t7l2.Image = &t7screen;
		t7l2.WindowNumberOfTextLines = 2;
		t7l2.WindowLineSpacing = 12; // Abstand von Y0
		t7l2.WindowTab = 100; // vermtl. ohne Verwendung in diesem Fenster
		t7l2.WindowX0 = 0;
		t7l2.WindowX1 = t7l1.WindowX1;
		t7l2.WindowY0 = 142;
		t7l2.WindowY1 = t7l1.WindowY0 - 5;

		t7l3.Image = &t7screen;
		t7l3.WindowNumberOfTextLines = 2;
		t7l3.WindowLineSpacing = 58; // Abstand von Y0
		t7l3.WindowTab = 100; // vermtl. ohne Verwendung in diesem Fenster
		t7l3.WindowX0 = 0;
		t7l3.WindowX1 = t7l1.WindowX1;
		t7l3.WindowY0 = 0;
		t7l3.WindowY1 = t7l2.WindowY0 - 5;

		t7r1.Image = &t7screen;
		t7r1.WindowNumberOfTextLines = 2;
		t7r1.WindowLineSpacing = t7l1.WindowLineSpacing;
		t7r1.WindowTab = 100;
		t7r1.WindowX0 = 550;
		t7r1.WindowX1 = 799;
		t7r1.WindowY0 = t7l1.WindowY0;
		t7r1.WindowY1 = 479;

		t7r2.Image = &t7screen;
		t7r2.WindowNumberOfTextLines = 2;
		t7r2.WindowLineSpacing = t7l2.WindowLineSpacing;
		t7r2.WindowTab = 100;
		t7r2.WindowX0 = 550;
		t7r2.WindowX1 = 799;
		t7r2.WindowY0 = t7l2.WindowY0;
		t7r2.WindowY1 = t7l2.WindowY1;

		t7r3.Image = &t7screen;
		t7r3.WindowNumberOfTextLines = 2;
		t7r3.WindowLineSpacing = 0;//t7l3.WindowLineSpacing;
		t7r3.WindowTab = 100;
		t7r3.WindowX0 = CUSTOMBOX_LINE_RIGHT + CUSTOMBOX_OUTSIDE_OFFSET;
		t7r3.WindowX1 = 799;
		t7r3.WindowY0 = t7l3.WindowY0;
		t7r3.WindowY1 = t7l3.WindowY1;

		t7cC.Image = &t7screen;
		t7cC.WindowNumberOfTextLines = 3;
		t7cC.WindowLineSpacing = 95; // Abstand von Y0
		t7cC.WindowTab = 100;
		t7cC.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
		t7cC.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
		t7cC.WindowY0 = 90;
		t7cC.WindowY1 = 434 - 95;

		t7cH.Image = &t7screen;
		t7cH.WindowNumberOfTextLines = 1;
		t7cH.WindowLineSpacing = 95; // Abstand von Y0
		t7cH.WindowTab = 100;
		t7cH.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
		t7cH.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
		t7cH.WindowY0 = 434 - 94;
		t7cH.WindowY1 = 434;

		t7cW.Image = &t7screen;
		t7cW.WindowNumberOfTextLines = 3;
		t7cW.WindowLineSpacing = 95; // Abstand von Y0
		t7cW.WindowTab = 100;
		t7cW.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
		t7cW.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
		t7cW.WindowY0 = 90;
		t7cW.WindowY1 = 434 - 95;


		t7surfaceL.Image = &t7screen;
		t7surfaceL.WindowNumberOfTextLines = 9;
		t7surfaceL.WindowLineSpacing = 53;
		t7surfaceL.WindowTab = 100;
		t7surfaceL.WindowX0 = 0;
		t7surfaceL.WindowX1 = CUSTOMBOX_LINE_LEFT - CUSTOMBOX_OUTSIDE_OFFSET;
		t7surfaceL.WindowY0 = 0;
		t7surfaceL.WindowY1 = 480;

		t7surfaceR.Image = &t7screen;
		t7surfaceR.WindowNumberOfTextLines = 9;
		t7surfaceR.WindowLineSpacing = 53;
		t7surfaceR.WindowTab = 100;
		t7surfaceR.WindowX0 = CUSTOMBOX_LINE_RIGHT + CUSTOMBOX_OUTSIDE_OFFSET;
		t7surfaceR.WindowX1 = 800;
		t7surfaceR.WindowY0 = 0;
		t7surfaceR.WindowY1 = 480;

		t7cY0free.Image = &t7screen;
		t7cY0free.WindowNumberOfTextLines = 1;
		t7cY0free.WindowLineSpacing = 95;
		t7cY0free.WindowTab = 100;
		t7cY0free.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
		t7cY0free.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
		t7cY0free.WindowY0 = 90;
		t7cY0free.WindowY1 = 434 - 95;

		t7batt.Image = &t7screen;
		t7batt.WindowNumberOfTextLines = 1;
		t7batt.WindowLineSpacing = 10;
		t7batt.WindowTab = 100;
		t7batt.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
		t7batt.WindowX0 = t7batt.WindowX1 - (52+52);
		t7batt.WindowY1 = 479;
		t7batt.WindowY0 = t7batt.WindowY1 - 25;

		t7charge.Image = &t7screen;
		t7charge.WindowNumberOfTextLines = 1;
		t7charge.WindowLineSpacing = 10;
		t7charge.WindowTab = 100;
		t7charge.WindowX1 = t7batt.WindowX1 - 18;
		t7charge.WindowX0 = t7charge.WindowX1 - 14;
		t7charge.WindowY1 = 479;
		t7charge.WindowY0 = t7batt.WindowY1 - 25;

		t7voltage.Image = &t7screen;
		t7voltage.WindowNumberOfTextLines = 1;
		t7voltage.WindowLineSpacing = 10;
		t7voltage.WindowTab = 100;
		t7voltage.WindowX0 = t7charge.WindowX0 - 10;
		t7voltage.WindowX1 = t7voltage.WindowX0 + (18*3)+ 9;
		t7voltage.WindowY1 = 479;
		t7voltage.WindowY0 = t7batt.WindowY1 - 25;

		t7c1.Image = &t7screen;
		t7c1.WindowNumberOfTextLines = 1;
		t7c1.WindowLineSpacing = 10;
		t7c1.WindowTab = 100;
		t7c1.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
		t7c1.WindowX1 = t7batt.WindowX0 - 18;
		t7c1.WindowY0 = 435;
		t7c1.WindowY1 = 479;

		t7c2.Image = &t7screen;
		t7c2.WindowNumberOfTextLines = 1;
		t7c2.WindowLineSpacing = 0; // Abstand von Y0
		t7c2.WindowTab = 100;
		t7c2.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
		t7c2.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
		t7c2.WindowY0 = 0;
		t7c2.WindowY1 = 69;
    }
    else
    {
/* 6 segments (left / right) used to show data during dive */

    t7l1.Image = &t7screen;
    t7l1.WindowNumberOfTextLines = 2;
    t7l1.WindowLineSpacing = 19; // Abstand von Y0
    t7l1.WindowTab = 100; // vermtl. ohne Verwendung in diesem Fenster
    t7l1.WindowX0 = CUSTOMBOX_LINE_RIGHT + CUSTOMBOX_OUTSIDE_OFFSET;
    t7l1.WindowX1 = 799;
    t7l1.WindowY0 = CUSTOMBOX_LINE_TOP;
    t7l1.WindowY1 = 150 + TOP_LINE_HIGHT;

    t7l2.Image = &t7screen;
    t7l2.WindowNumberOfTextLines = 2;
    t7l2.WindowLineSpacing = 12; // Abstand von Y0
    t7l2.WindowTab = 100; // vermtl. ohne Verwendung in diesem Fenster
    t7l2.WindowX0 = t7l1.WindowX0;
    t7l2.WindowX1 = t7l1.WindowX1;
    t7l2.WindowY0 = t7l1.WindowY1 + 5;
    t7l2.WindowY1 = t7l2.WindowY0 + 146;

    t7l3.Image = &t7screen;
    t7l3.WindowNumberOfTextLines = 2;
    t7l3.WindowLineSpacing = 58; // Abstand von Y0
    t7l3.WindowTab = 100; // vermtl. ohne Verwendung in diesem Fenster
    t7l3.WindowX0 = t7l1.WindowX0;
    t7l3.WindowX1 = t7l1.WindowX1;;
    t7l3.WindowY0 = 479 - 150;
    t7l3.WindowY1 = 479;

    t7r1.Image = &t7screen;
    t7r1.WindowNumberOfTextLines = 2;
    t7r1.WindowLineSpacing = t7l1.WindowLineSpacing;
    t7r1.WindowTab = 100;
    t7r1.WindowX0 = 0;
    t7r1.WindowX1 = CUSTOMBOX_LINE_LEFT - CUSTOMBOX_OUTSIDE_OFFSET;
    t7r1.WindowY0 = t7l1.WindowY0;
    t7r1.WindowY1 = t7l1.WindowY1;

    t7r2.Image = &t7screen;
    t7r2.WindowNumberOfTextLines = 2;
    t7r2.WindowLineSpacing = t7l2.WindowLineSpacing;
    t7r2.WindowTab = 100;
    t7r2.WindowX0 = t7r1.WindowX0;
    t7r2.WindowX1 = t7r1.WindowX1;
    t7r2.WindowY0 = t7l2.WindowY0;
    t7r2.WindowY1 = t7l2.WindowY1;

    t7r3.Image = &t7screen;
    t7r3.WindowNumberOfTextLines = 2;
    t7r3.WindowLineSpacing = 0;//t7l3.WindowLineSpacing;
    t7r3.WindowTab = 100;
    t7r3.WindowX0 = t7r1.WindowX0;
    t7r3.WindowX1 = t7r1.WindowX1;
    t7r3.WindowY0 = t7l3.WindowY0;
    t7r3.WindowY1 = t7l3.WindowY1;

/* screen for CustomText / serial number */
    t7cC.Image = &t7screen;
    t7cC.WindowNumberOfTextLines = 3;
    t7cC.WindowLineSpacing = 95; // Abstand von Y0
    t7cC.WindowTab = 100;
    t7cC.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t7cC.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
    t7cC.WindowY0 = 165; //90;
    t7cC.WindowY1 = 415;

    /* used by warning message box */
    t7cH.Image = &t7screen;
    t7cH.WindowNumberOfTextLines = 1;
    t7cH.WindowLineSpacing = 95; // Abstand von Y0
    t7cH.WindowTab = 100;
    t7cH.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t7cH.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
    t7cH.WindowY0 = 46; //480 - 434;
    t7cH.WindowY1 = 390 - 46;// - 90; //46 + 390; //480 - (434 - 94); //434;

    /* used by warning custom box */
    t7cW.Image = &t7screen;
    t7cW.WindowNumberOfTextLines = 3;
    t7cW.WindowLineSpacing = 95; // Abstand von Y0
    t7cW.WindowTab = 100;
    t7cW.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t7cW.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
    t7cW.WindowY0 = 480 - (434 - 90);
    t7cW.WindowY1 = 480 - 90; //434 - 95;

/* time and environment */
    t7surfaceL.Image = &t7screen;
    t7surfaceL.WindowNumberOfTextLines = 9;
    t7surfaceL.WindowLineSpacing = 53;
    t7surfaceL.WindowTab = 100;
    t7surfaceL.WindowX0 = CUSTOMBOX_LINE_RIGHT + CUSTOMBOX_OUTSIDE_OFFSET;
    t7surfaceL.WindowX1 = 799;
    t7surfaceL.WindowY0 = 0;
    t7surfaceL.WindowY1 = 479;

    t7surfaceR.Image = &t7screen;
    t7surfaceR.WindowNumberOfTextLines = 9;
    t7surfaceR.WindowLineSpacing = 53;
    t7surfaceR.WindowTab = 100;
    t7surfaceR.WindowX0 = 0;
    t7surfaceR.WindowX1 = CUSTOMBOX_LINE_LEFT - CUSTOMBOX_OUTSIDE_OFFSET;
    t7surfaceR.WindowY0 = 0;
    t7surfaceR.WindowY1 = 479;

/* info screen in the middle */
    t7cY0free.Image = &t7screen;
    t7cY0free.WindowNumberOfTextLines = 1;
    t7cY0free.WindowLineSpacing = 95;
    t7cY0free.WindowTab = 100;
    t7cY0free.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t7cY0free.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
    t7cY0free.WindowY0 = 115;
    t7cY0free.WindowY1 = 365;

/* voltage value (V or %) */
    t7voltage.Image = &t7screen;
    t7voltage.WindowNumberOfTextLines = 1;
    t7voltage.WindowLineSpacing = 10;
    t7voltage.WindowTab = 100;
    t7voltage.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t7voltage.WindowX1 = t7voltage.WindowX0 + (18*3) +9;
    t7voltage.WindowY1 = TOP_LINE_HIGHT;
    t7voltage.WindowY0 = 0;

/* battery symbol */
    t7batt.Image = &t7screen;
    t7batt.WindowNumberOfTextLines = 1;
    t7batt.WindowLineSpacing = 10;
    t7batt.WindowTab = 100;
    t7batt.WindowX0 = t7voltage.WindowX1;
    t7batt.WindowX1 = t7batt.WindowX0 + (52);
    t7batt.WindowY1 = TOP_LINE_HIGHT;
    t7batt.WindowY0 = 0;

/* charger symbol */
    t7charge.Image = &t7screen;
    t7charge.WindowNumberOfTextLines = 1;
    t7charge.WindowLineSpacing = 10;
    t7charge.WindowTab = 100;
    t7charge.WindowX1 = t7batt.WindowX0 - 18;
    t7charge.WindowX0 = t7charge.WindowX1 - 14;

    t7charge.WindowY1 = TOP_LINE_HIGHT;
    t7charge.WindowY0 = 0;

/* show dive mode OC / CC */
    t7c1.Image = &t7screen;
    t7c1.WindowNumberOfTextLines = 1;
    t7c1.WindowLineSpacing = 10;
    t7c1.WindowTab = 100;
    t7c1.WindowX0 = t7batt.WindowX1 + 18; //CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t7c1.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET; //t7batt.WindowX1 + 18;
    t7c1.WindowY0 = 0;
    t7c1.WindowY1 = 479 - 435;

/* Gas warnings and exit Sim*/
    t7c2.Image = &t7screen;
    t7c2.WindowNumberOfTextLines = 1;
    t7c2.WindowLineSpacing = 0; // Abstand von Y0
    t7c2.WindowTab = 100;
    t7c2.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t7c2.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
    t7c2.WindowY0 = 480 - 69;
    t7c2.WindowY1 = 479;

    }
}


void t7_refresh_sleepmode_fun(void)
{
    uint32_t oldScreen;

    oldScreen = t7screen.FBStartAdress;
    t7screen.FBStartAdress = getFrame(22);

    t7_refresh_sleep_design_fun();

    if(get_globalState() == StStop)
    {
        GFX_SetFramesTopBottom(t7screen.FBStartAdress, 0,480);
    }
    releaseFrame(22,oldScreen);
}


void t7_refresh(void)
{
    static uint8_t last_mode = MODE_SURFACE;
    SStateList status;
    get_globalStateList(&status);

    t7screen.FBStartAdress = getFrame(22);

    background.pointer = 0;

    if(stateUsed->mode == MODE_DIVE)
    {
    	/* T7 refresh is usally called at start of dive. Based on divesettings the design will be changed and T7 no longer called */
        if(stateUsed->diveSettings.diveMode == DIVEMODE_Gauge)
        {
            settingsGetPointer()->design = 5;
            releaseAllFramesExcept(22,t7screen.FBStartAdress);
            releaseFrame(22,t7screen.FBStartAdress);
            return;
        }
        else if(stateUsed->diveSettings.diveMode == DIVEMODE_Apnea)
        {
            settingsGetPointer()->design = 6;
            releaseAllFramesExcept(22,t7screen.FBStartAdress);
            releaseFrame(22,t7screen.FBStartAdress);
            return;
        }
        else
        {
			if(last_mode != MODE_DIVE)
			{
				last_mode = MODE_DIVE;
				/* lower left corner primary */
				selection_custom_field = settingsGetPointer()->tX_userselectedLeftLowerCornerPrimary;
				t7_change_field(1);	/* check if llc shows valid data */
				/* custom view primary OR debug if automatic return is off | T7 is default dive view => also initialize big font view */
				if((settingsGetPointer()->tX_customViewTimeout == 0) && (settingsGetPointer()->showDebugInfo))
				{
					selection_customview = CVIEW_noneOrDebug;
					t3_select_customview(CVIEW_noneOrDebug);
				}
				else
				{
					selection_customview = settingsGetPointer()->tX_customViewPrimary;
				}
				t7_change_customview(ACTION_END);

				if((settingsGetPointer()->MotionDetection != MOTION_DETECT_OFF))
				{
					InitMotionDetection();
				}

				if((settingsGetPointer()->extraDisplay == EXTRADISPLAY_BFACTIVE) && ( settingsGetPointer()->design == 7))
				{
					settingsGetPointer()->design = 3;
					t3_set_customview_to_primary();
					releaseAllFramesExcept(22,t7screen.FBStartAdress);
					releaseFrame(22,t7screen.FBStartAdress);
					set_globalState(StD);
					return;
				}
			}

			if(status.page == PageSurface)
				set_globalState(StD);

			t7_refresh_divemode();
       }
    }
    else // from if(stateUsed->mode == MODE_DIVE)
    {
        if(last_mode != MODE_SURFACE)
        {
            last_mode = MODE_SURFACE;
            selection_customview = customviewsSurface[0];
            InitMotionDetection();
            resetFocusState();
        }
        if(status.page == PageDive)
            set_globalState(StS);

#ifdef HAVE_DEBUG_VIEW
        if(gnssDebugPage && (selection_customview != CVIEW_Position))
        {
            gnssDebugPage = 0;		/* the view was left in another way than by the button */
        }
#endif

        if(settingsGetPointer()->showDebugInfo)
            t7_refresh_surface_debugmode();
#ifdef HAVE_DEBUG_VIEW
        else if(t7_isGnssDebugPageShown())
        {
            if(gnssDebugPage == 2)
                t7_refresh_gnss_hardware();
            else
                t7_refresh_gnss_debug();
        }
#endif
        else
            t7_refresh_surface();
    }

    tHome_show_lost_connection_count(&t7screen);

    if(status.base == BaseHome)
    {
        if(background.pointer)
        {
            GFX_SetFrameTop(t7screen.FBStartAdress);
            GFX_SetFrameBottom(background.pointer,background.x0 , background.y0, background.width, background.height);
        }
        else
            GFX_SetFramesTopBottom(t7screen.FBStartAdress, 0,480);
    }

    releaseAllFramesExcept(22,t7screen.FBStartAdress);
}

/* Private functions ---------------------------------------------------------*/

void t7_fill_surfacetime_helper(SSurfacetime *outArray, uint32_t inputMinutes, uint32_t inputSeconds)
{
    inputSeconds += inputMinutes * 60;

    outArray->Total = inputSeconds;

    outArray->Days = inputSeconds / 86400;// (24*60*60);
    inputSeconds -= 86400 * (uint32_t)outArray->Days;

    outArray->Hours = inputSeconds / 3600;// (60*60);
    inputSeconds -= 3600 * (uint32_t)outArray->Hours;

    outArray->Minutes = inputSeconds / 60;;
    inputSeconds -= 60 * (uint32_t)outArray->Minutes;

    outArray->Seconds = inputSeconds;
}

void t7_refresh_sleep_design_fun(void)
{
    static uint16_t state = 0;
    uint16_t ytop = 0;

    state +=1;
    if(state > 800)
        state = 1;

    if(state > 400)
        ytop = 800 - state;
    else
        ytop = 0 + state;
    Gfx_write_label_var(&t7screen,  300,800, ytop,&FontT48,CLUT_Font020,"Shutting down...");
}

void t7_refresh_surface(void)
{
	static float debounceAmbientPressure = 0;
	static uint8_t lastChargeStatus = 0;
	static float lastTemperature = 100.0;
    char text[200];
    char tmpString[20];

    uint8_t loop, textIdx;
    uint8_t date[3], year,month,day;
    uint32_t color;
    uint8_t  customview_warnings = 0;
    SButtonLock ButtonLockState;

    RTC_DateTypeDef Sdate;
    RTC_TimeTypeDef Stime;
    RTC_DateTypeDef SdateFirmware;

    uint8_t dateNotSet = 0;

    uint8_t oxygen_percentage, gasOffset, actualGasID;
#ifdef ENABLE_BOTTLE_SENSOR
	uint16_t bottleFirstGas_bar;
#endif
    point_t start, stop;//, other;

	SSettings* pSettings;
	pSettings = settingsGetPointer();


    // update in all customview modes
    if(sim_pin_update_necessary_off)
    {
        /* Sim pins the warning off; production path runs the real checks.
         * Reason: the CPU2 mock's lost_connection counters cycle faster
         * than the firmware polls them, so "Please update RTE" intermittently
         * appears mid-frame and desyncs from frame capture. Production sets
         * the flag to 0 (device_time_hooks_real.c) so RTE-update detection is
         * unaffected. */
        updateNecessary = 0;
    }
    else if(DataEX_check_RTE_version__needs_update() || font_update_required())
        updateNecessary = 1;
    else
        updateNecessary = 0;

    /* buttons */
    ButtonLockState = get_ButtonLock();
    if(ButtonLockState == LOCK_OFF)
    {
		text[0] = TXT_2BYTE;
		text[1] = TXT2BYTE_ButtonLogbook;
		text[2] = 0;
		write_content_simple(&t7screen, 0, 799, 479-TOP_LINE_HIGHT, &FontT24,text,CLUT_ButtonSurfaceScreen);

		text[0] = '\001';
		text[1] = TXT_2BYTE;
		text[2] = TXT2BYTE_ButtonView;
		text[3] = 0;
		write_content_simple(&t7screen, 0, 799, 479-TOP_LINE_HIGHT, &FontT24,text,CLUT_ButtonSurfaceScreen);

		text[0] = '\002';
		text[1] = TXT_2BYTE;
		text[2] = TXT2BYTE_ButtonMenu;
		text[3] = 0;
		write_content_simple(&t7screen, 0, 799, 479-TOP_LINE_HIGHT, &FontT24,text,CLUT_ButtonSurfaceScreen);
    }
    else
    {
    	switch(ButtonLockState)
    	{
    		default:
    		case LOCK_1:  	snprintf(text,255,"\a\001          ");
    			break;
    		case LOCK_2:  	snprintf(text,255,"\a\002          ");
    			break;
    		case LOCK_3:  	snprintf(text,255,"\a          ");
    			break;
    	}
    	write_content_simple(&t7screen, 0, 799, 479-TOP_LINE_HIGHT, &FontT24,text,CLUT_ButtonSurfaceScreen);
    	      //  GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);
    }
    /* was power on reset */
//.....
/* removed hw 160802 in V1.1.1
    if(errorsInSettings)
    {
        sprintf(text,"Settings: %u",errorsInSettings);
        GFX_write_string_color(&FontT42,&t7surfaceR,text,4,CLUT_WarningRed);
    }
    else
*/
    if(DataEX_was_power_on()) {
        GFX_write_string_color(&FontT42,&t7surfaceR,"cold start",4,CLUT_WarningRed);
    }

    /* time and date */
    translateDate(stateUsed->lifeData.dateBinaryFormat, &Sdate);
    translateTime(stateUsed->lifeData.timeBinaryFormat, &Stime);

    firmwareGetDate(&SdateFirmware);
    if(tHome_DateCode(&Sdate) < tHome_DateCode(&SdateFirmware))
        dateNotSet = 1;
    else
        dateNotSet = 0;



    if((dateNotSet) && (Stime.Seconds % 2 == 0))
    {
    	formatStringOfTime(tmpString,20,Stime,0,1);
    	snprintf(text,100,"\031\001%s",tmpString);
    }
    else
    {
    	formatStringOfTime(tmpString,20,Stime,1,1);
    	snprintf(text,100,"\030\001%s",tmpString);
    }
    GFX_write_string(&FontT48,&t7surfaceR,text,3);

    if(settingsGetPointer()->date_format == DDMMYY)
    {
        day = 0;
        month = 1;
        year = 2;
    }
    else
    if(settingsGetPointer()->date_format == MMDDYY)
    {
        day = 1;
        month = 0;
        year = 2;
    }
    else
    {
        day = 2;
        month = 1;
        year = 0;
    }
    date[day] = Sdate.Date;
    date[month] = Sdate.Month;
    date[year] = Sdate.Year;

    if((Stime.Seconds % 2) || (dateNotSet == 0))
        snprintf(text,255,"\001%02d.%02d.%02d",date[0],date[1],date[2]);
    else
        snprintf(text,255,"\001\031%02d.%02d.%02d",date[0],date[1],date[2]);

    GFX_write_string(&FontT54,&t7surfaceR,text,5);

    if(!DataEX_was_power_on() && !errorsInSettings)
    {
        text[0] = '\001';
        text[1] = '\004';
        text[2] = '\016';
        text[3] = '\016';
        text[4] = TXT_2BYTE;
        text[5] = TXT2BYTE_Sunday;
        text[6] = 0;
        if(Sdate.WeekDay != RTC_WEEKDAY_SUNDAY)
            text[5] += Sdate.WeekDay;

        if(!(Stime.Seconds % 2) && (dateNotSet == 1))
            text[1] = '\031';

        GFX_write_string(&FontT48,&t7surfaceR,text,4);
    }

    /* DEBUG uTick Pressure and Compass */
/*
    snprintf(text,255,"\001%u",stateRealGetPointer()->pressure_uTick_new - stateRealGetPointer()->pressure_uTick_old);
    GFX_write_string(&FontT42,&t7surfaceR,text,1);
    snprintf(text,255,"\001%u",HAL_GetTick() - stateRealGetPointer()->pressure_uTick_local_new);
    GFX_write_string(&FontT42,&t7surfaceR,text,2);

    snprintf(text,255,"\001%u",stateRealGetPointer()->compass_uTick_new - stateRealGetPointer()->compass_uTick_old);
    GFX_write_string(&FontT42,&t7surfaceR,text,6);
    snprintf(text,255,"\001%u",HAL_GetTick() - stateRealGetPointer()->compass_uTick_local_new);
    GFX_write_string(&FontT42,&t7surfaceR,text,7);

    static uint32_t bildschirmRefresh = 0;
    snprintf(text,255,"\001%u",HAL_GetTick() - bildschirmRefresh);
    GFX_write_string(&FontT42,&t7surfaceR,text,8);
    bildschirmRefresh = HAL_GetTick();

    static uint16_t bildschirmRefreshCount = 1;
    if(bildschirmRefreshCount>10)
        bildschirmRefreshCount = 1;
    for(int i=0;i<bildschirmRefreshCount;i++)
        text[i] = '.';
        text[bildschirmRefreshCount++] = 0;
    GFX_write_string(&FontT42,&t7surfaceR,text,4);
*/

    /* noFlyTime or DesaturationTime */
    if((stateUsed->lifeData.no_fly_time_minutes) && ((!display_count_high_time) || (stateUsed->lifeData.desaturation_time_minutes == 0)))
    {
        SSurfacetime NoFlyTime = {0,0,0,0};
        t7_fill_surfacetime_helper(&NoFlyTime,stateUsed->lifeData.no_fly_time_minutes, 0);

        if(NoFlyTime.Days)
        {
            snprintf(text,30,"\001%02d\016\016d\017 %02d\016\016h\017",NoFlyTime.Days, NoFlyTime.Hours);
        }
        else
        {
            snprintf(text,20,"\001%02d:%02d",NoFlyTime.Hours, NoFlyTime.Minutes);
        }

        GFX_write_string(&FontT54,&t7surfaceR,text,7);

        text[0] = '\001';
        text[1] = '\022';
        text[2] = '\016';
        text[3] = '\016';
        text[4] = TXT_2BYTE;
        text[5] = TXT2BYTE_noFly;
        text[6] = 0;
        GFX_write_string(&FontT48,&t7surfaceR,text,6);
    }
    else
    if(stateUsed->lifeData.desaturation_time_minutes)
    {
        SSurfacetime DesatTime = {0,0,0,0};
        t7_fill_surfacetime_helper(&DesatTime,stateUsed->lifeData.desaturation_time_minutes, 0);

        if(DesatTime.Days)
        {
            snprintf(text,30,"\001%02d\016\016d\017 %02d\016\016h\017",DesatTime.Days, DesatTime.Hours);
        }
        else
        {
            snprintf(text,20,"\001%02d:%02d",DesatTime.Hours, DesatTime.Minutes);
        }
        GFX_write_string(&FontT54,&t7surfaceR,text,7);

            text[0] = '\001';
            text[1] = '\022';
            text[2] = '\016';
            text[3] = '\016';
            text[4] = TXT_2BYTE;
            text[5] = TXT2BYTE_Desaturation;
            text[6] = 0;
            GFX_write_string(&FontT48,&t7surfaceR,text,6);
    }

    /* Time since last dive */
    if(stateUsed->lifeData.surface_time_seconds)
    {
        SSurfacetime SurfTime = {0,0,0,0};
        t7_fill_surfacetime_helper(&SurfTime, 0, stateUsed->lifeData.surface_time_seconds);

        if(SurfTime.Days == 0)
        {
            snprintf(text,20,"\001\022%02d:%02d",SurfTime.Hours, SurfTime.Minutes);
        }
        else
        {
            snprintf(text,30,"\001\022%02d\016\016d\017 %02d\016\016h\017",SurfTime.Days, SurfTime.Hours);
        }

        GFX_write_string(&FontT54,&t7surfaceR,text,2);


        text[0] = '\001';
        text[1] = '\022';
        text[2] = '\016';
        text[3] = '\016';
        text[4] = TXT_2BYTE;
        text[5] = TXT2BYTE_TimeSinceLastDive;
        text[6] = 0;
        GFX_write_string(&FontT48,&t7surfaceR,text,1);
    }

    if (stateUsed->timerState == TIMER_STATE_RUNNING && selection_customview != CVIEW_Timer) {
        int timerRemainingS = pSettings->timerDurationS - (current_second() - stateUsed->timerStartedS);
        snprintf(text, 20, "\001%u:%02u", timerRemainingS / 60, timerRemainingS % 60);
        GFX_write_string(&FontT54, &t7surfaceR, text, 8);
    }

    /* beta version */
    if( firmwareDataGetPointer()->versionBeta )
    {
        snprintf(text,255,"\025 BETA");
        GFX_write_string(&FontT48,&t7surfaceL,text,2);
    }

    /* surface pressure  and temperature */
    if(stateUsed->sensorErrorsRTE == 0)
    {
    	if(fabs(stateUsed->lifeData.pressure_surface_bar - stateUsed->lifeData.pressure_ambient_bar) < SHOW_AMBIENTE_SURFACE_DELTA)	/* show ambient pressure if difference to surface is significant*/
    	{
    		snprintf(text,30,"%01.0f\022\016\016 %s", stateUsed->lifeData.pressure_surface_bar * 1000.0f,TEXT_PRESSURE_UNIT);
    	}
    	else
    	{
    		if(fabsf(debounceAmbientPressure - stateUsed->lifeData.pressure_ambient_bar) > SHOW_AMBIENTE_DEBOUNCE)   /* there might be a jitter ~+-1 HPa on the pressure signal => update only if delta is bigger */
    		{
    			debounceAmbientPressure = stateUsed->lifeData.pressure_ambient_bar;
    		}
    		snprintf(text,30,"%01.0f\022\016\016 %s", debounceAmbientPressure * 1000.0f,TEXT_PRESSURE_UNIT);
    	}

        GFX_write_string(&FontT48,&t7surfaceL,text,3);

        if(fabsf(stateUsed->lifeData.temperature_celsius - lastTemperature) > SHOW_TEMPERATURE_DEBOUNCE)
        {
        	lastTemperature = stateUsed->lifeData.temperature_celsius;
        }
        if(settingsGetPointer()->nonMetricalSystem)
        	snprintf(text,40,"%01.0f\140\022\016\016 fahrenheit",unit_temperature_float(lastTemperature));
        else
        	snprintf(text,30,"%01.0f\140\022\016\016 celsius",lastTemperature);
        GFX_write_string(&FontT48,&t7surfaceL,text,4);
    }
    else
    {
        snprintf(text,30,"ERR\022\016\016 %s",TEXT_PRESSURE_UNIT);
        GFX_write_string(&FontT48,&t7surfaceL,text,3);

        if(settingsGetPointer()->nonMetricalSystem)
            snprintf(text,40,"ERR\022\016\016 fahrenheit");
        else
            snprintf(text,30,"ERR\022\016\016 celsius");
        GFX_write_string(&FontT48,&t7surfaceL,text,4);
    }


    /* gas mix and selection */
    if((stateUsed->diveSettings.diveMode == DIVEMODE_Gauge) || (stateUsed->diveSettings.diveMode == DIVEMODE_Apnea))
    {
        if(stateUsed->diveSettings.diveMode == DIVEMODE_Gauge)
            text[0] = TXT_Gauge;
        else
            text[0] = TXT_Apnoe;

        text[1] = 0;
        GFX_write_string(&FontT48,&t7surfaceL,text,6);
    }
    else
    {
    	textIdx = 0;

        if(isLoopMode(stateUsed->diveSettings.diveMode))
            gasOffset = NUM_OFFSET_DILUENT;
        else
            gasOffset = 0;

    	/* Display gas setup */
    	for(loop = 1; loop <= NUM_GASES; loop++)
    	{
#ifdef ENABLE_UNUSED_GAS_HIDING
    		if(stateUsed->diveSettings.gas[loop+gasOffset].note.ub.off)
    		{
    			text[textIdx++] = '\021';
    		}
    		else
#endif
    		if(stateUsed->diveSettings.gas[loop+gasOffset].note.ub.active)
    		{
    			text[textIdx++]= '\020';
    		}
    		else
    		{
    			text[textIdx++]= '\031';
    		}

   			text[textIdx++] = '0' + loop;
			text[textIdx++] = '\177';
			text[textIdx++] = '\177';
			text[textIdx++] = 10;
    	}
    	text[textIdx++] = 0;
    	if(!pSettings->FlipDisplay)
    	{
    		t7surfaceL.WindowX0 +=5;
    	}
    	else
    	{
    		t7surfaceL.WindowX1 -=2;
    	}
        GFX_write_string(&FontT48,&t7surfaceL,text,6);
        if(!pSettings->FlipDisplay)
        {
        	t7surfaceL.WindowX0 -=5;
        }
        else
        {
        	t7surfaceL.WindowX1 +=2;
        }
        oxygen_percentage = 100;
        oxygen_percentage -= stateUsed->lifeData.actualGas.nitrogen_percentage;
        oxygen_percentage -= stateUsed->lifeData.actualGas.helium_percentage;

        tHome_gas_writer(oxygen_percentage,stateUsed->lifeData.actualGas.helium_percentage,&text[0]);
        GFX_write_string(&FontT48,&t7surfaceL,text,7);

        actualGasID = stateUsed->lifeData.actualGas.GasIdInSettings;

#ifdef ENABLE_BOTTLE_SENSOR
        bottleFirstGas_bar = stateUsed->lifeData.bottle_bar[actualGasID];
        if(bottleFirstGas_bar)
        {
            snprintf(text,255,"%3u\022\016\016 bar",bottleFirstGas_bar);
            GFX_write_string(&FontT48,&t7surfaceL,text,8);
        }
#endif
        // after gas name :-)
        if(actualGasID > gasOffset) // security
        {
        	if(!pSettings->FlipDisplay)
        	{
        		start.y = t7surfaceL.WindowY0 + (3 * t7surfaceL.WindowLineSpacing);
        		start.x = t7surfaceL.WindowX0 + ((stateUsed->lifeData.actualGas.GasIdInSettings - gasOffset - 1) * 35) + 2;
        	}
			else
			{
				start.y = t7surfaceR.WindowY0 + (3 * t7surfaceR.WindowLineSpacing);
				start.x = t7surfaceR.WindowX0 + ((stateUsed->lifeData.actualGas.GasIdInSettings - gasOffset - 1) * 35);
			}

            stop.x = start.x + 29;
            stop.y = start.y + 52;
            GFX_draw_box2(&t7screen, start, stop, CLUT_Font020, 1);
        }
    }

    /* dive mode */
	switch (stateUsed->diveSettings.diveMode) {
	case DIVEMODE_CCR:
		GFX_write_string(&FontT24, &t7c1, "\f\002" "CCR", 0);
		break;
	case DIVEMODE_PSCR:
		GFX_write_string(&FontT24, &t7c1, "\f\002" "PSCR", 0);
		break;
	case DIVEMODE_OC:
		GFX_write_string(&FontT24, &t7c1, "\f\002" "OC", 0);
		break;
	case DIVEMODE_Gauge:
		GFX_write_string(&FontT24, &t7c1, "\f\002" "Gauge", 0);
		break;
	case DIVEMODE_Apnea:
		GFX_write_string(&FontT24, &t7c1, "\f\002" "Apnea", 0);
		break;
	default:
		GFX_write_string(&FontT24, &t7c1, "\f\002" "OC", 0);
	}

    /*battery */

    text[0] = '3';
    text[1] = '1';
    text[2] = '1';
    text[3] = '1';
    text[4] = '1';
    text[5] = '1';
    text[6] = '1';
    text[7] = '1';
    text[8] = '1';
    text[9] = '1';
    text[10] = '1';
    text[11] = '0';
    text[12] = 0;

    for(int i=1;i<=10;i++)
    {
        if(	stateUsed->lifeData.battery_charge > (9 * i))
            text[i] += 1;
    }

    if(stateUsed->chargeStatus == CHARGER_off)
    {
        if(stateUsed->warnings.lowBattery)
        {
            if(warning_count_high_time)
            {
                for(int i=1;i<=10;i++)
                    text[i] = '1';
            }
            else
            {
                text[1] = '2';
            }
            GFX_write_string_color(&Batt24,&t7batt,text,0,CLUT_WarningRed);
            if((stateUsed->lifeData.battery_charge > 0) && (stateUsed->lifeData.battery_charge < 140))
            {
#ifdef ALWAYS_SHOW_VOLTAGE
            	// show battery percent and voltage
                snprintf(text,16,"\f\002%u%% \f%.1fV",(uint8_t)stateUsed->lifeData.battery_charge,stateUsed->lifeData.battery_voltage);
#else
                snprintf(text,16,"\004\025\f\002%u%%",(uint8_t)stateUsed->lifeData.battery_charge);
#endif
                if(warning_count_high_time)
                    text[0] = '\a';
                GFX_write_string(&FontT24,&t7voltage,text,0);
            }
            else
            {
                snprintf(text,6,"\f%.1fV",stateUsed->lifeData.battery_voltage);
                GFX_write_string(&FontT24,&t7voltage,text,0);
            }
        }
        else
        {
            GFX_write_string_color(&Batt24,&t7batt,text,0,CLUT_BatteryStandard);

            if((stateUsed->lifeData.battery_charge > 0) && (stateUsed->lifeData.battery_charge < 140))
            {
#ifdef ALWAYS_SHOW_VOLTAGE
            	// show battery percent and voltage
                snprintf(text,16,"\f\002%u%% \f%.1fV",(uint8_t)stateUsed->lifeData.battery_charge,stateUsed->lifeData.battery_voltage);
#else
                 snprintf(text,16,"\f\002%u%%",(uint8_t)stateUsed->lifeData.battery_charge);
#endif
                GFX_write_string(&FontT24,&t7voltage,text,0);
            }
            else
            {
                snprintf(text,6,"\f%.1fV",stateUsed->lifeData.battery_voltage);
                GFX_write_string(&FontT24,&t7voltage,text,0);
            }
        }
    }
    else
    {
        if(lastChargeStatus == CHARGER_off)
        {
        	t7_select_customview(CVIEW_Charger);
        }

        GFX_write_string_color(&Batt24,&t7batt,text,0,CLUT_BatteryCharging);

        switch(stateUsed->chargeStatus)
        {
        case CHARGER_running:
        default:
            color = CLUT_BatteryStandard;
            break;
        case CHARGER_complete:
            color = CLUT_BatteryCharging;
            break;
        case CHARGER_lostConnection:
            color = CLUT_BatteryProblem;
            break;
        }
        text[0] = '4';
        text[1] = 0;
        GFX_write_string_color(&Batt24,&t7charge,text,0,color);
    }

    lastChargeStatus = stateUsed->chargeStatus;


    customview_warnings = t7_test_customview_warnings_surface_mode();
    if(customview_warnings && warning_count_high_time)
        t7_show_customview_warnings_surface_mode();
    else
        t7_refresh_customview();
    draw_frame(0,0, CLUT_pluginboxSurface, CLUT_Font020);
}

void t7_refresh_surface_debugmode(void)
{
    // could be warning, now just to set RTE variables
    DataEX_check_RTE_version__needs_update();


    char TextL1[4*TEXTSIZE];
    uint32_t color;
//	uint8_t gasIdFirst;
    SSettings* pSettings = settingsGetPointer();
    SDataExchangeSlaveToMaster *dataIn = get_dataInPointer();

    SWindowGimpStyle windowGimp;

    RTC_DateTypeDef Sdate;
    RTC_TimeTypeDef Stime;

    translateDate(stateUsed->lifeData.dateBinaryFormat, &Sdate);
    translateTime(stateUsed->lifeData.timeBinaryFormat, &Stime);


    if(stateUsed->data_old__lost_connection_to_slave)
    {
        Gfx_write_label_var(&t7screen,  500,800,  0,&FontT42,CLUT_DiveMainLabel,"old");
        snprintf(TextL1,TEXTSIZE,"%X %X %X %X",dataIn->header.checkCode[0],dataIn->header.checkCode[1],dataIn->header.checkCode[2],dataIn->header.checkCode[3]);
        Gfx_write_label_var(&t7screen,  500,800, 45,&FontT48,CLUT_Font020,TextL1);
    }
    else
    if(DataEX_lost_connection_count())
    {
        snprintf(TextL1,TEXTSIZE,"\002%ld",DataEX_lost_connection_count());
        Gfx_write_label_var(&t7screen,  600,800, 45,&FontT48,CLUT_Font020,TextL1);
    }

    snprintf(TextL1,TEXTSIZE,"\002%i",blockedFramesCount());
    Gfx_write_label_var(&t7screen,  600,800, 0,&FontT48,CLUT_Font020,TextL1);

    if(stateUsed->lifeData.compass_DX_f | stateUsed->lifeData.compass_DY_f | stateUsed->lifeData.compass_DZ_f)
    {
        snprintf(TextL1,TEXTSIZE,"X %i",stateUsed->lifeData.compass_DX_f);
        Gfx_write_label_var(&t7screen,  0,400, 45,&FontT48,CLUT_Font020,TextL1);
        snprintf(TextL1,TEXTSIZE,"Y %i",stateUsed->lifeData.compass_DY_f);
        Gfx_write_label_var(&t7screen,  0,400,145,&FontT48,CLUT_Font020,TextL1);
        snprintf(TextL1,TEXTSIZE,"Z %i",stateUsed->lifeData.compass_DZ_f);
        Gfx_write_label_var(&t7screen,  0,400,255,&FontT48,CLUT_Font020,TextL1);
        return;
    }
    snprintf(TextL1,TEXTSIZE,"%01.0f %s",stateUsed->lifeData.pressure_ambient_bar * 1000.0f,TEXT_PRESSURE_UNIT);
    Gfx_write_label_var(&t7screen,  0,400,  0,&FontT42,CLUT_DiveMainLabel,"Ambient Pressure");
    Gfx_write_label_var(&t7screen,  0,400, 45,&FontT48,CLUT_Font020,TextL1);

    snprintf(TextL1,TEXTSIZE,"%01.2f C",stateUsed->lifeData.temperature_celsius);
    Gfx_write_label_var(&t7screen,  0,400,100,&FontT42,CLUT_DiveMainLabel,"Temperature");
    Gfx_write_label_var(&t7screen,  0,400,145,&FontT48,CLUT_Font020,TextL1);

    snprintf(TextL1,TEXTSIZE,"%03.0f %03.0f %03.0f",stateUsed->lifeData.compass_heading,stateUsed->lifeData.compass_roll,stateUsed->lifeData.compass_pitch);
    Gfx_write_label_var(&t7screen,  0,400,200,&FontT42,CLUT_DiveMainLabel,"Heading Roll Pitch");
    Gfx_write_label_var(&t7screen,  0,400,255,&FontT48,CLUT_Font020,TextL1);

    snprintf(TextL1,TEXTSIZE,"%01.0f %s",stateUsed->lifeData.pressure_surface_bar * 1000.0f,TEXT_PRESSURE_UNIT);
    Gfx_write_label_var(&t7screen,  0,400,310,&FontT42,CLUT_DiveMainLabel,"Surface Pressure");
    Gfx_write_label_var(&t7screen,  0,400,355,&FontT48,CLUT_Font020,TextL1);

//	gasIdFirst = stateUsed->lifeData.actualGas.GasIdInSettings;
    snprintf(TextL1,TEXTSIZE,"%u.%u",dataIn->RTE_VERSION_high,dataIn->RTE_VERSION_low);
    Gfx_write_label_var(&t7screen,  320,500,100,&FontT42,CLUT_DiveMainLabel,"RTE");
    Gfx_write_label_var(&t7screen,  320,500,145,&FontT48,CLUT_Font020,TextL1);

    Gfx_write_label_var(&t7screen,  500,800,100,&FontT42,CLUT_DiveMainLabel,"Battery");
    snprintf(TextL1,TEXTSIZE,"%01.4f V",stateUsed->lifeData.battery_voltage);
    Gfx_write_label_var(&t7screen,  500,800,145,&FontT48,CLUT_Font020,TextL1);
    snprintf(TextL1,TEXTSIZE,"%03.1f %%",stateUsed->lifeData.battery_charge);
    Gfx_write_label_var(&t7screen,  500,800,200,&FontT48,CLUT_Font020,TextL1);
    if(stateUsed->chargeStatus != CHARGER_off)
    {
        switch(stateUsed->chargeStatus)
        {
        case CHARGER_running:
        default:
            color = CLUT_BatteryStandard;
            break;
        case CHARGER_complete:
            color = CLUT_BatteryCharging;
            break;
        case CHARGER_lostConnection:
            color = CLUT_BatteryProblem;
            break;
        }
        TextL1[0] = '4';
        TextL1[1] = 0;
        Gfx_write_label_var(&t7screen,  660,800,200,&Batt24,color,TextL1);
    }

extern uint32_t base_tempLightLevel;

    snprintf(TextL1,TEXTSIZE,"# %u (%ld)",stateUsed->lifeData.ambient_light_level, base_tempLightLevel);
    Gfx_write_label_var(&t7screen,  401,600,310,&FontT42,CLUT_DiveMainLabel,"Light");
    Gfx_write_label_var(&t7screen,  401,800,355,&FontT48,CLUT_Font020,TextL1);

//	snprintf(TextL1,TEXTSIZE,"# %u",stateUsed->lifeData.ambient_light_level);
//	Gfx_write_label_var(&t7screen,  601,800,310,&FontT42,CLUT_DiveMainLabel,"Light");
//	Gfx_write_label_var(&t7screen,  601,800,355,&FontT48,CLUT_Font020,TextL1);

/* show surface pressure state */
    if(stateUsed->lifeData.bool_temp1 )
    {
    	snprintf(TextL1,TEXTSIZE,"stable");
    }
    else
    {
    	snprintf(TextL1,TEXTSIZE,"unstable");
    }
    Gfx_write_label_var(&t7screen,  500,800,400,&FontT48,CLUT_Font020,TextL1);


    if(Sdate.Year < 15)
    {
        if(warning_count_high_time)
        {
            snprintf(TextL1,4*TEXTSIZE,"\017 %02d-%02d-%02d  %02d:%02d:%02d", Sdate.Date, Sdate.Month, 2000 + Sdate.Year,Stime.Hours, Stime.Minutes, Stime.Seconds);
            Gfx_write_label_var(&t7screen,  0,800,420,&FontT48,CLUT_Font020,TextL1);
        }
    }
    else
    {
        if(pSettings->customtext[0])
        {
            if(pSettings->customtext[59])
                pSettings->customtext[59] = 0;
            Gfx_write_label_var(&t7screen,  0,400,420,&FontT24,CLUT_Font020,pSettings->customtext);
        }
        else
        {
            snprintf(TextL1,4*TEXTSIZE,"\017 %02d-%02d-%02d  %02d:%02d:%02d  Dives: %u", Sdate.Date, Sdate.Month, 2000 + Sdate.Year,Stime.Hours, Stime.Minutes, Stime.Seconds,pSettings->totalDiveCounter			);
            Gfx_write_label_var(&t7screen,  0,800,420,&FontT48,CLUT_Font020,TextL1);
        }
    }

    windowGimp.left = 400;
    windowGimp.top = 0;
    GFX_draw_image_monochrome(&t7screen, windowGimp, &ImgOSTC, 0);
}

/* CUSTOMVIEW
 * in the middle of the screen
 */

uint8_t t7_test_customview_warnings(void)
{
    uint8_t count = 0;

    count = 0;
    count += stateUsed->warnings.decoMissed;
    count += stateUsed->warnings.ppO2Low;
    count += stateUsed->warnings.ppO2High;
    //count += stateUsed->warnings.lowBattery;
    count += stateUsed->warnings.sensorLinkLost;
    count += stateUsed->warnings.fallback;
    count += stateUsed->warnings.co2High;
#ifdef ENABLE_BOTTLE_SENSOR
    if(stateUsed->warnings.newPressure)
    {
    	count++;
    }
#endif
#ifdef HAVE_DEBUG_WARNINGS
    count += stateUsed->warnings.debug;
#endif
#ifdef ENABLE_CAVEMODE
    count += stateUsed->warnings.lowGasBudget;
#endif
    return count;
}


uint8_t t7_test_customview_warnings_surface_mode(void)
{
    uint8_t count = 0;
    count = 0;
    count += stateUsed->cnsHigh_at_the_end_of_dive;
    count += stateUsed->decoMissed_at_the_end_of_dive;
#ifdef ENABLE_BOTTLE_SENSOR
    if(stateUsed->warnings.newPressure)
    {
    	count++;
    }
#endif
    return count;
}


void t7_show_customview_warnings_surface_mode(void)
{
    char text[256];
    uint8_t textpointer, lineFree;

    text[0] = '\025';
    text[1] = '\f';
    text[2] = '\001';
    text[3] = TXT_Warning;
    text[4] = 0;
    GFX_write_string(&FontT42,&t7cH,text,0);

    textpointer = 0;
    lineFree = 5;

    if(stateUsed->decoMissed_at_the_end_of_dive)
    {
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnDecoMissed;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }

    if(stateUsed->cnsHigh_at_the_end_of_dive)
    {
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnCnsHigh;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }
#ifdef ENABLE_BOTTLE_SENSOR
    if(stateUsed->warnings.newPressure)
    {
    	sprintf(&text[textpointer] ,"  %u Bar\n", stateUsed->warnings.newPressure);
    	textpointer++;
        lineFree--;
    }
#endif
    if(textpointer != 0)
        GFX_write_string(&FontT48,&t7cW,text,1);
}


void t7_show_customview_warnings(void)
{
    char text[256];
    uint8_t textpointer, lineFree;
#ifdef HAVE_DEBUG_WARNINGS
    uint8_t index = 0;
#endif

    text[0] = '\025';
    text[1] = '\f';
    text[2] = '\001';
    text[3] = TXT_Warning;
    text[4] = 0;
    GFX_write_string(&FontT42,&t7cH,text,0);

    textpointer = 0;
    lineFree = 5;

    if(lineFree && stateUsed->warnings.decoMissed)
    {
    	text[textpointer++] = '\001';
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnDecoMissed;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }

    if(lineFree && stateUsed->warnings.fallback)
    {
    	text[textpointer++] = '\001';
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnFallback;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }

    if(lineFree && stateUsed->warnings.ppO2Low)
    {
    	text[textpointer++] = '\001';
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnPPO2Low;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }

    if(lineFree && stateUsed->warnings.ppO2High)
    {
    	text[textpointer++] = '\001';
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnPPO2High;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }

    if(lineFree && stateUsed->warnings.sensorLinkLost)
    {
    	text[textpointer++] = '\001';
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnSensorLinkLost;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }
#ifdef ENABLE_BOTTLE_SENSOR
    if(stateUsed->warnings.newPressure)
    {
    	text[textpointer++] = '\001';
    	sprintf(&text[textpointer],"  %u Bar\n", stateUsed->warnings.newPressure);
    	textpointer++;
        lineFree--;
    }
#endif
#ifdef ENABLE_CO2_SUPPORT
    if(lineFree && stateUsed->warnings.co2High)
    {
    	text[textpointer++] = '\001';
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnCO2High;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }
#endif
#ifdef HAVE_DEBUG_WARNINGS
    if(lineFree && stateUsed->warnings.debug)
    {
	    for(index=0; index<3; index++)
	    {
        	if(((stateUsed->lifeData.extIf_sensor_map[index] == SENSOR_DIGO2M) && (((SSensorDataDiveO2*)(stateUsed->lifeData.extIf_sensor_data[index]))->status & DVO2_FATAL_ERROR)))
        	{
        		textpointer += snprintf(&text[textpointer],32,"\001Debug: %lx\n",((SSensorDataDiveO2*)(stateUsed->lifeData.extIf_sensor_data[index]))->status);
        	}
	    }
        lineFree--;
    }
#endif
#ifdef ENABLE_CAVEMODE
    if(lineFree && stateUsed->warnings.lowGasBudget)
    {
    	textpointer += sprintf(&text[textpointer],"\001\016\016%c\017\n", TXT_AtemGasVorrat);
        lineFree--;
    }
#endif
/*
    if(lineFree && stateUsed->warnings.lowBattery)
    {
        text[textpointer++] = TXT_2BYTE;
        text[textpointer++] = TXT2BYTE_WarnBatteryLow;
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer] = 0;
        lineFree--;
    }
*/
    GFX_write_string(&FontT48,&t7cW,text,1);
   	requestBuzzerActivation(REQUEST_BUZZER_CONTINUOUS);
}


void t7_set_customview_to_primary(void)
{
    if(stateUsed->mode == MODE_DIVE)
            selection_customview = settingsGetPointer()->tX_customViewPrimary;
}

uint8_t t7_GetEnabled_customviews()
{
	int8_t i;
    uint8_t *pViews;
    uint8_t increment = 1;

    uint8_t enabledViewCnt = 0;
    uint32_t cv_config = settingsGetPointer()->cv_configuration;

    pViews = (uint8_t*)customviewsDive;

    while((*pViews != CVIEW_END))
    {
    	increment = 1;
    /* check if view is enabled */
    	i=0;
    	do
        {
             if(*pViews == cv_changelist[i])
             {
            	 if(!CHECK_BIT_THOME(cv_config, cv_changelist[i]))
             	 {
            	 	 increment = 0;
             	 }
                 break;
             }
             i++;
        } while(cv_changelist[i] != CVIEW_END);
    	if(cv_changelist[i] == CVIEW_END)
    	{
    		 increment = 0;
    	}
        if (((*pViews == CVIEW_sensors) || (*pViews == CVIEW_sensors_mV)) &&
           	((stateUsed->diveSettings.ppo2sensors_deactivated == 0x07) || (stateUsed->diveSettings.ccrOption == 0)))
        {
        	increment = 0;
        }

    	pViews++;
    	enabledViewCnt += increment;
    }
    return enabledViewCnt;
}

uint8_t t7_customview_disabled(uint8_t view)
{
	uint8_t i = 0;
	uint8_t cv_disabled = 0;
    SSettings *settings = settingsGetPointer();

   	while(cv_changelist[i] != CVIEW_END)
    {
         if((view == cv_changelist[i]) && !CHECK_BIT_THOME(settings->cv_configuration, cv_changelist[i]))
         {
        	 cv_disabled = 1;
               break;
         }
         i++;
    }

#ifndef ENABLE_CAVEMODE
  	if ((view == CVIEW_Cave) || (view == CVIEW_GasDemand))	/* demand calculation is made by cavemode functionality */
  	{
  		cv_disabled = 1;
  	}
#endif

  	if ((view == CVIEW_Position) && (!isGnssAvailable()))
  	{
  	  		cv_disabled = 1;
  	}

    if (((view == CVIEW_sensors) || (view == CVIEW_sensors_mV)) &&
       	((stateUsed->diveSettings.ppo2sensors_deactivated == 0x07) || (stateUsed->diveSettings.ccrOption == 0) || (stateUsed->warnings.fallback)))
    {
      	cv_disabled = 1;
    }

    if ((view == CVIEW_Charger) && (stateUsed->chargeStatus != CHARGER_running) && (stateUsed->chargeStatus != CHARGER_lostConnection))
    {
       	cv_disabled = 1;
    }

    if (view == CVIEW_CcrSummary && !isLoopMode(settings->dive_mode)) {
        cv_disabled = 1;
    }

    return cv_disabled;
}

uint8_t t7_change_customview(uint8_t action)
{
    uint8_t *pViews;
    uint8_t *pStartView,*pLastView;
    uint8_t *pCurView = NULL;
    _Bool cv_disabled = 0;

    if(stateUsed->mode == MODE_DIVE)
        pViews = (uint8_t*)customviewsDive;
    else
        pViews = (uint8_t*)customviewsSurface;

    pStartView = pViews;
    /* set pointer to currently selected view and count number of entries */
    while((*pViews != CVIEW_END))
    {
    	if (*pViews == selection_customview)
    	{
    		pCurView = pViews;
    	}
    	pViews++;
    }
    pLastView = pViews;
    pViews = pCurView;

    do
    {
		switch(action)
		{
			case ACTION_BUTTON_ENTER:
			case ACTION_PITCH_POS:
						if(*pViews != CVIEW_END)
					pViews++;
						if(*pViews == CVIEW_END)
				{
					pViews = pStartView;
				}
				break;
			case ACTION_PITCH_NEG:
				if(pViews == pStartView)
				{
					pViews = pLastView - 1;
				}
				else
				{
					pViews--;
				}
				break;
			default:
				break;
		}

		cv_disabled = t7_customview_disabled(*pViews);
		if((cv_disabled) && (action == ACTION_END))
		{
			action = ACTION_BUTTON_ENTER;
		}
    } while(cv_disabled);

    selection_customview = *pViews;
    return *pViews;
}

void t7_select_customview(uint8_t selectedCustomview)
{
	if(selectedCustomview < CVIEW_END)
	{
		selection_customview = selectedCustomview;
	}
}

uint8_t t7_get_length_of_customtext(void)
{
    uint8_t i = 0;
    settingsGetPointer()->customtext[60-1] = 0;
    while(settingsGetPointer()->customtext[i] > 0)
        i++;
    return i;
}


static bool setpointIsActive(SSettings *settings, unsigned setpointIndex)
{
    if (settings->autoSetpoint && setpointIndex == SETPOINT_INDEX_AUTO_DECO) {
        return settings->setpoint[setpointIndex].note.ub.active;
    }

    return true;
}


static void t7_CcrSummary(SSettings *settings)
{
    unsigned numLines = 1; // CCR Mode

    SGasLine *diluentsToShow[MAX_NUM_SUMMARY_LINES - 1] = { NULL };
    unsigned i = NUM_OFFSET_DILUENT + 1;
    // Add diluent at start of dive
    while (i <= NUM_OFFSET_DILUENT + 1 + NUM_GASES) {
        if (settings->gas[i].note.ub.active && settings->gas[i].note.ub.first) {
            diluentsToShow[0] = &settings->gas[i];
            numLines++;

            break;
        }

        i++;
    }

    bool showScrubberTime = false;
    if (isScrubberTimerEnabled(settings)) {
        numLines++;
        showScrubberTime = true;
    }

    bool showManualSetpoints = false;
    unsigned offset;
    SSetpointLine *setpointsToShow[MAX_NUM_SUMMARY_LINES - 1] = { NULL };
    if (settings->CCR_Mode == CCRMODE_FixedSetpoint || settings->fallbackToFixedSetpoint) {
        offset = numLines;
        if (settings->autoSetpoint) {
            setpointsToShow[numLines++ - offset] = &settings->setpoint[SETPOINT_INDEX_AUTO_LOW];
            setpointsToShow[numLines++ - offset] = &settings->setpoint[SETPOINT_INDEX_AUTO_HIGH];
            if (setpointIsActive(settings, SETPOINT_INDEX_AUTO_DECO)) {
                setpointsToShow[numLines++ - offset] = &settings->setpoint[SETPOINT_INDEX_AUTO_DECO];
            }
        } else {
            // Add setpoint at start of dive
            i = 1;
            while (i <= NUM_GASES) {
                if (setpointIsActive(settings, i) && settings->setpoint[i].note.ub.first) {
                    setpointsToShow[numLines - offset] = &settings->setpoint[i];
                    numLines++;

                    break;
                }

                i++;
            }

            showManualSetpoints = true;
        }
    }

    // Add remaining active diluents
    i = NUM_OFFSET_DILUENT + 1;
    offset = numLines;
    if (diluentsToShow[0]) {
        offset--;
    }
    while (i < NUM_OFFSET_DILUENT + 1 + NUM_GASES && numLines < MAX_NUM_SUMMARY_LINES) {
        if (settings->gas[i].note.ub.active && !settings->gas[i].note.ub.first) {
            diluentsToShow[numLines - offset] = &settings->gas[i];
            numLines++;
        }

        i++;
    }

    if (showManualSetpoints) {
        // Fill up the remaining lines with setpoints
        offset = numLines;
        if (setpointsToShow[0]) {
            offset--;
        }
        i = 2;
        while (i <= NUM_GASES && numLines < MAX_NUM_SUMMARY_LINES) {
            if (setpointIsActive(settings, i) && !settings->setpoint[i].note.ub.first) {
                setpointsToShow[numLines - offset] = &settings->setpoint[i];
                numLines++;
            }

            i++;
        }
    }

    char heading[128];
    unsigned headingIndex = 0;

    char data[128];
    unsigned dataIndex = 0;

    heading[headingIndex++] = '\032';
    heading[headingIndex++] = '\016';
    heading[headingIndex++] = '\016';
    heading[headingIndex++] = TXT_CCRmode;

    data[dataIndex++] = '\002';
    char *modeText;
    if (settings->CCR_Mode == CCRMODE_Sensors) {
        if (settings->fallbackToFixedSetpoint) {
            modeText = (char *)&"Sens/FB";
        } else {
            modeText = (char *)&"Sensor";
        }
    } else {
        modeText = (char *)&"Fixed";
    }
    dataIndex += snprintf(&data[dataIndex], 10, "\020%s", modeText);

    i = 0;
    while (setpointsToShow[i] != NULL && i < MAX_NUM_SUMMARY_LINES - 1) {
        heading[headingIndex++] = '\n';
        heading[headingIndex++] = '\r';
        if (settings->autoSetpoint) {
            heading[headingIndex++] = TXT_2BYTE;
            heading[headingIndex++] = TXT2BYTE_SetpointShort;
            headingIndex += printSetpointName(&heading[headingIndex], i + 1, settings, true);
        } else if (i == 0) {
            heading[headingIndex++] = TXT_2BYTE;
            heading[headingIndex++] = TXT2BYTE_Setpoint;
        }

        data[dataIndex++] = '\n';
        data[dataIndex++] = '\r';
        data[dataIndex++] = '\002';
        dataIndex += snprintf(&data[dataIndex], 10, "\020%01.2f", setpointsToShow[i]->setpoint_cbar / 100.0);
        if (setpointsToShow[i]->depth_meter && !(settings->autoSetpoint && i + 1 == SETPOINT_INDEX_AUTO_DECO)) {
            bool setpointDelayed = settings->autoSetpoint && i + 1 == SETPOINT_INDEX_AUTO_LOW && settings->delaySetpointLow;
            dataIndex += snprintf(&data[dataIndex], 10, "\016\016 %um%s\017", setpointsToShow[i]->depth_meter, setpointDelayed ? "(d)" : "");
        }

        i++;
    }

    i = 0;
    while (diluentsToShow[i] != NULL && i < MAX_NUM_SUMMARY_LINES - 1) {
        heading[headingIndex++] = '\n';
        heading[headingIndex++] = '\r';
        if (i == 0) {
            heading[headingIndex++] = TXT_Diluent_Gas_Edit;
        }

        data[dataIndex++] = '\n';
        data[dataIndex++] = '\r';
        data[dataIndex++] = '\002';
        data[dataIndex++] = '\020';
        dataIndex += write_gas(&data[dataIndex], diluentsToShow[i]->oxygen_percentage, diluentsToShow[i]->helium_percentage);
        if (diluentsToShow[i]->note.ub.deco) {
            char *space = " ";
            if (diluentsToShow[i]->depth_meter > 99) {
                space = (char *)"";
            }
            dataIndex += snprintf(&data[dataIndex], 10, "\016\016%s%um\017", space, diluentsToShow[i]->depth_meter);
        }

        i++;
    }

    if (showScrubberTime) {
        heading[headingIndex++] = '\n';
        heading[headingIndex++] = '\r';
        heading[headingIndex++] = TXT_2BYTE;
        heading[headingIndex++] = TXT2BYTE_Scrubber;

        data[dataIndex++] = '\n';
        data[dataIndex++] = '\r';
        data[dataIndex++] = '\002';
        dataIndex += printScrubberText(&data[dataIndex], 10, settings->scrubberData, settings, false);
    }

    heading[headingIndex++] = '\017';
    heading[headingIndex++] = 0;

    data[dataIndex++] = 0;

    t7cY0free.WindowLineSpacing = 48;
    t7cY0free.WindowNumberOfTextLines = 6;
    t7cY0free.WindowTab = 375;

    if (!settings->FlipDisplay) {
	    t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
        t7cY0free.WindowX0 += 10;
        t7cY0free.WindowX1 = t7cC.WindowX1 - 10;
        t7cY0free.WindowY0 += 10;
        t7cY0free.WindowY1 = 355;
        GFX_write_string(&FontT24, &t7cY0free, heading, 1);
        t7cY0free.WindowX0 -= 10;
        t7cY0free.WindowY0 -= 10;
    } else {
    	t7cY0free.WindowX0 -= 10;
	    t7cY0free.WindowY1 = 400;
        t7cY0free.WindowY1 -= 10;
        t7cY0free.WindowX1 -= 10;
        GFX_write_string(&FontT24, &t7cY0free, heading, 1);
        t7cY0free.WindowX0 += 10;
        t7cY0free.WindowY1 += 10;
        t7cY0free.WindowX1 += 10;
    }

    Gfx_colorsscheme_mod(data, 0);

    GFX_write_string(&FontT42, &t7cY0free, data, 1);
}


static void setTimerPrestart(int startTimeS)
{
    stateUsedWrite->timerState = TIMER_STATE_PRESTART;
    stateUsedWrite->timerStartedS = startTimeS;
}


static void setTimerFinished(int startTimeS)
{
    stateUsedWrite->timerState = TIMER_STATE_FINISHED;
    stateUsedWrite->timerStartedS = startTimeS;
}


static void updateTimer(SSettings *settings, int nowS, bool switchedToTimerView)
{
    int timerElapsedS = nowS - stateUsed->timerStartedS;

    if (stateUsed->timerState && timerElapsedS < 0) {
        disableTimer();
    } else {
        switch (stateUsed->timerState) {
        case TIMER_STATE_OFF:
            if (switchedToTimerView) {
                setTimerPrestart(nowS);
            }

            break;
        case TIMER_STATE_PRESTART:
            if (timerElapsedS <= TIMER_ACTION_DELAY_S) {
                if (switchedToTimerView) {
                    setTimerPrestart(nowS);
                }
            } else {
                if (selection_customview == CVIEW_Timer) {
                    stateUsedWrite->timerState = TIMER_STATE_RUNNING;
                    stateUsedWrite->timerStartedS = stateUsed->timerStartedS + TIMER_ACTION_DELAY_S;
                } else {
                    disableTimer();
                }
            }

            break;
        case TIMER_STATE_RUNNING:
            if (timerElapsedS >= settings->timerDurationS) {
                if (selection_customview == CVIEW_Timer) {
                    setTimerFinished(stateUsed->timerStartedS + settings->timerDurationS);
                } else {
                    stateUsedWrite->timerState = TIMER_STATE_WAIT_FINISHED;
                }
            }

            break;
        case TIMER_STATE_WAIT_FINISHED:
            if (switchedToTimerView) {
                setTimerFinished(nowS);
            }

            break;
        case TIMER_STATE_FINISHED:
            if (timerElapsedS <= TIMER_ACTION_DELAY_S) {
                if (switchedToTimerView) {
                    setTimerPrestart(stateUsed->timerStartedS);
                }
            } else {
                disableTimer();
            }

            break;
        }
    }
}


bool t7_isTimerRunning(bool includeBackground)
{
    return stateUsed->timerState && (selection_customview == CVIEW_Timer || (includeBackground && stateUsed->timerState == TIMER_STATE_RUNNING));
}


static void showTimer(SSettings *settings, int nowS)
{
    char heading[32];
    unsigned headingIndex = 0;

    char data[32];
    unsigned dataIndex = 0;

    heading[headingIndex++] = '\032';
    heading[headingIndex++] = '\016';
    heading[headingIndex++] = '\016';

    data[dataIndex++] = '\t';

    int timerRemainingS = settings->timerDurationS;
    switch (stateUsed->timerState) {
    case TIMER_STATE_RUNNING:
        timerRemainingS = settings->timerDurationS - (nowS - stateUsed->timerStartedS);

        break;
    case TIMER_STATE_PRESTART:
    case TIMER_STATE_FINISHED:
        if (stateUsed->timerState == TIMER_STATE_PRESTART) {
            heading[headingIndex++] = TXT_2BYTE;
            heading[headingIndex++] = TXT2BYTE_Starting;
        } else {
            heading[headingIndex++] = TXT_2BYTE;
            heading[headingIndex++] = TXT2BYTE_Finished;

            timerRemainingS = 0;
        }

        dataIndex += snprintf(&data[dataIndex], 10, "\020%u", TIMER_ACTION_DELAY_S - (nowS - stateUsed->timerStartedS));

        break;
    default:

        break;
    }

    char timer[16];
    snprintf(timer, 10, "\001\020%u:%02u", timerRemainingS / 60, timerRemainingS % 60);

    heading[headingIndex++] = 0;

    data[dataIndex++] = 0;

    t7cY0free.WindowLineSpacing = 48;
    t7cY0free.WindowNumberOfTextLines = 6;
    t7cY0free.WindowTab = 375;

    if (!settings->FlipDisplay) {
	    t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
        t7cY0free.WindowX0 += 10;
        t7cY0free.WindowY0 += 10;
        t7cY0free.WindowY1 = 355;
        GFX_write_string(&FontT24, &t7cY0free, heading, 1);
        t7cY0free.WindowX0 -= 10;
        t7cY0free.WindowY0 -= 10;
    } else {
	    t7cY0free.WindowY1 = 400;
        t7cY0free.WindowY1 -= 10;
        t7cY0free.WindowX1 -= 10;
        GFX_write_string(&FontT24, &t7cY0free, heading, 1);
        t7cY0free.WindowY1 += 10;
        t7cY0free.WindowX1 += 10;
    }

    Gfx_colorsscheme_mod(data, 0);

    GFX_write_string(&FontT42, &t7cY0free, data, 1);

    Gfx_colorsscheme_mod(timer, 0);

    GFX_write_string(&FontT105, &t7cY0free, timer, 4);
}


void t7_refresh_customview(void)
{
	static uint8_t last_customview = CVIEW_END;
#if defined ENABLE_GNSS_INTERN || defined ENABLE_GNSS_EXTERN
	static uint16_t targetHeading = 0;
	static uint16_t targetDistance = 0;
#endif
	static uint8_t onceASecond = 10;		/* used to update content just once in a second if faster updates are not needed */

    char text[100];

    uint16_t heading = 0;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(last_customview != selection_customview)		/* check if current selection is disabled and should be skipped */
	{
		if(t7_customview_disabled(selection_customview))
		{
			t7_change_customview(ACTION_BUTTON_ENTER);
		}
	}
    switch(selection_customview)
    {
    case CVIEW_noneOrDebug:
        if(settingsGetPointer()->showDebugInfo)
        {
            // header
            strcpy(text,"\032\f\001Debug");
            GFX_write_string(&FontT42,&t7cH,text,0);
            // content
            t7_cv_debug();
        }
        break;

    case CVIEW_Charger:
             snprintf(text,100,"\032\f\001%c",TXT_Charging);
            GFX_write_string(&FontT42,&t7cH,text,0);
            t7_ChargerView();

        break;

    case CVIEW_SummaryOfLeftCorner:
        snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_Summary);
        GFX_write_string(&FontT42,&t7cH,text,0);
        // content
        t7_SummaryOfLeftCorner();
        break;

    case CVIEW_CompassDebug:
        snprintf(text,100,"\032\f\001Compass raw");
        GFX_write_string(&FontT42,&t7cH,text,0);
        snprintf(text,255,"%1.1f\n\r%1.1f\n\r%1.1f\n\r%i\n\r%i\n\r%i"
                    ,stateUsed->lifeData.compass_heading
                    ,stateUsed->lifeData.compass_roll
                    ,stateUsed->lifeData.compass_pitch
                    ,stateUsed->lifeData.compass_DX_f
                    ,stateUsed->lifeData.compass_DY_f
                    ,stateUsed->lifeData.compass_DZ_f
            );

        t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
        t7cY0free.WindowLineSpacing = 48;
        t7cY0free.WindowNumberOfTextLines = 6;
        GFX_write_string(&FontT42, &t7cY0free, text, 1);
        break;

    case CVIEW_Hello: t7_cv_hello();
    	break;

    case CVIEW_Gaslist: t7_cv_gasList();
        break;

    case CVIEW_EADTime:  t7_cv_EADTime();
    	break;

    case CVIEW_Profile:
        snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_Profile);
        GFX_write_string(&FontT42,&t7cH,text,0);
        t7_miniLiveLogProfile();
        break;

    case CVIEW_Tissues:
        snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_Tissues);
        GFX_write_string(&FontT42,&t7cH,text,0);
        t7_tissues(stateUsed);
        break;

    case CVIEW_sensors: t7_cv_sensors();
        break;

    case CVIEW_sensors_mV: t7_cv_sensors_mV();
        break;

    case CVIEW_Compass:
    default:
        {
	    if(pSettings->compassInertia)
	    {
	    	heading = (uint16_t)compass_getCompensated();
	    }
	    else
	    {
	    	heading = (uint16_t)stateUsed->lifeData.compass_heading;
	    }
        /* Nav heading: tilt-corrected heading shown numerically.
           Raw heading drives the rose card (ticks/labels). */
        uint16_t nav = (uint16_t)(((int32_t)heading
                                   + compass_mount_phi(pSettings->compassMountTilt)
                                   + 360) % 360);
        snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE, TXT2BYTE_Compass);
        GFX_write_string(&FontT42,&t7cH,text,0);

#if defined ENABLE_GNSS_INTERN || defined ENABLE_GNSS_EXTERN
        if(((stateUsed->targetCoord.fLat != 0) && (stateUsed->targetCoord.fLon != 0))	/* use heading derived from GNSS coordinates */
        		&& (GNSS_HAS_FIX(&stateUsed->lifeData.gnssData)))							/* if we have a valid position */
        {
        	if((onceASecond == 0) || (targetHeading == 0))
        	{
        		getHeadingFromGnss(&stateUsed->lifeData.gnssData.coord, &stateUsed->targetCoord, &targetHeading, &targetDistance);
        	}
        	t7_compass(heading, (uint16_t)targetHeading);
        }
        else
#endif
        t7_compass(heading, stateUsed->diveSettings.compassHeading);

        if(!pSettings->FlipDisplay)
        {
        	t7cY0free.WindowX0 += 15;
        	t7cY0free.WindowY0 = 197;
        }
        else
        {
        	/* Exact screen-mirror (799-x, 479-y) of the unflipped readout window
        	   {X0=267,Y0=197}..{X1=547,Y1=365}. The flipped glyph path anchors at
        	   WindowX1/WindowY1, so mirroring the window lands the heading readout
        	   at the true 180-deg position; the old hand-tuned {Y0=0,Y1=250} sat
        	   several px off rotate180(unflipped). */
        	t7cY0free.WindowX0 = 799 - 547;   /* 252 = 799 - unflipped X1 */
        	t7cY0free.WindowX1 = 799 - 267;   /* 532 = 799 - unflipped X0 (252+15) */
        	t7cY0free.WindowY0 = 479 - 365;   /* 114 = 479 - unflipped Y1 */
        	t7cY0free.WindowY1 = 479 - 197;   /* 282 = 479 - unflipped Y0 */
        }

#if defined ENABLE_GNSS_INTERN || defined ENABLE_GNSS_EXTERN
        if(((stateUsed->targetCoord.fLat != 0) && (stateUsed->targetCoord.fLon != 0))	/* use heading derived from GNSS coordinates */
        		&& (GNSS_HAS_FIX(&stateUsed->lifeData.gnssData)))							/* if we have a valid position */
        {
			if(targetDistance < 1000)
			{
				 snprintf(text,100,"\030\001%03im",targetDistance);
			}
			else
			{
				snprintf(text,100,"\030\001%03ik",(targetDistance / 1000));
			}
        }
        else
#endif
        snprintf(text,100,"\030\001%03i`",nav);
        GFX_write_string(&FontT54,&t7cY0free,text,0);
        if(!pSettings->FlipDisplay)
        {
        	t7cY0free.WindowX0 -= 15;
        }
        }
        break;

    case CVIEW_Decolist: t7_cv_Decolist();
        break;

    case CVIEW_CcrSummary:
        snprintf(text, 100, "\032\f\001%c%c", TXT_2BYTE, TXT2BYTE_CcrSummary);
        GFX_write_string(&FontT42, &t7cH, text, 0);
        t7_CcrSummary(pSettings);
        break;
    case CVIEW_Timer:
        snprintf(text, 100, "\032\f\001%c%c", TXT_2BYTE, TXT2BYTE_Timer);
        GFX_write_string(&FontT42, &t7cH, text, 0);

        int nowS = current_second();

        updateTimer(pSettings, nowS, last_customview != CVIEW_Timer);

        showTimer(pSettings, nowS);
        break;

    case CVIEW_Position:
        /* the colour of the title shows if the receiver answers (a valid frame within GNSS_ALIVE_TIMEOUT_MS): white = yes, yellow = yes but it is in power save mode
         * (see the line below the bars), red = no.
         * Both colours are fixed ones, the label colour of the other titles depends on the colour scheme. White and red differ strongly in brightness,
         * so they can be told apart without relying on red and green. The colour is part of the string, it is not kept for other views */
        snprintf(text, 100, "%c\f\001%c%c", (stateUsed->lifeData.gnssData.alive & GNSS_ALIVE_STATE_ALIVE) ? (t7_getGnssPowerSaveState() ? '\024' : '\020') : '\025', TXT_2BYTE, TXT2BYTE_Position);
        GFX_write_string(&FontT42, &t7cH, text, 0);
        t7_showPosition();
        break;
#ifdef ENABLE_CAVEMODE
    case CVIEW_Cave: t7_cv_Cave();
    	break;
#endif
    }

    if(onceASecond == 0)
    {
    	onceASecond = 10;
    }
    else
    {
    	onceASecond--;
    }
    last_customview = selection_customview;
}


/* DIVE MODE
 */
void t7_refresh_divemode(void)
{
    char TextL1[TEXTSIZE];
    char TextL2[TEXTSIZE];

    char TextR1[TEXTSIZE];
    char TextR2[TEXTSIZE];
    char TextR3[TEXTSIZE];

    char TextC1[2*TEXTSIZE];
    char TextC2[TEXTSIZE];
    uint8_t textPointer;

    uint8_t color = 0;
    int textlength;

    uint16_t 	nextstopLengthSeconds = 0;
    uint8_t 	nextstopDepthMeter = 0;
    uint8_t oxygen_percentage = 0;
    SDivetime Divetime = {0,0,0, 0};
    SDivetime SafetyStopTime = {0,0,0,0};
    SDivetime TimeoutTime = {0,0,0,0};
    uint8_t  customview_warnings = 0;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

    Divetime.Total = stateUsed->lifeData.dive_time_seconds_without_surface_time;
    Divetime.Minutes = Divetime.Total / 60;
    Divetime.Seconds = Divetime.Total - ( Divetime.Minutes * 60 );

    SafetyStopTime.Total = timer_Safetystop_GetCountDown();
    SafetyStopTime.Minutes = SafetyStopTime.Total / 60;
    SafetyStopTime.Seconds = SafetyStopTime.Total - (SafetyStopTime.Minutes * 60);

    TimeoutTime.Total = pSettings->timeoutDiveReachedZeroDepth - stateUsed->lifeData.counterSecondsShallowDepth;
    if(TimeoutTime.Total > pSettings->timeoutDiveReachedZeroDepth)
    {
        TimeoutTime.Total = 0;
    }
    TimeoutTime.Minutes = TimeoutTime.Total / 60;
    TimeoutTime.Seconds = TimeoutTime.Total - (TimeoutTime.Minutes * 60);

    const SDecoinfo * pDecoinfo = getDecoInfo();
    if(pDecoinfo->output_time_to_surface_seconds)
    {
        tHome_findNextStop(pDecoinfo->output_stop_length_seconds, &nextstopDepthMeter, &nextstopLengthSeconds);
    }
    else
    {
        nextstopDepthMeter = 0;
        nextstopLengthSeconds = 0;
    }


    /* max depth */
    snprintf(TextL2,TEXTSIZE,"\032\f%c",TXT_MaxDepth);
    GFX_write_string(&FontT42,&t7l2,TextL2,0);

    if(unit_depth_float(stateUsed->lifeData.max_depth_meter) < 100)
        snprintf(TextL2,TEXTSIZE,"\020%01.1f",unit_depth_float(stateUsed->lifeData.max_depth_meter));
    else
        snprintf(TextL2,TEXTSIZE,"\020%01.0f",unit_depth_float(stateUsed->lifeData.max_depth_meter));

    Gfx_colorsscheme_mod(TextL2, 0);
    GFX_write_string(&FontT105,&t7l2,TextL2,1);

/* ascent rate graph */
    color = 0xff;
    if((pSettings->slowExitTime != 0) && (nextstopDepthMeter == 0) && (stateUsed->lifeData.depth_meter < pSettings->last_stop_depth_meter) && (stateUsed->lifeData.depth_meter > 0.3))
    {
    	color = t7_drawSlowExitGraph();
    }
    if(color == 0xff)
    {
    	color = drawingColor_from_ascentspeed(stateUsed->lifeData.ascent_rate_meter_per_min);
    	if(stateUsed->lifeData.ascent_rate_meter_per_min > 1)	/* a value < 1 would cause a bar in negative direction brush rectangle of 12 and step width of 6 */
    	{

    	    	t7_drawAcentGraph(color);
    	}
    }

    /* depth */
    float depth = unit_depth_float(stateUsed->lifeData.depth_meter);

    if(depth <= 0.3f)
        depth = 0;

    if(settingsGetPointer()->nonMetricalSystem)
        snprintf(TextL1,TEXTSIZE,"\032\f[feet]");
    else
        snprintf(TextL1,TEXTSIZE,"\032\f%c",TXT_Depth);

    GFX_write_string(&FontT24,&t7l1,TextL1,0);

    if((stateUsed->lifeData.ascent_rate_meter_per_min > 8) || (stateUsed->lifeData.ascent_rate_meter_per_min < -10))
    {
        snprintf(TextL1,TEXTSIZE,"\f\002%.0f %c%c/min "
            , unit_depth_float(stateUsed->lifeData.ascent_rate_meter_per_min)
            , unit_depth_char1()
            , unit_depth_char2()
        );
        GFX_write_string(&FontT24,&t7l1,TextL1,0);
    }

    if( depth < 100)
        snprintf(TextL1,TEXTSIZE,"\020%01.1f",depth);
    else
        snprintf(TextL1,TEXTSIZE,"\020%01.0f",depth);

    Gfx_colorsscheme_mod(TextL1, color);

    GFX_write_string(&FontT144,&t7l1,TextL1,0);

/* divetime */
    if(stateUsed->lifeData.counterSecondsShallowDepth)
    {
        snprintf(TextR1,TEXTSIZE,"\f\002\136 %u:%02u",TimeoutTime.Minutes, TimeoutTime.Seconds);
        GFX_write_string(&FontT42,&t7r1,TextR1,0);
    }
    else
    {
        snprintf(TextR1,TEXTSIZE,"\032\f\002%c",TXT_Divetime);
        GFX_write_string(&FontT42,&t7r1,TextR1,0);
    }

    if(Divetime.Minutes < 1000)
        snprintf(TextR1,TEXTSIZE,"\020\002\016%u:%02u",Divetime.Minutes, Divetime.Seconds);
    else
        snprintf(TextR1,TEXTSIZE,"\020\002\016%u'",Divetime.Minutes);
    Gfx_colorsscheme_mod(TextR1, 0);
    GFX_write_string(&FontT105,&t7r1,TextR1,1);

    /* next deco stop */
    if(nextstopDepthMeter)
    {
    	snprintf(TextR2,TEXTSIZE,"\032\f\002%c",TXT_Decostop);
    	GFX_write_string(&FontT42,&t7r2,TextR2,0);

    	if((stateUsed->diveSettings.deco_type.ub.standard == VPM_MODE) && (pSettings->VPM_conservatism.ub.alternative)
    			&& (fabs(stateUsed->lifeData.depth_meter - nextstopDepthMeter)) < 1.5)
    	{
    		TextR2[0] = '\026';
    		textlength = 1;
    	}
    	else
    	{
       		TextR2[0] = '\020';
       		textlength = 1;
    	}

        textlength += snprintf(&TextR2[textlength],TEXTSIZE,"\002%u%c%c %u'"
            , unit_depth_integer(nextstopDepthMeter)
            , unit_depth_char1_T105()
            , unit_depth_char2_T105()
            , (nextstopLengthSeconds+59)/60);
        Gfx_colorsscheme_mod(TextR2, 0);
        if(time_elapsed_ms(pDecoinfo->tickstamp, HAL_GetTick()) > MAX_AGE_DECOINFO_MS)
            TextR2[0] = '\031';
        if(textlength <= 9)
            GFX_write_string(&FontT105,&t7r2,TextR2,1);
        else if(textlength <= 10)
            GFX_write_string(&FontT84Spaced,&t7r2,TextR2,1);
        else
            GFX_write_string(&FontT54,&t7r2,TextR2,1);
    }
    else if(SafetyStopTime.Total && (depth > timer_Safetystop_GetDepthUpperLimit()))
    {
        snprintf(TextR2,TEXTSIZE,"\032\f\002%c%c",TXT_2BYTE,TXT2BYTE_SafetyStop2);
        GFX_write_string(&FontT42,&t7r2,TextR2,0);
        snprintf(TextR2,TEXTSIZE,"\020\002\016%u:%02u",SafetyStopTime.Minutes,SafetyStopTime.Seconds);
        Gfx_colorsscheme_mod(TextR2, 0);
        GFX_write_string(&FontT105,&t7r2,TextR2,1);
    }

    /* tts - option 1
     * ndl - option 2
     * empty - option 3 */
    if(pDecoinfo->output_time_to_surface_seconds)
    {
        snprintf(TextR3,TEXTSIZE,"\032\f\002%c",TXT_TTS);
        GFX_write_string(&FontT42,&t7r3,TextR3,0);
        if(pDecoinfo->output_time_to_surface_seconds < 1000 * 60)
            snprintf(TextR3,TEXTSIZE,"\020\002%i'",(pDecoinfo->output_time_to_surface_seconds + 59)/ 60);
        else
            snprintf(TextR3,TEXTSIZE,"\020\002%ih",(pDecoinfo->output_time_to_surface_seconds + 59)/ 3600);
        Gfx_colorsscheme_mod(TextR3, 0);
        if(time_elapsed_ms(pDecoinfo->tickstamp, HAL_GetTick()) > MAX_AGE_DECOINFO_MS)
            TextR2[0] = '\031';
        GFX_write_string(&FontT105,&t7r3,TextR3,1);
    }
    else if(pDecoinfo->output_ndl_seconds)
    {
        snprintf(TextR3,TEXTSIZE,"\032\f\002%c",TXT_Nullzeit);
        GFX_write_string(&FontT42,&t7r3,TextR3,0);
        if(pDecoinfo->output_ndl_seconds < 1000 * 60)
            snprintf(TextR3,TEXTSIZE,"\020\002%i'",pDecoinfo->output_ndl_seconds/60);
        else
            snprintf(TextR3,TEXTSIZE,"\020\002%ih",pDecoinfo->output_ndl_seconds/3600);
        Gfx_colorsscheme_mod(TextR3, 0);
        if(time_elapsed_ms(pDecoinfo->tickstamp, HAL_GetTick()) > MAX_AGE_DECOINFO_MS)
            TextR2[0] = '\031';
        GFX_write_string(&FontT105,&t7r3,TextR3,1);
    }

    /* Menu Selection (and gas mix) */
    if(get_globalState() == StDMGAS)
    {
        textPointer = 0;
        TextR1[textPointer++] = '\a';
//		TextR1[textPointer++] = '\f';
        TextR1[textPointer++] = '\001';
        TextR1[textPointer++] = ' ';
        textPointer += tHome_gas_writer(stateUsed->diveSettings.gas[actualBetterGasId()].oxygen_percentage,stateUsed->diveSettings.gas[actualBetterGasId()].helium_percentage,&TextR1[textPointer]);
        TextR1[textPointer++] = '?';
        TextR1[textPointer++] = ' ';
        TextR1[textPointer++] = 0;
        GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDMSPT)
    {
        textPointer = 0;
        TextR1[textPointer++] = '\a';
        TextR1[textPointer++] = '\001';
        TextR1[textPointer++] = ' ';
        textPointer += snprintf(&TextR1[textPointer],TEXTSIZE,"%01.2f",(float)(stateUsed->diveSettings.setpoint[actualBetterSetpointId()].setpoint_cbar) / 100.0);
        TextR1[textPointer++] = '?';
        TextR1[textPointer++] = ' ';
        TextR1[textPointer++] = 0;
        GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDMENU)
    {
        snprintf(TextR1,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveMenuQ);
        GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);
    } else if (get_globalState() == StDBAILOUT) {
        if (isLoopMode(stateUsed->diveSettings.diveMode)) {
            textPointer = snprintf(TextR1, TEXTSIZE, "\a\001 %c%c ", TXT_2BYTE, TXT2BYTE_BailoutShort);
            textPointer += tHome_gas_writer(stateUsed->diveSettings.gas[actualBetterBailoutGasId()].oxygen_percentage, stateUsed->diveSettings.gas[actualBetterBailoutGasId()].helium_percentage, &TextR1[textPointer]);
            TextR1[textPointer++] = '?';
            TextR1[textPointer++] = ' ';
            TextR1[textPointer++] = 0;
        } else {
            textPointer = snprintf(TextR1, TEXTSIZE, "\a\001 %c%c %01.2f/", TXT_2BYTE, TXT2BYTE_LoopShort, stateUsed->diveSettings.setpoint[actualBetterSetpointId()].setpoint_cbar / 100.0);
            textPointer += tHome_gas_writer(stateUsed->diveSettings.gas[stateUsed->lifeData.lastDiluent_GasIdInSettings].oxygen_percentage, stateUsed->diveSettings.gas[stateUsed->lifeData.lastDiluent_GasIdInSettings].helium_percentage, &TextR1[textPointer]);
            TextR1[textPointer++] = '?';
            TextR1[textPointer++] = ' ';
            TextR1[textPointer++] = 0;
        }

        GFX_write_string_color(&FontT48, &t7c2, TextR1, 0, CLUT_WarningYellow);
    } else if (get_globalState() == StDSETPOINT) {
        snprintf(TextR1, TEXTSIZE, "\a\001 %c%c %01.2f? ", TXT_2BYTE, TXT2BYTE_SetpointShort, getSwitchToSetpointCbar() / 100.0);

        GFX_write_string_color(&FontT48, &t7c2, TextR1, 0, CLUT_WarningYellow);
    }
    else if(get_globalState() == StDBEAR)
    {
        snprintf(TextR1, TEXTSIZE, "\a\001 %c%c? ", TXT_2BYTE, TXT2BYTE_DiveBearingQ);
        GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDSIM1)
    {
        snprintf(TextR1,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveQuitQ);
        GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDSIM2)
    {
        if(settingsGetPointer()->nonMetricalSystem)
            snprintf(TextR1,TEXTSIZE,"\a\001" " Sim:-3.33ft ");
        else
            snprintf(TextR1,TEXTSIZE,"\a\001" " Sim:-1m ");
        GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);

        snprintf(TextR1,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t7l1,TextR1,0,CLUT_WarningYellow);

    }
    else if(get_globalState() == StDSIM3)
    {
        if(settingsGetPointer()->nonMetricalSystem)
            snprintf(TextR1,TEXTSIZE,"\a\001" " Sim:+3.33ft ");
        else
            snprintf(TextR1,TEXTSIZE,"\a\001" " Sim:+1m ");
        GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);
        snprintf(TextR1,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t7l1,TextR1,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDSIM4)
    {
        snprintf(TextR1,TEXTSIZE,"\a\001" " Sim:+5' ");
        GFX_write_string_color(&FontT48,&t7c2,TextR1,0,CLUT_WarningYellow);
        snprintf(TextR1,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t7l1,TextR1,0,CLUT_WarningYellow);
    }
    else
    {
        /* gas mix */
        oxygen_percentage = 100;
        oxygen_percentage -= stateUsed->lifeData.actualGas.nitrogen_percentage;
        oxygen_percentage -= stateUsed->lifeData.actualGas.helium_percentage;

        textPointer = 0;
        TextC2[textPointer++] = '\020';
        if(stateUsed->warnings.betterGas && warning_count_high_time)
        {
            TextC2[textPointer++] = '\a';
        }
        else
        {
            float fPpO2limitHigh, fPpO2now;

            if(actualLeftMaxDepth(stateUsed))
                fPpO2limitHigh = settingsGetPointer()->ppO2_max_deco;
            else
                fPpO2limitHigh = settingsGetPointer()->ppO2_max_std;

            fPpO2now = (stateUsed->lifeData.pressure_ambient_bar - WATER_VAPOUR_PRESSURE) * oxygen_percentage;

            if((fPpO2now > fPpO2limitHigh) || (fPpO2now < (float)(settingsGetPointer()->ppO2_min)))
                TextC2[textPointer++] = '\025';
        }
        TextC2[textPointer++] = '\002';
        textPointer += tHome_gas_writer(oxygen_percentage,stateUsed->lifeData.actualGas.helium_percentage,&TextC2[textPointer]);

        if(stateUsed->warnings.betterGas && warning_count_high_time)
        {
            if(TextC2[0] == '\020')
            {
                TextC2[0] = '\004'; // NOP
            }
            GFX_write_string_color(&FontT48,&t7c2,TextC2,0,CLUT_WarningYellow);
        }
        else
        {
            Gfx_colorsscheme_mod(TextC2, 0);
            GFX_write_string(&FontT48,&t7c2,TextC2,0); // T54 has only numbers
        }

        if(stateUsed->diveSettings.ccrOption)
        {
        	if(isLoopMode(stateUsed->diveSettings.diveMode))
            {
                snprintf(TextC2,TEXTSIZE,"\020%01.2f",stateUsed->lifeData.ppO2);

                if(stateUsed->warnings.betterSetpoint && warning_count_high_time && (stateUsed->diveSettings.diveMode == DIVEMODE_CCR))
                {
                    TextC2[0] = '\a'; // inverse instead of color \020
                    GFX_write_string_color(&FontT48,&t7c2,TextC2,0,CLUT_WarningYellow);
                }
                else
                {
                    Gfx_colorsscheme_mod(TextC2, 0);
                    GFX_write_string(&FontT48,&t7c2,TextC2,0);
                }
            }
        }
        else if(settingsGetPointer()->alwaysShowPPO2)
        {
            snprintf(TextC2,TEXTSIZE,"\020%01.2f",stateUsed->lifeData.ppO2);
            Gfx_colorsscheme_mod(TextC2, 0);
            GFX_write_string(&FontT48,&t7c2,TextC2,0);
        }
    }

    /* algorithm, ccr, bailout and battery */
    /* and permanent warnings (CNS) */

    if((stateUsed->warnings.cnsHigh) && display_count_high_time)
    {
        TextC2[0] = '\f';
        TextC2[1] = TXT_2BYTE;
        TextC2[2] = TXT2BYTE_WarnCnsHigh;
        TextC2[3] = 0;
        GFX_write_string_color(&FontT42,&t7c1,TextC2,0,CLUT_WarningRed);
    }
    else
    {
        if(stateUsed->warnings.aGf)
        {
            GFX_write_string_color(&FontT48,&t7c1,"\f" "aGF",0,CLUT_WarningYellow);
        }
        else if(stateUsed->diveSettings.deco_type.ub.standard == GF_MODE)
        {
            GFX_write_string(&FontT48,&t7c1,"\027\f" "GF",0);
        }
        else
        {
            GFX_write_string(&FontT48,&t7c1,"\027\f" "VPM",0);
        }

        if(stateUsed->diveSettings.diveMode == DIVEMODE_CCR)
            GFX_write_string(&FontT24,&t7c1,"\027\f\002" "CCR",0);
        //  GFX_write_string(&FontT24,&t7c1,"\f\177\177\x80" "CCR",0);
        else
        if(stateUsed->diveSettings.diveMode == DIVEMODE_PSCR)
                GFX_write_string(&FontT24,&t7c1,"\027\f\002" "PSCR",0);
        else
        if(stateUsed->diveSettings.ccrOption)
            GFX_write_string(&FontT24,&t7c1,"\f\002\024" "Bailout",0);
        //  GFX_write_string(&FontT24,&t7c1,"\f\177\177\x80\024" "Bailout",0);
    }
    TextC1[0] = '\020';
    TextC1[1] = '3';
    TextC1[2] = '1';
    TextC1[3] = '1';
    TextC1[4] = '1';
    TextC1[5] = '1';
    TextC1[6] = '1';
    TextC1[7] = '1';
    TextC1[8] = '1';
    TextC1[9] = '1';
    TextC1[10] = '1';
    TextC1[11] = '1';
    TextC1[12] = '0';
    TextC1[13] = 0;

    for(int i=1;i<=10;i++)
    {
        if(	stateUsed->lifeData.battery_charge > (9 * i))
            TextC1[i+1] += 1;
    }

    if(stateUsed->warnings.lowBattery)
    {
        TextC1[0] = '\025';
        if(warning_count_high_time)
        {
            for(int i=2;i<=11;i++)
                TextC1[i] = '1';
        }
        else
        {
            TextC1[2] = '2';
        }
        GFX_write_string(&Batt24,&t7batt,TextC1,0);

        if((stateUsed->lifeData.battery_charge > 0) && (stateUsed->lifeData.battery_charge < 140))
        {
            snprintf(TextC1,16,"\004\025\f\002%u%%",(uint8_t)stateUsed->lifeData.battery_charge);
            if(warning_count_high_time)
                TextC1[0] = '\a';
            GFX_write_string(&FontT24,&t7voltage,TextC1,0);
        }
    }
    else
    {
        Gfx_colorsscheme_mod(TextC1, 0);
        GFX_write_string(&Batt24,&t7batt,TextC1,0);

        if((stateUsed->lifeData.battery_charge > 0) && (stateUsed->lifeData.battery_charge < 140))
        {
            snprintf(TextC1,16,"\020\f\002%u%%",(uint8_t)stateUsed->lifeData.battery_charge);
            Gfx_colorsscheme_mod(TextC1, 0);
            GFX_write_string(&FontT24,&t7voltage,TextC1,0); // t7batt
        }
    }

    /* customizable left lower corner */
    t7_refresh_divemode_userselected_left_lower_corner();


    /* customview - option 1
     * warning - option 2 */
    if(stateUsed->warnings.numWarnings)
        customview_warnings = t7_test_customview_warnings();

    background.pointer = 0;
    if(customview_warnings && warning_count_high_time)
        t7_show_customview_warnings();
    else
    {
        t7_refresh_customview();
        requestBuzzerActivation(REQUEST_BUZZER_OFF);
    }

    /* the frame */
    draw_frame(1,1, CLUT_DIVE_pluginbox, CLUT_DIVE_FieldSeperatorLines);
}

void t7_set_field_to_primary(void)
{
    if(stateUsed->mode == MODE_DIVE)
            selection_custom_field = settingsGetPointer()->tX_userselectedLeftLowerCornerPrimary;
}

void t7_change_field(uint8_t checkCurrentField)
{
	SSettings *settings = settingsGetPointer();
    uint8_t checkAgain = 0;
	if(checkCurrentField == 0)
	{
		selection_custom_field++;
	}

    do
    {
    	checkAgain = 0;
		if((stateUsed->diveSettings.deco_type.ub.standard == VPM_MODE) && ((selection_custom_field == LLC_GF) || (selection_custom_field == LCC_GF_SURF))) /* no GF if in VPM mode */
		{
			selection_custom_field++;
			checkAgain = 1;
		}

		if ((selection_custom_field == LLC_ScrubberTime) && ((!isScrubberTimerEnabled(settings)) || (!isLoopMode(stateUsed->diveSettings.diveMode))))
		{
			selection_custom_field++;
			checkAgain = 1;
		}
#ifdef ENABLE_PSCR_MODE
		if((selection_custom_field == LCC_SimPpo2) && (settings->dive_mode != DIVEMODE_PSCR))
		{
			selection_custom_field++;
			checkAgain = 1;
		}
#endif
#ifdef ENABLE_CO2_SUPPORT
		if((selection_custom_field == LCC_CO2) && (settings->co2_sensor_active == 0))
		{
			selection_custom_field++;
			checkAgain = 1;
		}
#endif
		if((selection_custom_field == LLC_ScrubberTime) && ((isScrubberTimerEnabled(settings) == 0) || (!isLoopMode(stateUsed->diveSettings.diveMode))))
		{
			selection_custom_field++;
			checkAgain = 1;
		}
		if((selection_custom_field == LCC_BottleBar) && (stateUsed->lifeData.bottle_bar_age_MilliSeconds[stateUsed->lifeData.actualGas.GasIdInSettings] >= BOTTLE_SENSOR_TIMEOUT))
		{
			selection_custom_field++;
			checkAgain = 1;
		}
		if(selection_custom_field >= LLC_END)
		{
			selection_custom_field = LLC_Empty;
		}
    } while (checkAgain);
}


void t7_refresh_divemode_userselected_left_lower_corner(void)
{
    if(!selection_custom_field)
        return;

    char  headerText[10];
    char  text[TEXTSIZE];
    uint8_t textpointer = 0;
    _Bool tinyHeaderFont = 0;
    uint8_t line = 0;
#ifdef ENABLE_BOTTLE_SENSOR
    uint16_t agedColor = 0;
#endif
#ifdef ENABLE_ADVANCED_GAS
    static uint8_t gasIdO2 = 0;
    static uint8_t gasIdDiluent = 0;
#endif
    SDivetime Stopwatch = {0,0,0,0};
    float fAverageDepth, fAverageDepthAbsolute;
    const SDecoinfo * pDecoinfoStandard;
    const SDecoinfo * pDecoinfoFuture;
    float fCNS;
    float temperature;
    SSettings* pSettings = settingsGetPointer();

    if(stateUsed->diveSettings.deco_type.ub.standard == GF_MODE)
    {
        pDecoinfoStandard = &stateUsed->decolistBuehlmann;
        pDecoinfoFuture = &stateUsed->decolistFutureBuehlmann;
    }
    else
    {
        pDecoinfoStandard = &stateUsed->decolistVPM;
        pDecoinfoFuture = &stateUsed->decolistFutureVPM;
    }

    Stopwatch.Total = timer_Stopwatch_GetTime();
    Stopwatch.Minutes = Stopwatch.Total / 60;
    Stopwatch.Seconds = Stopwatch.Total - ( Stopwatch.Minutes * 60 );
    fAverageDepth = timer_Stopwatch_GetAvarageDepth_Meter();
    fAverageDepthAbsolute = stateUsed->lifeData.average_depth_meter;

    headerText[0] = '\032';
    headerText[1] = '\f';

    switch(selection_custom_field)
    {
    /* Temperature */
    case LLC_Temperature:
    default:
    	temperature = unit_temperature_float(stateUsed->lifeData.temperature_celsius);
        headerText[2] = TXT_Temperature;
        textpointer = snprintf(text,TEXTSIZE,"\020\016%01.1f \140",temperature); // "\016\016%01.1f `" + C or F
        if(settingsGetPointer()->nonMetricalSystem == 0)
            text[textpointer++] = 'C';
        else
            text[textpointer++] = 'F';
        text[textpointer++] = 0;
        tinyHeaderFont = 0;
        break;

    /* Average Depth */
    case LLC_AverageDepth:
        headerText[2] = TXT_AvgDepth;
        if(pSettings->nonMetricalSystem)
            snprintf(text,TEXTSIZE,"\020%01.0f",unit_depth_float(fAverageDepthAbsolute));
        else
            snprintf(text,TEXTSIZE,"\020%01.1f",fAverageDepthAbsolute);
        break;

    /* ppO2 */
    case LLC_ppO2:
        headerText[2] = TXT_ppO2;
        snprintf(text,TEXTSIZE,"\020%01.2f",stateUsed->lifeData.ppO2);
        break;

    /* Stop Uhr */
    case LLC_Stopwatch:
        headerText[2] = TXT_Stopwatch;
        if(pSettings->nonMetricalSystem)
            snprintf(text,TEXTSIZE,"\020\016\016%u:%02u\n\r%01.0f",Stopwatch.Minutes, Stopwatch.Seconds,unit_depth_float(fAverageDepth));
        else
            snprintf(text,TEXTSIZE,"\020\016\016%u:%02u\n\r%01.1f",Stopwatch.Minutes, Stopwatch.Seconds,fAverageDepth);
        tinyHeaderFont = 1;
        line = 1;
        break;

    /* Ceiling */
    case LLC_Ceiling:
        headerText[2] = TXT_Ceiling;
        if((pDecoinfoStandard->output_ceiling_meter > 99.9f) || (pSettings->nonMetricalSystem))
            snprintf(text,TEXTSIZE,"\020%01.0f",unit_depth_float(pDecoinfoStandard->output_ceiling_meter));
        else
            snprintf(text,TEXTSIZE,"\020%01.1f",pDecoinfoStandard->output_ceiling_meter);
        break;

    /* Future TTS */
    case LLC_FutureTTS:
        headerText[2] = TXT_FutureTTS;
        if (pDecoinfoFuture->output_time_to_surface_seconds < 1000 * 60)
        	snprintf(text,TEXTSIZE,"\020\016\016@+%u'\n\r" "%i' TTS",pSettings->future_TTS, (pDecoinfoFuture->output_time_to_surface_seconds + 59) / 60);
        else
        	snprintf(text,TEXTSIZE,"\020\016\016@+%u'\n\r" "%ih TTS",pSettings->future_TTS, (pDecoinfoFuture->output_time_to_surface_seconds + 59) / 3600);
        tinyHeaderFont = 1;
        line = 1;
        break;

    /* CNS */
    case LLC_CNS:
        headerText[2] = TXT_CNS;
        fCNS = stateUsed->lifeData .cns;
        if(fCNS > 999)
            fCNS = 999;
        snprintf(text,TEXTSIZE,"\020%.0f\016\016%%\017",fCNS);
        break;

    /* actual GF */
    case LLC_GF:
        headerText[2] = TXT_ActualGradient;
        snprintf(text,TEXTSIZE,"\020%.0f\016\016%%\017",100 * pDecoinfoStandard->super_saturation);
        break;

    /* Surface GF */
    case LCC_GF_SURF:
        headerText[2] = TXT_2BYTE;
        headerText[3] = TXT2BYTE_GFSurf;
        snprintf(text,TEXTSIZE,"\020%.0f\016\016%%\017",100 * pDecoinfoStandard->gf_surf);
        break;

    case LLC_ScrubberTime:
    	tinyHeaderFont = 1;
        headerText[2] = TXT_ScrubTime;

        bool useTwoLines = (pSettings->scrubberActiveId == 0x03);
        textpointer = printScrubberText(text, TEXTSIZE, stateUsed->scrubberDataDive, pSettings, useTwoLines);

		break;
#ifdef ENABLE_PSCR_MODE
    case LCC_SimPpo2:
        headerText[2] = TXT_SimPpo2;
        snprintf(text,TEXTSIZE,"\020%.2f\016\016Bar\017",stateUsed->lifeData.ppo2Simulated_bar);
        break;
#endif

#ifdef ENABLE_BOTTLE_SENSOR
    case LCC_BottleBar:
        headerText[2] = TXT_AtemGasVorrat;
        tinyHeaderFont = 1;
        snprintf(text,TEXTSIZE,"%d\016\016\017", stateUsed->lifeData.bottle_bar[stateUsed->lifeData.actualGas.GasIdInSettings]);
        break;
#endif

#ifdef ENABLE_ADVANCED_GAS
    case LCC_BottleBar:
        headerText[2] = TXT_AtemGasVorrat;
        tinyHeaderFont = 1;
        if(isLoopMode(stateUsed->diveSettings.diveMode))
        {
        	if((gasIdO2 == 0) && (gasIdDiluent == 0))
        	{
        		DataEX_helper_Get_ID_Of_O2_Diluent(&gasIdO2, &gasIdDiluent);
        	}
        	if(gasIdO2 != 0)
        	{
        		textpointer = snprintf(text,TEXTSIZE,"\020\016\016%u",stateUsed->lifeData.bottle_bar[gasIdO2]);
        	}
        	if(gasIdDiluent != 0)
        	{
        		textpointer = snprintf(&text[textpointer],TEXTSIZE,"\n\r%u",stateUsed->lifeData.bottle_bar[gasIdDiluent]);
        	}
        }
        else
        {
        	snprintf(text,TEXTSIZE,"%d\016\016\017", stateUsed->lifeData.bottle_bar[stateUsed->lifeData.actualGas.GasIdInSettings]);
        }
        break;
#endif
#ifdef ENABLE_CO2_SUPPORT
    case LCC_CO2:
        headerText[2] = TXT_CO2Sensor;
        if(stateUsed->lifeData.CO2_data.CO2_ppm < CO2_WARNING_LEVEL_PPM)
        {
        	text[textpointer++] = '\020';
        }
        else if(stateUsed->lifeData.CO2_data.CO2_ppm < CO2_ALARM_LEVEL_PPM)
        {
        	text[textpointer++] = '\024';	/* yellow */
        }
        else
        {
        	text[textpointer++] = '\025'; 	/* red */
        }
        snprintf(&text[textpointer],TEXTSIZE,"\%5ldppm", stateUsed->lifeData.CO2_data.CO2_ppm);
      break;
#endif
    case LLC_Compass:
        headerText[2] = TXT_2BYTE;
        headerText[3] = TXT2BYTE_Compass;
        tinyHeaderFont = 1;

        uint16_t heading;
        if(settingsGetPointer()->compassInertia) {
            heading = (uint16_t)compass_getCompensated();
        } else {
            heading = (uint16_t)stateUsed->lifeData.compass_heading;
        }

        uint16_t userSetHeading = stateUsed->diveSettings.compassHeading;
        if (!userSetHeading) {
            snprintf(text, TEXTSIZE, "\020\002\034%u    ", heading);
        } else {
            snprintf(text, TEXTSIZE, "\020\002\034\016\016%u    ", heading);

            int16_t declinationFromForwardMark = ((userSetHeading - heading + 180 + 360) % 360) - 180;
            int16_t declinationFromNearestMark = ((declinationFromForwardMark + 90 + 180) % 180) - 90;

            uint16_t colour;
            if (abs(declinationFromForwardMark) <= 90) {
                colour = CLUT_CompassUserHeadingTick;
            } else {
                colour = CLUT_CompassUserBackHeadingTick;
            }

            char direction[] = "\001  \004 \004  ";
            if (abs(declinationFromNearestMark) <= 10) {
                direction[2] = '>';
                direction[6] = '<';

                if (abs(declinationFromForwardMark) <= 10) {
                    direction[4] = 'X';
                } else {
                    direction[4] = 'O';
                }

                if (abs(declinationFromNearestMark) <= 3) {
                    direction[3] = '\a';
                    direction[5] = '\a';
                }
            } else {
                if (declinationFromForwardMark < -90) {
                    direction[7] = 'O';
                } else if (declinationFromForwardMark < 0) {
                    direction[1] = 'X';
                } else if (declinationFromForwardMark <= 90) {
                    direction[7] = 'X';
                } else {
                    direction[1] = 'O';
                }

                if (declinationFromNearestMark >= 0) {
                    direction[6] = '>';
                }
                if (declinationFromNearestMark > 30) {
                    direction[4] = '>';
                }
                if (declinationFromNearestMark > 60 || declinationFromForwardMark == 90) {
                    direction[2] = '>';
                }
                if (declinationFromNearestMark < 0) {
                    direction[2] = '<';
                }
                if (declinationFromNearestMark < -30) {
                    direction[4] = '<';
                }
                if (declinationFromNearestMark < -60 || declinationFromForwardMark == -90) {
                    direction[6] = '<';
                }
            }

            GFX_write_string_color(&FontT48, &t7l3, direction, 1, colour);
        }

        break;
    }

    if (headerText[2] != TXT_2BYTE) {
        headerText[3] = 0;
    } else {
        headerText[4] = 0;
    }

    if(tinyHeaderFont)
        GFX_write_string(&FontT24,&t7l3,headerText,0);
    else
        GFX_write_string(&FontT42,&t7l3,headerText,0);

    Gfx_colorsscheme_mod(text, 0);
#ifndef ENABLE_BOTTLE_SENSOR
#ifdef ENABLE_CO2_SUPPORT
    if(selection_custom_field != LCC_CO2)
    {
    	GFX_write_string(&FontT105,&t7l3,text,line);
    }
    else
    {
        GFX_write_string(&FontT48,&t7l3,text,line);
    }
#else
    GFX_write_string(&FontT105,&t7l3,text,line);
#endif
#else
    if(selection_custom_field != LCC_BottleBar)			/* a changing color set is used for bar display */
    {
    	GFX_write_string(&FontT105,&t7l3,text,line);
    }
    else
    {
    	agedColor = stateUsed->lifeData.bottle_bar_age_MilliSeconds[stateUsed->lifeData.actualGas.GasIdInSettings];
    	if(agedColor > 1200)
    	{
    		agedColor = CLUT_WarningRed;
    	}
    	else
    	if(agedColor > 600)
        {
        	agedColor = CLUT_MenuLineUnselected;
        }
        else
    	if(agedColor > 20)
    	{
    		agedColor = CLUT_Font031;
    	}
    	else
    	{
    		agedColor = CLUT_Font020;
    	}

    	GFX_write_string_color(&FontT105,&t7l3,text,line,agedColor);
    }
#endif
}

/* Private functions ---------------------------------------------------------*/



void draw_frame(_Bool PluginBoxHeader, _Bool LinesOnTheSides, uint8_t colorBox, uint8_t colorLinesOnTheSide)
{
    point_t LeftLow, WidthHeight;
    point_t start, stop;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

    // plugin box
    LeftLow.x = CUSTOMBOX_LINE_LEFT;
    WidthHeight.x = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_LINE_LEFT;
    LeftLow.y = 60;
    WidthHeight.y = 440 - LeftLow.y;
    if((viewInFocus() && (!viewDetectionSuspended())))
    {
    	GFX_draw_box(&t7screen, LeftLow, WidthHeight, 1, CLUT_Font023);
    }
    else
    {
    	GFX_draw_box(&t7screen, LeftLow, WidthHeight, 1, colorBox);
    }

    if(PluginBoxHeader)
    {
        // plugin box - header
        start.x = CUSTOMBOX_LINE_LEFT;
        stop.x = CUSTOMBOX_LINE_RIGHT;
        stop.y = start.y = 440 - 60;
        GFX_draw_line(&t7screen, start, stop, colorBox);
    }

    if(LinesOnTheSides)
    {
        // aufteilung links
        start.x = 0;
        stop.x = CUSTOMBOX_LINE_LEFT;
        if(!pSettings->FlipDisplay)
        {
        	stop.y = start.y = t7l1.WindowY0 - 1;
        }
        else
        {
        	stop.y = start.y = 480 - t7l1.WindowY1 - 1;
        }

        GFX_draw_line(&t7screen, start, stop, colorLinesOnTheSide);
        if(!pSettings->FlipDisplay)
        {
        	stop.y = start.y = t7l2.WindowY0 -1;
        }
        else
        {
        	stop.y = start.y = 480 - t7l2.WindowY1 -1;
        }
        GFX_draw_line(&t7screen, start, stop, colorLinesOnTheSide);

        // aufteilung rechts
        start.x = CUSTOMBOX_LINE_RIGHT;
        stop.x = 799;
        if(!pSettings->FlipDisplay)
        {
        	stop.y = start.y = t7l1.WindowY0 - 1;
        }
        else
        {
        	stop.y = start.y = 480 - t7l1.WindowY1 - 1;
        }

        GFX_draw_line(&t7screen, start, stop, colorLinesOnTheSide);
        if(!pSettings->FlipDisplay)
        {
        	stop.y = start.y = t7l2.WindowY0 - 1;
        }
        else
        {
        	stop.y = start.y = 480 - t7l2.WindowY1 - 1;
        }
        GFX_draw_line(&t7screen, start, stop, colorLinesOnTheSide);
    }
}


/* range should be 0 to 30 bar if 300 meter with 100% of nitrogen or helium
 * T24 is 28 high
*/
void t7_tissues(const SDiveState * pState)
{
    point_t start, change, stop;
    float value;
    uint16_t front, cns100pixel;
    char text[256];
    uint8_t textpointer = 0;
    uint8_t color;

    float percent_N2;
    float percent_He;
    float partial_pressure_N2;
    float partial_pressure_He;

	SSettings* pSettings;
	pSettings = settingsGetPointer();


    /* N2 */
    t7cY0free.WindowLineSpacing = 28 + 48 + 14;
    if(!pSettings->FlipDisplay)
    {
    	t7cY0free.WindowY0 = t7cH.WindowY0 - 5 - 2 * t7cY0free.WindowLineSpacing;
    }
    else
    {
    	t7cY0free.WindowY0 = t7cH.WindowY0 + 15;
    	t7cY0free.WindowY1 = t7cY0free.WindowY0 + 250;
    }
    t7cY0free.WindowNumberOfTextLines = 3;

    text[textpointer++] = '\030';
    text[textpointer++] = TXT_2BYTE;
    text[textpointer++] = TXT2BYTE_Nitrogen;
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = TXT_2BYTE;
    text[textpointer++] = TXT2BYTE_Helium;
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = TXT_2BYTE;
    text[textpointer++] = TXT2BYTE_CNS;
    text[textpointer++] = 0;

    GFX_write_string(&FontT24, &t7cY0free, text, 1);

    if(!pSettings->FlipDisplay)
    {
    	start.y = t7cH.WindowY0 - 5;
    }
    else
    {
    	start.y = t7cH.WindowY1 - 5;
    }
    start.x = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    stop.x = start.x + CUSTOMBOX_SPACE_INSIDE;

    for(int i=0;i<16;i++)
    {
        stop.y = start.y;
        change.y = start.y;

        value = pState->lifeData.tissue_nitrogen_bar[i] - 0.7512f;
        value *= 80;

        if(value < 0)
            front = 0;
        else
        if(value > CUSTOMBOX_SPACE_INSIDE)
            front = CUSTOMBOX_SPACE_INSIDE;
        else
                front = (uint16_t)value;

        change.x = start.x + front;
        if(change.x != start.x)
            GFX_draw_thick_line(1,&t7screen, start, change, CLUT_Font030);
        if(change.x != stop.x)
            GFX_draw_thick_line(1,&t7screen, change, stop, CLUT_Font031);

        start.y -= 3;
    }

    /* He */
    start.y -= 28 + 14;
    for(int i=0;i<16;i++)
    {
        stop.y = start.y;
        change.y = start.y;

        value = pState->lifeData.tissue_helium_bar[i];
        value *= 80;//20

        if(value < 0)
            front = 0;
        else if(value > CUSTOMBOX_SPACE_INSIDE)
            front = CUSTOMBOX_SPACE_INSIDE;
        else
                front = (uint16_t)value;

        change.x = start.x + front;
        if(change.x != start.x)
            GFX_draw_thick_line(1,&t7screen, start, change, CLUT_Font030);
        if(change.x != stop.x)
            GFX_draw_thick_line(1,&t7screen, change, stop, CLUT_Font031);

        start.y -= 3;
    }

    /* CNS == Oxygen */
    start.y -= 28 + 14;

    cns100pixel = (8 * CUSTOMBOX_SPACE_INSIDE) / 10;
    value = pState->lifeData.cns;
    value *= cns100pixel;
    value /= 100;

    if(value < 0)
        front = 0;
    else if(value > CUSTOMBOX_SPACE_INSIDE)
        front = CUSTOMBOX_SPACE_INSIDE;
    else
            front = (uint16_t)value;

    if(pState->lifeData.cns < 95)
        color = CLUT_Font030;
    else if(pState->lifeData.cns < 100)
        color =  CLUT_WarningYellow;
    else
        color = CLUT_WarningRed;

    for(int i=0;i<16;i++)
    {
        stop.y = start.y;
        change.y = start.y;

        change.x = start.x + front;
        if(change.x != start.x)
            GFX_draw_thick_line(1,&t7screen, start, change, color);
        if(change.x != stop.x)
            GFX_draw_thick_line(1,&t7screen, change, stop, CLUT_Font031);

        start.y -= 3;
    }

    /* where is the onload/offload limit for N2 and He */
    decom_get_inert_gases(pState->lifeData.pressure_ambient_bar, &pState->lifeData.actualGas, &percent_N2, &percent_He);
    partial_pressure_N2 =  (pState->lifeData.pressure_ambient_bar - WATER_VAPOUR_PRESSURE) * percent_N2;
    partial_pressure_He = (pState->lifeData.pressure_ambient_bar - WATER_VAPOUR_PRESSURE) * percent_He;

    // Nitrogen vertical bar
    if(!pSettings->FlipDisplay)
    {
    	start.y = t7cH.WindowY0 + 1 - 5;
    }
    else
    {
    	start.y = t7cH.WindowY1 - 5;
    }

    stop.y = start.y - (3 * 15) - 1;
    if((percent_N2 > 0) && (partial_pressure_N2 > 0.7512f))
    {
        value = partial_pressure_N2 - 0.7512f;
        value *= 80;

        if(value < 0)
            front = 3;
        else if(value + 5 > CUSTOMBOX_SPACE_INSIDE)
            front = CUSTOMBOX_SPACE_INSIDE - 3;
        else
                front = (uint16_t)value;
    }
    else
    {
        front = 1;
    }
    start.x = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET + front;
    stop.x = start.x;
    GFX_draw_thick_line(2,&t7screen, start, stop, CLUT_EverythingOkayGreen);


    // Helium vertical bar
    if(!pSettings->FlipDisplay)
    {
    	start.y = t7cH.WindowY0 + 1 - 5 - 3*16 - 28 - 14;
    }
    else
    {
    	start.y = t7cH.WindowY1 - 5 - 3*16 - 28 - 14;
    }

    stop.y = start.y - (3 * 15) - 1;
    if((percent_He > 0) && (partial_pressure_He > 0.01f)) // 0.5f
    {

        value = partial_pressure_He;
        value *= 80;

        if(value < 0)
            front = 3;
        else if(value + 5 > CUSTOMBOX_SPACE_INSIDE)
            front = CUSTOMBOX_SPACE_INSIDE - 3;
        else
                front = (uint16_t)value;
    }
    else
    {
        front = 1;
    }

    start.x = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET + front;
    stop.x = start.x;
    GFX_draw_thick_line(2,&t7screen, start, stop, CLUT_EverythingOkayGreen);

    // Oxygen vertical bar
    if(!pSettings->FlipDisplay)
    {
    	start.y = t7cH.WindowY0 + 1 - 5 - 6*16 - 2*28 - 2*14;
    }
    else
    {
    	start.y = t7cH.WindowY1 - 5 - 6*16 - 2*28 - 2*14;
    }

    stop.y = start.y - (3 * 15) - 1;

    start.x = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET + cns100pixel;
    stop.x = start.x;
    GFX_draw_thick_line(2, &t7screen, start, stop, CLUT_WarningRed);

}




/* Position of the session line of the position view (progress or result of an acquire GNSS fix session) and of the line which shows that the receiver is in
 * power save mode. Both are lines of their own (font T24, 28 px high, ink 23 px) below the bars and the longitude of a fix and above the last known position.
 * Heights are counted from the bottom of the frame (the normal display shows 479 - height from the top, the flipped display the height from the top):
 * - normal display, Y0 of the value window is 83:
 *     session line: starts 60 px above it, ink 143..165 px (panel rows from the top 314..336): 53 px clearance to the bars (bottom edge 213 px) and 16 px to the
 *     longitude of a fix (ink bottom 182 px)
 *     power save line: starts 28 px above it, ink 111..133 px (346..368): 9 px below the session line, 6 px above the last known position (ink top 104 px)
 * - flipped display, Y1 of the label window is 311 (the rows are counted from the top):
 *     session line: Y1 - 8, ink 281..303: 42 px clearance to the bars (bottom edge 239) and 10 px to the longitude (ink ends at 271)
 *     power save line: Y1 + 24, ink 313..335: 9 px below the session line, 13 px above the last known position (ink top 349)
 * (the session line was 8 px lower before, 322..344 and 289..311). The widest text of the power save line is 248 px, centred in the 295 px window.
 * The "no signal" hint (font T24, 265 px wide, ink x 268..531 of the 800 px of the screen, ink 23 px high) is a line of its own in the free band between the bars and the
 * session line; it is only shown without a fix (so the longitude of a fix is not there) and at full power (so the power save line is not there):
 * - normal display: starts 95 px above Y0 of the value window, ink 178..200 px (panel rows from the top 279..301): 13 px below the bars (bottom edge 213 px) and 13 px
 *     above the session line (ink top 165 px)
 * - flipped display: Y1 of the label window - 40, ink 249..271 (rows from the top): 10 px below the bars (bottom edge 239) and 10 px above the session line (ink top 281) */
#define POSITION_SESSION_LINE_OFFSET_Y		(60u)
#define POSITION_SESSION_LINE_OFFSET_Y_FLIP	(8u)		/* the session line is this far above Y1 of the label window */
#define POSITION_HINT_LINE_OFFSET_Y			(95u)
#define POSITION_HINT_LINE_OFFSET_Y_FLIP	(40u)		/* the hint line is this far above Y1 of the label window */
#define POSITION_POWER_LINE_OFFSET_Y		(28u)
#define POSITION_POWER_LINE_OFFSET_Y_FLIP	(24u)		/* the power save line is this far below Y1 of the label window */

/* Position of the satellite bars of the position view. A bar is a 20 px brush drawn along a 20 px line, so it is 40 px high:
 * - normal display: start.y = Y0 of the value window (83) + 140 => the bars cover 213..252 px above the bottom of the frame, the ink of the
 *   satellite count (font T42, value line 1) ends at 272 px: 19 px clearance. The x position gives 327..466 px
 * - flipped display: start.y = Y1 of the value window (370) - 120 => the bars cover 200..239 px (viewed from the flipped side, the rows
 *   are counted from the top), the ink of the count ends at 181 px: 18 px clearance. The drawing function mirrors x like in the normal case.
 *   A previous version used a negative start.y for the flipped display, which only worked because it wrapped around in gfx_brush */
#define POSITION_BARS_OFFSET_X				(85u)
#define POSITION_BARS_OFFSET_Y				(140u)
#define POSITION_BARS_OFFSET_Y_FLIP			(120u)

/* Text of the session line of the position view. Returns the number of characters written */
static uint8_t t7_getGnssAcquireText(char* pText, uint8_t size)
{
    uint16_t remaining;
    uint8_t length = 0;

    switch(gnssAcquire_getState())
    {
        case GNSS_ACQUIRE_ACTIVE:
            remaining = gnssAcquire_getRemainingS();
            if(gnssAcquire_isWaitingForEphemeris())		/* fix OK, waiting for the ephemeris */
            {
                length = snprintf(pText, size, "\001\020Fix OK eph %u/%u %u:%02u", gnssAcquire_getEphemerisCount(), GNSS_ACQUIRE_EPH_MIN_SV, remaining / 60, remaining % 60);
            }
            else
            {
                length = snprintf(pText, size, "\001\020Fix %u:%02u BACK=stop", remaining / 60, remaining % 60);
            }
            break;
        case GNSS_ACQUIRE_COMPLETE:
            length = snprintf(pText, size, "\001\026Fix + ephemeris OK");
            break;
        case GNSS_ACQUIRE_FIX_ONLY:
            length = snprintf(pText, size, "\001\024Fix, eph %u/%u", gnssAcquire_getEphemerisCount(), GNSS_ACQUIRE_EPH_MIN_SV);
            break;
        case GNSS_ACQUIRE_POORFIX:
            length = snprintf(pText, size, "\001\024Poor fix only");
            break;
        case GNSS_ACQUIRE_TIMEOUT:
            length = snprintf(pText, size, gnssAcquire_wasSignalSeen() ? "\001\025No fix, timed out" : "\001\025No signal, timed out");
            break;
        case GNSS_ACQUIRE_LOWBATTERY:
            length = snprintf(pText, size, "\001\025Low battery");
            break;
        case GNSS_ACQUIRE_CANCELLED:
        case GNSS_ACQUIRE_ABORTED:
            length = snprintf(pText, size, "\001\020Fix search stopped");
            break;
        default:
            break;
    }
    return length;
}


/* Power save state of the receiver (psmState of UBX-NAV-PVT): 0 if it is not known (receiver does not answer, no UBX-NAV-PVT parsed yet) or not in power save
 * mode, else 1 enabled, 2 acquisition, 3 tracking, 4 power optimized tracking, 5 inactive */
static uint8_t t7_getGnssPowerSaveState(void)
{
    const SGnssDebugInfo* pDbg = DataEX_getGnssDebugInfo();

    if((stateUsed->lifeData.gnssData.alive & GNSS_ALIVE_STATE_ALIVE) && (pDbg->recordsSeen & 0x02u) && (pDbg->psmState >= 1) && (pDbg->psmState <= 5))
    {
        return pDbg->psmState;
    }
    return 0;
}

/* The user is told that no satellite signal is received: an acquire fix session has run for GNSS_ACQUIRE_NOSIGNAL_S, satellites are listed but none has a C/N0 above 0
 * (the tracked count and the best C/N0 of the debug records of the RTE, which the main CPU decodes in every build: it is the "tracked/listed" count of the position view),
 * the receiver answers, it reports full power (psmState 0; in power save mode the power save line explains why nothing is tracked) and there is no fix */
static uint8_t t7_gnssNoSignalHint(void)
{
    const SGnssDebugInfo* pDbg = DataEX_getGnssDebugInfo();
    const SGnssInfo* pGnss = &stateUsed->lifeData.gnssData;

    return (gnssAcquire_getState() == GNSS_ACQUIRE_ACTIVE) && (gnssAcquire_getElapsedS() >= GNSS_ACQUIRE_NOSIGNAL_S)
            && (pGnss->alive & GNSS_ALIVE_STATE_ALIVE) && (!GNSS_HAS_FIX(pGnss)) && (pGnss->numSat > 0)
            && ((pDbg->recordsSeen & 0x03u) == 0x03u) && (pDbg->psmState == 0) && (pDbg->numTracked == 0) && (pDbg->bestCno == 0);
}

void t7_showPosition(void)
{
    char text[256+50];
    uint8_t textpointer = 0;
    point_t start, stop;
    uint8_t index = 0;
    uint8_t color = 0;
    uint16_t sessionY0, sessionY1, sessionLines;
    uint16_t labelY1 = 0;
    SSettings* pSettings = settingsGetPointer();
    uint8_t gnssAlive = stateUsed->lifeData.gnssData.alive & GNSS_ALIVE_STATE_ALIVE;	/* satellite data is only current while the receiver answers */
    uint8_t powerSave = t7_getGnssPowerSaveState();

    t7cY0free.WindowLineSpacing = 28 + 48 + 14;
    t7cY0free.WindowY0 = t7cH.WindowY0 - 5 - 2 * t7cY0free.WindowLineSpacing;
    t7cY0free.WindowNumberOfTextLines = 3;
    t7cY0free.WindowY0 -= 20;

    t7cY0free.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t7cY0free.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;

    if(pSettings->FlipDisplay)
    {
       	t7cY0free.WindowY0 = t7cH.WindowY0 + 15;
        t7cY0free.WindowY1 = t7cY0free.WindowY0 + 250;
        labelY1 = t7cY0free.WindowY1;
    }

    if(!GNSS_HAS_FIX(&stateUsed->lifeData.gnssData))
    {
    	textpointer += snprintf(&text[textpointer],50,"\001Sats tracked/listed\n\r");
    }
    else
    {
		textpointer += snprintf(&text[textpointer],50,"\001Latitude\n\r");
		textpointer += snprintf(&text[textpointer],50,"\001Longitude\n\r");
    }
    GFX_write_string(&FontT24, &t7cY0free, text, 1);

    if(!pSettings->FlipDisplay)
    {
    	t7cY0free.WindowY0 -= 52;
    }
    else
    {
        t7cY0free.WindowY1 = 370;
    }

    if(!GNSS_HAS_FIX(&stateUsed->lifeData.gnssData))
    {
    	if(gnssAlive)
    	{
    		/* tracked: satellites with a C/N0 above 0, listed: all satellites of the receiver list, which includes those which are only searched for (the
    		 * number shown before was the listed one). The tracked count comes from the debug records, it is not available until the first one was received */
    		if(DataEX_getGnssDebugInfo()->recordsSeen & 0x01u)
    		{
    			snprintf(text,60,"\001%u/%u\n\r",DataEX_getGnssDebugInfo()->numTracked,stateUsed->lifeData.gnssData.numSat);
    		}
    		else
    		{
    			snprintf(text,60,"\001--/%u\n\r",stateUsed->lifeData.gnssData.numSat);
    		}
    	}
    	else
    	{
    		snprintf(text,60,"\001--\n\r");		/* the last value is outdated: do not show it as a current reading */
    	}
    }
    else
    {
    	snprintf(text,60,
    			"\001%0.5f\n\r"
    			"\001%0.5f\n\r"
    			,stateUsed->lifeData.gnssData.coord.fLat ,stateUsed->lifeData.gnssData.coord.fLon);
    }
    GFX_write_string(&FontT42, &t7cY0free, text, 1);

    if(!GNSS_HAS_FIX(&stateUsed->lifeData.gnssData))	/* draw status bars */
    {
    	/* the x position is the same for both orientations: the drawing functions mirror it for the flipped display */
    	start.x = t7cY0free.WindowX0 + POSITION_BARS_OFFSET_X;
    	stop.x = start.x;
    	if(!pSettings->FlipDisplay)
    	{
    		start.y = t7cY0free.WindowY0 + POSITION_BARS_OFFSET_Y;
    	}
    	else
    	{
    		start.y = t7cY0free.WindowY1 - POSITION_BARS_OFFSET_Y_FLIP;
    	}
    	stop.y = start.y + 20;

    	while((gnssAlive) && (index < stateUsed->lifeData.gnssData.numSat) && (index < 4))	/* no bars for outdated data */
    	{
    		if(stateUsed->lifeData.gnssData.signalQual[index] > 4) color = CLUT_NiceGreen;
    		if((stateUsed->lifeData.gnssData.signalQual[index] > 2) && (stateUsed->lifeData.gnssData.signalQual[index] <= 4)) color = CLUT_WarningYellow;
    		if(stateUsed->lifeData.gnssData.signalQual[index] <= 2) color = CLUT_WarningRed;
    		GFX_draw_thick_line(20, &t7screen, start, stop, color);
    		start.x += 40;
    		stop.x += 40;

    		index++;
    	}
    	if(stateUsed->lifeData.gnssData.alive & GNSS_ALIVE_POS_VALID)
    	{
    		snprintf(text,50,"\001%2.2f %2.2f", stateUsed->lifeData.gnssData.coord.fLat,stateUsed->lifeData.gnssData.coord.fLon);
    		GFX_write_string(&FontT24, &t7cY0free, text, 3);
    	}
    }

    /* session line: a line of its own which does not overlap the bars, the window is restored afterwards */
    if(t7_getGnssAcquireText(text, 50) != 0)
    {
    	sessionY0 = t7cY0free.WindowY0;
    	sessionY1 = t7cY0free.WindowY1;
    	sessionLines = t7cY0free.WindowNumberOfTextLines;
    	t7cY0free.WindowNumberOfTextLines = 1;
    	if(!pSettings->FlipDisplay)
    	{
    		t7cY0free.WindowY0 = sessionY0 + POSITION_SESSION_LINE_OFFSET_Y;
    	}
    	else
    	{
    		t7cY0free.WindowY1 = labelY1 - POSITION_SESSION_LINE_OFFSET_Y_FLIP;
    	}
    	GFX_write_string(&FontT24, &t7cY0free, text, 1);
    	t7cY0free.WindowY0 = sessionY0;
    	t7cY0free.WindowY1 = sessionY1;
    	t7cY0free.WindowNumberOfTextLines = sessionLines;
    }

    /* hint that no signal is received (see t7_gnssNoSignalHint): yellow, a line of its own between the bars and the session line */
    if(t7_gnssNoSignalHint())
    {
    	sessionY0 = t7cY0free.WindowY0;
    	sessionY1 = t7cY0free.WindowY1;
    	sessionLines = t7cY0free.WindowNumberOfTextLines;
    	t7cY0free.WindowNumberOfTextLines = 1;
    	if(!pSettings->FlipDisplay)
    	{
    		t7cY0free.WindowY0 = sessionY0 + POSITION_HINT_LINE_OFFSET_Y;
    	}
    	else
    	{
    		t7cY0free.WindowY1 = labelY1 - POSITION_HINT_LINE_OFFSET_Y_FLIP;
    	}
    	GFX_write_string_color(&FontT24, &t7cY0free, "\001No signal: need open sky", 1, CLUT_WarningYellow);
    	t7cY0free.WindowY0 = sessionY0;
    	t7cY0free.WindowY1 = sessionY1;
    	t7cY0free.WindowNumberOfTextLines = sessionLines;
    }

    /* power save line: shown only while the receiver reports a power save state (the normal view stays clean), yellow; the inactive state is red */
    if(powerSave != 0)
    {
    	static const char* const powerSaveText[] = { "", "Power save: starting", "Power save: acquiring", "Power save: tracking", "Power save: low power", "Power save: inactive" };

    	sessionY0 = t7cY0free.WindowY0;
    	sessionY1 = t7cY0free.WindowY1;
    	sessionLines = t7cY0free.WindowNumberOfTextLines;
    	t7cY0free.WindowNumberOfTextLines = 1;
    	if(!pSettings->FlipDisplay)
    	{
    		t7cY0free.WindowY0 = sessionY0 + POSITION_POWER_LINE_OFFSET_Y;
    	}
    	else
    	{
    		t7cY0free.WindowY1 = labelY1 + POSITION_POWER_LINE_OFFSET_Y_FLIP;
    	}
    	snprintf(text, 50, "\001%s", powerSaveText[powerSave]);
    	GFX_write_string_color(&FontT24, &t7cY0free, text, 1, (powerSave == 5) ? CLUT_WarningRed : CLUT_WarningYellow);
    	t7cY0free.WindowY0 = sessionY0;
    	t7cY0free.WindowY1 = sessionY1;
    	t7cY0free.WindowNumberOfTextLines = sessionLines;
    }
}
/* The debug pages of the receiver are only built with the debug view (HAVE_DEBUG_VIEW of configuration.h); without it ENTER is not used by the position view */
#ifndef HAVE_DEBUG_VIEW
bool t7_isGnssDebugPageShown(void)
{
    return false;
}

bool t7_handleGnssDebugPageAction(uint8_t action)
{
    (void)action;
    return false;
}
#else
bool t7_isGnssDebugPageShown(void)
{
    return gnssDebugPage && (selection_customview == CVIEW_Position) && (stateUsed->mode == MODE_SURFACE);
}


/* ENTER on the position view shows the debug page, the next ENTER the page of the receiver hardware and the third ENTER leaves it and continues with the next view.
 * Returns true if the action was used */
bool t7_handleGnssDebugPageAction(uint8_t action)
{
    if((action != ACTION_BUTTON_ENTER) || (stateUsed->mode != MODE_SURFACE))
    {
        return false;
    }
    if(gnssDebugPage == 1)
    {
        gnssDebugPage = 2;
        return true;
    }
    if(gnssDebugPage)
    {
        gnssDebugPage = 0;
        return false;
    }
    if(selection_customview == CVIEW_Position)
    {
        gnssDebugPage = 1;
        return true;
    }
    return false;
}


/* GNSS debug page: a full screen of values for a photograph. Rows of font T24 (28 px high, ink at most 23 px) every 34 px, in two columns.
 * The positions are counted from the top and the left of the screen, Gfx_write_label_var mirrors them for the flipped display. The widest text of the
 * left column is 375 px (x 12..387), the right column starts at 420 px and ends at 785 px at most.
 * The values of the receiver are those of the debug records of the RTE, see data_exchange.h and the u-blox 8 / u-blox M8 receiver description */
#define GNSSDBG_ROW_STEP		(34u)
#define GNSSDBG_LEFT_X			(12u)
#define GNSSDBG_LEFT_X_END		(410u)
#define GNSSDBG_RIGHT_X			(420u)
#define GNSSDBG_TITLE_Y			(6u)
#define GNSSDBG_ROW0_Y			(44u)
#define GNSSDBG_SAT0_Y			(GNSSDBG_ROW0_Y + GNSSDBG_ROW_STEP)
#define GNSSDBG_COL_CNO			(520u)
#define GNSSDBG_COL_QUALITY		(600u)
#define GNSSDBG_COL_USED		(650u)
#define GNSSDBG_COL_EPH			(690u)
#define GNSSDBG_COL_ELEV		(730u)

static void t7_gnssDebugWrite(uint16_t x, uint16_t xEnd, uint16_t y, uint8_t color, const char* text)
{
    Gfx_write_label_var(&t7screen, x, xEnd, y, &FontT24, color, text);
}

static const char* t7_gnssSessionText(void)
{
    switch(gnssAcquire_getState())
    {
        case GNSS_ACQUIRE_ACTIVE:		return "active";
        case GNSS_ACQUIRE_COMPLETE:		return "complete";
        case GNSS_ACQUIRE_FIX_ONLY:		return "fix only";
        case GNSS_ACQUIRE_POORFIX:		return "poor fix";
        case GNSS_ACQUIRE_TIMEOUT:		return "timeout";
        case GNSS_ACQUIRE_CANCELLED:	return "stopped";
        case GNSS_ACQUIRE_LOWBATTERY:	return "low battery";
        case GNSS_ACQUIRE_ABORTED:		return "aborted";
        default:						return "none";
    }
}

void t7_refresh_gnss_debug(void)
{
    const SGnssInfo* pGnss = &stateUsed->lifeData.gnssData;
    const SGnssDebugInfo* pDbg = DataEX_getGnssDebugInfo();
    const char* fixText[] = {"none", "DR", "2D", "3D", "GNSS+DR", "time only"};
    const char constellation[] = {'G', 'S', 'E', 'B', 'I', 'Q', 'R'};		/* gnssId 0..6 */
    char text[64];
    char text2[24];
    uint8_t alive = pGnss->alive & GNSS_ALIVE_STATE_ALIVE;
    uint8_t haveA = pDbg->recordsSeen & 0x01u;
    uint8_t haveB = pDbg->recordsSeen & 0x02u;
    uint8_t fixOk = pGnss->alive & GNSS_ALIVE_FIX_OK;
    uint8_t color;
    uint8_t row = 0;
    uint8_t i;
    uint16_t y;
    uint32_t now_s = HAL_GetTick() / 1000u;
    uint32_t firstFix_s = DataEX_getGnssFirstFix_s();
    uint32_t session_s = gnssAcquire_getElapsedS();
    const SGnssDebugSat* pSat;

    t7_gnssDebugWrite(GNSSDBG_LEFT_X, GNSSDBG_LEFT_X_END, GNSSDBG_TITLE_Y, CLUT_Font020, "GNSS debug 1/2");
    t7_gnssDebugWrite(GNSSDBG_RIGHT_X, 799, GNSSDBG_TITLE_Y, CLUT_Font020, "ENTER: next page");

#define GNSSDBG_LEFT_ROW(color, text)	t7_gnssDebugWrite(GNSSDBG_LEFT_X, GNSSDBG_LEFT_X_END, GNSSDBG_ROW0_Y + (row++) * GNSSDBG_ROW_STEP, (color), (text))

    /* receiver answers, age of the last valid frame */
    if(!haveA)
    {
        snprintf(text2, sizeof(text2), "--");
    }
    else if(pDbg->frameAge_s == 255)
    {
        snprintf(text2, sizeof(text2), "255+ s");
    }
    else
    {
        snprintf(text2, sizeof(text2), "%u s", pDbg->frameAge_s);
    }
    snprintf(text, sizeof(text), "Alive: %s   frame age %s", alive ? "YES" : "NO", text2);
    GNSSDBG_LEFT_ROW(alive ? CLUT_NiceGreen : CLUT_WarningRed, text);

    /* fix type, gnssFixOK, horizontal accuracy */
    if((!haveA) || (pDbg->hAcc_m == 255))
    {
        snprintf(text2, sizeof(text2), "--");
    }
    else
    {
        snprintf(text2, sizeof(text2), "%u m", pDbg->hAcc_m);
    }
    snprintf(text, sizeof(text), "Fix: %s  OK: %s  hAcc %s", (pGnss->fixType < 6) ? fixText[pGnss->fixType] : "?", fixOk ? "YES" : "NO", text2);
    if((pGnss->fixType == 3) && fixOk)
    {
        color = CLUT_NiceGreen;
    }
    else if(pGnss->fixType >= 2)
    {
        color = CLUT_WarningYellow;
    }
    else
    {
        color = CLUT_Font020;
    }
    GNSSDBG_LEFT_ROW(color, text);

    /* pDOP, satellites used in the solution */
    if((!haveA) || (pDbg->pDop_tenth == 255))
    {
        snprintf(text2, sizeof(text2), "--");
    }
    else
    {
        snprintf(text2, sizeof(text2), "%u.%u", pDbg->pDop_tenth / 10, pDbg->pDop_tenth % 10);
    }
    snprintf(text, sizeof(text), "pDOP %s   SV used (PVT) %u", text2, haveB ? pDbg->numSvUsedPvt : 0);
    GNSSDBG_LEFT_ROW(CLUT_Font020, text);

    /* satellites: listed, with C/N0 above 0, used (the counts are limited to 15) */
    snprintf(text, sizeof(text), "Listed %u  Tracked %u  Used %u", pGnss->numSat, haveA ? pDbg->numTracked : 0, haveA ? pDbg->numUsed : 0);
    GNSSDBG_LEFT_ROW(CLUT_Font020, text);

    /* ephemeris of GPS, Galileo, BeiDou and QZSS satellites */
    if(alive)
    {
        snprintf(text, sizeof(text), "Ephemeris %u (GPS GAL BDS QZSS)", GNSS_EPH_COUNT(pGnss));
    }
    else
    {
        snprintf(text, sizeof(text), "Ephemeris -- (GPS GAL BDS QZSS)");
    }
    GNSSDBG_LEFT_ROW(CLUT_Font020, text);

    snprintf(text, sizeof(text), "Best C/N0: %u dBHz", haveA ? pDbg->bestCno : 0);
    GNSSDBG_LEFT_ROW(CLUT_Font020, text);

    /* time flags of UBX-NAV-PVT */
    if(haveB)
    {
        snprintf(text, sizeof(text), "Time valid: date %s  time %s", (pDbg->timeFlags & 0x01u) ? "YES" : "NO", (pDbg->timeFlags & 0x02u) ? "YES" : "NO");
        GNSSDBG_LEFT_ROW(CLUT_Font020, text);
        if(pDbg->timeFlags & 0x08u)
        {
            snprintf(text2, sizeof(text2), "%s", ((pDbg->timeFlags & 0x30u) == 0x30u) ? "YES" : "NO");
        }
        else
        {
            snprintf(text2, sizeof(text2), "n/a");
        }
        snprintf(text, sizeof(text), "Resolved %s  Confirmed %s", (pDbg->timeFlags & 0x04u) ? "YES" : "NO", text2);
        GNSSDBG_LEFT_ROW(CLUT_Font020, text);
    }
    else
    {
        GNSSDBG_LEFT_ROW(CLUT_Font020, "Time valid: --");
        GNSSDBG_LEFT_ROW(CLUT_Font020, "Resolved --  Confirmed --");
    }

    if(pGnss->alive & GNSS_ALIVE_STATE_TIME)
    {
        snprintf(text, sizeof(text), "UTC %04u-%02u-%02u %02u:%02u:%02u", 2000 + pGnss->DateTime.year, pGnss->DateTime.month, pGnss->DateTime.day,
                 pGnss->DateTime.hour, pGnss->DateTime.min, pGnss->DateTime.sec);
    }
    else
    {
        snprintf(text, sizeof(text), "UTC --");
    }
    GNSSDBG_LEFT_ROW(CLUT_Font020, text);

    /* acquire fix session, time since the start of the main CPU and of the first fix */
    snprintf(text, sizeof(text), "Session: %s %lu:%02lu", t7_gnssSessionText(), session_s / 60u, session_s % 60u);
    GNSSDBG_LEFT_ROW(CLUT_Font020, text);

    if(firstFix_s == 0xFFFFFFFFu)
    {
        snprintf(text2, sizeof(text2), "--");
    }
    else
    {
        snprintf(text2, sizeof(text2), "%lu:%02lu", firstFix_s / 60u, firstFix_s % 60u);
    }
    snprintf(text, sizeof(text), "Up %lu:%02lu  TTFF %s", now_s / 60u, now_s % 60u, text2);
    GNSSDBG_LEFT_ROW(CLUT_Font020, text);

    /* power save state of UBX-NAV-PVT, rejected commands and UART errors of the RTE */
    if(haveB)
    {
        snprintf(text, sizeof(text), "PSM %u  NAK %u  UART err %u", pDbg->psmState, pDbg->nakCount, pDbg->uartErrors);
    }
    else
    {
        snprintf(text, sizeof(text), "PSM --  NAK --  UART err --");
    }
    GNSSDBG_LEFT_ROW(CLUT_Font020, text);
#undef GNSSDBG_LEFT_ROW

    /* satellites with the highest C/N0 */
    t7_gnssDebugWrite(GNSSDBG_RIGHT_X, 799, GNSSDBG_ROW0_Y, CLUT_Font020, "SV");
    t7_gnssDebugWrite(GNSSDBG_COL_CNO, 799, GNSSDBG_ROW0_Y, CLUT_Font020, "C/N0");
    t7_gnssDebugWrite(GNSSDBG_COL_QUALITY, 799, GNSSDBG_ROW0_Y, CLUT_Font020, "q");
    t7_gnssDebugWrite(GNSSDBG_COL_USED, 799, GNSSDBG_ROW0_Y, CLUT_Font020, "U");
    t7_gnssDebugWrite(GNSSDBG_COL_EPH, 799, GNSSDBG_ROW0_Y, CLUT_Font020, "E");
    t7_gnssDebugWrite(GNSSDBG_COL_ELEV, 799, GNSSDBG_ROW0_Y, CLUT_Font020, "el");
    for(i = 0; i < GNSS_DEBUG_SAT_CNT; i++)
    {
        pSat = &pDbg->sat[i];
        y = GNSSDBG_SAT0_Y + i * GNSSDBG_ROW_STEP;
        if(pSat->svId == 0)
        {
            continue;
        }
        if(pSat->quality > 4)
        {
            color = CLUT_NiceGreen;
        }
        else if(pSat->quality > 2)
        {
            color = CLUT_WarningYellow;
        }
        else
        {
            color = CLUT_Font020;
        }
        snprintf(text, sizeof(text), "%c%u", ((pSat->flags & 0x07u) < sizeof(constellation)) ? constellation[pSat->flags & 0x07u] : '?', pSat->svId);
        t7_gnssDebugWrite(GNSSDBG_RIGHT_X, 799, y, color, text);
        snprintf(text, sizeof(text), "%u", pSat->cno);
        t7_gnssDebugWrite(GNSSDBG_COL_CNO, 799, y, color, text);
        snprintf(text, sizeof(text), "%u", pSat->quality);
        t7_gnssDebugWrite(GNSSDBG_COL_QUALITY, 799, y, color, text);
        t7_gnssDebugWrite(GNSSDBG_COL_USED, 799, y, color, (pSat->flags & 0x08u) ? "U" : "-");
        t7_gnssDebugWrite(GNSSDBG_COL_EPH, 799, y, color, (pSat->flags & 0x10u) ? "E" : "-");
        if(pSat->elev == 255)
        {
            snprintf(text, sizeof(text), "--");
        }
        else
        {
            snprintf(text, sizeof(text), "%u", pSat->elev);
        }
        t7_gnssDebugWrite(GNSSDBG_COL_ELEV, 799, y, color, text);
    }

    /* legend */
    y = GNSSDBG_SAT0_Y + GNSS_DEBUG_SAT_CNT * GNSSDBG_ROW_STEP + 10;
    t7_gnssDebugWrite(GNSSDBG_RIGHT_X, 799, y, CLUT_Font020, "q: 0 none 1 searching 2 acquired");
    t7_gnssDebugWrite(GNSSDBG_RIGHT_X, 799, y + GNSSDBG_ROW_STEP, CLUT_Font020, "3 unusable 4 code lock 5-7 carrier");
    t7_gnssDebugWrite(GNSSDBG_RIGHT_X, 799, y + 2 * GNSSDBG_ROW_STEP, CLUT_Font020, "U = used in the fix, E = ephemeris");
}


/* Debug page 2: the hardware status of the receiver (UBX-MON-HW) and its identification (UBX-MON-VER), see data_exchange.h. One column of rows of font T24
 * every 34 px from the top, the widest row ends before x = 790. The colours flag a problem: red for an antenna short or open circuit, an absent RTC crystal
 * and a critical jamming state, yellow for a jamming warning, a high CW jamming indicator, an AGC close to its limits and an outdated UBX-MON-HW. The
 * thresholds for the AGC (below 10 % and above 90 %) and the CW jamming indicator (100 and more) are judgement calls: the receiver description gives no
 * limits, the CW indicator depends on the environment (it has to be compared with a value measured without interference) */
#define GNSSHW_X				(12u)
#define GNSSHW_X_END			(790u)
#define GNSSHW_AGC_MAX			(8191u)
#define GNSSHW_AGC_LOW_PERCENT	(10u)
#define GNSSHW_AGC_HIGH_PERCENT	(90u)
#define GNSSHW_JAMIND_WARN		(100u)
#define GNSSHW_AGE_WARN_S		(30u)		/* a UBX-MON-HW is polled every 6 s */

static void t7_gnssHwRow(uint16_t y, uint8_t color, const char* text)
{
    t7_gnssDebugWrite(GNSSHW_X, GNSSHW_X_END, y, color, text);
}

/* the field of the identification text is 0 terminated (see GNSS_ParseMonVerData), the limit protects against a wrong text */
static void t7_gnssIdentField(char* pDest, size_t destSize, const SGnssDebugInfo* pDbg, uint8_t offset, uint8_t size)
{
    if(pDbg->identValid && (pDbg->ident[offset] != 0))
    {
        snprintf(pDest, destSize, "%.*s", (int)(size - 1u), &pDbg->ident[offset]);
    }
    else
    {
        snprintf(pDest, destSize, "--");
    }
}

void t7_refresh_gnss_hardware(void)
{
    const SGnssDebugInfo* pDbg = DataEX_getGnssDebugInfo();
    const char* antStatusText[] = {"INIT", "DONTKNOW", "OK", "SHORT", "OPEN"};
    const char* antPowerText[] = {"OFF", "ON", "DONTKNOW"};
    const char* jamStateText[] = {"unknown", "ok", "warning", "critical"};
    char text[112];
    char text1[28];
    char text2[28];
    char text3[28];
    uint8_t color;
    uint8_t jamState = (pDbg->hwFlags >> 2) & 0x03u;
    uint8_t haveA = pDbg->recordsSeen & 0x08u;
    uint8_t haveB = pDbg->recordsSeen & 0x10u;
    uint8_t hwValid = haveA && haveB && pDbg->monHwValid;
    uint32_t agcPercent = ((uint32_t)pDbg->agcCnt * 100u) / GNSSHW_AGC_MAX;
    uint16_t y = GNSSDBG_ROW0_Y;
    uint8_t chunks = 0;

    t7_gnssDebugWrite(GNSSDBG_LEFT_X, GNSSDBG_LEFT_X_END, GNSSDBG_TITLE_Y, CLUT_Font020, "GNSS debug 2/2");
    t7_gnssDebugWrite(GNSSDBG_RIGHT_X, 799, GNSSDBG_TITLE_Y, CLUT_Font020, "ENTER: next view");

    /* identification of the receiver (UBX-MON-VER) */
    if(pDbg->identValid)
    {
        t7_gnssIdentField(text1, sizeof(text1), pDbg, GNSS_IDENT_SW_OFFSET, GNSS_IDENT_SW_SIZE);
        snprintf(text, sizeof(text), "Module: %s", text1);
    }
    else if(pDbg->identChunkMask != 0)
    {
        for(uint8_t i = 0; i < GNSS_IDENT_CHUNKS; i++)
        {
            chunks += (pDbg->identChunkMask >> i) & 1u;
        }
        snprintf(text, sizeof(text), "Module: receiving %u/%u", chunks, (unsigned int)GNSS_IDENT_CHUNKS);
    }
    else if(haveB && (pDbg->monVerRequests > 0))
    {
        snprintf(text, sizeof(text), "Module: not received (%u requests)", pDbg->monVerRequests);
    }
    else
    {
        snprintf(text, sizeof(text), "Module: --");
    }
    t7_gnssHwRow(y, CLUT_Font020, text);
    y += GNSSDBG_ROW_STEP;

    t7_gnssIdentField(text1, sizeof(text1), pDbg, GNSS_IDENT_HW_OFFSET, GNSS_IDENT_HW_SIZE);
    t7_gnssIdentField(text2, sizeof(text2), pDbg, GNSS_IDENT_FW_OFFSET, GNSS_IDENT_FW_SIZE);
    t7_gnssIdentField(text3, sizeof(text3), pDbg, GNSS_IDENT_PROTO_OFFSET, GNSS_IDENT_PROTO_SIZE);
    snprintf(text, sizeof(text), "HW %s   FW %s   Proto %s", text1, text2, text3);
    t7_gnssHwRow(y, CLUT_Font020, text);
    y += GNSSDBG_ROW_STEP;

    t7_gnssIdentField(text1, sizeof(text1), pDbg, GNSS_IDENT_MOD_OFFSET, GNSS_IDENT_MOD_SIZE);
    t7_gnssIdentField(text2, sizeof(text2), pDbg, GNSS_IDENT_GNSS_OFFSET, GNSS_IDENT_GNSS_SIZE);
    snprintf(text, sizeof(text), "Type %s   GNSS %s", text1, text2);
    t7_gnssHwRow(y, CLUT_Font020, text);
    y += GNSSDBG_ROW_STEP + 10u;

    /* hardware status (UBX-MON-HW), only with the payload length of the receiver description */
    if(!hwValid)
    {
        if(haveB && (pDbg->monHwLength != 0))
        {
            snprintf(text, sizeof(text), "MON-HW n/a: payload %u bytes, not %u", pDbg->monHwLength, (unsigned int)GNSS_MONHW_LEN);
            color = CLUT_WarningYellow;
        }
        else
        {
            snprintf(text, sizeof(text), "MON-HW: no answer yet");
            color = CLUT_Font020;
        }
        t7_gnssHwRow(y, color, text);
        y += GNSSDBG_ROW_STEP;
        t7_gnssHwRow(y, CLUT_Font020, "Antenna: n/a   Power: n/a");
        y += GNSSDBG_ROW_STEP;
        t7_gnssHwRow(y, CLUT_Font020, "AGC: n/a");
        y += GNSSDBG_ROW_STEP;
        t7_gnssHwRow(y, CLUT_Font020, "Noise: n/a");
        y += GNSSDBG_ROW_STEP;
        t7_gnssHwRow(y, CLUT_Font020, "Jam CW: n/a   Monitor: n/a");
        y += GNSSDBG_ROW_STEP;
        t7_gnssHwRow(y, CLUT_Font020, "RTC xtal: n/a   calibrated: n/a");
        y += GNSSDBG_ROW_STEP;
    }
    else
    {
        snprintf(text, sizeof(text), "Antenna: %s   Power: %s", (pDbg->aStatus < 5) ? antStatusText[pDbg->aStatus] : "?", (pDbg->aPower < 3) ? antPowerText[pDbg->aPower] : "?");
        t7_gnssHwRow(y, ((pDbg->aStatus == 3) || (pDbg->aStatus == 4)) ? CLUT_WarningRed : CLUT_Font020, text);
        y += GNSSDBG_ROW_STEP;

        snprintf(text, sizeof(text), "AGC: %u of %u (%lu %%)", pDbg->agcCnt, (unsigned int)GNSSHW_AGC_MAX, (unsigned long)agcPercent);
        t7_gnssHwRow(y, ((agcPercent < GNSSHW_AGC_LOW_PERCENT) || (agcPercent > GNSSHW_AGC_HIGH_PERCENT)) ? CLUT_WarningYellow : CLUT_Font020, text);
        y += GNSSDBG_ROW_STEP;

        snprintf(text, sizeof(text), "Noise: %u per ms", pDbg->noisePerMs);
        t7_gnssHwRow(y, CLUT_Font020, text);
        y += GNSSDBG_ROW_STEP;

        snprintf(text, sizeof(text), "Jam CW: %u of 255   Monitor: %s", pDbg->jamInd, jamStateText[jamState]);
        if(jamState == 3)
        {
            color = CLUT_WarningRed;
        }
        else if((jamState == 2) || (pDbg->jamInd >= GNSSHW_JAMIND_WARN))
        {
            color = CLUT_WarningYellow;
        }
        else
        {
            color = CLUT_Font020;
        }
        t7_gnssHwRow(y, color, text);
        y += GNSSDBG_ROW_STEP;

        snprintf(text, sizeof(text), "RTC xtal: %s   calibrated: %s", (pDbg->hwFlags & 0x10u) ? "ABSENT" : "present", (pDbg->hwFlags & 0x01u) ? "YES" : "NO");
        t7_gnssHwRow(y, (pDbg->hwFlags & 0x10u) ? CLUT_WarningRed : CLUT_Font020, text);
        y += GNSSDBG_ROW_STEP;
    }

    if(haveB && (pDbg->monHwAge_s != 255u))
    {
        snprintf(text, sizeof(text), "MON-HW age: %u s", pDbg->monHwAge_s);
        color = (pDbg->monHwAge_s > GNSSHW_AGE_WARN_S) ? CLUT_WarningYellow : CLUT_Font020;
    }
    else
    {
        snprintf(text, sizeof(text), "MON-HW age: --");
        color = haveB ? CLUT_WarningYellow : CLUT_Font020;
    }
    t7_gnssHwRow(y, color, text);
    y += GNSSDBG_ROW_STEP + 10u;

    t7_gnssHwRow(y, CLUT_Font020, "AGC is a percentage of the maximum gain.");
    t7_gnssHwRow(y + GNSSDBG_ROW_STEP, CLUT_Font020, "Monitor unknown: off by default, needs a fix first.");
}

#endif /* HAVE_DEBUG_VIEW */

void t7_SummaryOfLeftCorner(void)
{
    char text[256+60];
    uint8_t textpointer = 0;
    SSettings* pSettings = settingsGetPointer();

    const SDecoinfo * pDecoinfoStandard;
    const SDecoinfo * pDecoinfoFuture;
    float fCNS;

    if(stateUsed->diveSettings.deco_type.ub.standard == GF_MODE)
    {
        pDecoinfoStandard = &stateUsed->decolistBuehlmann;
        pDecoinfoFuture = &stateUsed->decolistFutureBuehlmann;
    }
    else
    {
        pDecoinfoStandard = &stateUsed->decolistVPM;
        pDecoinfoFuture = &stateUsed->decolistFutureVPM;
    }

    fCNS = stateUsed->lifeData .cns;
    if(fCNS > 999)
        fCNS = 999;

    t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
    if(pSettings->FlipDisplay)
    {
    	t7cY0free.WindowY1 = 400;
    }

    t7cY0free.WindowLineSpacing = 48;
    t7cY0free.WindowNumberOfTextLines = 6;
    t7cY0free.WindowTab = 380;

    // header
    textpointer = 0;
    text[textpointer++] = '\032';
    text[textpointer++] = '\016';
    text[textpointer++] = '\016';
    text[textpointer++] = TXT_ppO2;
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = TXT_Ceiling;
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = TXT_ActualGradient;
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = TXT_2BYTE;
    text[textpointer++] = TXT2BYTE_GFSurf;
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = TXT_CNS;
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = TXT_FutureTTS;
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    if (isScrubberTimerEnabled(pSettings)) {
		text[textpointer++] = TXT_2BYTE;
		text[textpointer++] = TXT2BYTE_Scrubber;
    }
    text[textpointer++] = '\017';
	text[textpointer++] = 0;
    if(!pSettings->FlipDisplay)
    {
		t7cY0free.WindowX0 += 10;
		t7cY0free.WindowY0 += 10;
		GFX_write_string(&FontT24, &t7cY0free, text, 1);
		t7cY0free.WindowX0 -= 10;
		t7cY0free.WindowY0 -= 10;
    }
    else
    {
		t7cY0free.WindowY1 -= 10;
		t7cY0free.WindowX1 -= 10;
		GFX_write_string(&FontT24, &t7cY0free, text, 1);
		t7cY0free.WindowY1 += 10;
		t7cY0free.WindowX1 += 10;
    }
    textpointer = 0;
    text[textpointer++] = '\002';
    textpointer += snprintf(&text[textpointer],10,"\020%01.2f",	stateUsed->lifeData.ppO2);
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = '\002';
    if((pDecoinfoStandard->output_ceiling_meter > 99.9f) || (settingsGetPointer()->nonMetricalSystem))
        textpointer += snprintf(&text[textpointer],10,"\020%01.0f",unit_depth_float(pDecoinfoStandard->output_ceiling_meter));
    else
        textpointer += snprintf(&text[textpointer],10,"\020%01.1f",pDecoinfoStandard->output_ceiling_meter);
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = '\002';
    textpointer += snprintf(&text[textpointer],10,"\020%.0f\016\016%%\017",		100 * pDecoinfoStandard->super_saturation);
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = '\002';
    textpointer += snprintf(&text[textpointer],10,"\020%.0f\016\016%%\017",		100 * pDecoinfoStandard->gf_surf);
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = '\002';
    textpointer += snprintf(&text[textpointer],10,"\020%.0f\016\016%%\017",fCNS);
    text[textpointer++] = '\n';
    text[textpointer++] = '\r';
    text[textpointer++] = '\002';
    if (pDecoinfoFuture->output_time_to_surface_seconds < 1000 * 60)
    	textpointer += snprintf(&text[textpointer],10,"\020%i'", (pDecoinfoFuture->output_time_to_surface_seconds + 59) / 60);
    else
    	textpointer += snprintf(&text[textpointer],10,"\020%ih", (pDecoinfoFuture->output_time_to_surface_seconds + 59) / 3600);

    if (isScrubberTimerEnabled(pSettings)) {
        text[textpointer++] = '\n';
        text[textpointer++] = '\r';
        text[textpointer++] = '\002';

        textpointer += printScrubberText(&text[textpointer], 10, stateUsed->scrubberDataDive, pSettings, false);
    }
    text[textpointer++] = 0;
    Gfx_colorsscheme_mod(text, 0);
    GFX_write_string(&FontT42, &t7cY0free, text, 1);
}

void t7_compass(uint16_t ActualHeading, uint16_t UserSetHeading)
{
    static int32_t LastHeading = 0;
    int32_t newHeading = 0;
    int32_t diff = 0;
    int32_t diff2 = 0;

    int32_t diffAbs = 0;
    int32_t diffAbs2 = 0;

    SSettings* pSettings;
    pSettings = settingsGetPointer();

    newHeading = ActualHeading;

    diff = newHeading - LastHeading;

    if(newHeading < LastHeading)
        diff2 = newHeading + 360 - LastHeading;
    else
        diff2 = newHeading - 360 - LastHeading;

    diffAbs = diff;
    if(diffAbs < 0)
        diffAbs *= -1;

    diffAbs2 = diff2;
    if(diffAbs2 < 0)
        diffAbs2 *= -1;


    /* Round the half-step away from zero so a remaining difference of 1
       still advances; plain (diff/2) truncates to 0 and stalls one degree
       short of the target, which knocks held cardinal headings off the
       exact-angle fast path (skewed letters + non-perpendicular ticks). */
    if(diffAbs <= diffAbs2)
        newHeading = LastHeading + (diff  + (diff  > 0 ? 1 : -1)) / 2;
    else
        newHeading = LastHeading + (diff2 + (diff2 > 0 ? 1 : -1)) / 2;

    if(newHeading < 0)
        newHeading += 360;
    else
    if(newHeading >= 360)
        newHeading -= 360;

    LastHeading = newHeading;
    ActualHeading = newHeading;

    /* Bv6 iOS-style fixed rose: ring + ticks + radial labels + red N triangle.
       Optional T-shape course markers when UserSetHeading is set. */
    RoseConfig cfg = t7_rose_cfg;
    cfg.flip_display = (bool)pSettings->FlipDisplay;

    /* Wire user-configurable compass variants from settings. */
    cfg.scale_variant         = pSettings->compassScaleVariant;
    cfg.show_minor_ticks      = pSettings->compassMinorTicks;
    cfg.show_secondary_labels = pSettings->compassSecondaryLabels;
    cfg.lubber_tilt_deg       = compass_mount_phi(pSettings->compassMountTilt);

    /* Nav heading: tilt-corrected heading used for the lubber-color decision
       and course-marker comparison.  The CARD itself (ticks/labels/north via
       compass_draw_*) uses raw ActualHeading so the rose rotates with the
       physical sensor, not the corrected reading. */
    uint16_t nav = (uint16_t)((((int32_t)ActualHeading + compass_mount_phi(pSettings->compassMountTilt))
                                 % 360 + 360) % 360);

    /* Course tolerance from settings (clamped 2..15 in the settings layer). */
    uint8_t tol = pSettings->compassCourseTolerance;

    /* Lubber turns the course-marker color when the heading is within
       tol deg of the forward / back mark (on-course confirmation). */
    cfg.color_lubber = compass_lubber_color(nav, UserSetHeading,
                                            CLUT_Font030,
                                            CLUT_CompassUserHeadingTick,
                                            CLUT_CompassUserBackHeadingTick, tol);

    /* Tick density (the 10deg minor tier) is now governed by the user's
       compassMinorTicks setting via cfg.show_minor_ticks, on both surface and
       dive, rather than an automatic dive-mode reduction. */
    compass_draw_ring          (&t7screen, &cfg);
    compass_draw_ticks         (&t7screen, &cfg, ActualHeading);
    compass_draw_labels        (&t7screen, &cfg, ActualHeading);
    compass_draw_lubber_line   (&t7screen, &cfg);
    /* North is marked by the monochrome "N" cardinal label; the separate
       north triangle is intentionally omitted. */
    if (UserSetHeading != 0u) {
        compass_draw_t_marker(&t7screen, &cfg, ActualHeading,
                              UserSetHeading, tol, CLUT_CompassUserHeadingTick);
        compass_draw_t_marker(&t7screen, &cfg, ActualHeading,
                              (uint16_t)(((uint32_t)UserSetHeading + 180u) % 360u),
                              tol, CLUT_CompassUserBackHeadingTick);
    }

    /* The numeric heading is drawn by the CVIEW_Compass case via the
       established windowed GFX_write_string(&FontT54, &t7cY0free, ...)
       path; t7_compass() only renders the rose graphics. */
}


void t7_miniLiveLogProfile(void)
{
    SWindowGimpStyle wintemp;
	SSettings* pSettings;
	pSettings = settingsGetPointer();

    wintemp.left = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    wintemp.right = wintemp.left + CUSTOMBOX_SPACE_INSIDE;
    if(!pSettings->FlipDisplay)
    {
    	wintemp.top = 480 - t7l1.WindowY0;
    	wintemp.bottom = wintemp. top + 200;
    }
    else
    {
    	wintemp.top = t7l1.WindowY1;
    	wintemp.bottom = wintemp. top + 200;
    }

    uint16_t max_depth = (uint16_t)(stateUsed->lifeData.max_depth_meter * 10);

    GFX_graph_print(&t7screen, &wintemp, 0,1,0, max_depth, getMiniLiveLogbookPointerToData(), getMiniLiveLogbookActualDataLength(), CLUT_Font030, NULL);
}



static uint16_t ChargerLog[60] = {10,10,10,10,10,10,10,10,10,10,
								  10,10,10,10,10,10,10,10,10,10,
								  10,10,10,10,10,10,10,10,10,10,
								  10,10,10,10,10,10,10,10,10,10,
								  10,10,10,10,10,10,10,10,10,10,
								  10,10,10,10,10,10,10,10,10,10};

uint16_t LogDeltaCharge(float charge)
{
	static uint8_t curIndex = 0;
	static float averageSpeed = 0.0;
	uint16_t level = 0;
	uint16_t completeSec = 0;

	if(charge > 0.003)
	{
		level = 2;
	}
	else if(charge > 0.0025)
	{
			level = 3;
	}
	else if(charge > 0.002)
	{
			level = 4;
	}
	else if(charge > 0.0015)
	{
			level = 5;
	}
	else if(charge > 0.001)
	{
			level = 6;
	}
	else if(charge > 0.0005)
	{
			level = 7;
	}
	else if(charge > 0.00)
	{
			level = 8;
	}
	else
	{
		level = 10;
	}
	if(curIndex > 1)
	{
		level = (level + ChargerLog[curIndex - 1]) / 2; /* smooth small jumps */
	}
	if(curIndex < 59)
	{
		ChargerLog[curIndex++] = level;
	}
	else
	{
		memmove(&ChargerLog[0], &ChargerLog[1], sizeof(ChargerLog) - 1);
		ChargerLog[curIndex] = level;
	}
	if(curIndex > 1)	/* estimate time til charging is complete */
	{
		averageSpeed = ((averageSpeed * (curIndex-1)) + charge) / curIndex;
		completeSec = (100.0 - stateUsed->lifeData.battery_charge) / averageSpeed;
	}
	else
	{
		completeSec = 0xffff;
	}
	return completeSec;
}

uint16_t* getChargeLog()
{
	return ChargerLog;
}

void t7_ChargerView(void)
{
	static float lastCharge = 0.0;
	float localCharge = 0.0;
	static uint32_t lastTick = 0;
	uint32_t curTick = 0;
	static float speed = 0.0;
	float deltatime = 0.0;

    char text[256+50];
    uint8_t textpointer = 0;
    static uint16_t remainingSec = 0;
    uint16_t hoursto100 = 0;
    char indicator = '~';

    point_t start, stop;

    SWindowGimpStyle wintemp;
	SSettings* pSettings;
	pSettings = settingsGetPointer();

    t7cY0free.WindowLineSpacing = 28 + 48 + 14;
    t7cY0free.WindowY0 = t7cH.WindowY0 - 5 - 2 * t7cY0free.WindowLineSpacing;
    t7cY0free.WindowNumberOfTextLines = 3;


    if(pSettings->FlipDisplay)
    {
       	t7cY0free.WindowY0 = t7cH.WindowY0 + 15;
        t7cY0free.WindowY1 = t7cY0free.WindowY0 + 250;
    }

    localCharge = stateUsed->lifeData.battery_charge;
    if(localCharge < 0.0)
    {
    	localCharge *= -1.0;
    }

    if(stateUsed->chargeStatus != CHARGER_off)
    {
		curTick = HAL_GetTick();
		deltatime = (curTick - lastTick);

		if((deltatime > 3000) || (lastCharge != localCharge))	/* Charge value update is expected every 2 second. */
		{														/* Added timeout to keep graph moving in case charger is temporary idle */
			if(lastCharge != localCharge)
			{
				if(lastCharge < localCharge)
				{
					speed = (localCharge - lastCharge) * 1000.0 / deltatime;
				}

				if(localCharge > 100.0)
				{
					localCharge = 100.0;
				}
				lastCharge = localCharge;
			}
			deltatime = 0;
			remainingSec = LogDeltaCharge(speed);
			speed = 0;
			lastTick = curTick;
		}
    }
    textpointer += snprintf(&text[textpointer],50,"\n\r");
    textpointer += snprintf(&text[textpointer],50,"\001%c\n\r",TXT_ChargeHour);

    GFX_write_string(&FontT24, &t7cY0free, text, 1);

    hoursto100 = remainingSec / 3600;		/* reduce to hours */
    if(hoursto100 < 1)
    {
    	indicator = '<';
    	hoursto100 = 1;
    }

    if(!pSettings->FlipDisplay)
    {
    	t7cY0free.WindowY0 -= 52;
    }
    else
    {
        t7cY0free.WindowY1 += 52;
    }

    if((stateUsed->lifeData.battery_charge > 0) && (stateUsed->chargeStatus != CHARGER_off))
    {
		snprintf(text,60,
			"\001%0.2f\016\016%%\017\n\r"
			"\001%c%d\n\r"
			,stateUsed->lifeData.battery_charge
			,indicator
			,hoursto100);
    }
    else
    {
		snprintf(text,60,
			"\001---\n\r"
			"\001---\n\r");
    }
    GFX_write_string(&FontT42, &t7cY0free, text, 1);

    wintemp.left = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET + 50;
    wintemp.right = wintemp.left + CUSTOMBOX_SPACE_INSIDE - 100;


    if(!pSettings->FlipDisplay)
    {
    	wintemp.top = 480 - t7l1.WindowY0 + 115;
    	wintemp.bottom = wintemp.top + 100;
    }
    else
    {
    	wintemp.top = t7l1.WindowY1 + 102;
    	wintemp.bottom = wintemp.top + 100;
    }

    start.x =  wintemp.left-5;
    start.y =  90;

    stop.x = wintemp.right + 5 - start.x;
    stop.y = 100;
    GFX_draw_box(&t7screen, start, stop,1, CLUT_Font020);

    if(stateUsed->chargeStatus != CHARGER_off)
    {
    	GFX_graph_print(&t7screen, &wintemp, 1,1,0, 10, getChargeLog(), 60, CLUT_Font030, NULL);
    }
    else
    {
        	GFX_graph_print(&t7screen, &wintemp, 1,1,0, 10, getChargeLog(), 60, CLUT_Font031, NULL);
    }

}


bool t7_isCompassShowing(void)
{
    return selection_customview == CVIEW_Compass || selection_custom_field == LLC_Compass;
}

bool t7_isPositionViewSelected(void)
{
    return selection_customview == CVIEW_Position;
}

void t7_drawAcentGraph(uint8_t color)
{
	point_t start, stop;

	SSettings* pSettings;
	pSettings = settingsGetPointer();


	if(!pSettings->FlipDisplay)
	{
		start.y = t7l1.WindowY0 - 1;
	}
	else
	{
		start.y = t7l3.WindowY0 - 25;
	}

    for(int i = 0; i<4;i++)
    {
        start.y += 5*6;
        stop.y = start.y;
        start.x = CUSTOMBOX_LINE_LEFT - 1;
        stop.x = start.x - 17;
        GFX_draw_line(&t7screen, start, stop, 0);
//			start.x = CUSTOMBOX_LINE_RIGHT + 2; old right too
//			stop.x = start.x + 17;
//			GFX_draw_line(&t7screen, start, stop, 0);
    }
    // new thick bar design Sept. 2015
    start.x = CUSTOMBOX_LINE_LEFT - CUSTOMBOX_OUTSIDE_OFFSET - 3 - 5;
    stop.x = start.x;
	if(!pSettings->FlipDisplay)
	{
		start.y = t7l1.WindowY0 - 1;
	}
	else
	{
		start.y = t7l3.WindowY0 - 25;
	}
    stop.y = start.y + (uint16_t)(stateUsed->lifeData.ascent_rate_meter_per_min * 6);
    stop.y -= 3; // wegen der Liniendicke von 12 anstelle von 9
    if(stop.y >= 470)
        stop.y = 470;
    start.y += 7; // starte etwas weiter oben
    if(color == 0)
    {
    	color = CLUT_EverythingOkayGreen; /* do not use white color for drawing graph */
    }

    GFX_draw_thick_line(12,&t7screen, start, stop, color);
}

#define ASCENT_GRAPH_YPIXEL 110


uint8_t t7_drawSlowExitGraph()  /* this function is only called if diver is below last last stop depth */
{
	static uint16_t countDownSec = 0;
	uint8_t drawingMeterStep;
	static float exitDepthMeter = 0.0;

	uint8_t index = 0;
	uint8_t color = 0;
	point_t start, stop;

	SSettings* pSettings;
	pSettings = settingsGetPointer();


	if(calculateSlowExit(&countDownSec, &exitDepthMeter, &color))	/* graph to be drawn? */
	{
		if(!pSettings->FlipDisplay)
		{
			start.y = t7l1.WindowY0 - 1;
		}
		else
		{
			start.y = t7l3.WindowY0 - 25;
		}
		drawingMeterStep = ASCENT_GRAPH_YPIXEL / pSettings->last_stop_depth_meter;		/* based on 120 / 4 = 30 of standard ascent graph */
		for(index = 0; index < pSettings->last_stop_depth_meter; index++)	/* draw meter indicators */
		{
			start.y += drawingMeterStep;
			stop.y = start.y;
			start.x = CUSTOMBOX_LINE_LEFT - 1;
			stop.x = start.x - 38;
			GFX_draw_line(&t7screen, start, stop, 0);
		}

		start.x = CUSTOMBOX_LINE_LEFT - CUSTOMBOX_OUTSIDE_OFFSET - 20;
		stop.x = start.x;
		if(!pSettings->FlipDisplay)
		{
			start.y = t7l1.WindowY0 + ASCENT_GRAPH_YPIXEL + 5;
		}
		else
		{
			start.y = t7l3.WindowY0 - 25 + ASCENT_GRAPH_YPIXEL + 5;
		}
		stop.y = start.y - countDownSec * (ASCENT_GRAPH_YPIXEL / (float)(pSettings->slowExitTime * 60.0));
		if(stop.y >= 470) stop.y = 470;
		if(!pSettings->FlipDisplay)
		{
			stop.y += 5;
		}
		GFX_draw_thick_line(15,&t7screen, start, stop, 3);
		/* mark diver depth */
		start.x = CUSTOMBOX_LINE_LEFT - CUSTOMBOX_OUTSIDE_OFFSET - 30;
		stop.x = start.x + 24;

		start.y = start.y - (stateUsed->lifeData.depth_meter * (ASCENT_GRAPH_YPIXEL) / pSettings->last_stop_depth_meter);
		stop.y = start.y;
		GFX_draw_thick_line(10,&t7screen, start, stop, 9);
	}
	else
	{
		color = 0xff;
	}
	return color;
}


void t7_tick(void)
{
    SSettings *settings = settingsGetPointer();

    int nowS = current_second();
    updateTimer(settings, nowS, false);
}
