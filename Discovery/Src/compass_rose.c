///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/compass_rose.c
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

#include "compass_rose.h"

/* Q1.15 sin LUT for 0..90 degrees. 91 entries.
   Value = round(sin(deg * PI / 180) * 32767). */
static const int16_t sin_q15_table[91] = {
        0,   572,  1144,  1715,  2286,  2856,  3425,  3993,  4560,  5126,
     5690,  6252,  6813,  7371,  7927,  8481,  9032,  9580, 10126, 10668,
    11207, 11743, 12275, 12803, 13328, 13848, 14365, 14876, 15384, 15886,
    16384, 16877, 17364, 17847, 18324, 18795, 19261, 19720, 20174, 20622,
    21063, 21498, 21926, 22348, 22763, 23170, 23571, 23965, 24351, 24730,
    25102, 25466, 25822, 26170, 26510, 26842, 27166, 27482, 27789, 28088,
    28378, 28660, 28932, 29197, 29452, 29698, 29935, 30163, 30382, 30592,
    30792, 30983, 31164, 31336, 31499, 31651, 31795, 31928, 32052, 32166,
    32270, 32365, 32449, 32523, 32588, 32643, 32688, 32723, 32748, 32762,
    32767
};

/* Mount-tilt offset in degrees. Left-hand mount tilts the lubber +30 (clockwise),
   right-hand -30 (counter-clockwise), matching how the canted device sits on each wrist. */
int16_t compass_mount_phi(uint8_t mountTilt)
{
    return (mountTilt == 1u) ? 30 : (mountTilt == 2u) ? -30 : 0;
}

int16_t compass_sin_q15(uint16_t deg)
{
    deg %= 360;
    if (deg <= 90)  return  sin_q15_table[deg];
    if (deg <= 180) return  sin_q15_table[180 - deg];
    if (deg <= 270) return -sin_q15_table[deg - 180];
    return                 -sin_q15_table[360 - deg];
}

static int16_t compass_cos_q15(uint16_t deg)
{
    return compass_sin_q15((deg + 90) % 360);
}

point_t compass_point(point_t center, uint16_t radius, uint16_t deg)
{
    /* OSTC4 firmware framebuffer convention is y-UP (positive y = up on the
       physical display). This is the net result of the column-major LTDC
       layout + ROTATE_90 at scan-out + 180-deg mounted display. Existing t7
       draws confirm: WindowY0=434 sits near the TOP of the customview box
       on the landscape PNG. To put deg=0 (compass North) at the TOP of the
       rose, we therefore ADD r*cos(deg) to center.y.
       x = center.x + r * sin(deg)   (East at positive x is unaffected)
       y = center.y + r * cos(deg)   (North at +y, South at -y) */
    int32_t s = compass_sin_q15(deg);
    int32_t c = compass_cos_q15(deg);
    int32_t sx = s * (int32_t)radius;
    int32_t sy = c * (int32_t)radius;
    point_t out;
    /* Round to nearest, symmetric about zero. The plain `>> 15` arithmetic
       shift floors toward negative infinity, so equal +/- offsets rounded
       unequally (e.g. +3.42 -> +3 but -3.42 -> -4), skewing symmetric features
       like the bearing-marker cap by ~1px at 0/180deg. */
    out.x = (int16_t)(center.x + (sx >= 0 ? (sx + 16384) >> 15 : -((-sx + 16384) >> 15)));
    out.y = (int16_t)(center.y + (sy >= 0 ? (sy + 16384) >> 15 : -((-sy + 16384) >> 15)));
    return out;
}

uint8_t compass_lubber_color(uint16_t heading, uint16_t userSetHeading,
                             uint8_t color_default, uint8_t color_fwd,
                             uint8_t color_back, uint8_t tol_deg)
{
    /* 0 is the sentinel for "no course set" (matches the T-marker draw guard),
       so North-as-course is not distinguishable - by design. */
    if (userSetHeading == 0u)
        return color_default;

    /* Signed smallest-angle declination in (-180, 180]. The +540 bias keeps the
       intermediate positive before the modulo so the result is well-defined for
       any 0..359 inputs, including across the 0/360 seam. */
    int16_t declFwd = (int16_t)((((int32_t)userSetHeading - heading + 540) % 360) - 180);
    if (declFwd < 0) declFwd = (int16_t)(-declFwd);
    if (declFwd <= (int16_t)tol_deg)
        return color_fwd;

    uint16_t back = (uint16_t)((userSetHeading + 180u) % 360u);
    int16_t declBack = (int16_t)((((int32_t)back - heading + 540) % 360) - 180);
    if (declBack < 0) declBack = (int16_t)(-declBack);
    if (declBack <= (int16_t)tol_deg)
        return color_back;

    return color_default;
}

