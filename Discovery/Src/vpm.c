///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/vpm.c
/// \brief  critical_volume comment by hw
/// \author Heinrichs Weikamp, Erik C. Baker
/// \date   19-April-2014
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
/// \par Varying Permeability Model (VPM) Decompression Program in c (converted from FORTRAN)
///
///     Author:  Erik C. Baker
///
///     "DISTRIBUTE FREELY - CREDIT THE AUTHORS"
///
///     This program extends the 1986 VPM algorithm (Yount & Hoffman) to include
///     mixed gas, repetitive, and altitude diving.  Developments to the algorithm
///     were made by David E. Yount, Eric B. Maiken, and Erik C. Baker over a
///     period from 1999 to 2001.  This work is dedicated in remembrance of
///     Professor David E. Yount who passed away on April 27, 2000.
///
///     Notes:
///     1.  This program uses the sixteen (16) half-time compartments of the
///         Buhlmann ZH-L16 model.  The optional Compartment 1b is used here with
///         half-times of 1.88 minutes for helium and 5.0 minutes for nitrogen.
///
///     2.  This program uses various DEC, IBM, and Microsoft extensions which
///         may not be supported by all FORTRAN compilers.  Comments are made with
///         a capital "C" in the first column or an exclamation point "!" placed
///         in a line after code.  An asterisk "*" in column 6 is a continuation
///         of the previous line.  All code, except for line numbers, starts in
///         column 7.
///
///     3.  Comments and suggestions for improvements are welcome.  Please
///         respond by e-mail to:  EBaker@se.aeieng.com
///
///     Acknowledgment:  Thanks to Kurt Spaugh for recommendations on how to clean
///     up the code.
/// ===============================================================================
///     Converted to vpmdeco.c using f2c; R.McGinnis (CABER Swe) 5/01
/// ===============================================================================
///
/// ************************ Heirichs Weipkamp **************************************
///
/// The original Yount & Baker code has been adjusted for real life calculation.
///
/// 1) The original main function has been split in several functions
///
/// 2) When the deco zone is reached (while ascending) the gradient factors are kept fix
///     and critical volume algorithm is switched of. maxfirststopdepth is kept fix
///     to make shure Boeyls Law algorithm works correctly
///
/// 4) gas_loadings_ascent_descend heeds all gaschanges and CCR support has been added
///

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "vpm.h"
#include "decom.h"

#define GAS_N2 0
#define GAS_HE 1

static const _Bool buehlmannSafety = true;
/* Common Block Declarations */

extern const float SURFACE_TENSION_GAMMA;				//!Adj. Range: 0.015 to 0.065 N/m
extern const float SKIN_COMPRESSION_GAMMAC;				//!Adj. Range: 0.160 to 0.290 N/m
extern const float UNITS_FACTOR;
extern const float WATER_VAPOR_PRESSURE;				// (Schreiner value)  based on respiratory quotien
extern const float CRIT_VOLUME_PARAMETER_LAMBDA;			//!Adj. Range: 6500 to 8300 fsw-min
//extern const float GRADIENT_ONSET_OF_IMPERM_ATM;			//!Adj. Range: 5.0 to 10.0 atm
extern const float REGENERATION_TIME_CONSTANT;			//!Adj. Range: 10080 to 51840 min
//extern const float PRESSURE_OTHER_GASES_MMHG;				//!Constant value for PO2 up to 2 atm
extern const float CONSTANT_PRESSURE_OTHER_GASES; // PRESSURE_OTHER_GASES_MMHG / 760. * UNITS_FACTOR;

extern const float HELIUM_TIME_CONSTANT[];
extern const float NITROGEN_TIME_CONSTANT[];

static float    minimum_deco_stop_time;
static float    run_time, run_time_first_stop;
static float    segment_time;
static short mix_number;
static float    barometric_pressure;
static _Bool altitude_dive_algorithm_off;
static _Bool units_equal_fsw, units_equal_msw;

/* by hw 11.06.2015 to allow */
static float gCNS_VPM;

static float    helium_pressure[16], nitrogen_pressure[16];
static float    surface_phase_volume_time[16];
static float    regenerated_radius_he[16], regenerated_radius_n2[16];
static float    allowable_gradient_he[16], allowable_gradient_n2[16];

//_Bool deco_zone_reached;
static _Bool critical_volume_algorithm_off;
static float max_first_stop_depth;
static float max_deco_ceiling_depth;
//Boylslaw compensation
static float deco_gradient_he[16];
static float deco_gradient_n2[16];
static int vpm_calc_what;
static int count_critical_volume_iteration;
static short number_of_changes;
static float   depth_change[11];
static float  step_size_change[11];
static float    rate_change[11];
static short mix_change[11];

static const _Bool vpm_b = true;

extern const float float_buehlmann_N2_factor_expositon_20_seconds[];
extern const float float_buehlmann_He_factor_expositon_20_seconds[];
extern const float float_buehlmann_N2_factor_expositon_one_minute[];
extern const float float_buehlmann_He_factor_expositon_one_minute[];
extern const float float_buehlmann_N2_factor_expositon_five_minutes[];
extern const float float_buehlmann_He_factor_expositon_five_minutes[];
extern const float float_buehlmann_N2_factor_expositon_one_hour[];
extern const float float_buehlmann_He_factor_expositon_one_hour[];

static float    depth_start_of_deco_calc;
static float    depth_start_of_deco_zone;
static float    first_stop_depth;
static float    run_time_start_of_deco_zone;

static float r_nint(float *x);
static float r_int(float *x);
static _Bool nullzeit_unter60;
static int vpm_calc_status;
static _Bool buehlmann_wait_exceeded = false;

static SLifeData* pInput = NULL;
static SVpm* pVpm = NULL;
static SDecoinfo* pDecoInfo = NULL;
static SDiveSettings* pDiveSettings = NULL;

static float r_nint(float *x)
{
    return( (*x)>=0 ?
    floorf(*x + 0.5f) : -floorf(0.5f - *x) );
}

static float r_int(float *x)
{
    return( (*x>0.0) ? floorf(*x) : -floorf(- *x) );
}

/** private functions
*/
extern int radius_root_finder (float *a, float *b, float *c,float *low_bound,  float *high_bound, float *ending_radius);

static int nuclear_regeneration(float *dive_time);// clock_();
static int calc_deco_ceiling(float *deco_ceiling_depth,_Bool fallowablw);
static int critical_volume(float *deco_phase_volume_time);	    ;
static int calc_start_of_deco_zone(float *starting_depth, float *rate, float *depth_start_of_deco_zone);
static int calc_initial_allowable_gradient(void);
static void decompression_stop(float *deco_stop_depth, float *step_size, _Bool final_deco_calculation);
static int gas_loadings_ascent_descen(float* helium_pressure, float* nitrogen_pressure, float starting_depth,float ending_depth, float rate,_Bool check_gas_change);
static int calc_surface_phase_volume_time(void);
static int calc_max_actual_gradient(float *deco_stop_depth);
static int projected_ascent(float *starting_depth, float *rate, float *deco_stop_depth, float *step_size);
static void vpm_calc_deco(void);
static int vpm_calc_critcal_volume(_Bool begin,_Bool calc_nulltime);
static int vpm_check_converged(_Bool calc_nulltime);
static int vpm_calc_final_deco(_Bool begin);
static void BOYLES_LAW_COMPENSATION (float* First_Stop_Depth,float * Deco_Stop_Depth,float* Step_Size);
static int vpm_calc_ndl(void);
static void  vpm_init_1(void);
static void vpm_calc_deco_ceiling(void);

static void vpm_init_1(void)
{
     units_equal_msw = true;
     units_equal_fsw = false;
     altitude_dive_algorithm_off= true;			//!Options: ON or OFF
     minimum_deco_stop_time=1.0;					//!Options: float positive number
     critical_volume_algorithm_off= false;		//!Options: ON or OFF
     run_time = 0.;
    //barometric_pressure = dive_data.surface * 10;

    //mix_number = dive_data.selected_gas + 1;

    max_first_stop_depth = 0;
    max_deco_ceiling_depth = 0;
    //deco_zone_reached = false;
    depth_start_of_deco_calc = 0;
    depth_start_of_deco_zone = 0;
    first_stop_depth = 0;
    run_time_start_of_deco_zone = 0;

    gCNS_VPM = 0;
}

float vpm_get_CNS(void)
{
    return gCNS_VPM;
}

int  vpm_calc(SLifeData* pINPUT,
              SDiveSettings* pSettings,
              SVpm* pVPM,
              SDecoinfo*
              pDECOINFO,
              int calc_what)
{
    vpm_init_1();
    //decom_CreateGasChangeList(pSettings, pINPUT);
    vpm_calc_what = calc_what;
    /**clear decoInfo*/
    pDECOINFO->output_time_to_surface_seconds = 0;
    pDECOINFO->output_ndl_seconds = 0;
    pDECOINFO->output_ceiling_meter = 0;
    pDECOINFO->super_saturation = 0;
    uint8_t tmp_calc_status;
    for(int i=0;i<DECOINFO_STRUCT_MAX_STOPS;i++)
    {
        pDECOINFO->output_stop_length_seconds[i] = 0;
    }

    if(pINPUT->dive_time_seconds_without_surface_time < 60)
    {
        vpm_calc_status = CALC_NDL;
        return vpm_calc_status;
    }
    pVpm = pVPM;
    pInput = pINPUT;
    pDecoInfo = pDECOINFO;
    pDiveSettings = pSettings;

    if(vpm_calc_status == CALC_NDL)
    {
        tmp_calc_status = vpm_calc_ndl();
    }
    else
    {
        tmp_calc_status = CALC_BEGIN;
    }
    //Normal Deco calculation
    if(tmp_calc_status != CALC_NDL)
    {
        max_first_stop_depth =  pVpm->max_first_stop_depth_save;
        run_time_start_of_deco_zone = pVpm->run_time_start_of_deco_zone_save;
        depth_start_of_deco_zone = pVpm->depth_start_of_deco_zone_save;
        for (int i = 0; i < 16; ++i) {
            helium_pressure[i] = pInput->tissue_helium_bar[i] * 10;
            nitrogen_pressure[i] = pInput->tissue_nitrogen_bar[i] * 10;
        }
        vpm_calc_deco();
        tmp_calc_status = vpm_calc_critcal_volume(true,false);
        if(vpm_calc_what == DECOSTOPS)
        {
            pVpm->max_first_stop_depth_save = max_first_stop_depth;
            pVpm->run_time_start_of_deco_zone_save = run_time_start_of_deco_zone;
            pVpm->depth_start_of_deco_zone_save = depth_start_of_deco_zone;
        }
    }

    //Only Decostops not futute stops
    if(vpm_calc_what == DECOSTOPS)
        vpm_calc_status = tmp_calc_status;
    return vpm_calc_status;
}

void  vpm_saturation_after_ascent(SLifeData* input)
{
    int i = 0;
    for (i = 0; i < 16; ++i) {
        pInput->tissue_helium_bar[i] = helium_pressure[i] / 10;
        pInput->tissue_nitrogen_bar[i] = nitrogen_pressure[i] / 10;
    }
    pInput->pressure_ambient_bar = pInput->pressure_surface_bar;
}
/* =============================================================================== */
/*     NOTE ABOUT PRESSURE UNITS USED IN CALCULATIONS: */
/*     It is the convention in decompression calculations to compute all gas */
/*     loadings, absolute pressures, partial pressures, etc., in the units of */
/*     depth pressure that you are diving - either feet of seawater (fsw) or */
/*     meters of seawater (msw).  This program follows that convention with the */
/*     the exception that all VPM calculations are performed in SI units (by */
/*     necessity).  Accordingly, there are several conversions back and forth */
/*     between the diving pressure units and the SI units. */
/* =============================================================================== */
/* =============================================================================== */
/*     FUNCTION SUBPROGRAM FOR GAS LOADING CALCULATIONS - ASCENT AND DESCENT */
/* =============================================================================== */



/* =============================================================================== */
/*     SUBROUTINE GAS_LOADINGS_ASCENT_DESCENT */
/*     Purpose: This subprogram applies the Schreiner equation to update the */
/*     gas loadings (partial pressures of helium and nitrogen) in the half-time */
/*     compartments due to a linear ascent or descent segment at a constant rate. */
/* =============================================================================== */

