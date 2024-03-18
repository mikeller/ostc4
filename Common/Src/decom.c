///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Common/Src/decom.c
/// \brief  This code is used to calculate desat, calculated by RTE and send to Firmware
/// \author heinrichs weikamp gmbh
/// \date   22-Feb-2016
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
							##### Changes	#####
	==============================================================================
	V1.0.2	1602220x	decom_oxygen_calculate_cns() changed to hwOS version

	@endverbatim
	******************************************************************************
	*/

#include "decom.h"

#include <math.h>
#include <string.h>
#include "settings.h"
#include "calc_crush.h"

#define	FRACTION_N2_AIR			0.7902

const float helium_time_constant[16] = {
										3.68695308808482E-001,
										2.29518933960247E-001,
										1.46853216220327E-001,
										9.91626867753856E-002,
										6.78890480470074E-002,
										4.78692804254106E-002,
										3.37626488338989E-002,
										2.38113081607676E-002,
										1.68239606932026E-002,
										1.25592893741610E-002,
										9.80544886914621E-003,
										7.67264977374303E-003,
										6.01220557342307E-003,
										4.70185307665137E-003,
										3.68225234041620E-003,
										2.88775228329769E-003};

 const float nitrogen_time_constant[16] = {
										1.38629436111989E-001,
										8.66433975699932E-002,
										5.54517744447956E-002,
										3.74674151654024E-002,
										2.56721177985165E-002,
										1.80978376125312E-002,
										1.27651414467762E-002,
										9.00191143584345E-003,
										6.35914844550409E-003,
										4.74758342849278E-003,
										3.70666941475907E-003,
										2.90019740820061E-003,
										2.27261370675392E-003,
										1.77730046297422E-003,
										1.39186180835330E-003,
										1.09157036308653E-003};


 const float buehlmann_N2_a[] = {
								1.1696f,
								1.0000f,
								0.8618f,
								0.7562f,
								0.6200f,
								0.5043f,
								0.4410f,
								0.4000f,
								0.3750f,
								0.3500f,
								0.3295f,
								0.3065f,
								0.2835f,
								0.2610f,
								0.2480f,
								0.2327f};

 const float buehlmann_N2_b[] = {
								0.5578f,
								0.6514f,
								0.7222f,
								0.7825f,
								0.8126f,
								0.8434f,
								0.8693f,
								0.8910f,
								0.9092f,
								0.9222f,
								0.9319f,
								0.9403f,
								0.9477f,
								0.9544f,
								0.9602f,
								0.9653f};

 const float buehlmann_He_a[] = {
								1.6189f,
								1.3830f,
								1.1919f,
								1.0458f,
								0.9220f,
								0.8205f,
								0.7305f,
								0.6502f,
								0.5950f,
								0.5545f,
								0.5333f,
								0.5189f,
								0.5181f,
								0.5176f,
								0.5172f,
								0.5119f};

 const float buehlmann_He_b[] = {
								0.4770f,
								0.5747f,
								0.6527f,
								0.7223f,
								0.7582f,
								0.7957f,
								0.8279f,
								0.8553f,
								0.8757f,
								0.8903f,
								0.8997f,
								0.9073f,
								0.9122f,
								0.9171f,
								0.9217f,
								0.9267f};

const float buehlmann_N2_t_halflife[] = {
									5.0f,
									8.0f,
								 12.5f,
								 18.5f,
								 27.0f,
								 38.3f,
								 54.3f,
								 77.0f,
								109.0f,
								146.0f,
								187.0f,
								239.0f,
								305.0f,
								390.0f,
								498.0f,
								635.0f};

const float buehlmann_He_t_halflife[] = {
									1.88f,
									3.02f,
									4.72f,
									6.99f,
								 10.21f,
								 14.48f,
								 20.53f,
								 29.11f,
								 41.20f,
								 55.19f,
								 70.69f,
								 90.34f,
								115.29f,
								147.42f,
								188.24f,
								240.03f};