#include "gfx_engine.h"
#include "settings.h"
#include "text_multilanguage.h"

/* No per-element flip compensation for the rose. Under FlipDisplay the GFX layer
   reverse-writes every primitive (180 deg framebuffer rotation) and
   data_exchange_main pre-rotates the heading by -180 deg
   (data_exchange_main.c:1126); together they place the card (North, ticks,
   labels, markers) and the lubber correctly. The earlier (360-display) form was
   a REFLECTION (mirror): it left North/South/lubber (on the mirror axis) looking
   right but swapped E/W, so the card read backwards. Verified on clean
   boot-flipped sim renders (FlipDisplay=1 in stored settings) at body-N and
   body-E (Phase 6, 2026-06-16). flip_display is retained in RoseConfig in case a
   future variant needs per-element handling. */
static uint16_t compass_flip_angle(const RoseConfig *cfg, uint16_t display)
{
    (void)cfg;
    return display;
}

static const char *compass_cardinal_for(uint16_t deg)
{
    /* Cardinal glyphs are language-independent except German East ("O" = Ost),
       so they are inlined here rather than consuming four scarce TXT2BYTE tokens.
       The OSTC 2-byte text space is one byte wide (see text_multilanguage.h); the
       compass menu strings alone would push it past the 255 ceiling. */
    switch (deg) {
        case   0u: return "N";
        case  90u: return (settingsGetPointer()->selected_language == LANGUAGE_German) ? "O" : "E";
        case 180u: return "S";
        case 270u: return "W";
        default:   return "?";
    }
}

/* Minimal unsigned->decimal for the 0..359 label range; avoids snprintf
   in the per-redraw compass path. Writes at most 3 digits + NUL. */
static void compass_u16_to_str(uint16_t v, char *out)
{
    char tmp[4];
    uint8_t n = 0;
    if (v == 0) { out[0] = '0'; out[1] = 0; return; }
    while (v && n < 3) { tmp[n++] = (char)('0' + (v % 10u)); v /= 10u; }
    uint8_t i = 0;
    while (n) out[i++] = tmp[--n];
    out[i] = 0;
}

void compass_draw_ring(GFX_DrawCfgScreen *s, const RoseConfig *cfg)
{
    /* Ring stroke weight: ring_thickness concentric opaque circles from r_out
     * inward, plus Wu-AA feathering one pixel outside the outer edge and one
     * pixel inside the inner edge.  Treat ring_thickness == 0 as 2 so existing
     * callers that leave the field at its C default (0) continue to produce the
     * historical 2-pixel ring without any change. */
    uint8_t t = (cfg->ring_thickness != 0u) ? cfg->ring_thickness : 2u;

    GFX_draw_circle_aa(s, cfg->center, (uint8_t)(cfg->r_out + 1u), cfg->color_ring); /* outer feather */
    for (uint8_t i = 0u; i < t; i++) {
        GFX_draw_circle(s, cfg->center, (uint8_t)(cfg->r_out - i), (int8_t)cfg->color_ring);
    }
    if (cfg->r_out >= t) {
        GFX_draw_circle_aa(s, cfg->center, (uint8_t)(cfg->r_out - t), cfg->color_ring); /* inner feather */
    }
}

/* Tier codes for compass_draw_one_tick(). */
enum { TICK_MINOR = 0, TICK_MID = 1, TICK_MAJOR = 2, TICK_CARDINAL = 3 };

/* Draw one tick at display angle `display` (already flip-compensated and
   heading-relative). `tier` selects per-tier radii/thickness/colour from cfg,
   preserving Medium behaviour exactly. */