static int gas_loadings_ascent_descen(float* helium_pressure,
                               float* nitrogen_pressure,
                               float starting_depth,
                               float ending_depth,
                               float rate,_Bool check_gas_change)
{
    short i;
    float initial_inspired_n2_pressure,
    initial_inspired_he_pressure, nitrogen_rate,
    last_run_time,
    starting_ambient_pressure,
    ending_ambient_pressure;
    float initial_helium_pressure[16];
    float initial_nitrogen_pressure[16];
    float helium_rate;
    float fraction_helium_begin;
    float fraction_helium_end;
    float fraction_nitrogen_begin;
    float fraction_nitrogen_end;
    float ending_depth_tmp = ending_depth;
    float segment_time_tmp = 0;
    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /* =============================================================================== */
    segment_time = (ending_depth_tmp - starting_depth) / rate;
    last_run_time = run_time;
    run_time = last_run_time + segment_time;
    do {
        ending_depth_tmp = ending_depth;
        if (starting_depth > ending_depth && check_gas_change && number_of_changes > 1)
        {
            for (i = 1; i < number_of_changes; ++i)
            {
                if (depth_change[i] < starting_depth && depth_change[i] > ending_depth)
                {
                    ending_depth_tmp = depth_change[i];
                    break;
                }
            }
            for (i = 1; i < number_of_changes; ++i)
            {
                if (depth_change[i] >= starting_depth)
                {
                    mix_number = mix_change[i];
                }
            }
        }
        segment_time_tmp = (ending_depth_tmp - starting_depth) / rate;
        ending_ambient_pressure = ending_depth_tmp + barometric_pressure;
        starting_ambient_pressure = starting_depth + barometric_pressure;
        decom_get_inert_gases( starting_ambient_pressure / 10,  (&pDiveSettings->decogaslist[mix_number]), &fraction_nitrogen_begin, &fraction_helium_begin );
        decom_get_inert_gases( ending_ambient_pressure   / 10,  (&pDiveSettings->decogaslist[mix_number]), &fraction_nitrogen_end, &fraction_helium_end );

        initial_inspired_he_pressure =	(starting_ambient_pressure - WATER_VAPOR_PRESSURE) * fraction_helium_begin;
        initial_inspired_n2_pressure =	(starting_ambient_pressure - WATER_VAPOR_PRESSURE) * fraction_nitrogen_begin;
        //helium_rate = *rate * fraction_helium[mix_number - 1];
        helium_rate = ((ending_ambient_pressure  - WATER_VAPOR_PRESSURE)* fraction_helium_end - initial_inspired_he_pressure)/segment_time_tmp;
        //nitrogen_rate2 = *rate * fraction_nitrogen[mix_number - 1];
        nitrogen_rate = ((ending_ambient_pressure  - WATER_VAPOR_PRESSURE)* fraction_nitrogen_end - initial_inspired_n2_pressure)/segment_time_tmp;


        decom_oxygen_calculate_cns_stage_SchreinerStyle(segment_time_tmp,&pDiveSettings->decogaslist[mix_number],starting_ambient_pressure/10,ending_ambient_pressure/10,&gCNS_VPM);
        //if(fabs(nitrogen_rate - nitrogen_rate2) > 0.000001)
            //return -2;
        for (i = 1; i <= 16; ++i)
        {
            initial_helium_pressure[i - 1] = helium_pressure[i - 1];
            initial_nitrogen_pressure[i - 1] = nitrogen_pressure[i - 1];
            helium_pressure[i - 1] =
            schreiner_equation__2(&initial_inspired_he_pressure,
            &helium_rate,
            &segment_time,
            &HELIUM_TIME_CONSTANT[i - 1],
            &initial_helium_pressure[i - 1]);
            nitrogen_pressure[i - 1] =
            schreiner_equation__2(&initial_inspired_n2_pressure,
            &nitrogen_rate,
            &segment_time,
            &NITROGEN_TIME_CONSTANT[i - 1],
            &initial_nitrogen_pressure[i - 1]);

            //nextround???

        }
        starting_depth = ending_depth_tmp;
    } while(ending_depth_tmp > ending_depth);

    return 0;
} /* gas_loadings_ascent_descen */

static float last_phase_volume_time[16];
static float n2_pressure_start_of_deco_zone[16];
static float he_pressure_start_of_deco_zone[16];
static float phase_volume_time[16];
static float n2_pressure_start_of_ascent[16];
static float  he_pressure_start_of_ascent[16];
static float    run_time_start_of_deco_calc;
static float starting_depth;
static float last_run_time;
static float deco_phase_volume_time;
static float run_time_start_of_ascent;
static float rate;
static float    step_size;
static _Bool vpm_violates_buehlmann;

static  void  vpm_calc_deco(void)
{
    /* System generated locals */

    //float   deepest_possible_stop_depth;
//	altitude_of_dive,
    short i;
    int j = 0;

    // float    rounding_operation;

    /* =============================================================================== */
    /*     INPUT PARAMETERS TO BE USED FOR STAGED DECOMPRESSION AND SAVE IN ARRAYS. */
    /*     ASSIGN INITAL PARAMETERS TO BE USED AT START OF ASCENT */
    /*     The user has the ability to change mix, ascent rate, and step size in any */
    /*     combination at any depth during the ascent. */
    /* =============================================================================== */

    run_time = ((float)pInput->dive_time_seconds )/ 60;
    count_critical_volume_iteration = 0;
    number_of_changes = 1;

    barometric_pressure = pInput->pressure_surface_bar * 10;
    depth_change[0] =(pInput->pressure_ambient_bar - pInput->pressure_surface_bar)* 10;
    mix_change[0] = 0;
    rate_change[0 ] = -10;// neu 160215 hw, zuvor: -12;
    step_size_change[0] = 3;
    vpm_violates_buehlmann = false;

    for (i = 1; i < BUEHLMANN_STRUCT_MAX_GASES; i++)
    {
            depth_change[i] = 0;
            mix_change[i] = 0;
    }
    j = 0;

    for (i = 1; i < BUEHLMANN_STRUCT_MAX_GASES; i++)
    {
        if((pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero  >= depth_change[0] + 1)
        		&& (pDiveSettings->gas[pDiveSettings->decogaslist[i].GasIdInSettings].note.ub.decocalc))
            continue;

        if((pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero <= 0)
        	|| (pDiveSettings->gas[pDiveSettings->decogaslist[i].GasIdInSettings].note.ub.decocalc == 0))
            break;

        j++;
        number_of_changes ++;
        depth_change[j] = pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero ;
        mix_change[j] = i;
        rate_change[j] = -10;// neu 160215 hw, zuvor: -12;
        step_size_change[j] = 3;
    }

    starting_depth = depth_change[0] ;
    mix_number = mix_change[0] ;
    rate = rate_change[0];
    step_size = step_size_change[0];

    for (i = 0; i < 16; ++i) {
        he_pressure_start_of_ascent[i ] = 	helium_pressure[i];
        n2_pressure_start_of_ascent[i] = 	nitrogen_pressure[i];
    }
    run_time_start_of_ascent = run_time;
    if(starting_depth <= depth_start_of_deco_zone && vpm_calc_what == DECOSTOPS)
    {
        pVpm->deco_zone_reached = true;
        depth_start_of_deco_calc = starting_depth;
        critical_volume_algorithm_off = true;
    }
    else
    {
        //if(deco_zone_reached)
        //{
            pVpm->deco_zone_reached = false;
            critical_volume_algorithm_off = false;
            //max_first_stop_depth = 0;
            //max_first_stop_depth_save = 0;
        //}
        /* =============================================================================== */
        /*     BEGIN PROCESS OF ASCENT AND DECOMPRESSION */
        /*     First, calculate the regeneration of critical radii that takes place over */
        /*     the dive time.  The regeneration time constant has a time scale of weeks */
        /*     so this will have very little impact on dives of normal length, but will */
        /*     have major impact for saturation dives. */
        /* =============================================================================== */

        nuclear_regeneration(&run_time);

        /* =============================================================================== */
        /*     CALCULATE INITIAL ALLOWABLE GRADIENTS FOR ASCENT */
        /*     This is based on the maximum effective crushing pressure on critical radii */
        /*     in each compartment achieved during the dive profile. */
        /* =============================================================================== */

        calc_initial_allowable_gradient();

        /* =============================================================================== */
        /*     SAVE VARIABLES AT START OF ASCENT (END OF BOTTOM TIME) SINCE THESE WILL */
        /*     BE USED LATER TO COMPUTE THE FINAL ASCENT PROFILE THAT IS WRITTEN TO THE */
        /*     OUTPUT FILE. */
        /*     The VPM uses an iterative process to compute decompression schedules so */
        /*     there will be more than one pass through the decompression loop. */
        /* =============================================================================== */

        /* =============================================================================== */
        /*     CALCULATE THE DEPTH WHERE THE DECOMPRESSION ZONE BEGINS FOR THIS PROFILE */
        /*     BASED ON THE INITIAL ASCENT PARAMETERS AND WRITE THE DEEPEST POSSIBLE */
        /*     DECOMPRESSION STOP DEPTH TO THE OUTPUT FILE */
        /*     Knowing where the decompression zone starts is very important.  Below */
        /*     that depth there is no possibility for bubble formation because there */
        /*     will be no supersaturation gradients.  Deco stops should never start */
        /*     below the deco zone.  The deepest possible stop deco stop depth is */
        /*     defined as the next "standard" stop depth above the point where the */
        /*     leading compartment enters the deco zone.  Thus, the program will not */
        /*     base this calculation on step sizes larger than 10 fsw or 3 msw.  The */
        /*     deepest possible stop depth is not used in the program, per se, rather */
        /*     it is information to tell the diver where to start putting on the brakes */
        /*     during ascent.  This should be prominently displayed by any deco program. */
        /* =============================================================================== */

        calc_start_of_deco_zone(&starting_depth, &rate, &depth_start_of_deco_zone);
        /* =============================================================================== */
        /*     TEMPORARILY ASCEND PROFILE TO THE START OF THE DECOMPRESSION ZONE, SAVE */
        /*     VARIABLES AT THIS POINT, AND INITIALIZE VARIABLES FOR CRITICAL VOLUME LOOP */
        /*     The iterative process of the VPM Critical Volume Algorithm will operate */
        /*     only in the decompression zone since it deals with excess gas volume */
        /*     released as a result of supersaturation gradients (not possible below the */
        /*     decompression zone). */
        /* =============================================================================== */
        gas_loadings_ascent_descen(helium_pressure,nitrogen_pressure, starting_depth, depth_start_of_deco_zone, rate, true);

        run_time_start_of_deco_zone = run_time;
        depth_start_of_deco_calc = depth_start_of_deco_zone;

        for (i = 0; i < 16; ++i)
        {
            pVpm->max_actual_gradient[i] = 0.;
        }
    }

    for (i = 0; i < 16; ++i)
    {
        surface_phase_volume_time[i] = 0.;
        last_phase_volume_time[i] = 0.;
        he_pressure_start_of_deco_zone[i] =	helium_pressure[i];
        n2_pressure_start_of_deco_zone[i] =	nitrogen_pressure[i];
        //pVpm->max_actual_gradient[i] = 0.;
    }
    run_time_start_of_deco_calc = run_time;
}
/* =============================================================================== */
/*     START OF CRITICAL VOLUME LOOP */
/*     This loop operates between Lines 50 and 100.  If the Critical Volume */
/*     Algorithm is toggled "off" in the program settings, there will only be */
/*     one pass through this loop.  Otherwise, there will be two or more passes */
/*     through this loop until the deco schedule is "converged" - that is when a */
/*     comparison between the phase volume time of the present iteration and the */
/*     last iteration is less than or equal to one minute.  This implies that */
/*     the volume of released gas in the most recent iteration differs from the */
/*     "critical" volume limit by an acceptably small amount.  The critical */
/*     volume limit is set by the Critical Volume Parameter Lambda in the program */
/*     settings (default setting is 7500 fsw-min with adjustability range from */
/*     from 6500 to 8300 fsw-min according to Bruce Wienke). */
/* =============================================================================== */
/* L50: */