const float float_buehlmann_N2_factor_expositon_one_second[] =	{ 2.30782347297664E-003f, 1.44301447809736E-003f, 9.23769302935806E-004f, 6.24261986779007E-004f, 4.27777107246730E-004f, 3.01585140931371E-004f, 2.12729727268379E-004f, 1.50020603047807E-004f, 1.05980191127841E-004f, 7.91232600646508E-005f, 6.17759153688224E-005f, 4.83354552742732E-005f, 3.78761777920511E-005f, 2.96212356654113E-005f, 2.31974277413727E-005f, 1.81926738960225E-005f};
const float float_buehlmann_N2_factor_expositon_003_second[] =	{ 6.90750456296407E-003f, 4.32279956671600E-003f, 2.76874864793053E-003f, 1.87161709452954E-003f, 1.28278242026003E-003f, 9.04482589432765E-004f, 6.38053429621421E-004f, 4.49994293975742E-004f, 3.17906879170993E-004f, 2.37350999218289E-004f, 1.85316297551252E-004f, 1.44999356986975E-004f, 1.13624229615916E-004f, 8.88610747694640E-005f, 6.95906688746861E-005f, 5.45770287740943E-005f};
const float float_buehlmann_N2_factor_expositon_008_second[] =	{ 1.83141447532454E-002f, 1.14859796471039E-002f, 7.36630472495203E-003f, 4.98319782231915E-003f, 3.41709742823104E-003f, 2.41013596224415E-003f, 1.70057124687550E-003f, 1.19953484034729E-003f, 8.47527105247492E-004f, 6.32810814525819E-004f, 4.94100480767923E-004f, 3.86618231662861E-004f, 3.02969256443353E-004f, 2.36945319086024E-004f, 1.85564355251966E-004f, 1.45532124251058E-004f};
const float float_buehlmann_N2_factor_expositon_10_seconds[] =	{ 2.28400315657541E-002f, 1.43368013598124E-002f, 9.19938673477072E-003f, 6.22511239287027E-003f, 4.69545762670800E-003f, 3.01176178733265E-003f, 2.12526200031782E-003f, 1.49919365737827E-003f, 1.05929662305226E-03f, 7.909509380171760E-004f, 6.17587450108648E-004f, 4.83249432061905E-004f, 3.78697227222391E-004f, 2.61728759809380E-004f, 2.31950063482533E-004f, 1.81911845881011E-004f};
const float float_buehlmann_N2_factor_expositon_18_seconds[] =	{ 4.07358806747357E-002f, 2.56581087982929E-002f, 1.64979259737517E-002f, 1.11772892486697E-002f, 7.67205373705648E-003f, 5.41463899418337E-003f, 3.82221908774349E-003f, 2.69693016270112E-003f, 1.90592594569927E-003f, 1.42326123023573E-003f, 1.11138278062062E-003f, 8.69680830683950E-004f, 6.81551750048359E-004f, 5.33048018290350E-004f, 4.17471377070378E-004f, 3.27417496114757E-004f};
const float float_buehlmann_N2_factor_expositon_20_seconds[] =	{ 4.51583960895835E-002f, 2.84680588463941E-002f, 1.83141447532454E-002f, 1.24114727614367E-002f, 8.52086250432193E-003f, 6.01445286560154E-003f, 4.24600726206570E-003f, 2.99613973313428E-003f, 2.11747113676897E-003f, 1.58127627264804E-003f, 1.23479348595879E-003f, 9.66265334110261E-004f, 7.57251042854845E-004f, 5.92258033589421E-004f, 4.63846326133055E-004f, 3.63790599842373E-004f};
const float float_buehlmann_N2_factor_expositon_one_minute[] =	{ 1.29449436703876E-001f, 8.29959567953288E-002f, 5.39423532744041E-002f, 3.67741962370398E-002f, 2.53453908775689E-002f, 1.79350552316596E-002f, 1.26840126026602E-002f, 8.96151553540825E-003f, 6.33897185233323E-003f, 4.73633146787078E-003f, 3.69980819572546E-003f, 2.89599589841472E-003f, 2.27003327536857E-003f, 1.77572199977927E-003f, 1.39089361795441E-003f, 1.09097481687104E-003f};
const float float_buehlmann_N2_factor_expositon_100_second[] =	{ 2.06299474015900E-001f, 1.34463438993857E-001f, 8.82775114417832E-002f, 6.05359181023788E-002f, 4.18844218114071E-002f, 2.97126970072147E-002f, 2.10505144045823E-002f, 1.48911986890571E-002f, 1.05426136839346E-002f, 7.88141652426455E-003f, 6.15873909572406E-003f, 4.82199900095137E-003f, 3.78052526350936E-003f, 2.95778454900952E-003f, 2.31708109427220E-003f, 1.81763004457269E-003f};
const float float_buehlmann_N2_factor_expositon_five_minutes[]=	{ 5.00000000000000E-001f, 3.51580222674495E-001f, 2.42141716744801E-001f, 1.70835801932547E-001f, 1.20463829104624E-001f, 8.65157896183918E-002f, 6.18314987350977E-002f, 4.40116547625051E-002f, 3.12955727186929E-002f, 2.34583889613009E-002f, 1.83626606868127E-002f, 1.43963540993090E-002f, 1.12987527093947E-002f, 8.84713405486026E-003f, 6.93514912851934E-003f, 5.44298480182925E-003f};
const float float_buehlmann_N2_factor_expositon_800_second[] =	{ 8.42509868763141E-001f, 6.85019737526282E-001f, 5.22579198044792E-001f, 3.93205767018569E-001f, 2.89861248917861E-001f, 2.14397627137602E-001f, 1.56505490290652E-001f, 1.13102166881646E-001f, 8.12935637814599E-002f, 6.13392112527207E-002f, 4.82208523469105E-002f, 3.79311861210304E-002f, 2.98470272862601E-002f, 2.34187624071612E-002f, 1.83870151711824E-002f, 1.44488700649190E-002f};
const float float_buehlmann_N2_factor_expositon_one_hour[]=			{ 9.99755859375000E-001f, 9.94475728271980E-001f, 9.64103176406343E-001f, 8.94394508891055E-001f, 7.85689004286732E-001f, 6.62392147498621E-001f, 5.35088626789486E-001f, 4.17318576947576E-001f, 3.17197008420226E-001f, 2.47876700002107E-001f, 1.99405069752929E-001f, 1.59713055172538E-001f, 1.27468761759271E-001f, 1.01149026804458E-001f, 8.01196838116008E-002f, 6.33955413542552E-002f};

const float float_buehlmann_He_factor_expositon_one_second[] =	{ 6.12608039419837E-003f, 3.81800836683133E-003f, 2.44456078654209E-003f, 1.65134647076792E-003f, 1.13084424730725E-003f, 7.97503165599123E-004f, 5.62552521860549E-004f, 3.96776399429366E-004f, 2.80360036664540E-004f, 2.09299583354805E-004f, 1.63410794820518E-004f, 1.27869320250551E-004f, 1.00198406028040E-004f, 7.83611475491108E-005f, 6.13689891868496E-005f, 4.81280465299827E-005f};
const float float_buehlmann_He_factor_expositon_003_second[] =	{ 1.82658845044263E-002f, 1.14103491926518E-002f, 7.31576933570466E-003f, 4.94586307993539E-003f, 3.38869776192019E-003f, 2.39060197012086E-003f, 1.68670834759044E-003f, 1.18985696621965E-003f, 8.40844326779777E-004f, 6.27767340286467E-004f, 4.90152279561396E-004f, 3.83558911153159E-004f, 3.00565099928485E-004f, 2.35065021719993E-004f, 1.84095669333084E-004f, 1.44377190774980E-004f}; // 3 He
const float float_buehlmann_He_factor_expositon_008_second[] =	{ 4.79706116082057E-002f, 3.01390075707096E-002f, 1.93899772993034E-002f, 1.31346689569831E-002f, 9.01102820363486E-003f, 6.36224538449637E-003f, 4.49156910795023E-003f, 3.16980660943422E-003f, 2.24068067793926E-003f, 1.67317060331207E-003f, 1.30653891641375E-003f, 1.02249686330114E-003f, 8.01306192375617E-004f, 6.26717274191169E-004f, 4.90846474157092E-004f, 3.84959521834594E-004f}; // 8 He
const float float_buehlmann_He_factor_expositon_10_seconds[] =	{ 5.95993001714799E-002f, 3.75307444923134E-002f, 2.41784389107607E-002f, 1.63912909924208E-002f, 1.25106927410620E-002f, 7.94647192918641E-003f, 5.61130562069978E-003f, 3.96068706690245E-003f, 2.80006593100546E-003f, 2.09102564918129E-003f, 1.63290683272987E-003f, 1.27795767799976E-003f, 1.00153239354972E-003f, 7.33352120986130E-004f, 6.13520442722559E-004f, 4.81176244777948E-004f};
const float float_buehlmann_He_factor_expositon_18_seconds[] =	{ 1.04710896899039E-001f, 6.65386126706349E-002f, 4.30995968284519E-002f, 2.93106657684409E-002f, 2.01607137751910E-002f, 1.42581599093282E-002f, 1.00776711616688E-002f, 7.11793906429403E-003f, 5.03447255531631E-003f, 3.76069760984632E-003f, 2.93731229281968E-003f, 2.29914783358365E-003f, 1.80203605181650E-003f, 1.40956155658090E-003f, 1.10406577253352E-003f, 8.65950533235460E-004f};
const float float_buehlmann_He_factor_expositon_20_seconds[] =	{ 1.15646523762030E-001f, 7.36529322024796E-002f, 4.77722809133601E-002f, 3.25139075644434E-002f, 2.23755519884017E-002f, 1.58297974422514E-002f, 1.11911244906306E-002f, 7.90568709176287E-003f, 5.59229149279306E-003f, 4.17767891009702E-003f, 3.26314728073529E-003f, 2.55428218017273E-003f, 2.00206171996409E-003f, 1.56605681014277E-003f, 1.22666447811148E-003f, 9.62120958977297E-004f};
const float float_buehlmann_He_factor_expositon_one_minute[] =	{ 3.08363886219441E-001f, 2.05084082411030E-001f, 1.36579295730211E-001f, 9.44046323514587E-002f, 6.56358626478964E-002f, 4.67416115355790E-002f, 3.31990512604121E-002f, 2.35300557146709E-002f, 1.66832281977395E-002f, 1.24807506400979E-002f, 9.75753219809561E-003f, 7.64329013320042E-003f, 5.99416843126677E-003f, 4.69081666943783E-003f, 3.67548116287808E-003f, 2.88358673732592E-003f};
const float float_buehlmann_He_factor_expositon_100_second[] =	{ 4.59084487437744E-001f, 3.17867635141657E-001f, 2.17103957783539E-001f, 1.52336166567559E-001f, 1.06981885584572E-001f, 7.66825160768219E-002f, 5.47171474343117E-002f, 3.89083581201959E-002f, 2.76504642556165E-002f, 2.07145921483078E-002f, 1.62096019995457E-002f, 1.27063337640768E-002f, 9.97030625587825E-003f, 7.80579708939710E-003f, 6.11829377951190E-003f, 4.80135692933603E-003f}; // 100 He
const float float_buehlmann_He_factor_expositon_five_minutes[]=	{ 8.41733751018722E-001f, 6.82600697933713E-001f, 5.20142493735619E-001f, 3.90924736715930E-001f, 2.87834706153524E-001f, 2.12857832580192E-001f, 1.55333364924147E-001f, 1.12242395185686E-001f, 8.06788883581406E-002f, 6.08653819753062E-002f, 4.78448115000141E-002f, 3.76366999883051E-002f, 2.96136888654287E-002f, 2.32350754744602E-002f, 1.82428098114835E-002f, 1.43350223887367E-002f}; // thre
const float float_buehlmann_He_factor_expositon_800_second[] =	{ 9.92671155759686E-001f, 9.53124140216102E-001f, 8.58865632718416E-001f, 7.33443528431762E-001f, 5.95533881446524E-001f, 4.71787742036413E-001f, 3.62479376011699E-001f, 2.72021750877104E-001f, 2.00940186773410E-001f, 1.54187175639359E-001f, 1.22553521140786E-001f, 9.72431193565182E-002f, 7.70338702477497E-002f, 6.07666995543268E-002f, 4.79109397391700E-002f, 3.77715319879068E-002f}; // 800 He
const float float_buehlmann_He_factor_expositon_one_hour[]=			{ 9.99999999753021E-001f, 9.99998954626205E-001f, 9.99850944669188E-001f, 9.97393537149572E-001f, 9.82979603888650E-001f, 9.43423231328217E-001f, 8.68106292901111E-001f, 7.60374619482322E-001f, 6.35576141220644E-001f, 5.29310840978539E-001f, 4.44744511849213E-001f, 3.68942936079581E-001f, 3.02834419265355E-001f, 2.45810174422126E-001f, 1.98231319020275E-001f, 1.59085372294989E-001f};

