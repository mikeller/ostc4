///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Common/Inc/gnss_accuracy.h
/// \brief  Accuracy information which is stored together with a GNSS position
/// \author Heinrichs Weikamp
///
/// \par Copyright (c) 2014-2025 Heinrichs Weikamp gmbh
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

#ifndef GNSS_ACCURACY_H
#define GNSS_ACCURACY_H

#include <stdint.h>

/* The accuracy data is sent from the RTE to the main CPU and written to the logbook as three bytes (same layout) */
#define GNSS_ACC_UNKNOWN		(0xFFu)		/* value of every field if the information is not available */
#define GNSS_ACC_HACC_MAX		(250u)		/* horizontal accuracy: 1..250 m, 250 stands for "250 m or worse" */
#define GNSS_ACC_NUMSAT_MAX		(254u)		/* satellites used: 0..254, 254 stands for "254 or more" */
#define GNSS_ACC_AGE_MAX		(254u)		/* age: 0..254 min, 254 stands for "254 min or older" */

typedef struct
{
	uint8_t hAcc;			/* horizontal accuracy estimate (hAcc of the receiver) of the position in m, rounded up */
	uint8_t numSatUsed;		/* number of satellites used to calculate the position */
	uint8_t posAge;			/* time between the reception of the position and the start of the dive in min */
} SGnssAccuracy;

/* Convert the horizontal accuracy estimate of the receiver (mm) to m. 0 and 0xFFFFFFFF are not valid values */
static inline uint8_t GNSS_EncodeHAcc(uint32_t hAcc_mm)
{
	uint32_t hAcc_m;

	if((hAcc_mm == 0) || (hAcc_mm == 0xFFFFFFFFu))
	{
		return GNSS_ACC_UNKNOWN;
	}
	hAcc_m = hAcc_mm / 1000u;
	if((hAcc_mm % 1000u) != 0)
	{
		hAcc_m++;
	}
	if(hAcc_m > GNSS_ACC_HACC_MAX)
	{
		hAcc_m = GNSS_ACC_HACC_MAX;
	}
	return (uint8_t)hAcc_m;
}

/* Convert the number of satellites used by the receiver. 0xFF is not a valid value */
static inline uint8_t GNSS_EncodeNumSat(uint8_t numSat)
{
	if(numSat == 0xFFu)
	{
		return GNSS_ACC_UNKNOWN;
	}
	if(numSat > GNSS_ACC_NUMSAT_MAX)
	{
		return GNSS_ACC_NUMSAT_MAX;
	}
	return numSat;
}

/* Convert an age in seconds to minutes (rounded down) */
static inline uint8_t GNSS_EncodeAge(uint32_t age_s)
{
	uint32_t age_min = age_s / 60u;

	if(age_min > GNSS_ACC_AGE_MAX)
	{
		age_min = GNSS_ACC_AGE_MAX;
	}
	return (uint8_t)age_min;
}

#endif /* GNSS_ACCURACY_H */