static float  deco_stop_depth;
static int vpm_calc_critcal_volume(_Bool begin,
                            _Bool calc_nulltime)
{   /* loop will run continuous there is an exit stateme */

    short i;

    float rounding_operation2;
    //float    ending_depth;
    float deco_ceiling_depth;

    //float deco_time;
    int count = 0;
    _Bool first_stop;
    int dp = 0;
    float tissue_He_saturation[16];
    float tissue_N2_saturation[16];
    float vpm_buehlmann_safety_gradient = 1.0f - (((float)pDiveSettings->vpm_conservatism) / 40);
    /* =============================================================================== */
    /*     CALCULATE CURRENT DECO CEILING BASED ON ALLOWABLE SUPERSATURATION */
    /*     GRADIENTS AND SET FIRST DECO STOP.  CHECK TO MAKE SURE THAT SELECTED STEP */
    /*     SIZE WILL NOT ROUND UP FIRST STOP TO A DEPTH THAT IS BELOW THE DECO ZONE. */
    /* =============================================================================== */
    if(begin)
    {
        if(depth_start_of_deco_calc < max_first_stop_depth )
        {
            if(vpm_b)
            {
                BOYLES_LAW_COMPENSATION(&max_first_stop_depth, &depth_start_of_deco_calc, &step_size);
            }
            calc_deco_ceiling(&deco_ceiling_depth, false);
        }
        else
            calc_deco_ceiling(&deco_ceiling_depth, true);


        if (deco_ceiling_depth <= 0.0f) {
            deco_stop_depth = 0.0f;
        } else {
            rounding_operation2 = deco_ceiling_depth / step_size + ( float)0.5f;
            deco_stop_depth = r_nint(&rounding_operation2) * step_size;
        }

        // buehlmann safety
        if(buehlmannSafety)
        {
            for (i = 0; i < 16; i++)
            {
                tissue_He_saturation[i] = helium_pressure[i] / 10.0;
                tissue_N2_saturation[i] = nitrogen_pressure[i] / 10.0;
            }

            if(!decom_tissue_test_tolerance(tissue_N2_saturation, tissue_He_saturation, vpm_buehlmann_safety_gradient, (deco_stop_depth / 10.0f) + pInput->pressure_surface_bar))
            {

                vpm_violates_buehlmann = true;
                do {
                    deco_stop_depth += 3.0;
                } while (!decom_tissue_test_tolerance(tissue_N2_saturation, tissue_He_saturation, vpm_buehlmann_safety_gradient, (deco_stop_depth / 10.0f) + pInput->pressure_surface_bar));
            }
        }

        /* =============================================================================== */
        /*     PERFORM A SEPARATE "PROJECTED ASCENT" OUTSIDE OF THE MAIN PROGRAM TO MAKE */
        /*     SURE THAT AN INCREASE IN GAS LOADINGS DURING ASCENT TO THE FIRST STOP WILL */
        /*     NOT CAUSE A VIOLATION OF THE DECO CEILING.  IF SO, ADJUST THE FIRST STOP */
        /*     DEEPER BASED ON STEP SIZE UNTIL A SAFE ASCENT CAN BE MADE. */
        /*     Note: this situation is a possibility when ascending from extremely deep */
        /*     dives or due to an unusual gas mix selection. */
        /*     CHECK AGAIN TO MAKE SURE THAT ADJUSTED FIRST STOP WILL NOT BE BELOW THE */
        /*     DECO ZONE. */
        /* =============================================================================== */
        if (deco_stop_depth < depth_start_of_deco_calc)
        {
            projected_ascent(&depth_start_of_deco_calc, &rate, &deco_stop_depth, &step_size);
        }

        /*if (deco_stop_depth > depth_start_of_deco_zone) {
            printf("\t\n");
            printf(fmt_905);
            printf(fmt_900);
            printf("\nPROGRAM TERMINATED\n");
            exit(1);
        }*/

        /* =============================================================================== */
        /*     HANDLE THE SPECIAL CASE WHEN NO DECO STOPS ARE REQUIRED - ASCENT CAN BE */
        /*     MADE DIRECTLY TO THE SURFACE */
        /*     Write ascent data to output file and exit the Critical Volume Loop. */
        /* =============================================================================== */

        if (deco_stop_depth == 0.0f)
        {
            if(calc_nulltime)
            {
                return CALC_END;
            }
            if(pVpm->deco_zone_reached)
            {
                for(dp = 0;dp < DECOINFO_STRUCT_MAX_STOPS;dp++)
                {
                    pDecoInfo->output_stop_length_seconds[dp] = 0;
                }
                pDecoInfo->output_ndl_seconds = 0;
            }

            return CALC_NDL;
                    /* exit the critical volume l */
        }

    /* =============================================================================== */
    /*     ASSIGN VARIABLES FOR ASCENT FROM START OF DECO ZONE TO FIRST STOP.  SAVE */
    /*     FIRST STOP DEPTH FOR LATER USE WHEN COMPUTING THE FINAL ASCENT PROFILE */
    /* =============================================================================== */
        deco_stop_depth = fmaxf(deco_stop_depth,(float)pDiveSettings->last_stop_depth_bar * 10.0);
        starting_depth = depth_start_of_deco_calc;
        first_stop_depth = deco_stop_depth;
        first_stop = true;
    }
    /* =============================================================================== */
    /*     DECO STOP LOOP BLOCK WITHIN CRITICAL VOLUME LOOP */
    /*     This loop computes a decompression schedule to the surface during each */
    /*     iteration of the critical volume loop.  No output is written from this */
    /*     loop, rather it computes a schedule from which the in-water portion of the */
    /*     total phase volume time (Deco_Phase_Volume_Time) can be extracted.  Also, */
    /*     the gas loadings computed at the end of this loop are used the subroutine */
    /*     which computes the out-of-water portion of the total phase volume time */
    /*     (Surface_Phase_Volume_Time) for that schedule. */

    /*     Note that exit is made from the loop after last ascent is made to a deco */
    /*     stop depth that is less than or equal to zero.  A final deco stop less */
    /*     than zero can happen when the user makes an odd step size change during */
    /*     ascent - such as specifying a 5 msw step size change at the 3 msw stop! */
    /* =============================================================================== */

    while(true)           /* loop will run continuous there is an break statement */
    {
        if(starting_depth > deco_stop_depth )
            gas_loadings_ascent_descen(helium_pressure, nitrogen_pressure, starting_depth, deco_stop_depth, rate,first_stop);

        first_stop = false;
        if (deco_stop_depth <= 0.0f)
        {
            break;
        }
        if (number_of_changes > 1)
        {
            int i1 = number_of_changes;
            for (i = 2; i <= i1; ++i) {
                if (depth_change[i - 1] >= deco_stop_depth)
                {
                    mix_number = mix_change[i - 1];
                    rate = rate_change[i - 1];
                    step_size = step_size_change[i - 1];
                }
            }
        }
        if(vpm_b)
        {
            float fist_stop_depth2 =  fmaxf(first_stop_depth,max_first_stop_depth);
            BOYLES_LAW_COMPENSATION(&fist_stop_depth2, &deco_stop_depth, &step_size);
        }
        decompression_stop(&deco_stop_depth, &step_size, false);
        starting_depth = deco_stop_depth;

        if(deco_stop_depth == (float)pDiveSettings->last_stop_depth_bar * 10.0)
            deco_stop_depth = 0;
        else
        {
            deco_stop_depth = deco_stop_depth - step_size;
            deco_stop_depth = fmaxf(deco_stop_depth,(float)pDiveSettings->last_stop_depth_bar * 10.0);
        }

        count++;
        //if(count > 14)
            //return CALC_CRITICAL2;
        /* L60: */
    }

        return vpm_check_converged(calc_nulltime);
}
/* =============================================================================== */
/*     COMPUTE TOTAL PHASE VOLUME TIME AND MAKE CRITICAL VOLUME COMPARISON */
/*     The deco phase volume time is computed from the run time.  The surface */
/*     phase volume time is computed in a subroutine based on the surfacing gas */
/*     loadings from previous deco loop block.  Next the total phase volume time */
/*     (in-water + surface) for each compartment is compared against the previous */
/*     total phase volume time.  The schedule is converged when the difference is */
/*     less than or equal to 1 minute in any one of the 16 compartments. */

/*     Note:  the "phase volume time" is somewhat of a mathematical concept. */
/*     It is the time divided out of a total integration of supersaturation */
/*     gradient x time (in-water and surface).  This integration is multiplied */
/*     by the excess bubble number to represent the amount of free-gas released */
/*     as a result of allowing a certain number of excess bubbles to form. */
/* =============================================================================== */
/* end of deco stop loop */

static int vpm_check_converged(_Bool calc_nulltime)
{

    short i;
    float    critical_volume_comparison;
    float    r1;
    _Bool schedule_converged = false;


    deco_phase_volume_time = run_time - run_time_start_of_deco_zone;
    calc_surface_phase_volume_time();

    for (i = 1; i <= 16; ++i)
    {
        phase_volume_time[i - 1] =
        deco_phase_volume_time + surface_phase_volume_time[i - 1];
        critical_volume_comparison = (r1 = phase_volume_time[i - 1] - last_phase_volume_time[i - 1], fabs(r1));

        if (critical_volume_comparison <= 1.0f)
        {
            schedule_converged = true;
        }
    }

/* =============================================================================== */
/*     CRITICAL VOLUME DECISION TREE BETWEEN LINES 70 AND 99 */
/*     There are two options here.  If the Critical Volume Agorithm setting is */
/*     "on" and the schedule is converged, or the Critical Volume Algorithm */
/*     setting was "off" in the first place, the program will re-assign variables */
/*     to their values at the start of ascent (end of bottom time) and process */
/*     a complete decompression schedule once again using all the same ascent */
/*     parameters and first stop depth.  This decompression schedule will match */
/*     the last iteration of the Critical Volume Loop and the program will write */
/*     the final deco schedule to the output file. */

/*     Note: if the Critical Volume Agorithm setting was "off", the final deco */
/*     schedule will be based on "Initial Allowable Supersaturation Gradients." */
/*     If it was "on", the final schedule will be based on "Adjusted Allowable */
/*     Supersaturation Gradients" (gradients that are "relaxed" as a result of */
/*     the Critical Volume Algorithm). */

/*     If the Critical Volume Agorithm setting is "on" and the schedule is not */
/*     converged, the program will re-assign variables to their values at the */
/*     start of the deco zone and process another trial decompression schedule. */
/* =============================================================================== */
/* L70: */
    //Not more than 4 iteration allowed
    count_critical_volume_iteration++;
    if(count_critical_volume_iteration > 4)
    {
        //return CALC_FINAL_DECO;
        if(calc_nulltime)
            return CALC_FINAL_DECO;
        else
            return vpm_calc_final_deco(true);
    }
    if (schedule_converged || critical_volume_algorithm_off)
    {

        //return CALC_FINAL_DECO;
        if(calc_nulltime)
            return CALC_FINAL_DECO;
        else
            return vpm_calc_final_deco(true);
/* final deco schedule */
/* exit critical volume l */

/* =============================================================================== */
/*     IF SCHEDULE NOT CONVERGED, COMPUTE RELAXED ALLOWABLE SUPERSATURATION */
/*     GRADIENTS WITH VPM CRITICAL VOLUME ALGORITHM AND PROCESS ANOTHER */
/*     ITERATION OF THE CRITICAL VOLUME LOOP */
/* =============================================================================== */

    } else {
        critical_volume(&deco_phase_volume_time);
        deco_phase_volume_time = 0.;
        run_time = run_time_start_of_deco_calc;
        starting_depth = depth_start_of_deco_calc;
        mix_number = mix_change[0];
        rate = rate_change[0];
        step_size = step_size_change[0];
        for (i = 1; i <= 16; ++i)
        {
                last_phase_volume_time[i - 1] = phase_volume_time[i - 1];
                helium_pressure[i - 1] = he_pressure_start_of_deco_zone[i - 1];
                nitrogen_pressure[i - 1] = n2_pressure_start_of_deco_zone[i - 1];
        }
        if(calc_nulltime)
            return CALC_CRITICAL;
        else
            return vpm_calc_critcal_volume(true, false);
    }
/* end of critical volume decision */
/* L100: */
    //  }/* end of critical vol loop */
}

