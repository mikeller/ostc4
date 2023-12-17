/*	getrennte Gase f�r die verschiedenen Modi
		um Gaswechsel Eintr�ge zu vereinfachen
		das heisst:
		oc == bailout in cc mode
*/

/* Konvention:
float extExample_variable_can_be_used_with_extern;
*/

#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "buehlmann.h"
#include "decom.h"

extern const float buehlmann_N2_a[];
extern const float buehlmann_N2_b[];
extern const float buehlmann_He_a[];
extern const float buehlmann_He_b[];

typedef struct
{
	float depth;
	int id;
} SStop;

#define DECO_STOPS_MAX_TTS_CALCULATON_IN_SECONDS 59940 // 999 minuten; before: 18000 // 5(h) * 60(min) * 60 sec = 18000 sec
#define DECO_STOPS_MAX_TTS_FOR_EVERY_SECOND_CALC_IN_SECONDS 7200
#define NINETY_NINE_MINUTES_IN_SECONDS 59940

# define PRESSURE_TEN_METER 1.0f
# define PRESSURE_THREE_METER 0.333334f
# define PRESSURE_150_CM 0.15f
# define PRESSURE_HALF_METER 0.05f

static void buehlmann_backup_and_restore(_Bool backup_restore_otherwise);
static float tissue_tolerance(void);
static void ambient_bar_to_deco_stop_depth_bar(SDiveSettings *pDiveSettings, float ceiling);
static int ascend_with_all_gaschanges(SDiveSettings *pDiveSettings, float pressure_decrease);
static float next_stop_depth_input_is_actual_stop_id(SDiveSettings *pDiveSettings, int actual_id);
static float get_gf_at_pressure(SDiveSettings *pDiveSettings, float pressure);
static int buehlmann_calc_ndl(SDiveSettings *pDiveSettings);
static _Bool dive1_check_deco(SDiveSettings *pDiveSettings);

static float gSurface_pressure_bar;
static float gPressure;
static int gGas_id;
static float gTissue_nitrogen_bar[16];
static float gTissue_helium_bar[16];
static float gGF_value;
static float gCNS;

float gGF_low_depth_bar;
SStop gStop;

void buehlmann_init(void)
{
}

static void buehlmann_backup_and_restore(_Bool backup_restore_otherwise)
{
	static float pressure;
	static float gas_id;
	static float tissue_nitrogen_bar[16];
	static float tissue_helium_bar[16];
	static float gf_value;
	static float cns;

	if(backup_restore_otherwise)
	{
		pressure = gPressure;
		gas_id = gGas_id;
		gf_value = gGF_value;
		cns = gCNS;
		memcpy(tissue_nitrogen_bar, gTissue_nitrogen_bar, (4*16));
		memcpy(tissue_helium_bar, gTissue_helium_bar, (4*16));
	}
	else
	{
		gPressure = pressure;
		gGas_id = gas_id;
		gGF_value = gf_value;
		gCNS = cns;
		memcpy(gTissue_nitrogen_bar, tissue_nitrogen_bar, (4*16));
		memcpy(gTissue_helium_bar, tissue_helium_bar, (4*16));
	}

}

float buehlmann_get_gCNS(void)
{
	return gCNS;
}

