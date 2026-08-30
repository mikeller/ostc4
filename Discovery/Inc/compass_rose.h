///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/compass_rose.h
/// \brief  Stateless drawing primitives for the iOS-style compass rose.
/// \author heinrichs weikamp gmbh
/// \date   11-June-2026
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2026 Heinrichs Weikamp gmbh
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

#ifndef COMPASS_ROSE_H
#define COMPASS_ROSE_H

#include <stdint.h>
#include <stdbool.h>
#include "gfx_engine.h"     /* GFX_DrawCfgScreen, tFont, point_t */

/* Mount-tilt offset in degrees: 0 = none, 1 = left-hand (-30), 2 = right-hand (+30). */
int16_t compass_mount_phi(uint8_t mountTilt);

/* Returns sin(deg) in Q1.15 fixed-point. deg may be any value; reduced mod 360 internally. */
int16_t compass_sin_q15(uint16_t deg);

/* Returns the point on a circle of given radius at compass-degree `deg`
   (0 = top / 12 o'clock, increases clockwise). Output uses the OSTC4
   firmware framebuffer convention: y-UP (positive y is physically up on
   the rotated landscape display). At deg=0 the point is
   (center.x, center.y + radius). */
point_t compass_point(point_t center, uint16_t radius, uint16_t deg);

/* Returns the lubber-line CLUT index for the current alignment to the set
   course. userSetHeading == 0 means "no course set" -> color_default. Heading
   within tol_deg of the forward mark -> color_fwd; within tol_deg of the back
   mark (course + 180) -> color_back; otherwise color_default. The two marks are
   180deg apart, so for any tol_deg < 90 at most one band matches; the forward
   mark is tested first. Pure; no gfx dependency. */
uint8_t compass_lubber_color(uint16_t heading, uint16_t userSetHeading,
                             uint8_t color_default, uint8_t color_fwd,
                             uint8_t color_back, uint8_t tol_deg);

typedef struct {
    point_t  center;
    uint16_t r_out;
    uint16_t r_ring_inner;       /* tick base = r_out - 2 */
    uint16_t r_tick_minor_tip;
    uint16_t r_tick_mid_tip;
    uint16_t r_tick_major_tip;
    uint16_t r_tick_cardinal_tip; /* N/E/S/W ticks (deg % 90); longest */
    uint16_t r_label_num;        /* numeric labels outside ring */
    uint16_t r_label_card;       /* cardinal letters outside ring */
    uint16_t r_tri_apex;         /* north-triangle apex (outside ring) */
    uint8_t  tick_step_deg;      /* 5 for t7, 10 for t3 */
    uint8_t  tick_thickness_minor;
    uint8_t  tick_thickness_mid;
    uint8_t  tick_thickness_major;
    uint8_t  tick_thickness_cardinal;
    uint8_t  cardinal_pencil;     /* 1 = add an inward-pointing triangle tip to N/E/S/W ticks (pencil shape) */
    uint8_t  color_tick_minor;
    uint8_t  color_tick_mid;
    uint8_t  color_tick_major;
    uint8_t  color_ring;
    uint8_t  color_label_num;
    uint8_t  color_label_card;
    uint8_t  color_label_card_north;
    uint8_t  color_north_tri;
    uint16_t r_lubber_out;       /* fixed top index line: outer radius */
    uint16_t r_lubber_in;        /* fixed top index line: inner radius */
    uint8_t  color_lubber;
    uint8_t  lubber_thickness;
    uint8_t  lubber_pointer;     /* 1 = filled triangle cap at the lubber tip, pointing outward (12 o'clock) */
    bool     flip_display;
    const tFont   *font_num;
    const tFont   *font_card;
    /* compass redesign */
    uint8_t  scale_variant;    /* 0 = Medium (step/major/cardinal), 1 = 8-point (22.5/45/90) */
    uint8_t  show_minor_ticks; /* 1 = draw the finest tier */
    uint8_t  ring_thickness;   /* ring stroke weight (was hard-coded 2) */
    int16_t  lubber_tilt_deg;  /* mount tilt phi applied to the lubber/needle (0 / +30 / -30) */
    uint8_t  marker_style;     /* 0 = T/arc-cap (t7), 1 = filled cone (t3) */
    uint16_t r_lubber_tail;    /* needle tail length below centre (t3); 0 = none (t7) */
    uint8_t  show_secondary_labels; /* 1 = draw degree numbers (Medium) / intercardinal letters (8-point) */
    uint8_t  lubber_head_deg;  /* arrowhead half-width in degrees; 0 => 5 (historical t7 width) */
} RoseConfig;

void compass_draw_ring          (GFX_DrawCfgScreen *s, const RoseConfig *cfg);
void compass_draw_ticks         (GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading);
void compass_draw_labels        (GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading);
void compass_draw_north_triangle(GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading);
void compass_draw_lubber_line   (GFX_DrawCfgScreen *s, const RoseConfig *cfg);
void compass_draw_t_marker      (GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading,
                                 uint16_t course, uint8_t tol_deg, uint8_t color);
void compass_draw_cone_marker   (GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading,
                                 uint16_t course, uint8_t tol_deg, uint8_t color, bool points_out);

#endif /* COMPASS_ROSE_H */