static void vpm_calc_deco_ceiling(void)
{

    short i;
// hw 1601209	 float    r1;
// hw 1601209	 float    stop_time;
// hw 1601209	 int count = 0;
    //static int dp_max;
    //static float surfacetime;
    // _Bool first_stop = false;
    float tissue_He_saturation[16];
    float tissue_N2_saturation[16];
    float vpm_buehlmann_safety_gradient = 1.0f - (((float)pDiveSettings->vpm_conservatism) / 40.0);
    //max_first_stop_depth = fmaxf(first_stop_depth,max_first_stop_depth);

    /** CALC DECO Ceiling ******************************************************************/
    /** Not when Future stops */
    if(vpm_calc_what == DECOSTOPS)
    {

        for (i = 1; i <= 16; ++i)
        {
            helium_pressure[i - 1] = he_pressure_start_of_deco_zone[i - 1];
            nitrogen_pressure[i - 1] = n2_pressure_start_of_deco_zone[i - 1];
        }
        run_time = run_time_start_of_ascent;// run_time_start_of_ascent;
        starting_depth = depth_change[0];
        mix_number = mix_change[0];
        rate = rate_change[0];
        //gas_loadings_ascent_descen(helium_pressure,nitrogen_pressure, starting_depth, depth_start_of_deco_calc, rate, true);

        float deco_ceiling_depth = 0.0f;
        if(depth_start_of_deco_calc > max_deco_ceiling_depth)
        {
            calc_deco_ceiling(&deco_ceiling_depth, true);
        }
        if(buehlmannSafety)
        {
            for (i = 0; i < 16; i++)
            {
                tissue_He_saturation[i] = helium_pressure[i] / 10.0;
                tissue_N2_saturation[i] = nitrogen_pressure[i] / 10.0;
            }

            if(!decom_tissue_test_tolerance(tissue_N2_saturation, tissue_He_saturation, vpm_buehlmann_safety_gradient, (deco_ceiling_depth / 10.0f) + pInput->pressure_surface_bar))
            {
                vpm_violates_buehlmann = true;
                do {
                    deco_ceiling_depth += 0.1f;
                } while (!decom_tissue_test_tolerance(tissue_N2_saturation, tissue_He_saturation, vpm_buehlmann_safety_gradient, (deco_ceiling_depth / 10.0f) + pInput->pressure_surface_bar));
            }
        }

        if (deco_ceiling_depth < depth_start_of_deco_calc)
        {
            projected_ascent(&depth_start_of_deco_calc, &rate, &deco_ceiling_depth, &step_size);
        }

        max_deco_ceiling_depth = fmaxf(max_deco_ceiling_depth,deco_ceiling_depth);

        if(depth_start_of_deco_calc > deco_ceiling_depth)
        {
            gas_loadings_ascent_descen(helium_pressure,nitrogen_pressure, depth_start_of_deco_calc,deco_ceiling_depth, rate, true);
            //surfacetime += segment_time;
        }

        if(vpm_b)
        {
            BOYLES_LAW_COMPENSATION(&max_deco_ceiling_depth, &deco_ceiling_depth, &step_size);
        }
        calc_deco_ceiling(&deco_ceiling_depth, false);

        // buehlmann safety
        if(vpm_violates_buehlmann)
        {
            for (i = 0; i < 16; i++)
            {
                tissue_He_saturation[i] = helium_pressure[i] / 10.0;
                tissue_N2_saturation[i] = nitrogen_pressure[i] / 10.0;
            }

            if(!decom_tissue_test_tolerance(tissue_N2_saturation, tissue_He_saturation, vpm_buehlmann_safety_gradient, (deco_ceiling_depth / 10.0f) + pInput->pressure_surface_bar))
            {
                vpm_violates_buehlmann = true;
                do {
                    deco_ceiling_depth += 0.1f;
                } while (!decom_tissue_test_tolerance(tissue_N2_saturation, tissue_He_saturation, vpm_buehlmann_safety_gradient, (deco_ceiling_depth / 10.0f) + pInput->pressure_surface_bar));
            }
        }
        // output_ceiling_meter
        if(deco_ceiling_depth > first_stop_depth)
            deco_ceiling_depth = first_stop_depth;
        pDecoInfo->output_ceiling_meter = deco_ceiling_depth ;
    }
    else
    {
         pDecoInfo->output_ceiling_meter = 0.0;
    }

    // fix hw 160627
    if(pDecoInfo->output_ceiling_meter < 0.0)
        pDecoInfo->output_ceiling_meter = 0.0;

    /*** End CALC ceiling    ***************************************************/
}


/* =============================================================================== */
/*     DECO STOP LOOP BLOCK FOR FINAL DECOMPRESSION SCHEDULE */
/* =============================================================================== */

static int vpm_calc_final_deco(_Bool begin)
{
    short i;
    float    r1;
    float    stop_time;
    int count = 0;
    static int dp_max;
    static float surfacetime;
    _Bool first_stop = false;

    short stop_time_seconds;

    max_first_stop_depth = fmaxf(first_stop_depth,max_first_stop_depth);
    if(begin)
    {
        gCNS_VPM = 0;
        dp_max = 0;
        for (i = 1; i <= 16; ++i)
        {
            helium_pressure[i - 1] =
            he_pressure_start_of_ascent[i - 1];
            nitrogen_pressure[i - 1] =
            n2_pressure_start_of_ascent[i - 1];
        }
        run_time = run_time_start_of_ascent;// run_time_start_of_ascent;
        starting_depth = depth_change[0];
        mix_number = mix_change[0];
        rate = rate_change[0];
        step_size = step_size_change[0];
        deco_stop_depth = first_stop_depth;
        max_first_stop_depth = fmaxf(first_stop_depth,max_first_stop_depth);
        last_run_time = 0.;



        /* =============================================================================== */
        /*     DECO STOP LOOP BLOCK FOR FINAL DECOMPRESSION SCHEDULE */
        /* =============================================================================== */
        surfacetime = 0;
        first_stop = true;
    }

    while(true)       /* loop will run continuous until there is an break statement */
    {
        if(starting_depth > deco_stop_depth)
        {
            gas_loadings_ascent_descen(helium_pressure,nitrogen_pressure, starting_depth,deco_stop_depth, rate, first_stop);
            surfacetime += segment_time;
        }

        /* =============================================================================== */
        /*     DURING FINAL DECOMPRESSION SCHEDULE PROCESS, COMPUTE MAXIMUM ACTUAL */
        /*     SUPERSATURATION GRADIENT RESULTING IN EACH COMPARTMENT */
        /*     If there is a repetitive dive, this will be used later in the VPM */
        /*     Repetitive Algorithm to adjust the values for critical radii. */
        /* =============================================================================== */
        if(vpm_calc_what == DECOSTOPS)
            calc_max_actual_gradient(&deco_stop_depth);

        if (deco_stop_depth <= 0.0f) {
            break;
        }
        if (number_of_changes > 1)
        {
            int i1 = number_of_changes;
            for (i = 2; i <= i1; ++i)
            {
                if (depth_change[i - 1] >= deco_stop_depth)
                {
                    mix_number = mix_change[i - 1];
                    rate = rate_change[i - 1];
                    step_size = step_size_change[i - 1];
                }
            }
        }

        if(first_stop)
        {
            run_time_first_stop = run_time;
            first_stop = false;
        }
        if(vpm_b)
        {
            BOYLES_LAW_COMPENSATION(&max_first_stop_depth, &deco_stop_depth, &step_size);
        }
        decompression_stop(&deco_stop_depth, &step_size, true);

        /* =============================================================================== */
        /*     This next bit justs rounds up the stop time at the first stop to be in */
        /*     whole increments of the minimum stop time (to make for a nice deco table). */
        /* =============================================================================== */

        if (last_run_time == 0.0f)
        {
            r1 = segment_time / minimum_deco_stop_time + 0.5f;
            stop_time = r_int(&r1) * minimum_deco_stop_time;
        } else {
            stop_time = run_time - last_run_time;
        }
        stop_time = segment_time;
        surfacetime += stop_time;
        if((vpm_calc_what == DECOSTOPS) || (vpm_calc_what == BAILOUTSTOPS))
        {
            int dp = 0;
            if(deco_stop_depth == (float)pDiveSettings->last_stop_depth_bar * 10)
            {
                dp = 0;
            }
            else
            {
                dp = 1 + (unsigned short)((deco_stop_depth - (pDiveSettings->input_second_to_last_stop_depth_bar * 10.0)) / step_size);
            }

            //dp_max = (int)fmaxf(dp_max,dp);
            if(dp > dp_max)
            {
            	dp_max = dp;
            }
            if(dp < DECOINFO_STRUCT_MAX_STOPS)
            {
                stop_time_seconds = (short) fminf((999.9 * 60.0), (stop_time *60.0));
                //

                //if(vpm_calc_what == DECOSTOPS)
                    pDecoInfo->output_stop_length_seconds[dp] = stop_time_seconds;
                //else
                    //decostop_bailout[dp] = (unsigned short)stop_time_seconds;
            }
        }


        /* =============================================================================== */
        /*     DURING FINAL DECOMPRESSION SCHEDULE, IF MINIMUM STOP TIME PARAMETER IS A */
        /*     WHOLE NUMBER (i.e. 1 minute) THEN WRITE DECO SCHEDULE USING short */
        /*     NUMBERS (looks nicer).  OTHERWISE, USE DECIMAL NUMBERS. */
        /*     Note: per the request of a noted exploration diver(!), program now allows */
        /*     a minimum stop time of less than one minute so that total ascent time can */
        /*     be minimized on very long dives.  In fact, with step size set at 1 fsw or */
        /*     0.2 msw and minimum stop time set at 0.1 minute (6 seconds), a near */
        /*     continuous decompression schedule can be computed. */
        /* =============================================================================== */

        starting_depth = deco_stop_depth;
        if(deco_stop_depth == (float)pDiveSettings->last_stop_depth_bar * 10.0)
            deco_stop_depth = 0.0;
        else
        {
            deco_stop_depth = deco_stop_depth - step_size;
            deco_stop_depth = fmaxf(deco_stop_depth,(float)pDiveSettings->last_stop_depth_bar * 10.0);
        }

        last_run_time = run_time;
        count++;
        //if(count > 14)
            //return CALC_FINAL_DECO2;
        /* L80: */
    } /* for final deco sche */

    if( (vpm_calc_what == DECOSTOPS)  || (vpm_calc_what == BAILOUTSTOPS))
    {
        for(int dp = dp_max +1;dp < DECOINFO_STRUCT_MAX_STOPS;dp++)
        {
            //if(vpm_calc_what == DECOSTOPS)
                pDecoInfo->output_stop_length_seconds[dp] = 0;
            //else
                //decostop_bailout[dp] = 0;
        }
    }
    pDecoInfo->output_time_to_surface_seconds = (int)(surfacetime * 60.0);
    pDecoInfo->output_ndl_seconds = 0;

    vpm_calc_deco_ceiling();
    /* end of deco stop lo */
    return CALC_END;
}

/* =============================================================================== */
/*     SUBROUTINE NUCLEAR_REGENERATION */
/*     Purpose: This subprogram calculates the regeneration of VPM critical */
/*     radii that takes place over the dive time.  The regeneration time constant */
/*     has a time scale of weeks so this will have very little impact on dives of */
/*     normal length, but will have a major impact for saturation dives. */
/* =============================================================================== */

static int nuclear_regeneration(float *dive_time)
{
    /* Local variables */
    float crush_pressure_adjust_ratio_he,
    ending_radius_n2,
    ending_radius_he;
    short i;
    float crushing_pressure_pascals_n2,
    crushing_pressure_pascals_he,
    adj_crush_pressure_n2_pascals,
    adj_crush_pressure_he_pascals,
    crush_pressure_adjust_ratio_n2;

    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /*     First convert the maximum crushing pressure obtained for each compartment */
    /*     to Pascals.  Next, compute the ending radius for helium and nitrogen */
    /*     critical nuclei in each compartment. */
    /* =============================================================================== */

    for (i = 1; i <= 16; ++i)
    {
        crushing_pressure_pascals_he =
        pVpm->max_crushing_pressure_he[i - 1] / UNITS_FACTOR * 101325.0f;
        crushing_pressure_pascals_n2 =
        pVpm->max_crushing_pressure_n2[i - 1] / UNITS_FACTOR * 101325.0f;
        ending_radius_he =
        1.0f / (crushing_pressure_pascals_he /
        ((SKIN_COMPRESSION_GAMMAC - SURFACE_TENSION_GAMMA) * 2.0f) +
        1.0f / pVpm->adjusted_critical_radius_he[i - 1]);
        ending_radius_n2 =
        1.0f / (crushing_pressure_pascals_n2 /
        ((SKIN_COMPRESSION_GAMMAC - SURFACE_TENSION_GAMMA) * 2.0f) +
        1.0f / pVpm->adjusted_critical_radius_n2[i - 1]);

        /* =============================================================================== */
        /*     A "regenerated" radius for each nucleus is now calculated based on the */
        /*     regeneration time constant.  This means that after application of */
        /*     crushing pressure and reduction in radius, a nucleus will slowly grow */
        /*     back to its original initial radius over a period of time.  This */
        /*     phenomenon is probabilistic in nature and depends on absolute temperature. */
        /*     It is independent of crushing pressure. */
        /* =============================================================================== */

        regenerated_radius_he[i - 1] =
        pVpm->adjusted_critical_radius_he[i - 1] +
        (ending_radius_he - pVpm->adjusted_critical_radius_he[i - 1]) *
        expf(-(*dive_time) / REGENERATION_TIME_CONSTANT);
        regenerated_radius_n2[i - 1] =
        pVpm->adjusted_critical_radius_n2[i - 1] +
        (ending_radius_n2 - pVpm->adjusted_critical_radius_n2[i - 1]) *
        expf(-(*dive_time) / REGENERATION_TIME_CONSTANT);

        /* =============================================================================== */
        /*     In order to preserve reference back to the initial critical radii after */
        /*     regeneration, an "adjusted crushing pressure" for the nuclei in each */
        /*     compartment must be computed.  In other words, this is the value of */
        /*     crushing pressure that would have reduced the original nucleus to the */
        /*     to the present radius had regeneration not taken place.  The ratio */
        /*     for adjusting crushing pressure is obtained from algebraic manipulation */
        /*     of the standard VPM equations.  The adjusted crushing pressure, in lieu */
        /*     of the original crushing pressure, is then applied in the VPM Critical */
        /*     Volume Algorithm and the VPM Repetitive Algorithm. */
        /* =============================================================================== */

        crush_pressure_adjust_ratio_he =
        ending_radius_he * (pVpm->adjusted_critical_radius_he[i - 1] -
        regenerated_radius_he[i - 1]) /
        (regenerated_radius_he[i - 1] *
        (pVpm->adjusted_critical_radius_he[i - 1] -
        ending_radius_he));
        crush_pressure_adjust_ratio_n2 =
        ending_radius_n2 * (pVpm->adjusted_critical_radius_n2[i - 1] -
        regenerated_radius_n2[i - 1]) /
        (regenerated_radius_n2[i - 1] *
        (pVpm->adjusted_critical_radius_n2[i - 1] -
        ending_radius_n2));
        adj_crush_pressure_he_pascals =
        crushing_pressure_pascals_he * crush_pressure_adjust_ratio_he;
        adj_crush_pressure_n2_pascals =
        crushing_pressure_pascals_n2 * crush_pressure_adjust_ratio_n2;
        pVpm->adjusted_crushing_pressure_he[i - 1] =
        adj_crush_pressure_he_pascals / 101325.0f * UNITS_FACTOR;
        pVpm->adjusted_crushing_pressure_n2[i - 1] =
        adj_crush_pressure_n2_pascals / 101325.0f * UNITS_FACTOR;
    }
    return 0;
} /* nuclear_regeneration */