void buehlmann_calc_deco(SLifeData* pLifeData, SDiveSettings * pDiveSettings, SDecoinfo * pDecoInfo)
{
	float ceiling;
	int ascend_time;
	int tts_seconds;
	float pressure_delta;
	float next_depth;
	_Bool deco_reached = false;
	unsigned short *stoplist;
	int i;

	gCNS = 0;
	pDecoInfo->output_time_to_surface_seconds = 0;
	pDecoInfo->output_ndl_seconds = 0;
	for(int i=0;i<DECOINFO_STRUCT_MAX_STOPS;i++)
	{
		pDecoInfo->output_stop_length_seconds[i] = 0;
	}

	/* internal copying */
	gSurface_pressure_bar = pLifeData->pressure_surface_bar;

	gPressure = pLifeData->pressure_ambient_bar;
	gGas_id = 0;
	memcpy(gTissue_nitrogen_bar, pLifeData->tissue_nitrogen_bar, (4*16));
	memcpy(gTissue_helium_bar, pLifeData->tissue_helium_bar, (4*16));
	gGF_value = ((float)pDiveSettings->gf_low) / 100.0f;
	
	stoplist = pDecoInfo->output_stop_length_seconds;

	if(pLifeData->dive_time_seconds_without_surface_time < 60)
		return;

	// clean stop list
	for(i = 0; i < DECOINFO_STRUCT_MAX_STOPS; i++)
		stoplist[i] = 0;

	if(pDiveSettings->internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero >= (gPressure - PRESSURE_150_CM))
	{
		deco_reached = true;
	}

	gGF_value = ((float)pDiveSettings->gf_high) / 100.0f;
	buehlmann_backup_and_restore(true);
	if(!dive1_check_deco(pDiveSettings) )
	{
		buehlmann_backup_and_restore(false);
		// no deco
	  	pDecoInfo->output_time_to_surface_seconds = 0;
		for(i = 0; i < DECOINFO_STRUCT_MAX_STOPS; i++)
			pDecoInfo->output_stop_length_seconds[i] = 0;
		// calc NDL
		pDecoInfo->output_ndl_seconds = buehlmann_calc_ndl(pDiveSettings);;
		return;
	}
	buehlmann_backup_and_restore(false);
	pDecoInfo->output_ndl_seconds = 0;

	gGF_value = get_gf_at_pressure(pDiveSettings, gPressure);
	//current ceiling at actual position
	ceiling = tissue_tolerance();
	ambient_bar_to_deco_stop_depth_bar(pDiveSettings, ceiling);

	// set the base for all upcoming parameters
	ceiling = gStop.depth + gSurface_pressure_bar;
	tts_seconds = 0;

	// modify parameters if there is ascend or parameter fine adjustment
	if(ceiling < (gPressure - PRESSURE_150_CM)) // more than 1.5 meter below ceiling
	{
		// ascend within 10 mtr to GF_low // speed 12 mtr/min -> 50 sec / 10 mtr;  15 sec / 3 mtr.
		if(ceiling < (gPressure - PRESSURE_TEN_METER) )
		{ do {
				ascend_time = ascend_with_all_gaschanges(pDiveSettings, PRESSURE_TEN_METER);
				tts_seconds += ascend_time;
				ceiling = tissue_tolerance();
				if(tts_seconds > DECO_STOPS_MAX_TTS_CALCULATON_IN_SECONDS)
			{
				pDecoInfo->output_time_to_surface_seconds = NINETY_NINE_MINUTES_IN_SECONDS;
				return;// NINETY_NINE_MINUTES_IN_SECONDS;
			}
			} while ((ascend_time > 0 ) && ((gPressure - PRESSURE_TEN_METER ) > gSurface_pressure_bar) && (ceiling < (gPressure - PRESSURE_TEN_METER)));
		}
		do {
			buehlmann_backup_and_restore(true);
			ascend_time = ascend_with_all_gaschanges(pDiveSettings, PRESSURE_THREE_METER);
			tts_seconds += ascend_time;
			ceiling = tissue_tolerance();
			if(tts_seconds > DECO_STOPS_MAX_TTS_CALCULATON_IN_SECONDS)
			{
				pDecoInfo->output_time_to_surface_seconds = NINETY_NINE_MINUTES_IN_SECONDS;
				return;// NINETY_NINE_MINUTES_IN_SECONDS;
			}
			ambient_bar_to_deco_stop_depth_bar(pDiveSettings, ceiling);
		} while ((ascend_time > 0 ) &&  ((gStop.depth + gSurface_pressure_bar) < gPressure));

		if(gStop.depth + gSurface_pressure_bar > gPressure)
		{
			gPressure += PRESSURE_THREE_METER;
			buehlmann_backup_and_restore(false);
			tts_seconds -= ascend_time;
		}
    // calculate first stop based on tissue saturation within 10 meters of stop
    //ambient_bar_to_deco_stop_depth_bar(ceiling);
	}
	else
	{
		// initial values, upper code might not be executed (is within 150 cm)
	}

	if (ceiling > gSurface_pressure_bar)
	{
		ceiling = gStop.depth + gSurface_pressure_bar;
		// ascend the last meters to first stop (especially consider any gas changes around)
		pressure_delta = gPressure - ceiling;
		ascend_time = (int) ceil(pressure_delta * 50.0f);
		tts_seconds += ascend_with_all_gaschanges(pDiveSettings, pressure_delta);
	}
	// NDL check
	if(ceiling <= gSurface_pressure_bar)
	{
		// NDL with GF_low
		pDecoInfo->output_time_to_surface_seconds = 0;
		return;
	}
	if (ceiling > pDiveSettings->internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero)
		pDiveSettings->internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero = ceiling;

	// calc gf loop
	if(deco_reached)
		gGF_low_depth_bar = pDiveSettings->internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero - gSurface_pressure_bar;
	else
		gGF_low_depth_bar = ceiling - gSurface_pressure_bar;

	while(gStop.depth > 0)
	{
		do
		{
			next_depth = next_stop_depth_input_is_actual_stop_id(pDiveSettings, gStop.id);
			gGF_value = get_gf_at_pressure(pDiveSettings, next_depth + gSurface_pressure_bar);
			buehlmann_backup_and_restore(true);
			ascend_time = ascend_with_all_gaschanges(pDiveSettings, gStop.depth - next_depth);
			ceiling = tissue_tolerance();
			/* pre check actual limit */
			if(pDecoInfo->output_stop_length_seconds[gStop.id] >= 999*60)
			{
				tts_seconds -= 999*60 - pDecoInfo->output_stop_length_seconds[gStop.id];
				pDecoInfo->output_stop_length_seconds[gStop.id] = 999*60;
			}
			else
			/* more deco on the actual depth */
			if(ceiling > next_depth + gSurface_pressure_bar)
			{
				next_depth = -1;
				buehlmann_backup_and_restore(false);
				decom_tissues_exposure2(10, &pDiveSettings->decogaslist[gGas_id], gPressure,gTissue_nitrogen_bar,gTissue_helium_bar); // some seconds at least at each stop
				decom_oxygen_calculate_cns_exposure(10, &pDiveSettings->decogaslist[gGas_id], gPressure, &gCNS);
				pDecoInfo->output_stop_length_seconds[gStop.id] += 10;
        tts_seconds += 10;
			}
		} while(next_depth == -1);
		tts_seconds += ascend_time;
		gStop.depth = next_depth;
    for(i = gGas_id + 1; i < BUEHLMANN_STRUCT_MAX_GASES; i++)
    {
        if(pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero == 0)
            break;
        float pressureChange =  ((float)pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero) / 10;
        if(gStop.depth <= pressureChange + 0.00001f)
        {
            gGas_id = i;
        }
        else
        {
            break;
        }
    }
		gStop.id--;
	}

	pDecoInfo->output_time_to_surface_seconds = tts_seconds;
}