static void compass_draw_one_tick(GFX_DrawCfgScreen *s, const RoseConfig *cfg,
                                  uint16_t display, int tier)
{
    uint16_t tip_r;
    uint8_t  thick, color;
    switch (tier) {
        case TICK_CARDINAL:
            tip_r = cfg->r_tick_cardinal_tip;
            thick = cfg->tick_thickness_cardinal;
            color = cfg->color_tick_major;
            break;
        case TICK_MAJOR:
            tip_r = cfg->r_tick_major_tip;
            thick = cfg->tick_thickness_major;
            color = cfg->color_tick_major;
            break;
        case TICK_MID:
            tip_r = cfg->r_tick_mid_tip;
            thick = cfg->tick_thickness_mid;
            color = cfg->color_tick_mid;
            break;
        default: /* TICK_MINOR */
            tip_r = cfg->r_tick_minor_tip;
            thick = cfg->tick_thickness_minor;
            color = cfg->color_tick_minor;
            break;
    }
    point_t p0 = compass_point(cfg->center, cfg->r_ring_inner, display);
    point_t p1 = compass_point(cfg->center, tip_r,             display);
    GFX_draw_thick_line_aa(thick, s, p0, p1, color);

    if ((tier == TICK_CARDINAL) && cfg->cardinal_pencil) {
        /* Pencil tip: a filled triangle at the inner end of the cardinal
           index, apex pointing further inward (toward the centre), so the
           whole index reads as a pencil/needle. */
        point_t apex = compass_point(cfg->center, (uint16_t)(tip_r - 9u), display);
        point_t b1   = compass_point(cfg->center, tip_r, (display + 357u) % 360u);
        point_t b2   = compass_point(cfg->center, tip_r, (display +   3u) % 360u);
        GFX_fill_triangle(s, apex, b1, b2, color);
    }
}

void compass_draw_ticks(GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading)
{
    if (cfg->scale_variant == 1u) {
        /* 8-point variant: 16 positions at half-integer 22.5-deg spacing,
           rounded to integer degrees. deg[i] = round(i * 22.5) via
           (i*45+1)/2 integer arithmetic. */
        for (int i = 0; i < 16; i++) {
            uint16_t deg = (uint16_t)((((uint16_t)i * 45u) + 1u) / 2u);
            uint16_t display = compass_flip_angle(cfg, (360u + deg - heading) % 360u);
            int tier;
            if (i % 4 == 0) {
                tier = TICK_CARDINAL;            /* 0, 90, 180, 270 */
            } else if (i % 2 == 0) {
                tier = TICK_MAJOR;               /* 45, 135, 225, 315 */
            } else {
                if (!cfg->show_minor_ticks) continue; /* ~22.5-deg minor ticks gated */
                tier = TICK_MINOR;
            }
            compass_draw_one_tick(s, cfg, display, tier);
        }
    } else {
        /* Medium variant: step loop; cardinal at %90, major at %30 only when a
           finer tick tier exists below 30deg (i.e. tick_step_deg < 30), mid at
           %10, minor otherwise (gated on show_minor_ticks).
           When tick_step_deg == 30 (t3 uncluttered dial), the 30-degree ticks
           are the finest tier present, so they fall through to TICK_MINOR and
           are gated by show_minor_ticks just like 10-degree ticks are in t7. */
        for (uint16_t deg = 0; deg < 360; deg += cfg->tick_step_deg) {
            uint16_t display = compass_flip_angle(cfg, (360u + deg - heading) % 360u);
            int tier;
            if (deg % 90 == 0) {
                tier = TICK_CARDINAL;
            } else if (deg % 30 == 0 && cfg->tick_step_deg < 30u) {
                tier = TICK_MAJOR;
            } else if (deg % 10 == 0 && cfg->tick_step_deg < 10u) {
                tier = TICK_MID;
            } else {
                if (!cfg->show_minor_ticks) continue;
                tier = TICK_MINOR;
            }
            compass_draw_one_tick(s, cfg, display, tier);
        }
    }
}

void compass_draw_north_triangle(GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading)
{
    uint16_t disp = compass_flip_angle(cfg, (360u - heading) % 360u);

    /* Triangle sits INSIDE the ring with its tip pointing outward toward
       North: apex at r_tri_apex (just inside the ring), base two points
       further in. */
    uint16_t r_base = (cfg->r_tri_apex > 18u) ? (uint16_t)(cfg->r_tri_apex - 18u) : 0u;
    point_t apex = compass_point(cfg->center, cfg->r_tri_apex, disp);
    point_t b1   = compass_point(cfg->center, r_base, (disp + 353u) % 360u);
    point_t b2   = compass_point(cfg->center, r_base, (disp +   7u) % 360u);
    GFX_fill_triangle(s, apex, b1, b2, cfg->color_north_tri);
}

/* Filled disc: concentric outline circles from 0..radius, matching the
   ring-stroke idiom (no GFX_fill_circle primitive exists). Used to give the
   lubber/needle tail a rounded cap so it reads clean tilted and untilted. */
