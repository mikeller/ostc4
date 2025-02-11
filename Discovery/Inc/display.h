///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/display.h
/// \brief
/// \author Heinrichs Weikamp
/// \date   2018
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

#ifndef DISPLAY_H
#define DISPLAY_H

void display_power_on__1_of_2__pre_RGB(void);
void display_power_on__2_of_2__post_RGB(void);
void display_power_off(void);
void display_1_brightness_max(void);
void display_1_brightness_high(void);
void display_1_brightness_std(void);
void display_1_brightness_eco(void);
void display_1_brightness_cave(void);

void display_sleep(void);
void display_sleep_release(void);

#endif /* DISPLAY_H */