static float tissue_tolerance(void)
{
	float tissue_inertgas_saturation;
	float inertgas_a;
	float inertgas_b;
	float ceiling;
	float global_ceiling;
	int ci;

	global_ceiling = -1;

	for (ci = 0; ci < 16; ci++)
	{
		if(gTissue_helium_bar[ci] == 0)
		{
			tissue_inertgas_saturation = gTissue_nitrogen_bar[ci];
			//
			inertgas_a = buehlmann_N2_a[ci];
			inertgas_b = buehlmann_N2_b[ci];
		}
		else
		{
			tissue_inertgas_saturation =  gTissue_nitrogen_bar[ci] + gTissue_helium_bar[ci];
			//
			inertgas_a = ( ( buehlmann_N2_a[ci] *  gTissue_nitrogen_bar[ci]) + ( buehlmann_He_a[ci] * gTissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
			inertgas_b = ( ( buehlmann_N2_b[ci] *  gTissue_nitrogen_bar[ci]) + ( buehlmann_He_b[ci] * gTissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
		}
		//
		ceiling = (inertgas_b * ( tissue_inertgas_saturation - gGF_value * inertgas_a ) ) / (gGF_value - (inertgas_b * gGF_value) + inertgas_b);
		if(ceiling > global_ceiling)
			global_ceiling = ceiling;
	}
	return global_ceiling;
}

void buehlmann_super_saturation_calculator(SLifeData* pLifeData, SDecoinfo * pDecoInfo)
{
	float tissue_inertgas_saturation;
	float inertgas_a;
	float inertgas_b;
	float ceiling;
	float super_saturation;
	float pres_respiration = pLifeData->pressure_ambient_bar;
	int ci;

	pDecoInfo->super_saturation = 0;

	for (ci = 0; ci < 16; ci++)
	{
		if(gTissue_helium_bar[ci] == 0)
		{
			tissue_inertgas_saturation = gTissue_nitrogen_bar[ci];
			inertgas_a = buehlmann_N2_a[ci];
			inertgas_b = buehlmann_N2_b[ci];
		}
		else
		{
			tissue_inertgas_saturation =  gTissue_nitrogen_bar[ci] + gTissue_helium_bar[ci];
			inertgas_a = ( ( buehlmann_N2_a[ci] *  gTissue_nitrogen_bar[ci]) + ( buehlmann_He_a[ci] * gTissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
			inertgas_b = ( ( buehlmann_N2_b[ci] *  gTissue_nitrogen_bar[ci]) + ( buehlmann_He_b[ci] * gTissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
		}

		ceiling = pres_respiration / inertgas_b + inertgas_a;
		if(tissue_inertgas_saturation > pres_respiration)
		{
			super_saturation =
					(tissue_inertgas_saturation - pres_respiration) / (ceiling - pres_respiration);

			if (super_saturation > pDecoInfo->super_saturation)
				pDecoInfo->super_saturation = super_saturation;
		}
	}
}


static float buehlmann_tissue_test_tolerance(float depth_in_bar_absolute)
{
	float tissue_inertgas_saturation;
	float inertgas_a;
	float inertgas_b;
	float inertgas_tolerance;
	float gf_minus_1;

	gf_minus_1 = gGF_value - 1.0f;

	for (int ci = 0; ci < 16; ci++)
	{
		if(gTissue_helium_bar[ci] == 0)
		{
			tissue_inertgas_saturation = gTissue_nitrogen_bar[ci];
			inertgas_a = buehlmann_N2_a[ci];
			inertgas_b = buehlmann_N2_b[ci];
		}
		else
		{
			tissue_inertgas_saturation =  gTissue_nitrogen_bar[ci] + gTissue_helium_bar[ci];
			inertgas_a = ( ( buehlmann_N2_a[ci] *  gTissue_nitrogen_bar[ci]) + ( buehlmann_He_a[ci] * gTissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
			inertgas_b = ( ( buehlmann_N2_b[ci] *  gTissue_nitrogen_bar[ci]) + ( buehlmann_He_b[ci] * gTissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
		}
		inertgas_tolerance = ( (gGF_value / inertgas_b - gf_minus_1) * depth_in_bar_absolute ) + ( gGF_value * inertgas_a );
		if(inertgas_tolerance < tissue_inertgas_saturation)
			return tissue_inertgas_saturation - inertgas_tolerance; // positive
	}
	return tissue_inertgas_saturation - inertgas_tolerance; // negative
}


static void ambient_bar_to_deco_stop_depth_bar(SDiveSettings *pDiveSettings, float ceiling)
{
	int i;

	ceiling -= gSurface_pressure_bar;

	if(ceiling <= 0)
	{
		gStop.depth = pDiveSettings->last_stop_depth_bar;
		gStop.id = 0;
		return;
	}

	if((ceiling -  pDiveSettings->last_stop_depth_bar) <= 0)
	{
		gStop.depth =  pDiveSettings->last_stop_depth_bar;
		gStop.id = 0;
		return;
	}

	gStop.depth = pDiveSettings->input_second_to_last_stop_depth_bar;
	gStop.id = 1;
	ceiling -= pDiveSettings->input_second_to_last_stop_depth_bar;

	if(ceiling <= 0)
		return;

	for(i = 1; i < (DECOINFO_STRUCT_MAX_STOPS - 2); i++)
	{
		ceiling -= pDiveSettings->input_next_stop_increment_depth_bar;
		if(ceiling <= 0)
			break;
	}
	gStop.depth += i * pDiveSettings->input_next_stop_increment_depth_bar;
	gStop.id += i;
	return;
}

static float next_stop_depth_input_is_actual_stop_id(SDiveSettings *pDiveSettings, int actual_id)
{
	if(actual_id == 0)
		return 0;

	if(actual_id == 1)
		return pDiveSettings->last_stop_depth_bar;

	actual_id -= 2;
	return pDiveSettings->input_second_to_last_stop_depth_bar + (actual_id * pDiveSettings->input_next_stop_increment_depth_bar);
}

static int ascend_with_all_gaschanges(SDiveSettings *pDiveSettings, float pressure_decrease)
{
	float pressureTop, pressureTop_tmp, pressureBottom, pressureChange, ascendrate_in_seconds_for_one_bar, pressure_difference;
	int time_for_ascend = 0;
	int seconds;
	int i;

	ascendrate_in_seconds_for_one_bar = 60 * 10 / pDiveSettings->ascentRate_meterperminute;

	if(fabsf(gPressure - gSurface_pressure_bar) < PRESSURE_HALF_METER)
	{
		gPressure = gSurface_pressure_bar;
		return 0;
	}

	pressureTop = gPressure - pressure_decrease;
	if( gSurface_pressure_bar > pressureTop)
		pressureTop = gSurface_pressure_bar;
	pressureBottom = gPressure;
	seconds = 0;
    do{
        pressureTop_tmp = pressureTop;
        for(i = gGas_id + 1; i < BUEHLMANN_STRUCT_MAX_GASES; i++)
        {
            if(pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero == 0)
                break;
            pressureChange = gSurface_pressure_bar + ((float)pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero) / 10;
            if(pressureBottom <= pressureChange)
            {
                gGas_id = i;
            }
            else
            {
                 break;
            }

        }
        for(i = gGas_id + 1; i < BUEHLMANN_STRUCT_MAX_GASES; i++)
        {
            if(pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero == 0)
                break;
            pressureChange = gSurface_pressure_bar + ((float)pDiveSettings->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero)/ 10;
            if((pressureChange < pressureBottom) && (pressureChange > pressureTop))
            {
                pressureTop_tmp = pressureChange;
            }
        }
        pressure_difference = pressureBottom - pressureTop_tmp;
        if(pressure_difference > 0.0001f)
        {
          time_for_ascend = (int)ceilf(pressure_difference * ascendrate_in_seconds_for_one_bar);
          decom_tissues_exposure_stage_schreiner(time_for_ascend, &pDiveSettings->decogaslist[gGas_id],
                                              pressureBottom, pressureTop_tmp, gTissue_nitrogen_bar, gTissue_helium_bar);
					decom_oxygen_calculate_cns_stage_SchreinerStyle(time_for_ascend,&pDiveSettings->decogaslist[gGas_id],
                                              pressureBottom, pressureTop_tmp, &gCNS);
        }
        pressureBottom = pressureTop_tmp;
        seconds += time_for_ascend;
    }while(pressureTop_tmp > pressureTop);
    gPressure = pressureTop;
	return seconds;
}


static float get_gf_at_pressure(SDiveSettings *pDiveSettings, float pressure)
{
	float gfSteigung = 0.0f;

	if(gGF_low_depth_bar < 0)
			gGF_low_depth_bar = PRESSURE_THREE_METER; // just to prevent erratic behaviour if variable is not set

    gfSteigung = ((float)(pDiveSettings->gf_high - pDiveSettings->gf_low))/ gGF_low_depth_bar;


	if((pressure - gSurface_pressure_bar) <= PRESSURE_HALF_METER)
		return ((float)pDiveSettings->gf_high) / 100.0f;

	if(pressure >= gSurface_pressure_bar + gGF_low_depth_bar)
		return ((float)pDiveSettings->gf_low) / 100.0f;

	return (pDiveSettings->gf_high - gfSteigung * (pressure - gSurface_pressure_bar) )/ 100.0f;
}

#define MAX_NDL 240
static int buehlmann_calc_ndl(SDiveSettings *pDiveSettings)
{
	float local_tissue_nitrogen_bar[16];
	float local_tissue_helium_bar[16];
	int i;
	int ndl = 0;

	//Check ndl always use gHigh
	gGF_value = ((float)pDiveSettings->gf_high) / 100.0f;
	//10 minutes steps
	while(ndl < (MAX_NDL * 60))
	{
		memcpy(local_tissue_nitrogen_bar, gTissue_nitrogen_bar, (4*16));
		memcpy(local_tissue_helium_bar, gTissue_helium_bar, (4*16));
		//
		ndl += 600;
		decom_tissues_exposure2(600, &pDiveSettings->decogaslist[gGas_id], gPressure,gTissue_nitrogen_bar,gTissue_helium_bar);
		decom_oxygen_calculate_cns_exposure(600,&pDiveSettings->decogaslist[gGas_id],gPressure,&gCNS);
		buehlmann_backup_and_restore(true);
		if(dive1_check_deco(pDiveSettings))
		{
			buehlmann_backup_and_restore(false);
			break;
		}
		buehlmann_backup_and_restore(false);
	}

	if(ndl < (MAX_NDL * 60))
		ndl -= 600;

	if(ndl > (MAX_NDL/2 * 60))
		return ndl;

	// refine
	memcpy(gTissue_nitrogen_bar, local_tissue_nitrogen_bar, (4*16));
	memcpy(gTissue_helium_bar, local_tissue_helium_bar, (4*16));

	//One minutes step
	for(i = 0; i < 10; i++)
	{
		ndl += 60;
		decom_tissues_exposure2(60, &pDiveSettings->decogaslist[gGas_id], gPressure,gTissue_nitrogen_bar,gTissue_helium_bar);
		decom_oxygen_calculate_cns_exposure(60,&pDiveSettings->decogaslist[gGas_id],gPressure,&gCNS);
		buehlmann_backup_and_restore(true);
		if(dive1_check_deco(pDiveSettings))
			break;
		buehlmann_backup_and_restore(false);
	}
	return ndl;
}


//  ===============================================================================
//	dive1_check_deco
/// @brief	for NDL calculations
///					160614 using ceilingOther and not ceiling
//  ===============================================================================
static _Bool dive1_check_deco(SDiveSettings *pDiveSettings)
{
	// gGF_value is set in call routine;
	// internes Backup!

	// calc like in deco
	float ceiling;
	float ceilingOther; // new hw 160614

	ceiling = tissue_tolerance();
	ambient_bar_to_deco_stop_depth_bar(pDiveSettings, ceiling); // this will set gStop.depth :-) (and gStop.id)

	// set the base for all upcoming parameters
	ceilingOther = gStop.depth + gSurface_pressure_bar;

	// modify parameters if there is ascend or parameter fine adjustment
	if(ceilingOther < (gPressure - PRESSURE_150_CM)) // more than 1.5 meter below ceiling
	{
		// ascend within 10 mtr to GF_low // speed 12 mtr/min -> 50 sec / 10 mtr;  15 sec / 3 mtr.
		while(((gPressure - PRESSURE_TEN_METER ) > gSurface_pressure_bar) && (ceiling < (gPressure - PRESSURE_TEN_METER)))
		{
			ascend_with_all_gaschanges(pDiveSettings, PRESSURE_TEN_METER);
			ceiling = tissue_tolerance();
		}
		while(((gPressure - PRESSURE_THREE_METER )> gSurface_pressure_bar) && (ceiling < gPressure))
		{
			ascend_with_all_gaschanges(pDiveSettings, PRESSURE_THREE_METER);
			ceiling = tissue_tolerance();
		}
	}
	return ceiling > gSurface_pressure_bar;
}

// compute ceiling recursively, with a resolution of 10cm. Notice
// that the initial call shall guarantee that the found ceiling
// is between low and high parameters.
static float compute_ceiling(float low, float high)
{
	if ((high - low) < 0.01)
		return low;
	else {
		float next_pressure_absolute = (low + high)/2;
		float test_result = buehlmann_tissue_test_tolerance(next_pressure_absolute);
		if (test_result < 0)
			return compute_ceiling(low, next_pressure_absolute);
		else
			return compute_ceiling(next_pressure_absolute, high);
	}
}

void buehlmann_ceiling_calculator(SLifeData *pLifeData, SDecoinfo *pDecoInfo)
{
	float ceiling;

	memcpy(gTissue_nitrogen_bar, pLifeData->tissue_nitrogen_bar, (4*16));
	memcpy(gTissue_helium_bar, pLifeData->tissue_helium_bar, (4*16));

	// this is just performance optimizing. The code below runs just fine
	// without this. There is never a ceiling in NDL deco state
	if (!pDecoInfo->output_time_to_surface_seconds) {
		pDecoInfo->output_ceiling_meter = 0;
		return;
	}

	ceiling = compute_ceiling(pLifeData->pressure_surface_bar, 1.0f + pLifeData->max_depth_meter/10.0f);
	pDecoInfo->output_ceiling_meter = (ceiling - pLifeData->pressure_surface_bar) * 10.0f;
}
