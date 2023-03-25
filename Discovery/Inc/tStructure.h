///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tStructure.h
/// \brief  Header file for All
/// \author heinrichs weikamp gmbh
/// \date   04-July-2014
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
#ifndef TSTRUCTURE_H
#define TSTRUCTURE_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/** @addtogroup Template
	* @{
	*/


#ifdef __cplusplus
}
#endif

#define _MB(base,menuPage,menuLine,menuEditField,menuEditWhichMode) ((base << 28) + (menuPage << 24) + (menuLine << 16) + (menuEditField << 8) + menuEditWhichMode)

#define BaseInfo 0
#define BaseHome 1
#define BaseMenu 2
#define BaseComm 3
#define BaseStop 6
#define BaseBoot 7

#define PageDebug 0
#define PageSurface 1
#define PageDive 2

#define MaskLineFieldDigit 	_MB(7,15,0,0,0)
#define MaskFieldDigit 		_MB(7,15,255,0,0)
#define MaskAllButLine 		_MB(0,0,255,0,0)
#define MaskLineDigit		_MB(7,15,0,255,0)
#define InfoPageLogList 2
#define InfoPageLogShow 3
#define InfoPageCompass 6
#define InfoPageSensor  8

#define StI 			_MB(0,1,0,0,0)
#define StILOGLIST		_MB(0,2,0,0,0)
#define StILOGSHOW		_MB(0,3,0,0,0)
#define StIDIVE 		_MB(0,4,0,0,0)
#define StISIM			_MB(0,5,0,0,0)
#define StICOMPASS		_MB(0,6,0,0,0)
#define StIDEBUG		_MB(0,7,0,0,0)
#define StISENINFO		_MB(0,8,0,0,0)

#define StI_GoToLogbook			_MB(0,1,1,0,0)
#define StI_GoToPlanner			_MB(0,1,2,0,0)
#define StI_StartSimulator	_MB(0,1,3,0,0)

#define StISIM_Descend			_MB(0,5,1,0,0)
#define StISIM_Ascend				_MB(0,5,2,0,0)
#define StISIM_Divetime			_MB(0,5,3,0,0)
#define StISIM_Exit					_MB(0,5,4,0,0)
#define StISIM_FollowDeco		_MB(0,5,5,0,0)


#define StDEBUG	_MB(1,0,0,0,0)
#define StS			_MB(1,1,0,0,0)
#define StD			_MB(1,2,0,0,0)
#define StDMGAS	_MB(1,2,1,0,0)
#define StDMSPT	_MB(1,2,2,0,0)
#define StDMENU	_MB(1,2,3,0,0)
#define StDSIM1	_MB(1,2,4,0,0)
#define StDSIM2	_MB(1,2,5,0,0)
#define StDSIM3	_MB(1,2,6,0,0)
#define StDSIM4	_MB(1,2,7,0,0)
#define StDBEAR	_MB(1,2,8,0,0)
#define StDRAVG	_MB(1,2,9,0,0)
#define StDQUIT	_MB(1,2,10,0,0)
#define StDMARK	_MB(1,2,11,0,0)
#define StDCHECK _MB(1,2,12,0,0)
#define StDSIM5	_MB(1,2,13,0,0)
#define StDSIM6	_MB(1,2,14,0,0)
#define StDBAILOUT	_MB(1,2,15,0,0)

#define StUART_STANDARD		_MB(3,1,0,0,0)
#define StUART_RTECONNECT	_MB(3,2,0,0,0)

#define StStop			_MB(6,0,0,0,0)
#define StBoot0			_MB(7,0,0,0,0)

/* globalState defines in which visible range the cursor can move */
/* position of the cursor does not affect globalState */
/* next page is a different globalState */
/* next line, field or digit is not a different state */
/* digit is empty at the moment and be reused for future features */

/* PAGE 1 */
#define StMOG 		_MB(2,1,0,0,0)

/* PAGE 1 MENU EDIT */

/* PAGE 1 EDIT FIELD CONTENT */
/* used for PAGE 2 AS WELL */
#define StMOG_Mix						_MB(2,1,255,1,0)
#define StMOG_GasType				_MB(2,1,255,2,0)
#define StMOG_ChangeDepth		_MB(2,1,255,3,0)
#define StMOG_SetToMOD			_MB(2,1,255,4,0)
#define StMOG_Bottle				_MB(2,1,255,5,0)

#define StMOG_MOD					_MB(2,1,255,9,0)

#define StMOG_First					_MB(2,1,255,21,0)
#define StMOG_Deco					_MB(2,1,255,22,0)
#define StMOG_Travel				_MB(2,1,255,23,0)
#define StMOG_Inactive				_MB(2,1,255,24,0)
#define StMOG_Off					_MB(2,1,255,25,0)