void decom_get_inert_gases(const float ambient_pressure_bar,const SGas* pGas, float* fraction_nitrogen, float* fraction_helium )
{
	float fraction_all_inertgases;
	float ppo2_fraction_setpoint;
	float diluent_divisor;

	*fraction_nitrogen = ((float)pGas->nitrogen_percentage) / 100.0f;
	*fraction_helium = ((float)pGas->helium_percentage) / 100.0f;

	if(pGas->AppliedDiveMode == DIVEMODE_CCR)
	{
		// continue with CCR
		fraction_all_inertgases = *fraction_nitrogen + *fraction_helium;

		ppo2_fraction_setpoint = (float)pGas->setPoint_cbar/ (100 * ambient_pressure_bar);

		diluent_divisor = (1.0f - ppo2_fraction_setpoint) / fraction_all_inertgases;
		if(diluent_divisor < 0)
			diluent_divisor = 0;

		*fraction_nitrogen *= diluent_divisor;
		*fraction_helium   *= diluent_divisor;
	}
	if(pGas->AppliedDiveMode == DIVEMODE_PSCR)
	{
		fraction_all_inertgases = *fraction_nitrogen + *fraction_helium;
		ppo2_fraction_setpoint =  decom_calc_SimppO2(ambient_pressure_bar, pGas) / ambient_pressure_bar;
		diluent_divisor = (1.0f - ppo2_fraction_setpoint) / fraction_all_inertgases;
		if(diluent_divisor < 0)
			diluent_divisor = 0;

		*fraction_nitrogen *= diluent_divisor;
		*fraction_helium   *= diluent_divisor;
	}
}


void decom_tissues_exposure(int period_in_seconds, SLifeData * pLifeData)
{
		decom_tissues_exposure2(period_in_seconds, &pLifeData->actualGas,  pLifeData->pressure_ambient_bar, pLifeData->tissue_nitrogen_bar, pLifeData->tissue_helium_bar);
}


void decom_tissues_exposure2(int period_in_seconds, SGas* pActualGas,  float ambiant_pressure_bar, float *tissue_N2_selected_stage, float *tissue_He_selected_stage)
{
	int ci;
	float percent_N2;
	float percent_He;
	float partial_pressure_N2;
	float partial_pressure_He;



	int period_in_seconds_left;

	if(period_in_seconds > 0)
	{

		decom_get_inert_gases(ambiant_pressure_bar, pActualGas, &percent_N2, &percent_He);

		partial_pressure_N2 =  (ambiant_pressure_bar - WATER_VAPOUR_PRESSURE) * percent_N2;
		partial_pressure_He = (ambiant_pressure_bar - WATER_VAPOUR_PRESSURE) * percent_He;
		period_in_seconds_left = period_in_seconds;

		while(period_in_seconds_left)
		{
			if(period_in_seconds_left >= 3600)
				period_in_seconds = 3600;
			else
			if(period_in_seconds_left >= 800)
				period_in_seconds = 800;
			else
			if(period_in_seconds_left >= 300)
				period_in_seconds = 300;
			else
			if(period_in_seconds_left >= 100)
				period_in_seconds = 100;
			else
			if(period_in_seconds_left >= 60)
				period_in_seconds = 60;
			else
			if(period_in_seconds_left == 36)
				period_in_seconds = 18;
			else
			if(period_in_seconds_left >= 20)
				period_in_seconds = 20;
			else
			if(period_in_seconds_left >= 18)
				period_in_seconds = 18;
			else
			if(period_in_seconds_left >= 10)
				period_in_seconds = 10;
			else
			if(period_in_seconds_left >= 8)
				period_in_seconds = 8;
			else
			if(period_in_seconds_left >= 3)
				period_in_seconds = 3;
			else
				period_in_seconds = 1;

			period_in_seconds_left -= period_in_seconds;

			switch (period_in_seconds)
			{
				case 1:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_one_second[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_one_second[ci];
				}
				break;
				case 3:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_003_second[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_003_second[ci];
				}
				break;
				case 8:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_008_second[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_008_second[ci];
				}
				break;
				case 10:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_10_seconds[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_10_seconds[ci];
				}
				break;
				case 18:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_18_seconds[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_18_seconds[ci];
				}
				break;
				case 20:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_20_seconds[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_20_seconds[ci];
				}
				break;
				case 60:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_one_minute[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_one_minute[ci];
				}
				break;
				case 100:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_100_second[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_100_second[ci];
				}
				break;
				case 300:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_five_minutes[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_five_minutes[ci];
				}
				break;
				case 800:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_800_second[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_800_second[ci];
				}
				break;
				case 3600:
				for (ci=0;ci<16;ci++)
				{
					tissue_N2_selected_stage[ci] += (partial_pressure_N2 - tissue_N2_selected_stage[ci]) * float_buehlmann_N2_factor_expositon_one_hour[ci];
					tissue_He_selected_stage[ci] += (partial_pressure_He - tissue_He_selected_stage[ci]) * float_buehlmann_He_factor_expositon_one_hour[ci];
				}
				break;
			}
		}
	}
}

