///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/configuration.h
/// \brief  Header file for variant specific firmware adaptations at compile time
/// \author heinrichs weikamp gmbh
/// \date   29-February-2020
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2020 Heinrichs Weikamp gmbh
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

#ifndef CONFIGURATION_HEADER
#define CONFIGURATION_HEADER

/* Enable this to make the simulator write a logbook entry */
/* #define SIM_WRITES_LOGBOOK 1 */

/* Enable this for support of optical bottle pressure interface */
/* #define ENABLE_BOTTLE_SENSOR */

/* Enable this to show voltage in parallel to charge state */
#define ALWAYS_SHOW_VOLTAGE

/* Enable this to skip coplete scan of dive log during startup */
#define TRUST_LOG_CONSISTENCY

/* Enable this to transfer additional data list last dive ID and last sample index during raw data requests */
/* define SEND_DATA_DETAILS */

/* Enable to activate a menu item in reset menu which provide sample ring analysis / repair functionality */
/* #define ENABLE_ANALYSE_SAMPLES */

/* Enable to have access to the debug view options (turn on / off via menu instead of compile switch) */
/* #define HAVE_DEBUG_VIEW */

/* Enable to have runtime information displayed in t7 debug view */
/* #define T7_DEBUG_RUNTIME */

/* Enable to have event based warnings being displayed as warning messages when they occur */
/* #define HAVE_DEBUG_WARNINGS */

/* Enable to have access to the motion control selection menu */
/* #define ENABLE_MOTION_CONTROL */

/* Enable to have option to hide not needed gases from dive views */
/* #define ENABLE_UNUSED_GAS_HIDING */

/* Enable to have the new T3 profile view available */
#define ENABLE_T3_PROFILE_VIEW

/* Enable to have PPO2 adjustments in T3 sensor view during dive simulation */
/* #define ENABLE_T3_PPO_SIM */

/* Enable to have PSCR functionality available */
#define ENABLE_PSCR_MODE

/* Enable to have CO2 sensor functionality available */
#define ENABLE_CO2_SUPPORT

/* Enable to have GPS sensor functionality available */
/* #define ENABLE_GNSS_SUPPORT */

/* Enable to have Sentinel rebreather interface available */
/* #define ENABLE_SENTINEL_MODE */

/* Enable if you are using sensors with a voltage range 8..16 mV at surface / air level */
#define ENABLE_ALTERNATIVE_SENSORTYP

/* Enable if an external pressure sensor is connected at ADC channel3 (used for sensor verification) */
/* #define ENABLE_EXTERNAL_PRESSURE */

/* Enable if the menu item predive check shall be available */
/* #define ENABLE_PREDIVE_CHECK */

/* Enable to have a faster transfer speed between bluetooth module and CPU */
#define ENABLE_FAST_COMM

/* Enable to have position sensor support active */
/* #define ENABLE_GPIO_V2 */

/* Enable RTE sleep mode debugging */
/* #define ENABLE_SLEEP_DEBUG */


#endif