#define StMOG_NoTransmitter	_MB(2,1,255,30,0)
#define StMOG_Transmitter1	_MB(2,1,255,31,0)
#define StMOG_Transmitter2	_MB(2,1,255,32,0)
#define StMOG_Transmitter3	_MB(2,1,255,33,0)
#define StMOG_Transmitter4	_MB(2,1,255,34,0)
#define StMOG_Transmitter5	_MB(2,1,255,35,0)
//#define StMOG_Transmitter6	_MB(2,1,255,36,0)
//#define StMOG_Transmitter7	_MB(2,1,255,37,0)
//#define StMOG_Transmitter8	_MB(2,1,255,38,0)
//#define StMOG_Transmitter9	_MB(2,1,255,39,0)

#define StMOG_DM_ActiveBase	_MB(2,1,6,255,0)
#define StMOG_DM_ExtraMix		_MB(2,1,6,6,0)
/*
#define StMOG_DM_Active1		_MB(2,1,6,255,1)
#define StMOG_DM_Active2		_MB(2,1,6,255,2)
#define StMOG_DM_Active3		_MB(2,1,6,255,3)
#define StMOG_DM_Active4		_MB(2,1,6,255,4)
#define StMOG_DM_Active5		_MB(2,1,6,255,5)
#define StMOG_DM_Active6		_MB(2,1,6,255,6)
#define StMOG_DM_Active7		_MB(2,1,6,255,7)
#define StMOG_DM_Active8		_MB(2,1,6,1,8)
#define StMOG_DM_Active9		_MB(2,1,6,1,9)
#define StMOG_DM_Active10		_MB(2,1,6,1,10)
*/

/*
#define StMOG_Depth					_MB(2,1,255, 2,0)
#define StMOG_DefaultDepth	_MB(2,1,255, 4,0)
#define StMOG_Reset					_MB(2,1,255, 5,0)
#define StMOG_ToggleDefault	_MB(2,1,255, 6,0)
#define StMOG_SensorLink		_MB(2,1,255, 9,0)
#define StMOG_Size					_MB(2,1,255,10,0)
*/

/* PAGE 2 */
#define StMCG			_MB(2,2,0,0,0)
/* PAGE 3 */
#define StMSP			_MB(2,3,0,0,0)

/* PAGE 3 EDIT FIELD CONTENT */
#define StMSP_ppo2_setting	_MB(2,3,255,1,0)
#define StMSP_Depth					_MB(2,3,255,2,0)
#define StMSP_Active				_MB(2,3,255,3,0)
#define StMSP_First					_MB(2,3,255,4,0)
#define StMSP_Select				_MB(2,3,255,5,0)
#define StMSP_Sensor1				_MB(2,3,255,6,0)
#define StMSP_Sensor2				_MB(2,3,255,7,0)
#define StMSP_Sensor3				_MB(2,3,255,8,0)

/* PAGE 4 */
#define StMXTRA   _MB(2,4,0,0,0)

/* PAGE 4 MENU EDIT */
/* DIVE MODE */
#define StMXTRA_ResetStopwatch	_MB(2,4,1,1,0)
#define StMXTRA_CompassHeading	_MB(2,4,2,1,0)
#define StMXTRA_CompassHeadingClear	_MB(2,4,2,2,0)
#define StMXTRA_CompassHeadingReset	_MB(2,4,2,3,0)

 /* SURFACE MODE */

#define StMXTRA_CCRmode					_MB(2,4,1,1,0)
#define StMXTRA_O2_Fallback				_MB(2,4,2,1,0)
#define StMXTRA_ScrubTimer				_MB(2,4,3,1,0)
#define StMXTRA_ScrubTimer_Max			_MB(2,4,3,2,0)
#define StMXTRA_ScrubTimer_Reset		_MB(2,4,3,3,0)
#define StMXTRA_ScrubTimer_OP_Mode		_MB(2,4,3,5,0)

#define StMXTRA_PSCR_O2_Drop			_MB(2,4,4,1,0)
#define StMXTRA_PSCR_LUNG_RATIO			_MB(2,4,4,2,0)
#define StMXTRA_CO2_Sensor				_MB(2,4,5,1,0)
#define StMXTRA_CO2_Sensor_Calib		_MB(2,4,5,2,0)




/* PAGE 5 */
#define StMDECO		_MB(2,5,0,0,0)