void decom_reset_with_1000mbar(SLifeData * pLifeData)
{
	double saturation = 1.0;

	saturation -= WATER_VAPOUR_PRESSURE;
	saturation *= FRACTION_N2_AIR;

	for(int i=0;i<16;i++)
	{
		pLifeData->tissue_nitrogen_bar[i] = saturation;
		pLifeData->tissue_helium_bar[i] = 0;
	}
	pLifeData->otu = 0;
	pLifeData->cns = 0;
	pLifeData->desaturation_time_minutes = 0;
	pLifeData->no_fly_time_minutes = 0;
}

void decom_reset_with_ambientmbar(float ambient, SLifeData * pLifeData)
{

	float saturation = 1.0;
	saturation = ambient;
	saturation -= WATER_VAPOUR_PRESSURE;
	saturation *= FRACTION_N2_AIR;

	for(int i=0;i<16;i++)
	{
		pLifeData->tissue_nitrogen_bar[i] = saturation;
		pLifeData->tissue_helium_bar[i] = 0;
	}
	pLifeData->otu = 0;
	pLifeData->cns = 0;
	pLifeData->desaturation_time_minutes = 0;
	pLifeData->no_fly_time_minutes = 0;
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


float decom_schreiner_equation(float *initial_inspired_gas_pressure,
float *rate_change_insp_gas_pressure,
float *interval_time_minutes,
const float *gas_time_constant,
float *initial_gas_pressure)
{
	/* System generated locals */
	float ret_val;
	float time_null_pressure = 0.0f;
	float time_rest = 0.0f;
	float time = *interval_time_minutes;
	/* =============================================================================== */
	/*     Note: The Schreiner equation is applied when calculating the uptake or */
	/*     elimination of compartment gases during linear ascents or descents at a */
	/*     constant rate.  For ascents, a negative number for rate must be used. */
	/* =============================================================================== */
	if( *rate_change_insp_gas_pressure < 0.0f)
	{
		time_null_pressure = -1.0f * *initial_inspired_gas_pressure / *rate_change_insp_gas_pressure;
		if(time > time_null_pressure )
		{
			time_rest = time - time_null_pressure;
			time = time_null_pressure;
		}
	}
	ret_val =
	*initial_inspired_gas_pressure +
	*rate_change_insp_gas_pressure *
	(time - 1.f / *gas_time_constant) -
	(*initial_inspired_gas_pressure -
	*initial_gas_pressure -
	*rate_change_insp_gas_pressure / *gas_time_constant) *
	expf(-(*gas_time_constant) * time);

	if(time_rest > 0.0f)
	{
		ret_val = ret_val * expf(-(*gas_time_constant) * time_rest);
	}


	return ret_val;
}; /* schreiner_equation__2 */

void decom_tissues_exposure_stage_schreiner(int period_in_seconds, SGas* pGas, float  starting_ambient_pressure_bar, float ending_ambient_pressure_bar,
																		 float* pTissue_nitrogen_bar,  float* pTissue_helium_bar)
{

	float initial_pressure_N2;
	float initial_pressure_He;

	float ending_pressure_N2;
	float ending_pressure_He;

	float fraction_N2_begin;
	float fraction_N2_end;
	float fraction_He_begin;
	float fraction_He_end;

	float rate_N2;
	float rate_He;

	float period_in_minutes;

	int ci;

	if(period_in_seconds <= 0)
		return;

	decom_get_inert_gases(starting_ambient_pressure_bar, pGas, &fraction_N2_begin, &fraction_He_begin );
	decom_get_inert_gases(ending_ambient_pressure_bar, pGas, &fraction_N2_end, &fraction_He_end );

	initial_pressure_N2 = (starting_ambient_pressure_bar - WATER_VAPOUR_PRESSURE) * fraction_N2_begin;
	initial_pressure_He = (starting_ambient_pressure_bar - WATER_VAPOUR_PRESSURE) * fraction_He_begin;

	ending_pressure_N2 = (ending_ambient_pressure_bar - WATER_VAPOUR_PRESSURE) * fraction_N2_end;
	ending_pressure_He = (ending_ambient_pressure_bar - WATER_VAPOUR_PRESSURE) * fraction_He_end;

	rate_N2 = (ending_pressure_N2 - initial_pressure_N2) / period_in_seconds;
	rate_He = (ending_pressure_He - initial_pressure_He) / period_in_seconds;

	period_in_minutes = ((float)period_in_seconds) / 60.0f;

	for (ci=0;ci<16;ci++)
	{
		pTissue_nitrogen_bar[ci] =
		decom_schreiner_equation(
			&initial_pressure_N2,
			&rate_N2,
			&period_in_minutes,
			&nitrogen_time_constant[ci],
			&pTissue_nitrogen_bar[ci]);

		pTissue_helium_bar[ci] =
		decom_schreiner_equation(
			&initial_pressure_He,
			&rate_He,
			&period_in_minutes,
			&helium_time_constant[ci],
			&pTissue_helium_bar[ci]);
	}
}

_Bool nextSetpointChange(SDiveSettings* pDiveSettings, uint8_t depth_meter, uint8_t* change_depth_meter, char* setpoint)
{
	uint8_t new_depth = 0;
	char new_setpoint = 0;
	for(int i = 1; i <= 5; i++)
	{
		if(pDiveSettings->setpoint[i].setpoint_cbar > 0 && pDiveSettings->setpoint[i].depth_meter > 0  )
		{
			if( pDiveSettings->setpoint[i].depth_meter > new_depth && pDiveSettings->setpoint[i].depth_meter < depth_meter)
			{
				new_depth = pDiveSettings->setpoint[i].depth_meter;
				new_setpoint = pDiveSettings->setpoint[i].setpoint_cbar;
			}
		}
	}
	if(new_depth)
	{
		* change_depth_meter = new_depth;
		* setpoint = new_setpoint;
		return 1;
	}
	return 0;
}


void insertGasIntoList(SGasLine* pGas, SGasLine** pGasList, uint8_t gasInSettings, uint8_t* pGasInSettingsList, uint8_t GasListLength)
{
	uint8_t localGasIndex = GasListLength;
	if(pGas != 0)
	{
		while(localGasIndex != 0)	/* first entry */
		{
			if(pGasList[localGasIndex-1]->depth_meter > pGas->depth_meter)	/* switch depth of existing gas is deeper then new one => move down */
			{
				pGasList[localGasIndex] = pGasList[localGasIndex-1];
				pGasInSettingsList[localGasIndex] = pGasInSettingsList[localGasIndex - 1];
				localGasIndex--;
			}
			else
			{
				break;
			}
		}
		pGasList[localGasIndex] = pGas;
		pGasInSettingsList[localGasIndex] = gasInSettings;
	}
}

void decom_CreateGasChangeList(SDiveSettings* pInput, const SLifeData* pLifeData)
{
	SGasLine localPSCRFirst;
	SGasLine *pLocalGasList[5] = {0,0,0,0,0};
	uint8_t localGasInSettingsList[5] = {0,0,0,0,0};

	uint8_t gasStart = 1;
	uint8_t gasIndex = 0;
	uint8_t gasEntryCnt = 0;

	int i=0;
	for(i=0;i< 5;i++)	/* reset list */
	{
		pInput->decogaslist[i].change_during_ascent_depth_meter_otherwise_zero = 0;
		pInput->decogaslist[i].GasIdInSettings = 255;
		pInput->decogaslist[i].setPoint_cbar = 0;
		pInput->decogaslist[i].helium_percentage = 0;
		pInput->decogaslist[i].nitrogen_percentage = 0;
	}
	/* FirstGas
	 * 0 = special gas, 1 to 5 is OC gas, 6 to 10 is diluent
	 */
	pInput->decogaslist[0] = pLifeData->actualGas;
	/* Add Deco Gases
	 * special (gasId == 0) is never a deco/travel gas but actual gas only
	*/

	if(pInput->diveMode != DIVEMODE_OC)
	{
		gasStart = 6;	/* CCR or PSCR => CC gaslist */
	}

	if(pInput->diveMode == DIVEMODE_PSCR)				/* Handle first gas as deco gas */
	{
		for(gasIndex = gasStart; gasIndex < gasStart + 5; gasIndex++)
		{
			if(pInput->gas[gasIndex].note.ub.first)
			{
				if (pLifeData->actualGas.GasIdInSettings != gasIndex)
				{
					memcpy(&localPSCRFirst, &pInput->gas[gasIndex], sizeof(SGasLine));
					localPSCRFirst.depth_meter = calc_MOD(gasIndex);
					insertGasIntoList(&localPSCRFirst, pLocalGasList, gasIndex, localGasInSettingsList, gasEntryCnt);
					gasEntryCnt++;
					break;
				}
			}
		}
	}


	for(gasIndex = gasStart; gasIndex < gasStart + 5; gasIndex++)
	{
		if(((pInput->gas[gasIndex].note.ub.active) && (pInput->gas[gasIndex].depth_meter))		/* ready for deco calculation */
			&& (pLifeData->actualGas.GasIdInSettings != gasIndex)							/* not the actual gas */
			&& (pInput->gas[gasIndex].depth_meter < pLifeData->depth_meter ))				/* a gas which is on the way to surface */
		{
			insertGasIntoList(&pInput->gas[gasIndex], pLocalGasList, gasIndex, localGasInSettingsList, gasEntryCnt);
			gasEntryCnt++;
		}
	}
	for(gasIndex = 1; gasIndex < gasEntryCnt+1; gasIndex++)									/* move gasLine Information into deco List */
	{
		pInput->decogaslist[gasIndex].change_during_ascent_depth_meter_otherwise_zero = pLocalGasList[gasIndex-1]->depth_meter;
		pInput->decogaslist[gasIndex].nitrogen_percentage = 100;
		pInput->decogaslist[gasIndex].nitrogen_percentage -= pLocalGasList[gasIndex-1]->oxygen_percentage;
		pInput->decogaslist[gasIndex].nitrogen_percentage -= pLocalGasList[gasIndex-1]->helium_percentage;
		pInput->decogaslist[gasIndex].helium_percentage = pLocalGasList[gasIndex-1]->helium_percentage;
		pInput->decogaslist[gasIndex].GasIdInSettings = localGasInSettingsList[gasIndex-1];
		pInput->decogaslist[gasIndex].AppliedDiveMode = pInput->diveMode;
	}
}
void test_decom_CreateGasChangeList(void)
{
	SDiveSettings diveSetting;
	SLifeData lifeData;
	lifeData.depth_meter = 100;
	lifeData.actualGas.helium_percentage = 30;
	lifeData.actualGas.nitrogen_percentage = 60;
	lifeData.actualGas.setPoint_cbar = 18;
	lifeData.actualGas.GasIdInSettings = 0;
	lifeData.actualGas.change_during_ascent_depth_meter_otherwise_zero = 0;
	diveSetting.diveMode = DIVEMODE_CCR;
	diveSetting.gas[6].depth_meter = 0;
	diveSetting.gas[6].helium_percentage = 30;
	diveSetting.gas[6].oxygen_percentage = 10;
	 diveSetting.gas[6].note.ub.active = 1;

	 diveSetting.gas[7].depth_meter = 60;
	diveSetting.gas[7].helium_percentage = 0;
	diveSetting.gas[7].oxygen_percentage = 10;
	 diveSetting.gas[7].note.ub.active = 1;
		diveSetting.gas[8].note.ub.active = 0;
		diveSetting.gas[9].note.ub.active = 0;
		diveSetting.gas[10].note.ub.active = 0;

	diveSetting.setpoint[0].depth_meter = 0;
	diveSetting.setpoint[1].depth_meter = 80;
	diveSetting.setpoint[1].setpoint_cbar = 20;
	diveSetting.setpoint[2].depth_meter = 60;
	diveSetting.setpoint[2].setpoint_cbar = 25;
	diveSetting.setpoint[3].depth_meter = 0;
	diveSetting.setpoint[4].depth_meter = 0;
	diveSetting.setpoint[5].depth_meter = 0;


	decom_CreateGasChangeList(&diveSetting, &lifeData);
}

uint8_t decom_tissue_test_tolerance(float* Tissue_nitrogen_bar, float* Tissue_helium_bar, float GF_value, float depth_in_bar_absolute)
{
	float tissue_inertgas_saturation;
	float inertgas_a;
	float inertgas_b;
	float inertgas_tolerance;
	float gf_minus_1;

	gf_minus_1 = GF_value - 1.0f;

	for (int ci = 0; ci < 16; ci++)
	{
		if(Tissue_helium_bar[ci] == 0)
		{
			tissue_inertgas_saturation = Tissue_nitrogen_bar[ci];
			//
			inertgas_a = buehlmann_N2_a[ci];
			inertgas_b = buehlmann_N2_b[ci];
		}
		else
		{
			tissue_inertgas_saturation =  Tissue_nitrogen_bar[ci] + Tissue_helium_bar[ci];
			//
			inertgas_a = ( ( buehlmann_N2_a[ci] *  Tissue_nitrogen_bar[ci]) + ( buehlmann_He_a[ci] * Tissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
			inertgas_b = ( ( buehlmann_N2_b[ci] *  Tissue_nitrogen_bar[ci]) + ( buehlmann_He_b[ci] * Tissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
		}
		//
		inertgas_tolerance = ( (GF_value / inertgas_b - gf_minus_1) * depth_in_bar_absolute ) + ( GF_value * inertgas_a );
		//
		if(inertgas_tolerance < tissue_inertgas_saturation)
			return 0;
	}
	return 1;
}


void decom_tissues_desaturation_time(const SLifeData* pLifeData, SLifeData2* pOutput)
{
	float pressure_in_gas_for_complete;
	float pressure_in_gas_for_desat;
	float diff_to_complete;
	float diff_to_desatpoint;
	float necessary_halftimes;
	float desattime;

	pressure_in_gas_for_complete = 0.7902f * ( pLifeData->pressure_surface_bar - 0.0627f);
	pressure_in_gas_for_desat = 1.05f * pressure_in_gas_for_complete;
	for(int i=0; i<16; i++)
	{
		diff_to_complete = pressure_in_gas_for_complete - pLifeData->tissue_nitrogen_bar[i];
		diff_to_desatpoint = pressure_in_gas_for_desat - pLifeData->tissue_nitrogen_bar[i];

		if((diff_to_desatpoint >= 0) || (diff_to_complete >= 0))
			pOutput->tissue_nitrogen_desaturation_time_minutes[i] = 0;
		else
		{
			necessary_halftimes = (logf(1.0f - (diff_to_desatpoint/diff_to_complete)) / -0.6931f);
			desattime = buehlmann_N2_t_halflife[i] * necessary_halftimes;
			if(desattime <= (float)0xFFFF)
				pOutput->tissue_nitrogen_desaturation_time_minutes[i] = desattime;
			else
				pOutput->tissue_nitrogen_desaturation_time_minutes[i] = 0xFFFF;
		}
	}

	for(int i=0; i<16; i++)
	{
		diff_to_desatpoint = 0.05f - pLifeData->tissue_helium_bar[i];
		diff_to_complete = -1.0f * pLifeData->tissue_helium_bar[i];

		if((diff_to_desatpoint >= 0) || (diff_to_complete >= 0))
			pOutput->tissue_helium_desaturation_time_minutes[i] = 0;
		else
		{
			necessary_halftimes = (logf(1.0f - (diff_to_desatpoint/diff_to_complete)) / -0.6931f);
			desattime = buehlmann_He_t_halflife[i] * necessary_halftimes;
			if(desattime <= (float)0xFFFF)
				pOutput->tissue_helium_desaturation_time_minutes[i] = desattime;
			else
				pOutput->tissue_helium_desaturation_time_minutes[i] = 0xFFFF;
		}
	}
}

#define MAX_DEGRADE_OTU_TIME_MINUTES (1440)
//CNS&OTU:
#define OXY_TEN_MINUTES_IN_SECONDS (600)
#define OXY_HALF_LIVE_OF_TEN_MINUTES__INVERSE_NINTH_ROOT_OF_TWO (0.92587471f)
#define OXY_NINE_DAYS_IN_TEN_MINUTES 	(1296)
#define OXY_ONE_SIXTIETH_PART 			(0.0166667f)
#define OXY_NEGATIVE_FIVE_SIXTH_PARTS (-0.8333333f)
void decom_oxygen_calculate_otu(float* oxygen_otu, float pressure_oxygen_real)
{
	if(pressure_oxygen_real <= 0.5f)
		return;
	*oxygen_otu += (pow((double)(0.5f / (pressure_oxygen_real - 0.5f)),OXY_NEGATIVE_FIVE_SIXTH_PARTS)) * OXY_ONE_SIXTIETH_PART;
}

void decom_oxygen_calculate_otu_degrade(float* oxygen_otu, long seconds_since_last_dive)
{
	static long  otu_time_ticker = 0;
	static double  otu_degrade_every_10_minutes = 999.9;
	long cycles_since_last_call;

	if((*oxygen_otu <= 0) || (seconds_since_last_dive == 0))
		*oxygen_otu = 0;
	else if(seconds_since_last_dive < OXY_TEN_MINUTES_IN_SECONDS)
	{
		otu_time_ticker = 1;
		otu_degrade_every_10_minutes = *oxygen_otu / (MAX_DEGRADE_OTU_TIME_MINUTES / 10);
	}
	else
	{
		cycles_since_last_call = seconds_since_last_dive / (otu_time_ticker * OXY_TEN_MINUTES_IN_SECONDS);
		*oxygen_otu -= ((double)cycles_since_last_call) * otu_degrade_every_10_minutes;
		otu_time_ticker += cycles_since_last_call;
		if((*oxygen_otu < 0) ||  (otu_time_ticker > (MAX_DEGRADE_OTU_TIME_MINUTES / 10)))
			*oxygen_otu = 0;
	}
}



void decom_oxygen_calculate_cns_degrade(float* oxygen_cns, long seconds_since_last_dive)
{
	static long cns_time_ticker = 0;
	int cns_max_cycles;

	if((*oxygen_cns <= 0.5f)  || (seconds_since_last_dive == 0))
		*oxygen_cns = 0;
	else if(seconds_since_last_dive < OXY_TEN_MINUTES_IN_SECONDS)
		cns_time_ticker = 1;
	else
	{
		cns_max_cycles = OXY_NINE_DAYS_IN_TEN_MINUTES;
		while((*oxygen_cns >= 0.5f) && ((cns_time_ticker * OXY_TEN_MINUTES_IN_SECONDS) < seconds_since_last_dive) && cns_max_cycles)
		{
			cns_time_ticker++;
			cns_max_cycles--;
			*oxygen_cns *= OXY_HALF_LIVE_OF_TEN_MINUTES__INVERSE_NINTH_ROOT_OF_TWO;
		}
	}
}


// new hwOS style
void decom_oxygen_calculate_cns(float* oxygen_cns, float pressure_oxygen_real)
{
	uint8_t char_I_actual_ppO2;
	float CNS_fraction = 0;
	const float time_factor = 3000.0f;

	if(pressure_oxygen_real < 0.15f)
		char_I_actual_ppO2 = 15;
	else
	if(pressure_oxygen_real >= 2.5f)
		char_I_actual_ppO2 = 255;
	else
		char_I_actual_ppO2 = (uint8_t)(pressure_oxygen_real * 100);

	if (char_I_actual_ppO2 < 50)
			(void)0;   // no changes
	//------------------------------------------------------------------------
	// Below (and including) 1.60 bar
	else if (char_I_actual_ppO2 < 61)
			CNS_fraction += time_factor/(-533.07f * char_I_actual_ppO2 + 54000.0f);
	else if (char_I_actual_ppO2 < 71)
			CNS_fraction += time_factor/(-444.22f * char_I_actual_ppO2 + 48600.0f);
	else if (char_I_actual_ppO2 < 81)
			CNS_fraction += time_factor/(-355.38f * char_I_actual_ppO2 + 42300.0f);
	else if (char_I_actual_ppO2 < 91)
			CNS_fraction += time_factor/(-266.53f * char_I_actual_ppO2 + 35100.0f);
	else if (char_I_actual_ppO2 < 111)
			CNS_fraction += time_factor/(-177.69f * char_I_actual_ppO2 + 27000.0f);
	else if (char_I_actual_ppO2 < 152)
			CNS_fraction += time_factor/( -88.84f * char_I_actual_ppO2 + 17100.0f);
	else if (char_I_actual_ppO2 < 167)
			CNS_fraction += time_factor/(-222.11f * char_I_actual_ppO2 + 37350.0f);
	//------------------------------------------------------------------------
	// Arieli et all.(2002): Modeling pulmonary and CNS O2 toxicity:
	// J Appl Physiol 92: 248--256, 2002, doi:10.1152/japplphysiol.00434.2001
	// Formula (A1) based on value for 1.55 and c=20
	// example calculation: Sqrt((1.7/1.55)^20)*0.000404
	else if (char_I_actual_ppO2 < 172)
			CNS_fraction += time_factor*0.00102f;
	else if (char_I_actual_ppO2 < 177)
			CNS_fraction += time_factor*0.00136f;
	else if (char_I_actual_ppO2 < 182)
			CNS_fraction += time_factor*0.00180f;
	else if (char_I_actual_ppO2 < 187)
			CNS_fraction += time_factor*0.00237f;
	else if (char_I_actual_ppO2 < 192)
			CNS_fraction += time_factor*0.00310f;
	else if (char_I_actual_ppO2 < 198)
			CNS_fraction += time_factor*0.00401f;
	else if (char_I_actual_ppO2 < 203)
			CNS_fraction += time_factor*0.00517f;
	else if (char_I_actual_ppO2 < 233)
			CNS_fraction += time_factor*0.0209f;
	else
			CNS_fraction += time_factor*0.0482f; // value for 2.5

	if( CNS_fraction > 999.0f)    // Limit display to 999%
			CNS_fraction = 999.0f;
	if( CNS_fraction < 0.0f )
			CNS_fraction = 0.0f;

	//calculate cns for the actual ppo2 for 1 second
	*oxygen_cns += OXY_ONE_SIXTIETH_PART * CNS_fraction;

	if( *oxygen_cns > 999.0f)    // Limit display to 999%
			*oxygen_cns = 999.0f;
	if( *oxygen_cns < 0.0f )
			*oxygen_cns = 0.0f;
}

/* old DR5 style
void decom_oxygen_calculate_cns(float* oxygen_cns, float pressure_oxygen_real)
{
	int cns_no_range = 0;
	_Bool not_found = 1;
		//for the cns calculation
		const float cns_ppo2_ranges[60][2] = {
										{0.50f, 0.00f}, {0.60f, 0.14f}, {0.64f, 0.15f}, {0.66f, 0.16f}, {0.68f, 0.17f}, {0.70f, 0.18f},
										{0.74f, 0.19f}, {0.76f, 0.20f}, {0.78f, 0.21f}, {0.80f, 0.22f}, {0.82f, 0.23f}, {0.84f, 0.24f},
										{0.86f, 0.25f}, {0.88f, 0.26f}, {0.90f, 0.28f}, {0.92f, 0.29f}, {0.94f, 0.30f}, {0.96f, 0.31f},
										{0.98f, 0.32f}, {1.00f, 0.33f}, {1.02f, 0.35f}, {1.04f, 0.36f}, {1.06f, 0.38f}, {1.08f, 0.40f},
										{1.10f, 0.42f}, {1.12f, 0.43f}, {1.14f, 0.43f}, {1.16f, 0.44f}, {1.18f, 0.46f}, {1.20f, 0.47f},
										{1.22f, 0.48f}, {1.24f, 0.51f},	{1.26f, 0.52f}, {1.28f, 0.54f}, {1.30f, 0.56f}, {1.32f, 0.57f},
										{1.34f, 0.60f}, {1.36f, 0.62f}, {1.38f, 0.63f}, {1.40f, 0.65f}, {1.42f, 0.68f}, {1.44f, 0.71f},
										{1.46f, 0.74f}, {1.48f, 0.78f}, {1.50f, 0.83f}, {1.52f, 0.93f}, {1.54f, 1.04f}, {1.56f, 1.19f},
										{1.58f, 1.47f}, {1.60f, 2.22f}, {1.62f, 5.00f}, {1.65f, 6.25f}, {1.67f, 7.69f}, {1.70f, 10.0f},
										{1.72f,12.50f}, {1.74f,20.00f}, {1.77f,25.00f}, {1.79f,31.25f}, {1.80f,50.00f}, {1.82f,100.0f}};
	//find the correct cns range for the corresponding ppo2
	cns_no_range = 58;
	while (cns_no_range && not_found)
	{
		if (pressure_oxygen_real > cns_ppo2_ranges[cns_no_range][0])
		{
			cns_no_range++;
			not_found = 0;
		}
		else
			cns_no_range--;
	}

	//calculate cns for the actual ppo2 for 1 second
	*oxygen_cns += OXY_ONE_SIXTIETH_PART * cns_ppo2_ranges[cns_no_range][1];
}
*/

void decom_oxygen_calculate_cns_exposure(int period_in_seconds, SGas* pActualGas, float pressure_ambient_bar, float* oxygen_cns)
{
	float pressure_oxygen_real;
	float one_second_cns;

	pressure_oxygen_real = decom_calc_ppO2(pressure_ambient_bar, pActualGas);
	one_second_cns = 0;
	decom_oxygen_calculate_cns(&one_second_cns, pressure_oxygen_real);
	*oxygen_cns += one_second_cns * period_in_seconds;
}


void decom_oxygen_calculate_cns_stage_SchreinerStyle(int period_in_seconds, SGas* pGas, float  starting_ambient_pressure_bar, float ending_ambient_pressure_bar, float* oxygen_cns)
{
	if(ending_ambient_pressure_bar == starting_ambient_pressure_bar)
	{
		decom_oxygen_calculate_cns_exposure(period_in_seconds, pGas, starting_ambient_pressure_bar, oxygen_cns);
		return;
	}

	float pressure_oxygen_real;
	float initial_pressure_oxygen;
	float ending_pressure_oxygen;
	float rate_oxygen;

	initial_pressure_oxygen = decom_calc_ppO2(starting_ambient_pressure_bar, pGas);
	ending_pressure_oxygen = decom_calc_ppO2(ending_ambient_pressure_bar, pGas);

	rate_oxygen = (ending_pressure_oxygen - initial_pressure_oxygen) / period_in_seconds;

	pressure_oxygen_real = initial_pressure_oxygen;
	for(int i = 0; i < period_in_seconds; i++)
	{
		decom_oxygen_calculate_cns(oxygen_cns, pressure_oxygen_real);
		pressure_oxygen_real += rate_oxygen;
	}
}


float decom_calc_ppO2(const float ambiant_pressure_bar, const SGas* pGas)
{
	float percent_N2 = 0;
	float percent_He = 0;
	float percent_O2 = 0;

	decom_get_inert_gases(ambiant_pressure_bar, pGas, &percent_N2, &percent_He);
	percent_O2 = 1 - percent_N2 - percent_He;

	return  (ambiant_pressure_bar - WATER_VAPOUR_PRESSURE) * percent_O2;
}


float decom_calc_SimppO2(float ambiant_pressure_bar, const SGas* pGas)
{
	float o2Ratio = 0.0;
	float inertGasRatio = 0.0;
	float simulatedPSCRppo2 = 0.0;

	o2Ratio = (100.0 - pGas->nitrogen_percentage - pGas->helium_percentage) / 100.0;
	inertGasRatio = 1.0 - o2Ratio;
	simulatedPSCRppo2 = (ambiant_pressure_bar - WATER_VAPOUR_PRESSURE) * o2Ratio;
	simulatedPSCRppo2 -= (inertGasRatio * pGas->pscr_factor);
	if(simulatedPSCRppo2 < 0.0)
	{
		simulatedPSCRppo2 = 0.0;
	}
	return simulatedPSCRppo2;
}

float decom_calc_SimppO2_O2based(float ambiant_pressure_bar, uint8_t O2PerCent, float factor)
{
	float o2Ratio = 0.0;
	float inertGasRatio = 0.0;
	float simulatedPSCRppo2 = 0.0;

	o2Ratio = O2PerCent / 100.0;
	inertGasRatio = 1.0 - o2Ratio;
	simulatedPSCRppo2 = (ambiant_pressure_bar - WATER_VAPOUR_PRESSURE) * o2Ratio;
	simulatedPSCRppo2 -= (inertGasRatio * factor);
	if(simulatedPSCRppo2 < 0.0)
	{
		simulatedPSCRppo2 = 0.0;
	}
	return simulatedPSCRppo2;
}

uint8_t decom_get_actual_deco_stop(SDiveState* pDiveState)
{
		SDecoinfo* pDecoinfo;
		uint8_t depthNext, depthLast, depthSecond, depthInc;
		if(pDiveState->diveSettings.deco_type.ub.standard == GF_MODE)
			pDecoinfo = &pDiveState->decolistBuehlmann;
		else
			pDecoinfo = &pDiveState->decolistVPM;

		depthLast 		= (uint8_t)(pDiveState->diveSettings.last_stop_depth_bar * 10);
		depthSecond 	= (uint8_t)(pDiveState->diveSettings.input_second_to_last_stop_depth_bar * 10);
		depthInc 			= (uint8_t)(pDiveState->diveSettings.input_next_stop_increment_depth_bar * 10);
		if(pDecoinfo->output_stop_length_seconds[0] > 0)
		{
			 depthNext = depthLast;
		}
		else
			return 0;
		for(int i = DECOINFO_STRUCT_MAX_STOPS -1 ;i > 0; i--)
		{
			if(pDecoinfo->output_stop_length_seconds[i] > 0)
			{
				depthNext = depthSecond + ( (i - 1) * depthInc);
				break;
			}
		}
		return depthNext;
}


//  ===============================================================================
//	decom_calc_desaturation_time
/// @brief	This code is used to calculate desat, calculated by RTE and send to Firmware
///					similar but more technical in code than decom_tissues_desaturation_time()
/// 				the later has 0.05 for helium in contrast to this one.
///					This one goes down to 70%, the oterh
///
/// output is desat time in minutes
//  ===============================================================================
int decom_calc_desaturation_time(float* Tissue_nitrogen_bar, float* Tissue_helium_bar, float surface_pressure_bar)
{
	const float N2_ratio = 0.7902; // FIXED sum as stated in b"uhlmann

	float pres_surface;
	float temp_atem;
	float float_desaturation_multiplier;
	float temp1,temp2,temp3,temp4;
	int ci;
	int int_temp;
	int int_O_desaturation_time;
	pres_surface = ((float)surface_pressure_bar);
	temp_atem = N2_ratio * (pres_surface - 0.0627f);

	int_O_desaturation_time = 0;
	float_desaturation_multiplier = 100 / 142.0f; // new in v.101	(70,42%/100.=142)

	for (ci=0;ci<16;ci++)
	{
		// saturation_time (for flight) and N2_saturation in multiples of halftime
		// version v.100: 1.1 = 10 percent distance to totally clean (totally clean is not possible, would take infinite time )
		// new in version v.101: 1.07 = 7 percent distance to totally clean (totally clean is not possible, would take infinite time )
		// changes in v.101: 1.05 = 5 percent dist to totally clean is new desaturation point for display and noFly calculations

		// N2
		 temp1 = 1.05f * temp_atem;
		 temp1 = temp1 - 		(float)Tissue_nitrogen_bar[ci];
		 temp2 = temp_atem - 	(float)Tissue_nitrogen_bar[ci];
		 if (temp2 >= 0)
		 {
			 temp1 = 0;
			 temp2 = 0;
		 }
		 else
			 temp1 = temp1 / temp2;

		 if (temp1 > 0)
		 {
			 temp1 = logf(1.0f - temp1);
			 temp1 = temp1 / -0.6931f;	// temp1 is the multiples of half times necessary.
										// 0.6931 is ln(2), because the math function log() calculates with a base of e not 2 as requested.
										// minus because log is negative
			 temp2 = buehlmann_N2_t_halflife[ci] * temp1 / float_desaturation_multiplier; // time necessary (in minutes ) for complete desaturation (see comment about 10 percent) , new in v.101: float_desaturation_multiplier
			}
		 else
		 {
			 temp1 = 0;
			 temp2 = 0;
		 }

		 // He
		 temp3 = 0.1f - (float)Tissue_helium_bar[ci];
		 if (temp3 >= 0)
		 {
			 temp3 = 0;
			 temp4 = 0;
		 }
		 else
			 temp3 = -1.0f * temp3 / (float)Tissue_helium_bar[ci];
		 if (temp3 > 0)
		 {
			 temp3 = logf(1.0f - temp3);
			 temp3 = temp3 / -0.6931f;	// temp1 is the multiples of half times necessary.
										// 0.6931 is ln(2), because the math function log() calculates with a base of e  not 2 as requested.
										// minus because log is negative
			 temp4 = buehlmann_He_t_halflife[ci] * temp3 / float_desaturation_multiplier; // time necessary (in minutes ) for "complete" desaturation, new in v.101 float_desaturation_multiplier
		 }
		 else
		 {
			 temp3 = 0;
			 temp4 = 0;
		 }

		 // saturation_time (for flight)
		if (temp4 > temp2)
			int_temp = (int)temp4;
		else
			int_temp = (int)temp2;
		if(int_temp > int_O_desaturation_time)
			int_O_desaturation_time = int_temp;

		/*// N2 saturation in multiples of halftime for display purposes
		temp2 = temp1 * 20.0;  // 0 = 1/8, 120 = 0, 249 = 8
		temp2 = temp2 + 80.0; // set center
		if (temp2 < 0.0)
			temp2 = 0.0;
		if (temp2 > 255.0)
			temp2 = 255.0;
		U8_tissue_N2_saturation[ci] = (U8)temp2;
		// He saturation in multiples of halftime for display purposes
		temp4 = temp3 * 20.0;  // 0 = 1/8, 120 = 0, 249 = 8
		temp4 = temp4 + 80.0; // set center
		if (temp4 < 0.0)
			temp4 = 0.0;
		if (temp4 > 255.0)
			temp4 = 255.0;
		U8_tissue_He_saturation[ci] = (char)temp4;*/
	}

	return int_O_desaturation_time;
}