/* =============================================================================== */
/*     SUBROUTINE CALC_INITIAL_ALLOWABLE_GRADIENT */
/*     Purpose: This subprogram calculates the initial allowable gradients for */
/*     helium and nitrogren in each compartment.  These are the gradients that */
/*     will be used to set the deco ceiling on the first pass through the deco */
/*     loop.  If the Critical Volume Algorithm is set to "off", then these */
/*     gradients will determine the final deco schedule.  Otherwise, if the */
/*     Critical Volume Algorithm is set to "on", these gradients will be further */
/*     "relaxed" by the Critical Volume Algorithm subroutine.  The initial */
/*     allowable gradients are referred to as "PssMin" in the papers by Yount */
/*     and colleauges, i.e., the minimum supersaturation pressure gradients */
/*     that will probe bubble formation in the VPM nuclei that started with the */
/*     designated minimum initial radius (critical radius). */

/*     The initial allowable gradients are computed directly from the */
/*     "regenerated" radii after the Nuclear Regeneration subroutine.  These */
/*     gradients are tracked separately for helium and nitrogen. */
/* =============================================================================== */

static int calc_initial_allowable_gradient()
{
    float initial_allowable_grad_n2_pa,
    initial_allowable_grad_he_pa;
    short i;

    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /*     The initial allowable gradients are computed in Pascals and then converted */
    /*     to the diving pressure units.  Two different sets of arrays are used to */
    /*     save the calculations - Initial Allowable Gradients and Allowable */
    /*     Gradients.  The Allowable Gradients are assigned the values from Initial */
    /*     Allowable Gradients however the Allowable Gradients can be changed later */
    /*     by the Critical Volume subroutine.  The values for the Initial Allowable */
    /*     Gradients are saved in a global array for later use by both the Critical */
    /*     Volume subroutine and the VPM Repetitive Algorithm subroutine. */
    /* =============================================================================== */

    for (i = 1; i <= 16; ++i)
    {
        initial_allowable_grad_n2_pa =
            SURFACE_TENSION_GAMMA * 2.0f *
            (SKIN_COMPRESSION_GAMMAC - SURFACE_TENSION_GAMMA) /
            (regenerated_radius_n2[i - 1] * SKIN_COMPRESSION_GAMMAC);

        initial_allowable_grad_he_pa =
            SURFACE_TENSION_GAMMA * 2.0f *
            (SKIN_COMPRESSION_GAMMAC - SURFACE_TENSION_GAMMA) /
            (regenerated_radius_he[i - 1] * SKIN_COMPRESSION_GAMMAC);

        pVpm->initial_allowable_gradient_n2[i - 1] =
            initial_allowable_grad_n2_pa / 101325.0f * UNITS_FACTOR;

        pVpm->initial_allowable_gradient_he[i - 1] =
            initial_allowable_grad_he_pa / 101325.0f * UNITS_FACTOR;

        allowable_gradient_he[i - 1] =
            pVpm->initial_allowable_gradient_he[i - 1];

        allowable_gradient_n2[i - 1] =
            pVpm->initial_allowable_gradient_n2[i - 1];
    }
    return 0;
} /* calc_initial_allowable_gradient */

/* =============================================================================== */
/*     SUBROUTINE CALC_DECO_CEILING */
/*     Purpose: This subprogram calculates the deco ceiling (the safe ascent */
/*     depth) in each compartment, based on the allowable gradients, and then */
/*     finds the deepest deco ceiling across all compartments.  This deepest */
/*     value (Deco Ceiling Depth) is then used by the Decompression Stop */
/*     subroutine to determine the actual deco schedule. */
/* =============================================================================== */

static int calc_deco_ceiling(float *deco_ceiling_depth,_Bool fallowable)
{
    /* System generated locals */
    float r1, r2;
    /* Local variables */
    float weighted_allowable_gradient;
    short i;
    float compartment_deco_ceiling[16],
    gas_loading,
    tolerated_ambient_pressure;
    float gradient_he, gradient_n2;

    if(!vpm_b)
        fallowable = true;
    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /*     Since there are two sets of allowable gradients being tracked, one for */
    /*     helium and one for nitrogen, a "weighted allowable gradient" must be */
    /*     computed each time based on the proportions of helium and nitrogen in */
    /*     each compartment.  This proportioning follows the methodology of */
    /*     Buhlmann/Keller.  If there is no helium and nitrogen in the compartment, */
    /*     such as after extended periods of oxygen breathing, then the minimum value */
    /*     across both gases will be used.  It is important to note that if a */
    /*     compartment is empty of helium and nitrogen, then the weighted allowable */
    /*     gradient formula cannot be used since it will result in division by zero. */
    /* =============================================================================== */

    for (i = 1; i <= 16; ++i)
    {

        // abfrage raus und pointer stattdessen
        if(fallowable){
            gradient_he = allowable_gradient_he[i-1];
            gradient_n2 = allowable_gradient_n2[i-1];
        }
        else{
            gradient_he = deco_gradient_he[i-1];
            gradient_n2 = deco_gradient_n2[i-1];
        }

        gas_loading =	helium_pressure[i - 1] + nitrogen_pressure[i - 1];

        if (gas_loading > 0)
        {
            weighted_allowable_gradient =
            (gradient_he * helium_pressure[i - 1] +
            gradient_n2 * nitrogen_pressure[i - 1]) /
            (helium_pressure[i - 1] + nitrogen_pressure[i - 1]);

            tolerated_ambient_pressure =
            gas_loading +
            CONSTANT_PRESSURE_OTHER_GASES -
            weighted_allowable_gradient;
        }
        else
        {
            /* Computing MIN */
            r1 = gradient_he;
            r2 = gradient_n2;
            weighted_allowable_gradient = fminf(r1,r2);

            tolerated_ambient_pressure =
            CONSTANT_PRESSURE_OTHER_GASES - weighted_allowable_gradient;
        }

        /* =============================================================================== */
        /*     The tolerated ambient pressure cannot be less than zero absolute, i.e., */
        /*     the vacuum of outer space! */
        /* =============================================================================== */

        if (tolerated_ambient_pressure < 0.0) {
            tolerated_ambient_pressure = 0.0;
        }
        compartment_deco_ceiling[i - 1] =
        tolerated_ambient_pressure - barometric_pressure;
    }

    /* =============================================================================== */
    /*     The Deco Ceiling Depth is computed in a loop after all of the individual */
    /*     compartment deco ceilings have been calculated.  It is important that the */
    /*     Deco Ceiling Depth (max deco ceiling across all compartments) only be */
    /*     extracted from the compartment values and not be compared against some */
    /*     initialization value.  For example, if MAX(Deco_Ceiling_Depth . .) was */
    /*     compared against zero, this could cause a program lockup because sometimes */
    /*     the Deco Ceiling Depth needs to be negative (but not less than zero */
    /*     absolute ambient pressure) in order to decompress to the last stop at zero */
    /*     depth. */
    /* =============================================================================== */

    *deco_ceiling_depth = compartment_deco_ceiling[0];
    for (i = 2; i <= 16; ++i)
    {
        /* Computing MAX */
        r1 = *deco_ceiling_depth;
        r2 = compartment_deco_ceiling[i - 1];
        *deco_ceiling_depth = fmaxf(r1,r2);
    }
    return 0;
} /* calc_deco_ceiling */



/* =============================================================================== */
/*     SUBROUTINE CALC_MAX_ACTUAL_GRADIENT */
/*     Purpose: This subprogram calculates the actual supersaturation gradient */
/*     obtained in each compartment as a result of the ascent profile during */
/*     decompression.  Similar to the concept with crushing pressure, the */
/*     supersaturation gradients are not cumulative over a multi-level, staged */
/*     ascent.  Rather, it will be the maximum value obtained in any one discrete */
/*     step of the overall ascent.  Thus, the program must compute and store the */
/*     maximum actual gradient for each compartment that was obtained across all */
/*     steps of the ascent profile.  This subroutine is invoked on the last pass */
/*     through the deco stop loop block when the final deco schedule is being */
/*     generated. */
/*   */
/*     The max actual gradients are later used by the VPM Repetitive Algorithm to */
/*     determine if adjustments to the critical radii are required.  If the max */
/*     actual gradient did not exceed the initial alllowable gradient, then no */
/*     adjustment will be made.  However, if the max actual gradient did exceed */
/*     the intitial allowable gradient, such as permitted by the Critical Volume */
/*     Algorithm, then the critical radius will be adjusted (made larger) on the */
/*     repetitive dive to compensate for the bubbling that was allowed on the */
/*     previous dive.  The use of the max actual gradients is intended to prevent */
/*     the repetitive algorithm from being overly conservative. */
/* =============================================================================== */

static int calc_max_actual_gradient(float *deco_stop_depth)
{
    /* System generated locals */
    float r1;

    /* Local variables */
    short i;
    float compartment_gradient;

    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /*     Note: negative supersaturation gradients are meaningless for this */
    /*     application, so the values must be equal to or greater than zero. */
    /* =============================================================================== */

    for (i = 1; i <= 16; ++i)
    {
        compartment_gradient =
        helium_pressure[i - 1] +
        nitrogen_pressure[i - 1] +
        CONSTANT_PRESSURE_OTHER_GASES -
        (*deco_stop_depth + barometric_pressure);
        if (compartment_gradient <= 0.0f) {
            compartment_gradient = 0.0f;
        }
        /* Computing MAX */
        r1 = pVpm->max_actual_gradient[i - 1];
        pVpm->max_actual_gradient[i - 1] = fmaxf(r1, compartment_gradient);
    }
    return 0;
} /* calc_max_actual_gradient */

/* =============================================================================== */
/*     SUBROUTINE CALC_SURFACE_PHASE_VOLUME_TIME */
/*     Purpose: This subprogram computes the surface portion of the total phase */
/*     volume time.  This is the time factored out of the integration of */
/*     supersaturation gradient x time over the surface interval.  The VPM */
/*     considers the gradients that allow bubbles to form or to drive bubble */
/*     growth both in the water and on the surface after the dive. */

/*     This subroutine is a new development to the VPM algorithm in that it */
/*     computes the time course of supersaturation gradients on the surface */
/*     when both helium and nitrogen are present.  Refer to separate write-up */
/*     for a more detailed explanation of this algorithm. */
/* =============================================================================== */