static void compass_fill_disc(GFX_DrawCfgScreen *s, point_t c, uint8_t radius, uint8_t color)
{
    for (uint8_t r = 0u; r <= radius; r++) {
        GFX_draw_circle(s, c, r, (int8_t)color);
    }
}

void compass_draw_lubber_line(GFX_DrawCfgScreen *s, const RoseConfig *cfg)
{
    /* The lubber line is the fixed index mark at the dial reference position.
       cfg->lubber_tilt_deg offsets it from 12 o'clock for tilted-mount variants
       (e.g. +30 or -30 deg); 0 preserves the historical straight-up position.
       Route through compass_flip_angle so a flipped display mirrors correctly
       (historical bug: the lubber previously ignored flip_display). */
    uint16_t a_out = compass_flip_angle(cfg,
                         (uint16_t)(((int32_t)360 + cfg->lubber_tilt_deg) % 360));
    point_t p_out = compass_point(cfg->center, cfg->r_lubber_out, a_out);
    point_t p_in  = (cfg->r_lubber_tail > 0u)
                      ? compass_point(cfg->center, cfg->r_lubber_tail,
                                      (uint16_t)((a_out + 180u) % 360u))
                      : compass_point(cfg->center, cfg->r_lubber_in, a_out);
    GFX_draw_thick_line(cfg->lubber_thickness, s, p_out, p_in, cfg->color_lubber);

    /* Rounded tail cap. The tail is the inner end (t7: r_lubber_in, stopped
       short of the central heading readout; t3: the r_lubber_tail arm past
       centre). A round cap of half the line weight hides the square line end,
       which otherwise looks ragged once the lubber is tilted. */
    compass_fill_disc(s, p_in, (uint8_t)(cfg->lubber_thickness / 2u), cfg->color_lubber);

    if (cfg->lubber_pointer) {
        /* Filled arrowhead at the outer tip pointing further outward, so the
           fixed index reads as a pointer. head_deg is the arrowhead half-width
           in degrees (0 => the historical 5-deg t7 width); t3 widens it for a
           bolder needle that still clears the ticks/markers via r_lubber_out. */
        uint16_t head_deg = (cfg->lubber_head_deg != 0u) ? cfg->lubber_head_deg : 5u;
        point_t apex = compass_point(cfg->center, (uint16_t)(cfg->r_lubber_out + 9u), a_out);
        point_t b1   = compass_point(cfg->center, cfg->r_lubber_out, (uint16_t)((a_out + 360u - head_deg) % 360u));
        point_t b2   = compass_point(cfg->center, cfg->r_lubber_out, (uint16_t)((a_out + head_deg) % 360u));
        GFX_fill_triangle(s, apex, b1, b2, cfg->color_lubber);
    }
}

void compass_draw_t_marker(GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading,
                           uint16_t course, uint8_t tol_deg, uint8_t color)
{
    uint16_t disp = compass_flip_angle(cfg, (360u + course - heading) % 360u);

    /* Bearing marker: an ARC cap concentric with the ring whose angular
       half-width equals the course tolerance (so the cap draws the on-course
       window), plus a longer radial stem. Both 5px thick. The cap is a short
       sequence of thick line segments stepped 1deg along the ring. */
    point_t prev = compass_point(cfg->center, cfg->r_out,
                                 (uint16_t)(((int32_t)disp + 360 - (int16_t)tol_deg) % 360));
    for (int16_t a = -(int16_t)tol_deg + 1; a <= (int16_t)tol_deg; a++) {
        point_t p = compass_point(cfg->center, cfg->r_out,
                                  (uint16_t)(((int32_t)disp + 360 + a) % 360));
        GFX_draw_thick_line(5, s, prev, p, color);
        prev = p;
    }

    point_t st0 = compass_point(cfg->center, cfg->r_out,        disp);
    point_t st1 = compass_point(cfg->center, cfg->r_out - 24u,  disp);
    GFX_draw_thick_line(5, s, st0, st1, color);
}

/* Intercardinal label strings for the 8-point variant secondary labels.
   Not localized: these abbreviations are universal in diving practice. */
static const struct { uint16_t deg; const char *label; } intercardinal_labels[4] = {
    {  45u, "NE" },
    { 135u, "SE" },
    { 225u, "SW" },
    { 315u, "NW" },
};