/* PAGE 5 MENU EDIT */
/*
#define StMDECO1	_MB(2,5,1,0,0)
#define StMDECO2	_MB(2,5,2,0,0)
#define StMDECO3	_MB(2,5,3,0,0)
#define StMDECO4	_MB(2,5,4,0,0)
#define StMDECO5	_MB(2,5,5,0,0)
#define StMDECO6	_MB(2,5,6,0,0)
*/

/* PAGE 5 EDIT FIELD CONTENT */
#define StMDECO1_OC						_MB(2,5,1,1,0)
#define StMDECO1_CC						_MB(2,5,1,2,0)
#define StMDECO1_Apnea					_MB(2,5,1,3,0)
#define StMDECO1_Gauge					_MB(2,5,1,4,0)
#define StMDECO1_PSCR					_MB(2,5,1,5,0)

#define StMDECO3_PPO2Max			_MB(2,5,2,1,0)
#define StMDECO4_SafetyStop			_MB(2,5,3,1,0)
#define StMDECO5_FUTURE				_MB(2,5,4,1,0)
#define StMDECO6_SALINITY			_MB(2,5,5,1,0)

/* PAGE 6 */
#define StMDECOP	_MB(2,6,0,0,0)

#define StMDECOP1_Algorithm		_MB(2,6,1,1,0)
#define StMDECOP2_VPM					_MB(2,6,2,1,0)
#define StMDECOP3_GF					_MB(2,6,3,1,0)
#define StMDECOP4_AltGF				_MB(2,6,4,1,0)
#define StMDECOP5_LASTSTOP		_MB(2,6,5,1,0)

#define StMDECOP7_ActiveGF		_MB(2,6,7,1,0)
#define StMDECOP8_ActiveVPM		_MB(2,6,8,1,0)
#define StMDECOP9_ActiveAltGF	_MB(2,6,9,1,0)

/* PAGE 7 */
#define StMHARD		_MB(2,7,0,0,0)

/* PAGE 7 EDIT FIELD CONTENT */


#define StMHARD1_Bluetooth			_MB(2,7,1,1,0)

#define StMHARD2_Compass_SetCourse		_MB(2,7,2,2,0)
#define StMHARD2_Compass_ResetCourse	_MB(2,7,2,3,0)
#define StMHARD2_Compass_Calibrate		_MB(2,7,2,4,0)
#define StMHARD2_Compass_Inertia		_MB(2,7,2,5,0)

//#define StMHARD2_Exit						_MB(2,7,2,2,0)

#define StMHARD3_Sensors			_MB(2,7,3,0,0)
#define StMHARD3_O2_Sensor1			_MB(2,7,3,1,0)
#define StMHARD3_O2_Sensor2			_MB(2,7,3,2,0)
#define StMHARD3_O2_Sensor3			_MB(2,7,3,3,0)
#define StMHARD3_O2_Calibrate		_MB(2,7,3,4,0)
#define StMHARD3_Sensor_Info		_MB(2,7,3,5,0)
#define StMHARD3_Sensor_Detect		_MB(2,7,3,6,0)


#define StMHARD4_BrightnessEco	_MB(2,7,4,1,0)
#define StMHARD4_BrightnessStd	_MB(2,7,4,2,0)
#define StMHARD4_BrightnessHigh	_MB(2,7,4,3,0)
#define StMHARD4_BrightnessMax	_MB(2,7,4,4,0)

#define StMHARD5_Button1		_MB(2,7,5,1,0)
#define StMHARD5_ButtonBalance1	_MB(2,7,5,2,0)
#define StMHARD5_ButtonBalance2	_MB(2,7,5,3,0)
#define StMHARD5_ButtonBalance3	_MB(2,7,5,4,0)
#define StMHARD5_ButtonLock		_MB(2,7,5,5,0)

//#define StMHARD6_UpdateCPU2_No	_MB(2,7,6,1,0)
//#define StMHARD6_UpdateCPU2_Yes	_MB(2,7,6,2,0)
//#define StMHARD6_UpdateCPU2_Now	_MB(2,7,6,3,0)

/* PAGE 8 */
#define StMSYS		_MB(2,8,0,0,0)

/* PAGE 8 EDIT FIELD CONTENT */
#define StMSYS1_Date		_MB(2,8,1,1,0)
#define StMSYS1_Time		_MB(2,8,1,2,0)
#define StMSYS1_DDMMYY	_MB(2,8,1,3,0)
#define StMSYS1_MMDDYY	_MB(2,8,1,4,0)
#define StMSYS1_YYMMDD	_MB(2,8,1,5,0)
#define StMSYS1_DST			_MB(2,8,1,6,0)
#define StMSYS1_12HR    _MB(2,8,1,7,0)

