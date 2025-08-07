///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/cv_heartbeat.h
/// \brief  Function definitions for connecting to a Polar HC10 heartbeat sensor
/// \date   3 July 2025

///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2015 Heinrichs Weikamp gmbh
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

#ifndef INC_CV_HEARTBEAT_H_
#define INC_CV_HEARTBEAT_H_

#include <stdint.h>


#define BLUEMOD_ADDR_SIZE		(20u)		/* length of address respond */
#define BLUEMOD_RSSI_SIZE		(5u)
#define BLUEMOD_NAME_SIZE		(40u)

void openEdit_Heartbeat(void);

typedef enum
{
	NO_INDICATOR = 0,
	DEVICE_INDICATOR,
	CONNECTION_INDICATOR,
	SERVICE_INDICATOR,
	CHARACTERISTIC_INDICATOR,
	DESCRIPTOR_INDICATOR,
	PULSE_INDICATOR,
	OK_INDICATOR,			/* module control */
	ERROR_INDICATOR			/* module control */
} indicatior_t;

typedef enum
{
	BT_READ_NOTHING = 0,
	BT_READ_DEVICE_ADDR,
	BT_READ_DEVICE_RSSI,
	BT_READ_DEVICE_NAME,
	BT_READ_CON_DETAILS,
	BT_READ_SERV_HANDLE,
	BT_READ_SERV_START,
	BT_READ_SERV_END,
	BT_READ_SERV_UUID,
	BT_READ_CHAR_CONHANDLE,
	BT_READ_CHAR_ATTRIBUTE,
	BT_READ_CHAR_PROPERTY,
	BT_READ_CHAR_VALUEHANDLE,
	BT_READ_CHAR_UUID,
	BT_READ_DESC_CONHANDLE,
	BT_READ_DESC_CHARHANDLE,
	BT_READ_DESC_DESCHANDLE,
	BT_READ_DESC_UUID,
	BT_READ_PULSE_CONHANDLE,
	BT_READ_PULSE_VALUEHANDLE,
	BT_READ_PULSE_DATA,
} readDataType_t;

typedef enum
 {
	SENSOR_HB_OFFLINE = 0,		/* Default Status no data available  */
	SENSOR_HB_ENABLE_BLE,
	SENSOR_HB_CHECK_CONFIG,
	SENSOR_HB_DISCOVER,
	SENSOR_HB_CONNECT,
	SENSOR_HB_DISCONNECT,
	SENSOR_HB_SERVICES,
	SENSOR_HB_CHARACTERISTIC,
	SENSOR_HB_DESCRIPTOR,
	SENSOR_HB_SUBSCRIBE,
	SENSOR_HB_RESTART,
	SENSOR_HB_DETECTION_INDICATOR,		/* searching for indicators to identify data items */
	SENSOR_HB_DETECTION_RSSI,
	SENSOR_HB_DETECTION_NAME,
	SENSOR_HB_DETECTION_MAN,
	SENSOR_HB_DETECTION_UUID,
	SENSOR_HB_FOUND,		/* A device providing the requested service was found */
	SENSOR_HB_CONNECTED,	/* Connection to heartbeat sensor established */
 	SENSOR_HB_OFFLINEMODE,	/* Oflline measurement started */
 } sensorHeartbeat_State_t;

typedef struct
{
	uint8_t address[BLUEMOD_ADDR_SIZE];
	uint8_t rssi[BLUEMOD_RSSI_SIZE];
	uint8_t name[BLUEMOD_NAME_SIZE];
} btDdeviceData_t;


typedef struct
{
	uint8_t handle;
	uint8_t start[6];
	uint8_t end[6];
	uint8_t uuid[50];
} btDeviceService_t;

typedef struct
{
	uint8_t conHandle;
	uint8_t attrHandle[10];
	uint8_t properties[10];
	uint8_t valueHandle[10];
	uint8_t uuid[50];
} btDeviceCharacteristic_t;

typedef struct
{
	uint8_t conHandle;
	uint8_t charHandle[10];
	uint8_t descHandle[10];
	uint8_t uuid[50];
} btDeviceDescriptor_t;

typedef struct {
    uint16_t heart_rate;
    uint16_t energy_expended;
    uint16_t rr_intervals[10];
    uint8_t rr_count;
} HRMeasurement_t;

sensorHeartbeat_State_t cv_heartbeat_getState();
void refresh_Heartbeat(void);
void cv_heartbeat_Control(void);
uint8_t cv_heartbeat_HandleData();

#endif /* INC_CV_HEARTBEAT_H_ */