void compass_draw_labels(GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading)
{
    /* Both variants always draw N/E/S/W cardinals. */
    for (uint16_t deg = 0; deg < 360; deg += 90u) {
        uint16_t display = compass_flip_angle(cfg, (360u + deg - heading) % 360u);
        /* Both cardinals and numbers are rotated radially so the top of
           the glyphs points OUTWARD (away from the dial centre). The y-UP
           compass_point() flip reflects label positions across the
           horizontal axis, which negates the glyph-rotation sign, so the
           outward rotation is (360 - display). The N/E/S/W letters are
           rotationally symmetric, so the sign only shows on asymmetric
           numerals. */
        uint16_t label_rot = (360u - display) % 360u;
        point_t pt = compass_point(cfg->center, cfg->r_label_card, display);
        uint8_t color = (deg == 0u) ? cfg->color_label_card_north : cfg->color_label_card;
        const char *text = compass_cardinal_for(deg);
        GFX_write_string_rotated(cfg->font_card, s, text, pt.x, pt.y, label_rot, color);
    }

    if (cfg->scale_variant == 1u) {
        /* 8-point variant: secondary labels are intercardinal letters at
           45/135/225/315, drawn only when show_secondary_labels is set. */
        if (cfg->show_secondary_labels) {
            for (uint8_t i = 0u; i < 4u; i++) {
                uint16_t deg     = intercardinal_labels[i].deg;
                uint16_t display = compass_flip_angle(cfg, (360u + deg - heading) % 360u);
                uint16_t label_rot = (360u - display) % 360u;
                point_t pt = compass_point(cfg->center, cfg->r_label_num, display);
                GFX_write_string_rotated(cfg->font_num, s,
                                         intercardinal_labels[i].label,
                                         pt.x, pt.y, label_rot, cfg->color_label_num);
            }
        }
    } else {
        /* Medium variant: 30-degree numeric labels, gated on show_secondary_labels. */
        if (cfg->show_secondary_labels) {
            for (uint16_t deg = 30u; deg < 360u; deg += 30u) {
                if (deg % 90u == 0u) continue; /* cardinals already drawn above */
                uint16_t display = compass_flip_angle(cfg, (360u + deg - heading) % 360u);
                uint16_t label_rot = (360u - display) % 360u;
                char buf[4];
                compass_u16_to_str(deg, buf);
                point_t pt = compass_point(cfg->center, cfg->r_label_num, display);
                GFX_write_string_rotated(cfg->font_num, s, buf,
                                         pt.x, pt.y, label_rot, cfg->color_label_num);
            }
        }
    }
}

void compass_draw_cone_marker(GFX_DrawCfgScreen *s, const RoseConfig *cfg, uint16_t heading,
                              uint16_t course, uint8_t tol_deg, uint8_t color, bool points_out)
{
    /* Filled-triangle fan forming a cone on the ring edge. ONE primitive is
       used for both directions so green and red are congruent (identical size):
       the base shape has its arc base on the ring (r_out) and its tip inward
       (r_out - H) -- that is the back/red marker. The forward/green marker is
       the SAME shape point-reflected through the cone's radial midpoint
       (r_out - H/2), which lands the tip exactly on the ring while preserving
       the arc length. A naive r_tip/r_base swap would instead shrink the green
       base (inner radius => shorter arc), so we reflect rather than swap. */
    const uint16_t H = 28u;
    uint16_t disp = compass_flip_angle(cfg, (uint16_t)((360u + course - heading) % 360u));
    point_t pivot = compass_point(cfg->center, (uint16_t)(cfg->r_out - H / 2u), disp);
    point_t apex  = compass_point(cfg->center, (uint16_t)(cfg->r_out - H), disp);
    if (points_out) { apex.x = (int16_t)(2 * pivot.x - apex.x); apex.y = (int16_t)(2 * pivot.y - apex.y); }
    for (int16_t a = -(int16_t)tol_deg; a < (int16_t)tol_deg; a++) {
        point_t e0 = compass_point(cfg->center, cfg->r_out,
                                   (uint16_t)(((int32_t)disp + 360 + a) % 360));
        point_t e1 = compass_point(cfg->center, cfg->r_out,
                                   (uint16_t)(((int32_t)disp + 361 + a) % 360));
        if (points_out) {
            e0.x = (int16_t)(2 * pivot.x - e0.x); e0.y = (int16_t)(2 * pivot.y - e0.y);
            e1.x = (int16_t)(2 * pivot.x - e1.x); e1.y = (int16_t)(2 * pivot.y - e1.y);
        }
        GFX_fill_triangle(s, apex, e0, e1, color);
    }
}