static int calc_surface_phase_volume_time()
{
    /* Local variables */
    float decay_time_to_zero_gradient;
    short i;
    float integral_gradient_x_time,
    surface_inspired_n2_pressure;

    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /* =============================================================================== */

    surface_inspired_n2_pressure =
    (barometric_pressure - WATER_VAPOR_PRESSURE) * 0.79f;
    for (i = 1; i <= 16; ++i)
    {
        if (nitrogen_pressure[i - 1] > surface_inspired_n2_pressure)
        {
            surface_phase_volume_time[i - 1] =
            (helium_pressure[i - 1] / HELIUM_TIME_CONSTANT[i - 1] +
            (nitrogen_pressure[i - 1] - surface_inspired_n2_pressure) /
            NITROGEN_TIME_CONSTANT[i - 1]) /
            (helium_pressure[i - 1] + nitrogen_pressure[i - 1] -
            surface_inspired_n2_pressure);
        } else if (nitrogen_pressure[i - 1] <= surface_inspired_n2_pressure &&
        helium_pressure[i - 1] + nitrogen_pressure[i - 1] >= surface_inspired_n2_pressure)
        {
            decay_time_to_zero_gradient =
            1.0f / (NITROGEN_TIME_CONSTANT[i - 1] - HELIUM_TIME_CONSTANT[i - 1]) *
            log((surface_inspired_n2_pressure - nitrogen_pressure[i - 1]) /
            helium_pressure[i - 1]);
            integral_gradient_x_time =
            helium_pressure[i - 1] /
            HELIUM_TIME_CONSTANT[i - 1] *
            (1.0f - expf(-HELIUM_TIME_CONSTANT[i - 1] *
            decay_time_to_zero_gradient)) +
            (nitrogen_pressure[i - 1] - surface_inspired_n2_pressure) /
            NITROGEN_TIME_CONSTANT[i - 1] *
            (1.0f - expf(-NITROGEN_TIME_CONSTANT[i - 1] *
            decay_time_to_zero_gradient));
            surface_phase_volume_time[i - 1] =
            integral_gradient_x_time /
            (helium_pressure[i - 1] +
            nitrogen_pressure[i - 1] -
            surface_inspired_n2_pressure);
        } else {
            surface_phase_volume_time[i - 1] = 0.0f;
        }
    }
    return 0;
} /* calc_surface_phase_volume_time */

/* =============================================================================== */
/*     SUBROUTINE CRITICAL_VOLUME */
/*     Purpose: This subprogram applies the VPM Critical Volume Algorithm.  This */
/*     algorithm will compute "relaxed" gradients for helium and nitrogen based */
/*     on the setting of the Critical Volume Parameter Lambda. */
/* =============================================================================== */

static int critical_volume(float *deco_phase_volume_time)
{
    /* System generated locals */
    float r1;

    /* Local variables */
    float initial_allowable_grad_n2_pa,
    initial_allowable_grad_he_pa,
    parameter_lambda_pascals, b,
    c;
    short i;
    float new_allowable_grad_n2_pascals,
    phase_volume_time[16],
    new_allowable_grad_he_pascals,
    adj_crush_pressure_n2_pascals,
    adj_crush_pressure_he_pascals;

    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /*     Note:  Since the Critical Volume Parameter Lambda was defined in units of */
    /*     fsw-min in the original papers by Yount and colleauges, the same */
    /*     convention is retained here.  Although Lambda is adjustable only in units */
    /*     of fsw-min in the program settings (range from 6500 to 8300 with default */
    /*     7500), it will convert to the proper value in Pascals-min in this */
    /*     subroutine regardless of which diving pressure units are being used in */
    /*     the main program - feet of seawater (fsw) or meters of seawater (msw). */
    /*     The allowable gradient is computed using the quadratic formula (refer to */
    /*     separate write-up posted on the Deco List web site). */
    /* =============================================================================== */

/**
    ******************************************************************************
    * @brief		critical_volume comment by hw
    * @version 	V0.0.1
    * @date   	19-April-2014
    * @retval 	global: allowable_gradient_he[i], allowable_gradient_n2[i]
    ******************************************************************************
    */

    parameter_lambda_pascals =
    CRIT_VOLUME_PARAMETER_LAMBDA / 33.0f * 101325.0f;
    for (i = 1; i <= 16; ++i)
    {
        phase_volume_time[i - 1] =
        *deco_phase_volume_time + surface_phase_volume_time[i - 1];
    }
    for (i = 1; i <= 16; ++i)
    {

        adj_crush_pressure_he_pascals =
        pVpm->adjusted_crushing_pressure_he[i - 1] / UNITS_FACTOR * 101325.0f;

        initial_allowable_grad_he_pa =
        pVpm->initial_allowable_gradient_he[i - 1] / UNITS_FACTOR * 101325.0f;

        b = initial_allowable_grad_he_pa + parameter_lambda_pascals *
        SURFACE_TENSION_GAMMA / (
        SKIN_COMPRESSION_GAMMAC * phase_volume_time[i - 1]);

        c = SURFACE_TENSION_GAMMA * (
        SURFACE_TENSION_GAMMA * (
        parameter_lambda_pascals * adj_crush_pressure_he_pascals)) /
        (SKIN_COMPRESSION_GAMMAC *
        (SKIN_COMPRESSION_GAMMAC * phase_volume_time[i - 1]));
        /* Computing 2nd power */

        r1 = b;

        new_allowable_grad_he_pascals =
        (b + sqrtf(r1 * r1 - c * 4.0f)) / 2.0f;

        /* modify global variable */
        allowable_gradient_he[i - 1] =
        new_allowable_grad_he_pascals / 101325.0f * UNITS_FACTOR;
    }

    for (i = 1; i <= 16; ++i)
    {
        adj_crush_pressure_n2_pascals =
        pVpm->adjusted_crushing_pressure_n2[i - 1] / UNITS_FACTOR * 101325.0f;

        initial_allowable_grad_n2_pa =
        pVpm->initial_allowable_gradient_n2[i - 1] / UNITS_FACTOR * 101325.0f;

        b = initial_allowable_grad_n2_pa + parameter_lambda_pascals *
        SURFACE_TENSION_GAMMA / (
        SKIN_COMPRESSION_GAMMAC * phase_volume_time[i - 1]);

        c = SURFACE_TENSION_GAMMA *
        (SURFACE_TENSION_GAMMA *
        (parameter_lambda_pascals * adj_crush_pressure_n2_pascals)) /
        (SKIN_COMPRESSION_GAMMAC *
        (SKIN_COMPRESSION_GAMMAC * phase_volume_time[i - 1]));
        /* Computing 2nd power */

        r1 = b;

        new_allowable_grad_n2_pascals =
        (b + sqrtf(r1 * r1 - c * 4.0f)) / 2.0f;

        /* modify global variable */
        allowable_gradient_n2[i - 1] =
        new_allowable_grad_n2_pascals / 101325.0f * UNITS_FACTOR;
    }
    return 0;
} /* critical_volume */

/* =============================================================================== */
/*     SUBROUTINE CALC_START_OF_DECO_ZONE */
/*     Purpose: This subroutine uses the Bisection Method to find the depth at */
/*     which the leading compartment just enters the decompression zone. */
/*     Source:  "Numerical Recipes in Fortran 77", Cambridge University Press, */
/*     1992. */
/* =============================================================================== */

static int calc_start_of_deco_zone(float *starting_depth,
                            float *rate,
                            float *depth_start_of_deco_zone)
{
    /* Local variables */
    float last_diff_change,
    initial_helium_pressure,
    mid_range_nitrogen_pressure;
    short i, j;
    float initial_inspired_n2_pressure,
    cpt_depth_start_of_deco_zone,
    low_bound,
    initial_inspired_he_pressure,
    high_bound_nitrogen_pressure,
    nitrogen_rate,
    function_at_mid_range,
    function_at_low_bound,
    high_bound,
    mid_range_helium_pressure,
    mid_range_time,
    starting_ambient_pressure,
    initial_nitrogen_pressure,
    function_at_high_bound;

    float time_to_start_of_deco_zone,
    high_bound_helium_pressure,
    helium_rate,
    differential_change;
    float fraction_helium_begin;
    float fraction_helium_end;
    float fraction_nitrogen_begin;
    float fraction_nitrogen_end;
    float ending_ambient_pressure;
    float time_test;


    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /*     First initialize some variables */
    /* =============================================================================== */

    *depth_start_of_deco_zone = 0.0f;
    starting_ambient_pressure =	*starting_depth + barometric_pressure;

    //>>>>>>>>>>>>>>>>>>>>
    //Test depth to calculate helium_rate and nitrogen_rate
    ending_ambient_pressure = starting_ambient_pressure/2;

    time_test = (ending_ambient_pressure - starting_ambient_pressure) / *rate;
    decom_get_inert_gases(starting_ambient_pressure / 10.0, (&pDiveSettings->decogaslist[mix_number]), &fraction_nitrogen_begin, &fraction_helium_begin );
    decom_get_inert_gases(ending_ambient_pressure   / 10.0, (&pDiveSettings->decogaslist[mix_number]), &fraction_nitrogen_end, &fraction_helium_end );
    initial_inspired_he_pressure =	(starting_ambient_pressure - WATER_VAPOR_PRESSURE) * fraction_helium_begin;
    initial_inspired_n2_pressure =	(starting_ambient_pressure - WATER_VAPOR_PRESSURE) * fraction_nitrogen_begin;
    helium_rate = ((ending_ambient_pressure  - WATER_VAPOR_PRESSURE)* fraction_helium_end - initial_inspired_he_pressure)/time_test;
    nitrogen_rate = ((ending_ambient_pressure  - WATER_VAPOR_PRESSURE)* fraction_nitrogen_end - initial_inspired_n2_pressure)/time_test;
    //>>>>>>>>>>>>>>>>>>>>>
    /*initial_inspired_he_pressure =
    (starting_ambient_pressure - water_vapor_pressure) *
    fraction_helium[mix_number - 1];
    initial_inspired_n2_pressure =
    (starting_ambient_pressure - water_vapor_pressure) *
    fraction_nitrogen[mix_number - 1];
    helium_rate = *rate * fraction_helium[mix_number - 1];
    nitrogen_rate = *rate * fraction_nitrogen[mix_number - 1];*/

    /* =============================================================================== */
    /*     ESTABLISH THE BOUNDS FOR THE ROOT SEARCH USING THE BISECTION METHOD */
    /*     AND CHECK TO MAKE SURE THAT THE ROOT WILL BE WITHIN BOUNDS.  PROCESS */
    /*     EACH COMPARTMENT INDIVIDUALLY AND FIND THE MAXIMUM DEPTH ACROSS ALL */
    /*     COMPARTMENTS (LEADING COMPARTMENT) */
    /*     In this case, we are solving for time - the time when the gas tension in */
    /*     the compartment will be equal to ambient pressure.  The low bound for time */
    /*     is set at zero and the high bound is set at the time it would take to */
    /*     ascend to zero ambient pressure (absolute).  Since the ascent rate is */
    /*     negative, a multiplier of -1.0 is used to make the time positive.  The */
    /*     desired point when gas tension equals ambient pressure is found at a time */
    /*     somewhere between these endpoints.  The algorithm checks to make sure that */
    /*     the solution lies in between these bounds by first computing the low bound */
    /*     and high bound function values. */
    /* =============================================================================== */

    low_bound = 0.0;
    high_bound = starting_ambient_pressure / *rate * -1.0f;
    for (i = 1; i <= 16; ++i)
    {
        initial_helium_pressure = helium_pressure[i - 1];
        initial_nitrogen_pressure = nitrogen_pressure[i - 1];
        function_at_low_bound =
        initial_helium_pressure +
        initial_nitrogen_pressure +
        CONSTANT_PRESSURE_OTHER_GASES -
        starting_ambient_pressure;
        high_bound_helium_pressure =
        schreiner_equation__2(&initial_inspired_he_pressure,
        &helium_rate,
        &high_bound,
        &HELIUM_TIME_CONSTANT[i - 1],
        &initial_helium_pressure);
        high_bound_nitrogen_pressure =
        schreiner_equation__2(&initial_inspired_n2_pressure,
        &nitrogen_rate,
        &high_bound,
        &NITROGEN_TIME_CONSTANT[i - 1],
        &initial_nitrogen_pressure);
        function_at_high_bound = high_bound_helium_pressure +
        high_bound_nitrogen_pressure +
        CONSTANT_PRESSURE_OTHER_GASES;
        if (function_at_high_bound * function_at_low_bound >= 0.0f)
        {
            printf("\nERROR! ROOT IS NOT WITHIN BRACKETS");
        }

        /* =============================================================================== */
        /*     APPLY THE BISECTION METHOD IN SEVERAL ITERATIONS UNTIL A SOLUTION WITH */
        /*     THE DESIRED ACCURACY IS FOUND */
        /*     Note: the program allows for up to 100 iterations.  Normally an exit will */
        /*     be made from the loop well before that number.  If, for some reason, the */
        /*     program exceeds 100 iterations, there will be a pause to alert the user. */
        /* =============================================================================== */

        if (function_at_low_bound < 0.0f)
        {
            time_to_start_of_deco_zone = low_bound;
            differential_change = high_bound - low_bound;
        } else {
            time_to_start_of_deco_zone = high_bound;
            differential_change = low_bound - high_bound;
        }
        for (j = 1; j <= 100; ++j)
        {
            last_diff_change = differential_change;
            differential_change = last_diff_change * 0.5f;
            mid_range_time =
            time_to_start_of_deco_zone +
            differential_change;
            mid_range_helium_pressure =
            schreiner_equation__2(&initial_inspired_he_pressure,
            &helium_rate,
            &mid_range_time,
            &HELIUM_TIME_CONSTANT[i - 1],
            &initial_helium_pressure);
            mid_range_nitrogen_pressure =
            schreiner_equation__2(&initial_inspired_n2_pressure,
            &nitrogen_rate,
            &mid_range_time,
            &NITROGEN_TIME_CONSTANT[i - 1],
            &initial_nitrogen_pressure);
            function_at_mid_range =
            mid_range_helium_pressure +
            mid_range_nitrogen_pressure +
            CONSTANT_PRESSURE_OTHER_GASES -
            (starting_ambient_pressure + *rate * mid_range_time);
            if (function_at_mid_range <= 0.0f) {
                time_to_start_of_deco_zone = mid_range_time;
            }
            if( fabs(differential_change) < 0.001f
             || function_at_mid_range == 0.0f)
            {
                goto L170;
            }
            /* L150: */
        }
        printf("\nERROR! ROOT SEARCH EXCEEDED MAXIMUM ITERATIONS");
        //pause();

        /* =============================================================================== */
        /*     When a solution with the desired accuracy is found, the program jumps out */
        /*     of the loop to Line 170 and assigns the solution value for the individual */
        /*     compartment. */
        /* =============================================================================== */

        L170:
        cpt_depth_start_of_deco_zone =
        starting_ambient_pressure +
        *rate * time_to_start_of_deco_zone -
        barometric_pressure;

        /* =============================================================================== */
        /*     The overall solution will be the compartment with the maximum depth where */
        /*     gas tension equals ambient pressure (leading compartment). */
        /* =============================================================================== */

        *depth_start_of_deco_zone =
        fmaxf(*depth_start_of_deco_zone, cpt_depth_start_of_deco_zone);
        /* L200: */
    }
    return 0;
} /* calc_start_of_deco_zone */