#define StMSYS2_English	_MB(2,8,2,1,0)
#define StMSYS2_German	_MB(2,8,2,2,0)
#define StMSYS2_French	_MB(2,8,2,3,0)
#define StMSYS2_Italian	_MB(2,8,2,4,0)
#define StMSYS2_Espanol	_MB(2,8,2,5,0)

#define StMSYS3_Units		_MB(2,8,3,1,0)
#define StMSYS3_Colors	_MB(2,8,3,2,0)
#define StMSYS3_Debug		_MB(2,8,3,3,0)

#define StMSYS4_Info		_MB(2,8,4,1,0)

#define StMSYS5_Exit			_MB(2,8,5,1,0)
#define StMSYS5_LogbookOffset	_MB(2,8,5,7,0)
#define StMSYS5_ResetAll		_MB(2,8,5,2,0)
#define StMSYS5_ResetDeco		_MB(2,8,5,3,0)
#define StMSYS5_Reboot			_MB(2,8,5,4,0)
#define StMSYS5_Maintenance		_MB(2,8,5,5,0)
#define StMSYS5_ResetLogbook	_MB(2,8,5,6,0)
#define StMSYS5_SetBattCharge	_MB(2,8,5,7,0)
#define StMSYS5_RebootRTE		_MB(2,8,5,8,0)
#define StMSYS5_RebootMainCPU	_MB(2,8,5,9,0)
#define StMSYS5_ScreenTest		_MB(2,8,5,10,0)
#define StMSYS5_SetFactoryBC	_MB(2,8,5,11,0)
#define StMSYS5_ResetBluetooth	_MB(2,8,5,12,0)
#define StMSYS5_SetSampleIndx   _MB(2,8,5,13,0)

 /* PAGE 9 */

 # define StMCustom 	_MB(2,9,0,0,0)

 /* PAGE 9 EDIT FIELD CONTENT */
//

#define StMCustom1_CViewTimeout		_MB(2,9,1,1,0)
#define StMCustom1_CViewStandard	_MB(2,9,1,2,0)
#define StMCustom1_CViewStandardBF	_MB(2,9,1,3,0)
#define StMCustom1_CornerTimeout	_MB(2,9,1,4,0)
#define StMCustom1_CornerStandard	_MB(2,9,1,5,0)

#define StMCustom2_BFSelection		_MB(2,9,2,1,0)

#define StMCustom3_CViewSelection1	_MB(2,9,3,1,0)
#define StMCustom3_CViewSelection2	_MB(2,9,3,2,0)
#define StMCustom3_CViewSelection3	_MB(2,9,3,3,0)
#define StMCustom3_CViewSelection4	_MB(2,9,3,4,0)
#define StMCustom3_CViewSelection5	_MB(2,9,3,5,0)
#define StMCustom3_CViewSelection6	_MB(2,9,3,6,0)

#define StMCustom4_CViewSelection1	_MB(2,9,4,1,0)

#define StMCustom5_CViewPortCalib	_MB(2,9,5,1,0)
#define StMCustom5_CViewPortSpotSize _MB(2,9,5,2,0)
#define StMCustom5_CViewPortLayout	_MB(2,9,5,3,0)
#define StMCustom5_CViewPortAmbient	_MB(2,9,5,4,0)
#define StMCustom5_CViewPortControl	_MB(2,9,5,5,0)


/* PAGE 10 */
#define StMPLAN		_MB(2,10,0,0,0)

/* PAGE 10 EDIT FIELD CONTENT */
#define StMPLAN2_Interval			_MB(2,10,2,1,0)
#define StMPLAN3_DiveTime			_MB(2,10,3,1,0)
#define StMPLAN4_MaxDepth			_MB(2,10,4,1,0)
#define StMPLAN5_ExitResult			_MB(2,10,5,1,0)
#define StMPLAN4_Settings			_MB(2,10,6,1,0)





//#define StMDECO6_SALINITY			_MB(2,5,6,1,0)
//#define StMDECO6_DefaultSAL		_MB(2,5,6,2,0)




//#define StMDECO7_AMV				_MB(2,5,7,1,0)
//#define StMDECO7_DefaultAMV		_MB(2,5,7,2,0)
//#define StMDECO7_RESERVE			_MB(2,5,7,3,0)


/*
typedef struct
{
	uint32_t 	id;
	int8_t 		implemented;
	int8_t 		enabled;
	uint32_t 	pointer;
	uint8_t		type;
} SStateOfMachine;
*/

typedef struct
{
	uint8_t 	base;
	uint8_t 	page;
	uint8_t 	line;
	uint8_t 	field;
	uint8_t		mode;
} SStateList;



#endif /* TSTRUCTURE_H */

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