/* =============================================================================== */
/*     SUBROUTINE PROJECTED_ASCENT */
/*     Purpose: This subprogram performs a simulated ascent outside of the main */
/*     program to ensure that a deco ceiling will not be violated due to unusual */
/*     gas loading during ascent (on-gassing).  If the deco ceiling is violated, */
/*     the stop depth will be adjusted deeper by the step size until a safe */
/*     ascent can be made. */
/* =============================================================================== */

static int projected_ascent(float *starting_depth,
                     float *rate,
                     float *deco_stop_depth,
                     float *step_size)
{
    /* Local variables */
    float weighted_allowable_gradient,
    ending_ambient_pressure,
    temp_gas_loading[16];
    int i;
    float allowable_gas_loading[16];
    float temp_nitrogen_pressure[16];
    float temp_helium_pressure[16];
    float run_time_save = 0;

    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /* =============================================================================== */


    L665:
    ending_ambient_pressure = *deco_stop_depth + barometric_pressure;
    for (i = 1; i <= 16; ++i) {
        temp_helium_pressure[i - 1] = helium_pressure[i - 1];
        temp_nitrogen_pressure[i - 1] = nitrogen_pressure[i - 1];
    }
    run_time_save = run_time;
    gas_loadings_ascent_descen(temp_helium_pressure, temp_nitrogen_pressure, *starting_depth,*deco_stop_depth,*rate,true);
    run_time = run_time_save;

    for (i = 1; i <= 16; ++i)
    {
        temp_gas_loading[i - 1] =
        temp_helium_pressure[i - 1] +
        temp_nitrogen_pressure[i - 1];
        if (temp_gas_loading[i - 1] > 0.0f)
        {
            weighted_allowable_gradient =
            (allowable_gradient_he[i - 1] *
            temp_helium_pressure[i - 1] +
            allowable_gradient_n2[i - 1] *
            temp_nitrogen_pressure[i - 1]) / temp_gas_loading[i - 1];
        } else {
            /* Computing MIN */
            weighted_allowable_gradient = fminf(allowable_gradient_he[i - 1],allowable_gradient_n2[i - 1]);
        }
        allowable_gas_loading[i - 1] =
        ending_ambient_pressure +
        weighted_allowable_gradient -
        CONSTANT_PRESSURE_OTHER_GASES;
        /* L670: */
    }
    for (i = 1; i <= 16; ++i) {
        if (temp_gas_loading[i - 1] > allowable_gas_loading[i - 1]) {
            *deco_stop_depth += *step_size;
            goto L665;
        }
        /* L671: */
    }
    return 0;
} /* projected_ascent */

/* =============================================================================== */
/*     SUBROUTINE DECOMPRESSION_STOP */
/*     Purpose: This subprogram calculates the required time at each */
/*     decompression stop. */
/* =============================================================================== */

static void decompression_stop(float *deco_stop_depth,
                        float *step_size,
                        _Bool final_deco_calculation)
{
    /* Local variables */
    float inspired_nitrogen_pressure;
//	short last_segment_number;
//	float weighted_allowable_gradient;
    float initial_helium_pressure[16];
 /* by hw */
    float initial_CNS = gCNS_VPM;

    //static float time_counter;
    short i;
    float ambient_pressure;
    float inspired_helium_pressure,
    next_stop;
    //last_run_time,
    //temp_segment_time;

    float deco_ceiling_depth,
    initial_nitrogen_pressure[16];
    //round_up_operation;
    float fraction_helium_begin;
    float fraction_nitrogen_begin;
    int count = 0;
    _Bool buehlmann_wait = false;
    float tissue_He_saturation[16];
    float tissue_N2_saturation[16];
    float vpm_buehlmann_safety_gradient = 1.0f - (((float)pDiveSettings->vpm_conservatism) / 40.0);
    /* loop */
    /* =============================================================================== */
    /*     CALCULATIONS */
    /* =============================================================================== */

    segment_time = 0;
//	temp_segment_time = segment_time;
    ambient_pressure = *deco_stop_depth + barometric_pressure;
    //ending_ambient_pressure = ambient_pressure;
    decom_get_inert_gases(ambient_pressure / 10, (&pDiveSettings->decogaslist[mix_number]), &fraction_nitrogen_begin, &fraction_helium_begin );

    if(*deco_stop_depth == (pDiveSettings->last_stop_depth_bar * 10.0))
        next_stop = 0;
    else
    {
        next_stop = *deco_stop_depth - *step_size;
        next_stop = fmaxf(next_stop, pDiveSettings->last_stop_depth_bar * 10.0);
    }

    inspired_helium_pressure =
    (ambient_pressure - WATER_VAPOR_PRESSURE) * fraction_helium_begin;
    inspired_nitrogen_pressure =
    (ambient_pressure - WATER_VAPOR_PRESSURE) * fraction_nitrogen_begin;

    /* =============================================================================== */
    /*     Check to make sure that program won't lock up if unable to decompress */
    /*     to the next stop.  If so, write error message and terminate program. */
    /* =============================================================================== */

    //deco_ceiling_depth = next_stop +1;	//deco_ceiling_depth = next_stop + 1;
    if(!vpm_violates_buehlmann)
    {
        calc_deco_ceiling(&deco_ceiling_depth, false); //weg, weil auf jeden Fall schleife fÃ¼r safety und so konservativer
    }
    else
    {
        deco_ceiling_depth = next_stop + 1.0;
    }
        if(deco_ceiling_depth > next_stop)
        {
            while (deco_ceiling_depth > next_stop)
            {

                segment_time  +=  60.0;
                if(segment_time >= 999.0 )
                {
                    segment_time = 999.0 ;
                    run_time += segment_time;
                    return;
                }
                //goto L700;
                initial_CNS = gCNS_VPM;
                decom_oxygen_calculate_cns_exposure(60*60,&pDiveSettings->decogaslist[mix_number],ambient_pressure/10,&gCNS_VPM);
                for (i = 0; i < 16; i++)
                {
                    initial_helium_pressure[i] = helium_pressure[i];
                    initial_nitrogen_pressure[i] = nitrogen_pressure[i];
                    helium_pressure[i] 		+=	(inspired_helium_pressure - helium_pressure[i]) 		*	float_buehlmann_He_factor_expositon_one_hour[i];
                    nitrogen_pressure[i]	+= 	(inspired_nitrogen_pressure - nitrogen_pressure[i]) *	float_buehlmann_N2_factor_expositon_one_hour[i];
                }
                calc_deco_ceiling(&deco_ceiling_depth, false);
            }
            if(deco_ceiling_depth < next_stop)
            {
                segment_time  -=  60.0;
                gCNS_VPM = initial_CNS;
                for (i = 0; i < 16; i++)
                {
                    helium_pressure[i] = initial_helium_pressure[i];
                    nitrogen_pressure[i] = initial_nitrogen_pressure[i];
                }
                deco_ceiling_depth = next_stop +1;
            }
            count = 0;
            while (deco_ceiling_depth > next_stop && count < 13)
            {
                count++;
                segment_time  +=  5;
                //goto L700;
                initial_CNS = gCNS_VPM;
                decom_oxygen_calculate_cns_exposure(60*5,&pDiveSettings->decogaslist[mix_number],ambient_pressure/10,&gCNS_VPM);
                for (i = 0; i < 16; i++)
                {
                    initial_helium_pressure[i] = helium_pressure[i];
                    initial_nitrogen_pressure[i] = nitrogen_pressure[i];
                    helium_pressure[i] 		+=	(inspired_helium_pressure - helium_pressure[i]) 		*	float_buehlmann_He_factor_expositon_five_minutes[i];
                    nitrogen_pressure[i]	+= 	(inspired_nitrogen_pressure - nitrogen_pressure[i]) *	float_buehlmann_N2_factor_expositon_five_minutes[i];
                }
                calc_deco_ceiling(&deco_ceiling_depth, false);
            }
            if(deco_ceiling_depth < next_stop)
            {
                segment_time  -=  5;
                gCNS_VPM = initial_CNS;
                for (i = 0; i < 16; i++) {
                    helium_pressure[i] = initial_helium_pressure[i];
                    nitrogen_pressure[i] = initial_nitrogen_pressure[i];
                }
                deco_ceiling_depth = next_stop +1;
            }
            buehlmann_wait = false;
            while (buehlmann_wait || (deco_ceiling_depth > next_stop))
            {
                //time_counter = temp_segment_time;
                segment_time  +=  1.0;

                if(segment_time >= 999.0 )
                {
                    segment_time = 999.0 ;
                    run_time += segment_time;
                    return;
                }
                //goto L700;
                initial_CNS = gCNS_VPM;
                decom_oxygen_calculate_cns_exposure(60*1,&pDiveSettings->decogaslist[mix_number],ambient_pressure/10.0, &gCNS_VPM);
                for (i = 0; i < 16; i++)
                {
                    initial_helium_pressure[i] = helium_pressure[i];
                    initial_nitrogen_pressure[i] = nitrogen_pressure[i];
                    helium_pressure[i] 		+=	(inspired_helium_pressure - helium_pressure[i]) 		*	float_buehlmann_He_factor_expositon_one_minute[i];
                    nitrogen_pressure[i]	+= 	(inspired_nitrogen_pressure - nitrogen_pressure[i]) *	float_buehlmann_N2_factor_expositon_one_minute[i];
                }
                if(!buehlmann_wait)
                    calc_deco_ceiling(&deco_ceiling_depth, false);

                if(buehlmannSafety && final_deco_calculation && !(deco_ceiling_depth > next_stop))
                {
                    for (i = 0; i < 16; i++)
                    {
                        tissue_He_saturation[i] = helium_pressure[i] / 10;
                        tissue_N2_saturation[i] = nitrogen_pressure[i] / 10;
                    }
                    if( (fabsf(nitrogen_pressure[15] - inspired_nitrogen_pressure) < 0.00001f) && (fabsf(helium_pressure[15] - inspired_helium_pressure) < 0.00001f)
                     && (fabsf(nitrogen_pressure[0] - inspired_nitrogen_pressure) < 0.00001f) && (fabsf(helium_pressure[0] - inspired_helium_pressure) < 0.00001f))
                    {
                         buehlmann_wait_exceeded = true;
                        break;
                    }

                    if(decom_tissue_test_tolerance(tissue_N2_saturation, tissue_He_saturation, vpm_buehlmann_safety_gradient, (next_stop / 10.0f) + pInput->pressure_surface_bar))
                        break;

                    buehlmann_wait = true;
                }
            }
            if(buehlmann_wait)
            {
                vpm_violates_buehlmann = true;
            }
            if(!buehlmann_wait)
            {
                if(deco_ceiling_depth < next_stop)
                {
                    segment_time  -=  1;
                    gCNS_VPM = initial_CNS;
                    for (i = 0; i < 16; i++) {
                        helium_pressure[i] = initial_helium_pressure[i];
                        nitrogen_pressure[i] = initial_nitrogen_pressure[i];
                    }
                    deco_ceiling_depth = next_stop +1;
                }
                while (deco_ceiling_depth > next_stop)
                {
                    //time_counter = temp_segment_time;
                    segment_time  +=  (float) 1.0f / 3.0f;
                    //goto L700;
                    initial_CNS = gCNS_VPM;
                    decom_oxygen_calculate_cns_exposure(20,&pDiveSettings->decogaslist[mix_number],ambient_pressure/10,&gCNS_VPM);
                    for (i = 0; i < 16; i++)
                    {
                        helium_pressure[i] 		+=	(inspired_helium_pressure - helium_pressure[i]) 		*	float_buehlmann_He_factor_expositon_20_seconds[i];
                        nitrogen_pressure[i]	+= 	(inspired_nitrogen_pressure - nitrogen_pressure[i]) *	float_buehlmann_N2_factor_expositon_20_seconds[i];
                    }
                    calc_deco_ceiling(&deco_ceiling_depth, false);
                }
            }
    }

/*float pressure_save =dive_data.pressure;
dive_data.pressure = ambient_pressure/10;
tissues_exposure_stage(st_deco_test,(int)(segment_time * 60), &dive_data, &gaslist);
dive_data.pressure = pressure_save;*/
    run_time += segment_time;
    return;
} /* decompression_stop */

/* =============================================================================== */
// SUROUTINE BOYLES_LAW_COMPENSATION
//     Purpose: This subprogram calculates the reduction in allowable gradients
//    with decreasing ambient pressure during the decompression profile based
//    on Boyle's Law considerations.
//===============================================================================
static void BOYLES_LAW_COMPENSATION (float* First_Stop_Depth,
                              float*  Deco_Stop_Depth,
                              float* Step_Size)
{
    short i;

    float Next_Stop;
    float Ambient_Pressure_First_Stop, Ambient_Pressure_Next_Stop;
    float Amb_Press_First_Stop_Pascals, Amb_Press_Next_Stop_Pascals;
    float A, B, C, Low_Bound, High_Bound, Ending_Radius;
    float Deco_Gradient_Pascals;
    float Allow_Grad_First_Stop_He_Pa, Radius_First_Stop_He;
    float Allow_Grad_First_Stop_N2_Pa, Radius_First_Stop_N2;

    //===============================================================================
    //     LO//AL ARRAYS
    //===============================================================================
//	float Radius1_He[16], Radius2_He[16];
//	float Radius1_N2[16], Radius2_N2[16];
    float root_factor;

    //===============================================================================
    //     CALCULATIONS
    //===============================================================================
    Next_Stop = *Deco_Stop_Depth - *Step_Size;

    Ambient_Pressure_First_Stop = *First_Stop_Depth +
                                 barometric_pressure;

    Ambient_Pressure_Next_Stop = Next_Stop + barometric_pressure;

    Amb_Press_First_Stop_Pascals =  (Ambient_Pressure_First_Stop/UNITS_FACTOR) * 101325.0f;

    Amb_Press_Next_Stop_Pascals =
            (Ambient_Pressure_Next_Stop/UNITS_FACTOR) * 101325.0f;
    root_factor = powf(Amb_Press_First_Stop_Pascals/Amb_Press_Next_Stop_Pascals,1.0f / 3.0f);

    for( i = 0; i < 16;i++)
    {
            Allow_Grad_First_Stop_He_Pa =
                        (allowable_gradient_he[i]/UNITS_FACTOR) * 101325.0f;

            Radius_First_Stop_He = (2.0f * SURFACE_TENSION_GAMMA) /
                                    Allow_Grad_First_Stop_He_Pa;

//			Radius1_He[i] = Radius_First_Stop_He;
            A = Amb_Press_Next_Stop_Pascals;
            B = -2.0f * SURFACE_TENSION_GAMMA;
            C = (Amb_Press_First_Stop_Pascals + (2.0f * SURFACE_TENSION_GAMMA)/
                     Radius_First_Stop_He)* Radius_First_Stop_He*
                     (Radius_First_Stop_He*(Radius_First_Stop_He));
            Low_Bound = Radius_First_Stop_He;
            High_Bound = Radius_First_Stop_He * root_factor;
                        //*pow(Amb_Press_First_Stop_Pascals/Amb_Press_Next_Stop_Pascals,1.0/3.0);
                    //*(Amb_Press_First_Stop_Pascals/Amb_Press_Next_Stop_Pascals)**(1.0/3.0);

            radius_root_finder(&A,&B,&C, &Low_Bound, &High_Bound,
                                             &Ending_Radius);

//			Radius2_He[i] = Ending_Radius;
            Deco_Gradient_Pascals = (2.0f * SURFACE_TENSION_GAMMA) /
                                    Ending_Radius;

            deco_gradient_he[i] = (Deco_Gradient_Pascals / 101325.0f)*
                                     UNITS_FACTOR;

    }

    for( i = 0; i < 16;i++)
    {
        Allow_Grad_First_Stop_N2_Pa =
                             (allowable_gradient_n2[i]/UNITS_FACTOR) * 101325.0f;

        Radius_First_Stop_N2 = (2.0f * SURFACE_TENSION_GAMMA) /
                                                         Allow_Grad_First_Stop_N2_Pa;

//		Radius1_N2[i] = Radius_First_Stop_N2;
        A = Amb_Press_Next_Stop_Pascals;
        B = -2.0f * SURFACE_TENSION_GAMMA;
        C = (Amb_Press_First_Stop_Pascals + (2.0f * SURFACE_TENSION_GAMMA)/
                        Radius_First_Stop_N2)* Radius_First_Stop_N2*
                        (Radius_First_Stop_N2*(Radius_First_Stop_N2));
        Low_Bound = Radius_First_Stop_N2;
        High_Bound = Radius_First_Stop_N2* root_factor;//pow(Amb_Press_First_Stop_Pascals/Amb_Press_Next_Stop_Pascals,1.0/3.0);

        //High_Bound = Radius_First_Stop_N2*exp(log(Amb_Press_First_Stop_Pascals/Amb_Press_Next_Stop_Pascals)/3);
        radius_root_finder(&A,&B,&C, &Low_Bound, &High_Bound,
                                                                        &Ending_Radius);

//		Radius2_N2[i] = Ending_Radius;
        Deco_Gradient_Pascals = (2.0f * SURFACE_TENSION_GAMMA) /
                                                         Ending_Radius;

        deco_gradient_n2[i] = (Deco_Gradient_Pascals / 101325.0f)*
                                                        UNITS_FACTOR;
    }
}

/* =============================================================================== */
//	vpm_calc_ndl
//     Purpose: This function computes NDL (time where no decostops are needed)
//===============================================================================
#define MAX_NDL	240

static int vpm_calc_ndl(void)
{
    static float future_helium_pressure[16];
    static float future_nitrogen_pressure[16];
    static int temp_segment_time;
    static int mix_number;
    static float inspired_helium_pressure;
    static float inspired_nitrogen_pressure;

    float previous_helium_pressure[16];
    float previous_nitrogen_pressure[16];
    float ambient_pressure;
    float fraction_helium_begin;
    float fraction_nitrogen_begin;
    int i = 0;
    int count = 0;
    int status = CALC_END;

        for(i = 0; i < 16;i++)
        {
            future_helium_pressure[i] =  pInput->tissue_helium_bar[i] * 10.0;//tissue_He_saturation[st_dive][i] * 10;
            future_nitrogen_pressure[i] = pInput->tissue_nitrogen_bar[i] * 10.0;
        }
        temp_segment_time = 0;

        mix_number = 0;
        ambient_pressure = pInput->pressure_ambient_bar  * 10.0;
        decom_get_inert_gases( ambient_pressure / 10.0,  (&pDiveSettings->decogaslist[mix_number]) , &fraction_nitrogen_begin, &fraction_helium_begin );
        inspired_helium_pressure =(ambient_pressure - WATER_VAPOR_PRESSURE) * fraction_helium_begin;
        inspired_nitrogen_pressure =(ambient_pressure - WATER_VAPOR_PRESSURE) *fraction_nitrogen_begin;

            status = CALC_END;
            while (status == CALC_END)
            {
                count++;
                temp_segment_time  +=  60;
                if(temp_segment_time >= MAX_NDL)
                {
                    pDecoInfo->output_ndl_seconds = temp_segment_time * 60;
                    return CALC_NDL;
                }
                run_time += 60;
                //goto L700;
                for (i = 1; i <= 16; ++i) {
                    previous_helium_pressure[i-1] = future_helium_pressure[i - 1];
                    previous_nitrogen_pressure[i - 1] = future_nitrogen_pressure[i - 1];
                    future_helium_pressure[i - 1] = future_helium_pressure[i - 1] + (inspired_helium_pressure - future_helium_pressure[i - 1]) * float_buehlmann_He_factor_expositon_one_hour[i-1];
                    future_nitrogen_pressure[i - 1] = future_nitrogen_pressure[i - 1] + (inspired_nitrogen_pressure - future_nitrogen_pressure[i - 1]) * float_buehlmann_N2_factor_expositon_one_hour[i-1];
                    helium_pressure[i - 1] = future_helium_pressure[i - 1];
                    nitrogen_pressure[i - 1] = future_nitrogen_pressure[i - 1];
                }
                vpm_calc_deco();
                while((status = vpm_calc_critcal_volume(true,true)) == CALC_CRITICAL);

            }

            temp_segment_time  -=  60;
            run_time -= 60;
            for (i = 1; i <= 16; ++i)
            {
                future_helium_pressure[i - 1] = previous_helium_pressure[i-1];
                future_nitrogen_pressure[i - 1] = previous_nitrogen_pressure[i - 1];
            }

        status = CALC_END;
        if(temp_segment_time < 60)
            nullzeit_unter60 = true;

        while (status == CALC_END)
        {
            temp_segment_time  +=  5;
            if(temp_segment_time >= MAX_NDL)
            {
                pDecoInfo->output_ndl_seconds = temp_segment_time * 60;
                return CALC_NDL;
            }
            if(nullzeit_unter60 && temp_segment_time > 60)
            {
                nullzeit_unter60 = false;
                return CALC_NDL;
            }
            run_time += 5;
            //goto L700;
            for (i = 1; i <= 16; ++i) {
                previous_helium_pressure[i-1] = future_helium_pressure[i - 1];
                previous_nitrogen_pressure[i - 1] = future_nitrogen_pressure[i - 1];
                future_helium_pressure[i - 1] = future_helium_pressure[i - 1] + (inspired_helium_pressure - future_helium_pressure[i - 1]) * float_buehlmann_He_factor_expositon_five_minutes[i-1];
                future_nitrogen_pressure[i - 1] = future_nitrogen_pressure[i - 1] + (inspired_nitrogen_pressure - future_nitrogen_pressure[i - 1]) * float_buehlmann_N2_factor_expositon_five_minutes[i-1];
                helium_pressure[i - 1] = future_helium_pressure[i - 1];
                nitrogen_pressure[i - 1] = future_nitrogen_pressure[i - 1];
            }
            vpm_calc_deco();
            while((status =vpm_calc_critcal_volume(true,true)) == CALC_CRITICAL);
        }
        temp_segment_time  -=  5;
        run_time -= 5;
        for (i = 1; i <= 16; ++i) {
            future_helium_pressure[i - 1] = previous_helium_pressure[i-1];
            future_nitrogen_pressure[i - 1] = previous_nitrogen_pressure[i - 1];
        }
        status = CALC_END;

    if(temp_segment_time <= 20)
    {
        while (status == CALC_END)
        {
            temp_segment_time  +=  minimum_deco_stop_time;
            run_time += minimum_deco_stop_time;
            //goto L700;
            for (i = 1; i <= 16; ++i) {
                future_helium_pressure[i - 1] = future_helium_pressure[i - 1] + (inspired_helium_pressure - future_helium_pressure[i - 1]) *	float_buehlmann_He_factor_expositon_one_minute[i-1];
                future_nitrogen_pressure[i - 1] = future_nitrogen_pressure[i - 1] + (inspired_nitrogen_pressure - future_nitrogen_pressure[i - 1]) * float_buehlmann_N2_factor_expositon_one_minute[i-1];
                helium_pressure[i - 1] = future_helium_pressure[i - 1];
                nitrogen_pressure[i - 1] =future_nitrogen_pressure[i - 1];

            }
            vpm_calc_deco();
            while((status =vpm_calc_critcal_volume(true,true)) == CALC_CRITICAL);

        }
    }
    else
        temp_segment_time += 5;
    pDecoInfo->output_ndl_seconds = temp_segment_time * 60;
    if(temp_segment_time > 1)
        return CALC_NDL;
    else
        return CALC_BEGIN;
}
