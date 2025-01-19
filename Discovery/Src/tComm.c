///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tComm.c
/// \brief  Main file for communication with PC
/// \author heinrichs weikamp gmbh
/// \date   08-Aug-2014
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

/**
  ==============================================================================
                        ##### How to use #####
  ==============================================================================
    ==============================================================================
              ##### History #####
  ==============================================================================
    160211 added 4 bytes Serial in update Files after checksum prior to binary
    160211 0x6B changed to version only
    160623 fixed 0x72 (in V1.0.9)
    160623 fixed rebuild menu (before update) for V1.0.10

    ==============================================================================
              ##### CTS / RTS #####
  ==============================================================================
    RTS is Output, CTS is Input

    BlueMod Pin D7 UART-RTS# is Output
    connected to STM32F429 PA11 CTS (Input)
    also STM32 PA12 RTS is connected to BlueMod UART-CTS# F3

    see BlueMod_SR_HWreference_r06.pdf, page 156
    and MAIN_CPU STM32F4 Reference manual DM00031020.pdf, page 990


    ==============================================================================
              ##### Codes #####
  ==============================================================================
  [0x73] upload CPU2 firmware in SDRAM and update CPU2

  [0x74] upload MainCPU firmware in EEPROM and start bootloader

  */

/* Includes ------------------------------------------------------------------*/

#include "tComm.h"

#include "externCPU2bootloader.h"
#include "externLogbookFlash.h"
#include "gfx_colors.h"
#include "gfx_engine.h"
#include "gfx_fonts.h"
#include "ostc.h"

#ifndef BOOTLOADER_STANDALONE
#	include "base.h"
#	include "tHome.h"
#	include "logbook.h"
#	include "tMenu.h"
#else
#	include "base_bootloader.h"
#	include "firmwareEraseProgram.h"
#	include "text_multilanguage.h"
#endif

#ifdef SPECIALPROGRAMM
#	include "firmwareEraseProgram.h"
#endif
#include <stdlib.h>
#include <string.h>


/* Private variables ---------------------------------------------------------*/
GFX_DrawCfgScreen	tCscreen;
GFX_DrawCfgWindow	tCwindow;

uint8_t receiveStartByteUart = 0;
uint8_t bluetoothActiveLastTime = 0;

uint8_t StartListeningToUART = 0;
char display_text[256] = { 0 };

uint8_t setForcedBluetoothName = 0;

uint8_t updateSettingsAndMenuOnExit = 0;

/* Private types -------------------------------------------------------------*/
#define BYTE_DOWNLOAD_MODE			(0xBB)
#define BYTE_SERVICE_MODE			(0xAA)

#define UART_OPERATION_TIMEOUT		(500u)		/* Timeout for common read / write operations (ms) */
#define UART_TIMEOUT_SECONDS		(120u)		/* Timeout for keeping connection open and waiting for data */
#define UART_TIMEOUT_LARGE_BLOCK 	(6000u)		/* Timeout (ms) for reception of an 16K data block (typical RX time ~4,5seconds) */

#define UART_CMD_BUF_SIZE			(30u)		/* size of buffer for command exchange */

const uint8_t id_Region1_firmware = 0xFF;
const uint8_t id_RTE = 0xFE;
const uint8_t id_FONT = 0x10;
const uint8_t id_FONT_OLD = 0x00;

static BlueModTmpConfig_t BmTmpConfig = BM_CONFIG_OFF;	/* Config BlueMod without storing the changes */
static uint8_t EvaluateBluetoothSignalStrength = 0;
#ifndef BOOTLOADER_STANDALONE
static uint8_t RequestDisconnection = 0; 				/* Disconnection from remote device requested */
static void tComm_Disconnect(void);
#endif
/* Private function prototypes -----------------------------------------------*/
static void tComm_Error_Handler(void);
static uint8_t select_mode(uint8_t aRxByte);
static uint8_t tComm_CheckAnswerOK(void);
static uint8_t tComm_HandleBlueModConfig(void);
static void tComm_EvaluateBluetoothStrength(void);
uint8_t receive_update_flex(uint8_t isRTEupdateALLOWED);
uint8_t receive_update_data_flex(uint8_t* pBuffer1, uint8_t* pBuffer2, uint8_t RTEupdateALLOWED);
uint8_t receive_update_data_mainCPU_firmware(void);
uint8_t receive_update_data_mainCPU_variable_firmware(void);
uint8_t receive_update_data_mainCPU_firmware_subroutine(uint8_t region, uint8_t* pBuffer1, uint8_t* pBuffer2);
HAL_StatusTypeDef receive_uart_large_size(UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size);
static uint8_t openComm(uint8_t aRxByte);
uint8_t HW_Set_Bluetooth_Name(uint16_t serial, uint8_t withEscapeSequence);
uint8_t prompt4D4C(uint8_t mode);
uint8_t tComm_GetBTCmdStr(BTCmd cmdId, char* pCmdStr);

#ifdef BOOTLOADER_STANDALONE
    static uint8_t receive_update_data_cpu2(void);
    uint8_t receive_update_data_cpu2_sub(uint8_t* pBuffer);
#endif

/* Exported functions --------------------------------------------------------*/

void tComm_init(void)
{
    tCscreen.FBStartAdress = 0;
    tCscreen.ImageHeight = 480;
    tCscreen.ImageWidth = 800;
    tCscreen.LayerIndex = 1;

    tCwindow.Image = &tCscreen;
    tCwindow.WindowNumberOfTextLines = 6;
    tCwindow.WindowLineSpacing = 65;
    tCwindow.WindowTab = 400;
    tCwindow.WindowX0 = 20;
    tCwindow.WindowX1 = 779;


    if(!settingsGetPointer()->FlipDisplay)
    {
        tCwindow.WindowY0 = 0;
        tCwindow.WindowY1 = 479;
    }
    else
    {
    	tCwindow.WindowY0 = 479 - 390;
    	tCwindow.WindowY1 = 479 - 25;
    }

    StartListeningToUART = 1;
}

uint8_t tComm_control(void)
{
    uint8_t answer  = 0;
#ifndef BOOTLOADER_STANDALONE

    /* should do something like reset UART ... */
    if(	settingsGetPointer()->bluetoothActive == 0)
    {
        if(bluetoothActiveLastTime)
        {
        	HAL_UART_AbortReceive_IT(&UartHandle);
            HAL_UART_DeInit(&UartHandle);
            HAL_Delay(1);
            UartHandle.Init.BaudRate   = 115200;	/* Module will be operating at default baud rate if powered again */
            BmTmpConfig = BM_CONFIG_OFF;			/* Restart configuration if powered again */
            HAL_UART_Init(&UartHandle);
            HAL_Delay(1);
            UartReady = RESET;
            StartListeningToUART = 1;
            bluetoothActiveLastTime = 0;
            receiveStartByteUart = 0;
            RequestDisconnection = 0;
        }
        return 0;
    }
    else
    {
        bluetoothActiveLastTime = 1;
        if(RequestDisconnection)
        {
        	RequestDisconnection = 0;
            tComm_Disconnect();
        }
    }

#endif

    if(BmTmpConfig != BM_CONFIG_DONE)
    {
    	tComm_HandleBlueModConfig();
    }
    else
    {
    /*##-2- Put UART peripheral in reception process ###########################*/

		if((UartReady == RESET) && StartListeningToUART)
		{
				StartListeningToUART = 0;
				if(HAL_UART_Receive_IT(&UartHandle, &receiveStartByteUart, 1) != HAL_OK)
						tComm_Error_Handler();
		}
		/* Reset transmission flag */
		if(UartReady == SET)
		{
				UartReady = RESET;
				if((receiveStartByteUart == BYTE_DOWNLOAD_MODE) || (receiveStartByteUart == BYTE_SERVICE_MODE))
					answer = openComm(receiveStartByteUart);
				StartListeningToUART = 1;
				return answer;
		}
    }
    return 0;
}


void tComm_refresh(void)
{
	char localString[255];

    if(tCscreen.FBStartAdress == 0)
    {
        GFX_hwBackgroundOn();
        tCscreen.FBStartAdress = getFrame(18);
        write_content_simple(&tCscreen, 0, 800, 480-24, &FontT24,"Exit",CLUT_ButtonSurfaceScreen);
        write_content_simple(&tCscreen, 800 - 70, 800, 480-24, &FontT24,"Signal",CLUT_ButtonSurfaceScreen);

        if(receiveStartByteUart == BYTE_SERVICE_MODE)
            GFX_write_string(&FontT48, &tCwindow, "Service mode enabled",2);
        else
            GFX_write_string(&FontT48, &tCwindow, "Download mode enabled",2);
        GFX_SetFramesTopBottom(tCscreen.FBStartAdress, 0,480);
        display_text[0] = 0;
        display_text[255] = 0;
    }
    else if(display_text[255])
    {
        display_text[(uint8_t)display_text[255]] = 0;
        localString[0] = TXT_MINIMAL;
        strcpy (&localString[1],display_text);
        releaseFrame(18,tCscreen.FBStartAdress);
        tCscreen.FBStartAdress = getFrame(18);
        write_content_simple(&tCscreen, 0, 800, 480-24, &FontT24,"Exit",CLUT_ButtonSurfaceScreen);
        write_content_simple(&tCscreen, 800 - 70, 800, 480-24, &FontT24,"Signal",CLUT_ButtonSurfaceScreen);
        GFX_write_string(&FontT48, &tCwindow, localString,2);
        GFX_SetFrameTop(tCscreen.FBStartAdress);
        display_text[0] = 0;
        display_text[255] = 0;
    }
}


void tComm_verlauf(uint8_t percentage_complete)
{
    uint32_t pDestination;

    pDestination = (uint32_t)tCscreen.FBStartAdress;
    pDestination += 150 * tCscreen.ImageHeight * 2;
    pDestination += 100 * 2;

    if(percentage_complete > 100)
        percentage_complete = 100;

    int i = 1;
    while(i<=percentage_complete)
    {
        i += 1;
        for(int y=0;y<4;y++)
        {
            for(int x=0;x<40;x++)
            {
                *(__IO uint16_t*)pDestination = 0xFF00 + 00;
                    pDestination += 2;
            }
            pDestination += (tCscreen.ImageHeight  - 40 )* 2;
        }
        pDestination += tCscreen.ImageHeight * 2; // one spare line
    }
}


void tComm_exit(void)
{
    SStateList status;
    get_globalStateList(&status);

    releaseFrame(18,tCscreen.FBStartAdress);
    tCscreen.FBStartAdress = 0;
    GFX_hwBackgroundOff();

    if(setForcedBluetoothName)
    {
        setForcedBluetoothName = 0;
        MX_Bluetooth_PowerOff();
        HAL_Delay(1000);
        MX_Bluetooth_PowerOn();
        tComm_Set_Bluetooth_Name(1);
        tComm_StartBlueModConfig();
    }
#ifndef BOOTLOADER_STANDALONE
    if(updateSettingsAndMenuOnExit)
    {
        check_and_correct_settings();
        createDiveSettings();
        tM_rebuild_menu_after_tComm();
    }
#endif
    updateSettingsAndMenuOnExit = 0;

    if(status.base == BaseComm)
    {
#ifndef BOOTLOADER_STANDALONE
    set_globalState_tHome();
#else
    set_globalState_Base();
#endif
    }
    settingsGetPointer()->bluetoothActive = 0;
    MX_Bluetooth_PowerOff();						// Power down Bluetooth on the way out
}


uint8_t tComm_Set_Bluetooth_Name(uint8_t force)
{
    uint8_t answer = 0;

    if(hardwareDataGetPointer()->secondarySerial != 0xFFFF)
    {
        if(force || (hardwareDataGetPointer()->secondary_bluetooth_name_set == 0xFF))
            answer = HW_Set_Bluetooth_Name(hardwareDataGetPointer()->secondarySerial, 0);
#ifdef BOOTLOADER_STANDALONE
            if(answer == HAL_OK)
                hardware_programmSecondaryBluetoothNameSet();
#endif
    }
    else
    if(hardwareDataGetPointer()->primarySerial != 0xFFFF)
    {
        if(force || (hardwareDataGetPointer()->production_bluetooth_name_set == 0xFF))
            answer = HW_Set_Bluetooth_Name(hardwareDataGetPointer()->primarySerial, 0);
#ifdef BOOTLOADER_STANDALONE
            if(answer == HAL_OK)
                hardware_programmPrimaryBluetoothNameSet();
#endif
    }
    else	/* no serial set at all => do default configuration of the module */
    {
#define NINAB22103B00
#ifdef NINAB22103B00
    	answer = 0xFF;
#endif
    }
    return answer;
}


uint8_t HW_Set_Bluetooth_Name(uint16_t serial, uint8_t withEscapeSequence)
{
    uint8_t answer = HAL_OK;
    uint8_t aRxBuffer[50];
    char aTxBufferName[50];

//	char aTxFactoryDefaults[50] = "AT&F1\r";

    char aTxBufferEscapeSequence[50] = "+++";	/* factory default */
    // limit is 19 chars, with 7 chars shown in BLE advertising mode
    //________________________123456789012345678901

    tComm_GetBTCmdStr(BT_CMD_NAME, aTxBufferName);

    char answerOkay[6] = "\r\nOK\r\n";

    gfx_number_to_string(5,1,&aTxBufferName[15],serial);

    // store active configuration in non-volatile memory
    char aTxBufferWrite[50] = "AT&W\r";

//	char aTxBufferReset[50] = "AT+RESET\r";


    HAL_Delay(1010);
    if(withEscapeSequence)
    {
        aRxBuffer[0] = 0;
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferEscapeSequence, 3, 2000)!= HAL_OK)
            answer = HAL_ERROR;
        HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 3, 2000);
        HAL_Delay(1010);

        for(int i=0;i<3;i++)
        if(aRxBuffer[i] != '+')
            answer = HAL_ERROR;
    }

    aRxBuffer[0] = 0;
    if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferName, 21, 2000)!= HAL_OK)
        answer = HAL_ERROR;
    HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 21+6, 2000);

    for(int i=0;i<21;i++)
    if(aRxBuffer[i] != aTxBufferName[i])
        answer = HAL_ERROR;

    for(int i=0;i<6;i++)
    if(aRxBuffer[21+i] != answerOkay[i])
        answer = HAL_ERROR;

    HAL_Delay(200);

    if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferWrite, 5, 2000)!= HAL_OK)
        answer = HAL_ERROR;
    HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 5+6, 2000);

    for(int i=0;i<5;i++)
    if(aRxBuffer[i] != aTxBufferWrite[i])
        answer = HAL_ERROR;

    for(int i=0;i<6;i++)
    if(aRxBuffer[5+i] != answerOkay[i])
        answer = HAL_ERROR;

    answer = HAL_OK;
    return answer;
}
#ifndef BOOTLOADER_STANDALONE
void tComm_Disconnect()
{
	uint8_t answer;
	uint8_t retrycnt = 3;
	char aTxDisconnect[] ="ATH\r";
	char aTxBufferEnd[10];
	char aTxBufferEscapeSequence[] = "+++";

	uint8_t sizeDisconnect = sizeof(aTxDisconnect) -1;

	tComm_GetBTCmdStr(BT_CMD_EXIT_CMD, aTxBufferEnd);
	HAL_UART_AbortReceive_IT(&UartHandle);
	do
	{
		HAL_Delay(200);
		if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferEscapeSequence, 3, UART_OPERATION_TIMEOUT)== HAL_OK)
		{
			answer = tComm_CheckAnswerOK();
		}
		retrycnt--;
	}
	while((answer != HAL_OK) && (retrycnt > 0));

	if(answer == HAL_OK)
	{
		answer = HAL_ERROR;
		if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxDisconnect,sizeDisconnect , UART_OPERATION_TIMEOUT)== HAL_OK)
		{
			answer = HAL_ERROR;
			if(tComm_CheckAnswerOK() == HAL_OK)
			{
				answer = HAL_ERROR;
				if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferEnd, 4, UART_OPERATION_TIMEOUT) == HAL_OK)	/* exit terminal mode */
				{
					answer = tComm_CheckAnswerOK();
				}
			}
		}
	}

	if(answer != HAL_OK)		/* we are somehow not able to do a clean disconnect => fallback to "previous" power off implementation" */
	{
		settingsGetPointer()->bluetoothActive = 0;
		MX_Bluetooth_PowerOff();
	}
}
#endif

uint8_t openComm(uint8_t aRxByte)
{
	SStateList status;
	uint8_t localRx;
	uint8_t timeoutCounter = 0;
    uint8_t answer = 0;
    uint8_t service_mode_last_three_bytes[3];
    uint8_t service_mode_response[5] =
    {
        0x4B,
        0xAB,
        0xCD,
        0xEF,
        0x4C
    };
    uint8_t download_mode_response[2] =
    {
        0xBB,
        0x4D
    };

    if((aRxByte != BYTE_DOWNLOAD_MODE) && (aRxByte != BYTE_SERVICE_MODE))
        return 0;

    set_globalState(StUART_STANDARD);

    /* service mode is four bytes
    0xAA 0xAB 0xCD 0xEF
    answer is
    */
    localRx = aRxByte;

    if(aRxByte == BYTE_SERVICE_MODE)
    {
        if((HAL_UART_Receive(&UartHandle, (uint8_t*)service_mode_last_three_bytes, 3, 2000)!= HAL_OK))
            answer = 0x00;
        else
        {
            if((service_mode_last_three_bytes[0] != 0xAB) || (service_mode_last_three_bytes[1] != 0xCD) || (service_mode_last_three_bytes[2] != 0xEF))
                answer = 0x00;
            else
            {
                if(HAL_UART_Transmit(&UartHandle, (uint8_t*)service_mode_response, 5, 2000)!= HAL_OK)
                    answer = 0x00;
                else
                    answer = prompt4D4C(receiveStartByteUart);
            }
        }
    }
    else	//if(aRxByte == BYTE_SERVICE_MODE)
    {
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)download_mode_response, 2, 2000)!= HAL_OK)
            answer = 0x00;
        else
            answer = prompt4D4C(receiveStartByteUart);
    }

    while((answer == prompt4D4C(receiveStartByteUart)) && (timeoutCounter < UART_TIMEOUT_SECONDS)) 	/* try receive once a second */
    {
    	if(HAL_UART_Receive(&UartHandle, (uint8_t*)&localRx, 1, UART_OPERATION_TIMEOUT)!= HAL_OK)
    	{
    		timeoutCounter++;
    		get_globalStateList(&status);
    		if (status.base != BaseComm)
    		{
    			timeoutCounter = UART_TIMEOUT_SECONDS; /* Abort action triggered outside main loop => exit */
    		}
    		if(EvaluateBluetoothSignalStrength)
    		{
    			tComm_EvaluateBluetoothStrength();
    		}
    	}
    	else
    	{
    		answer = select_mode(localRx);
    		timeoutCounter = 0;
    	}
    }
    set_returnFromComm();
    return 1;
}


uint8_t prompt4D4C(uint8_t mode)
{
    if(mode == BYTE_SERVICE_MODE)
        return 0x4C;
    else
        return 0x4D;
}


uint8_t select_mode(uint8_t type)
{
#ifndef BOOTLOADER_STANDALONE
    SLogbookHeader logbookHeader;
    SLogbookHeaderOSTC3 * plogbookHeaderOSTC3;
    SLogbookHeaderOSTC3compact * plogbookHeaderOSTC3compact;
    uint32_t sampleTotalLength;
    SSettings* pSettings = settingsGetPointer();
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;
    uint16_t index;
    uint32_t header_profileLength, OSTC3_profileLength;
#else
    uint8_t dummyForBootloader[256] = {0};
#endif
    uint8_t count;
    uint8_t aTxBuffer[128];
    uint8_t aRxBuffer[68];
    uint8_t answer;
    aTxBuffer[0] = type;
    aTxBuffer[1] = prompt4D4C(receiveStartByteUart);
    uint8_t tempHigh, tempLow;
    count = 0;

    // service mode only commands
    if(receiveStartByteUart == BYTE_SERVICE_MODE)
    {
        // first part
        switch(type)
        {
        // start communication (again)
        case 0xAA:
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 2, 1000)!= HAL_OK)
                return 0;
            else
                return prompt4D4C(receiveStartByteUart);

/*
        // update firmware main preparation
        case 0x74:
            ext_flash_erase_firmware_if_not_empty();
            break;

        // update firmware main with variable full access memory location preparation
        case 0x76:
            ext_flash_erase_firmware2_if_not_empty();
            break;
*/
        default:
            break;
        }

#ifndef BOOTLOADER_STANDALONE
        uint32_t logCopyDataPtr = 0;
        convert_Type logCopyDataLength;
        uint32_t logCopyDataPtrTemp = 0;
        uint32_t logCopyDataLengthTemp = 0;
        uint8_t logDummyByte = 0;
        uint8_t logStepBackwards = 0;
        convert16_Type totalDiveCount;
        logCopyDataLength.u32bit = 0;
        totalDiveCount.u16bit = 0;
#endif

        // Exit communication on Text like RING, CONNECT, ... or 0xFF command
        if((type < 0x60) || (type == 0xFF))
            return 0;

        // return of command for (almost) all commands
        switch(type)
        {
        // not supported yet case 0x20: // 	send hi:lo:temp1 bytes starting from ext_flash_address:3
        // not supported yet case 0x22: // 	Resets all logbook pointers and the logbook (!)
        // not supported yet case 0x23: // 	Resets battery gauge registers
        // not supported yet case 0x30: // 	write bytes starting from ext_flash_address:3 (Stop when timeout)
        // not supported yet case 0x40: // 	erases 4kB block from ext_flash_address:3 (Warning: No confirmation or built-in security here...)
        // not supported yet case 0x42: // 	erases range in 4kB steps (Get 3 bytes address and 1byte amount of 4kB blocks)
        // not supported yet case 0x50: // 	sends firmware from external flash from 0x3E0000 to 0x3FD000 (118784bytes) via comm
        case 0xFE: // hw unit_tests
        case 0x71: // hw read manufacturing data
        case 0x73: // hw update FLEX
        case 0x79: // hw read device data
#ifdef BOOTLOADER_STANDALONE
        case 0x74: // hw update Firmware
        case 0x75: // hw update RTE
        case 0x76: // hw update Fonts
        case 0x80: // hw write manufacturing data
        case 0x81: // hw write second serial
        case 0x82: // hw set bluetooth name
#else
        case 0x83: // hw copy logbook entry - read
        case 0x84: // hw copy logbook entry - write
        case 0x85: // hw read entire logbook memory
        case 0x86: // hw overwrite entire logbook memory
        case 0x87: // hw ext_flash_repair_SPECIAL_dive_numbers_starting_count_with memory(x)
        case 0x88: /* read entire sample memory  */
        case 0x89: /* write entire sample memory */

#endif
        case 0xC1: // 	Start low-level bootloader
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 1, UART_OPERATION_TIMEOUT)!= HAL_OK)
                return 0;
            break;
        default:
            break;
        }

        // now send content or update firmware
        switch(type)
        {
        case 0xFE:
            // work to do :-)  12. Oct. 2015
            // 256 bytes output
            memset(aTxBuffer,0,128);
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 128,5000)!= HAL_OK)
                return 0;
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 128,5000)!= HAL_OK)
                return 0;
            aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            break;

        case 0x71:
            memcpy(aTxBuffer,hardwareDataGetPointer(),64);
            count += 64;
            aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            break;

        case 0x73:
#ifndef BOOTLOADER_STANDALONE
            answer = receive_update_flex(1);
#else
            answer = receive_update_flex(0);
#endif
            if(answer == 0)
                return 0;
            else if(answer == 2) // 2 = RTE without bootToBootloader
            {
                aTxBuffer[0] = 0xFF;
                HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 1,10000);
                return 0;
            }
            else
            {
                aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
                if(answer == 1) /* 0xFF is checksum error, 2 = RTE without bootToBootloader */
                {
                    extern	uint8_t bootToBootloader;
                    bootToBootloader = 1;
                }
            }
            break;

        case 0x79:
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 1,10000)!= HAL_OK)
                return 0;
            ext_flash_read_fixed_16_devicedata_blocks_formated_128byte_total(aTxBuffer);
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 128,5000)!= HAL_OK)
                return 0;
            aTxBuffer[0] = prompt4D4C(receiveStartByteUart);
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 1,10000)!= HAL_OK)
                return 0;
            else
                return prompt4D4C(receiveStartByteUart);

        case 0x82:
#ifdef BOOTLOADER_STANDALONE
            setForcedBluetoothName = 1;
            return 0;
#else
        settingsGetPointer()->debugModeOnStart = 1;
        extern	uint8_t bootToBootloader;
        bootToBootloader = 1;
        return prompt4D4C(receiveStartByteUart);
#endif

#ifdef BOOTLOADER_STANDALONE
        case 0x74:
            answer = receive_update_data_mainCPU_firmware();
            if(answer != 0)
            {
                aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
                if(answer == 1) // 0xFF is checksum error
                {
                    extern	uint8_t bootToBootloader;
                    bootToBootloader = 1;
                }
            }
            else
                return 0;
            break;

        case 0x75:
            receive_update_data_cpu2();
            aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            break;

        case 0x76:
            answer = receive_update_data_mainCPU_variable_firmware();
            if(answer != 0)
            {
                aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
                if(answer == 1) // 0xFF is checksum error
                {
                    extern	uint8_t bootToBootloader;
                    bootToBootloader = 1;
                }
            }
            else
                return 0;
            break;

        case 0x80:
            if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer,  52, 5000)!= HAL_OK)
                return 0;
            if(hardware_programmProductionData(aRxBuffer) == HAL_OK)
            {
                aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            }
            else
                return 0;
            break;

        case 0x81:
            if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer,  12, 1000)!= HAL_OK)
                return 0;
            if(hardware_programmSecondarySerial(aRxBuffer) == HAL_OK)
            {
                aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            }
            else
                return 0;
            break;

#else

#ifdef SPECIALPROGRAMM
        case 0x80:
            if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer,  52, 5000)!= HAL_OK)
                return 0;
            if(hardware_programmProductionData(aRxBuffer) == HAL_OK)
            {
                aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            }
            else
                return 0;
            break;
#endif
        case 0x83:
            if(HAL_UART_Receive(&UartHandle, &logStepBackwards,  1, 1000)!= HAL_OK)
                return 0;
            logCopyDataPtr = getFrame(98);
            logCopyDataPtrTemp = logCopyDataPtr;
            logCopyDataLength.u32bit = ext_flash_read_dive_raw_with_double_header_1K((uint8_t *)logCopyDataPtr, 1000000,logStepBackwards);
            answer = HAL_OK;
            if(answer == HAL_OK)
                answer  = HAL_UART_Transmit(&UartHandle, &(logCopyDataLength.u8bit.byteLow), 1,2000);
            if(answer == HAL_OK)
                answer  = HAL_UART_Transmit(&UartHandle, &(logCopyDataLength.u8bit.byteMidLow), 1,2000);
            if(answer == HAL_OK)
                answer  = HAL_UART_Transmit(&UartHandle, &(logCopyDataLength.u8bit.byteMidHigh), 1,2000);
            if(answer == HAL_OK)
                answer  = HAL_UART_Transmit(&UartHandle, &(logCopyDataLength.u8bit.byteHigh), 1,2000);
            logCopyDataLengthTemp = logCopyDataLength.u32bit;
            while((logCopyDataLengthTemp >= 0xFFFF) && (answer == HAL_OK))
            {
                answer  = HAL_UART_Transmit(&UartHandle, (uint8_t *)logCopyDataPtrTemp, 0xFFFF,30000);
                logCopyDataLengthTemp -= 0xFFFF;
                logCopyDataPtrTemp += 0xFFFF;
            }
            if((logCopyDataLengthTemp > 0) && (answer == HAL_OK))
                answer  = HAL_UART_Transmit(&UartHandle, (uint8_t *)logCopyDataPtrTemp, (uint16_t)logCopyDataLengthTemp,30000);
            releaseFrame(98,logCopyDataPtr);
            if(answer == HAL_OK)
                aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            else
                return 0;
            break;

        case 0x84:
            logCopyDataPtr = getFrame(98);
            logCopyDataPtrTemp = logCopyDataPtr;
            answer = HAL_OK;
            if(answer == HAL_OK)
                    answer  = HAL_UART_Receive(&UartHandle, &logDummyByte, 1,2000);
            if(answer == HAL_OK)
                    answer  = HAL_UART_Receive(&UartHandle, &(logCopyDataLength.u8bit.byteLow), 1,2000);
            if(answer == HAL_OK)
                answer  = HAL_UART_Receive(&UartHandle, &(logCopyDataLength.u8bit.byteMidLow), 1,2000);
            if(answer == HAL_OK)
                answer  = HAL_UART_Receive(&UartHandle, &(logCopyDataLength.u8bit.byteMidHigh), 1,2000);
            if(answer == HAL_OK)
                answer  = HAL_UART_Receive(&UartHandle, &(logCopyDataLength.u8bit.byteHigh), 1,2000);
            logCopyDataLengthTemp = logCopyDataLength.u32bit;
            while((logCopyDataLengthTemp >= 0xFFFF) && (answer == HAL_OK))
            {
                answer  = HAL_UART_Receive(&UartHandle, (uint8_t *)logCopyDataPtrTemp, 0xFFFF,30000);
                logCopyDataLengthTemp -= 0xFFFF;
                logCopyDataPtrTemp += 0xFFFF;
            }
            if((logCopyDataLengthTemp > 0) && (answer == HAL_OK))
                answer  = HAL_UART_Receive(&UartHandle, (uint8_t *)logCopyDataPtrTemp, (uint16_t)logCopyDataLengthTemp,30000);
            if(answer == HAL_OK)
                ext_flash_write_dive_raw_with_double_header_1K((uint8_t *)logCopyDataPtr, logCopyDataLength.u32bit);
            releaseFrame(98,logCopyDataPtr);
            if(answer == HAL_OK)
                aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            else
                return 0;
            break;

        case 0x85:
            aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            logCopyDataPtr = getFrame(98);
            ext_flash_read_header_memory((uint8_t *)logCopyDataPtr);
            for(int i=0;i<8;i++)
                HAL_UART_Transmit(&UartHandle, (uint8_t *)(logCopyDataPtr + (0x8000 * i)), (uint16_t)0x8000,60000);
            releaseFrame(98,logCopyDataPtr);
#ifdef SEND_DATA_DETAILS
            HAL_UART_Transmit(&UartHandle, (uint8_t*)&pSettings->lastDiveLogId, 1,60000);
#endif
            break;

        case 0x86:
            aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            logCopyDataPtr = getFrame(98);
            for(int i=0;i<8;i++)
            {
                HAL_UART_Receive(&UartHandle, (uint8_t *)(logCopyDataPtr + (0x8000 * i)), (uint16_t)0x8000,60000);
            }
            ext_flash_write_header_memory((uint8_t *)logCopyDataPtr);
#ifdef SEND_DATA_DETAILS
            if(HAL_UART_Receive(&UartHandle, (uint8_t *)(logCopyDataPtr + (0x8000 * 8)), (uint16_t)0x01,60000) == HAL_OK)	/* receive lastlogID */
            {
            	pSettings->lastDiveLogId = *(uint8_t*)(logCopyDataPtr + (0x40000));
            }
#endif
            releaseFrame(98,logCopyDataPtr);
            break;

        case 0x87:
            if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer,  4, 1000)!= HAL_OK)
                return 0;
            if(((aRxBuffer[0] ^ aRxBuffer[2]) != 0xFF) || ((aRxBuffer[1] ^ aRxBuffer[3]) != 0xFF))
                return 0;
            totalDiveCount.u8bit.byteLow  = aRxBuffer[1];
            totalDiveCount.u8bit.byteHigh = aRxBuffer[0];
            ext_flash_repair_SPECIAL_dive_numbers_starting_count_with(totalDiveCount.u16bit);
            aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
            break;
        case 0x88:
             aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
             logCopyDataPtr = getFrame(98);

             for(index = 0; index <384; index++)		/* transmit in 32k blocks */
             {
            	 ext_flash_read_sample_memory((uint8_t *)logCopyDataPtr, index);
                 if(HAL_UART_Transmit(&UartHandle, (uint8_t *)(logCopyDataPtr), (uint16_t)0x8000,60000) != HAL_OK)
                 {
                	 break;
                 }
             }
             releaseFrame(98,logCopyDataPtr);
             HAL_UART_Transmit(&UartHandle, (uint8_t*)&pSettings->logFlashNextSampleStartAddress, 4,60000);	/* send next sample pos */
        	break;
        case 0x89:
             aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
             logCopyDataPtr = getFrame(98);

             for(index = 0; index <384; index++)		/* transmit in 32k blocks  384*/
             {

                 if(HAL_UART_Receive(&UartHandle, (uint8_t *)(logCopyDataPtr), (uint16_t)0x8000,60000) != HAL_OK)
                 {
                	 break;
                 }
                 ext_flash_write_sample_memory((uint8_t *)logCopyDataPtr, index);
             }

             releaseFrame(98,logCopyDataPtr);
             HAL_UART_Receive(&UartHandle, (uint8_t*)&pSettings->logFlashNextSampleStartAddress, 4,60000);	/* send next sample pos */
        	break;

#endif
        }

        // was service command? Yes, finish and exit
        if(count)
        {
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, count,10000)!= HAL_OK)
                return 0;
            else
                return prompt4D4C(receiveStartByteUart);
        }
    }


    // download mode commands
    switch(type)
    {
    // return of command for almost all commands
    case 0x60: // get model + features
    case 0x61: // get all headers full (256 bytes)
    case 0x62: // set clock
    case 0x63: // set custom text
    case 0x66: // get dive profile
    case 0x69: // get serial, old version numbering, custom text
    case 0x6A: // get model
    case 0x6B: // get specific firmware version
    case 0x6C: /* Display Bluetooth signal strength */
    case 0x6D: // get all compact headers (16 byte)
    case 0x6E: // display text
    case 0x70: // read min, default, max setting
    case 0x72: // read setting
    case 0x77: // write setting
    case 0x78: // reset all settings
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 1, 1000)!= HAL_OK)
            return 0;
        break;

    // start communication (again)
    case 0xBB:
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 2, 1000)!= HAL_OK)
            return 0;
        else
            return prompt4D4C(receiveStartByteUart);

    // stop communication
    case 0xFF:
        HAL_UART_Transmit(&UartHandle, (uint8_t*)&aTxBuffer, 1, 1000);
        return 0;

    default:
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
    }

    switch(type)
    {
    case 0x62:
        if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer,  6, 2000)!= HAL_OK)
            return 0;
        break;
    case 0x63:
        if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 60, 5000)!= HAL_OK)
            return 0;
        break;
    case 0x66:
        if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer,  1, 1000)!= HAL_OK)
            return 0;
        break;
    case 0x6B:
        if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer,  1, 1000)!= HAL_OK)
            return 0;
        break;
    case 0x6E:
        if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 16, 5000)!= HAL_OK)
            return 0;
        break;
    case 0x77:
  if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 5, 5000)!= HAL_OK)
            return 0;
  break;
    case 0x72:
  if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 1, 5000)!= HAL_OK)
            return 0;
        break;
    case 0x70:
  if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 1, 5000)!= HAL_OK)
            return 0;
        break;
    }

    switch(type)
    {
    /* common to standard and bootloader */

    // get model + features
    case 0x60:
        aTxBuffer[count++] = 0x00; // hardware descriptor HIGH byte
        aTxBuffer[count++] = 0x3B; // hardware descriptor LOW byte // 0x3B is OSTC4 //  0x1A is OTSC3
        aTxBuffer[count++] = 0x00; // feature descriptor HIGH byte
        aTxBuffer[count++] = 0x00; // feature descriptor LOW byte
        aTxBuffer[count++] = 0x43; // model id
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    // get model
    case 0x6A:
        aTxBuffer[count++] = 0x3B; // 0x3B is OSTC4 //  0x1A is OTSC3
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    // get all firmware version and status (OSTC4 only)
    case 0x6B:
        switch(*aRxBuffer)
        {
            case 0xFF:
            // firmware
            aTxBuffer[count++] = firmwareDataGetPointer()->versionFirst;
            aTxBuffer[count++] = firmwareDataGetPointer()->versionSecond;
            aTxBuffer[count++] = firmwareDataGetPointer()->versionThird;
            aTxBuffer[count++] = firmwareDataGetPointer()->versionBeta;
            break;
            case 0xFE:
            // RTE
            getActualRTEandFONTversion(&tempHigh, &tempLow, 0, 0); // RTE
            aTxBuffer[count++] = tempHigh;
            aTxBuffer[count++] = tempLow;
            aTxBuffer[count++] = 0;
            aTxBuffer[count++] = 0;
            break;
            case 0x10:
            getActualRTEandFONTversion( 0, 0, &tempHigh, &tempLow); // font
            aTxBuffer[count++] = tempHigh;
            aTxBuffer[count++] = tempLow;
            aTxBuffer[count++] = 0;
            aTxBuffer[count++] = 0;
            break;
            default:
            // not supported
            aTxBuffer[count++] = 0xFF;
            aTxBuffer[count++] = 0xFF;
            aTxBuffer[count++] = 0xFF;
            aTxBuffer[count++] = 0xFF;
            break;
/* Jef Driesen Test
            default:
            // not supported
            aTxBuffer[count++] = 0x1;
            aTxBuffer[count++] = 0x1;
            aTxBuffer[count++] = 0x1;
            aTxBuffer[count++] = 0x1;
            break;
*/
        }
/*
        // serial
        aTxBuffer[count++] = pSettings->serialLow;
        aTxBuffer[count++] = pSettings->serialHigh;
        // batch code (date)
        hardwareBatchCode(&tempHigh, &tempLow);
        aTxBuffer[count++] = tempLow;
        aTxBuffer[count++] = tempHigh;
        // status and status detail (future feature)
        aTxBuffer[count++] = 0;
        aTxBuffer[count++] = 0;
        aTxBuffer[count++] = 0;
        aTxBuffer[count++] = 0;
*/
        // prompt
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    /* Trigger Bluetooth signal strength evaluation */
    case 0x6C:  tComm_EvaluateBluetoothStrength();
    			aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
    	break;
    // display text
    case 0x6E:
        for(int i=0;i<16;i++)
            display_text[i] = aRxBuffer[i];
        display_text[15] = 0;
        display_text[255] = 16;
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    // version / identify
    case 0x69:
#ifndef BOOTLOADER_STANDALONE
        aTxBuffer[count++] = pSettings->serialLow;
        aTxBuffer[count++] = pSettings->serialHigh;
        aTxBuffer[count++] = firmwareVersion_16bit_low();
        aTxBuffer[count++] = firmwareVersion_16bit_high();
        memcpy(&aTxBuffer[count], pSettings->customtext, 60);
#else
        aTxBuffer[count++] = 0;//pSettings->serialLow;
        aTxBuffer[count++] = 0;//pSettings->serialHigh;
        aTxBuffer[count++] = 0;//firmwareVersion_16bit_low();
        aTxBuffer[count++] = 0;//firmwareVersion_16bit_high();
        memset(&aTxBuffer[count], 0, 60);
#endif
        count += 60;
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

#ifndef BOOTLOADER_STANDALONE
    //Reset all setting
    case 0x78:
        set_settings_to_Standard();
        updateSettingsAndMenuOnExit = 1;
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
#endif

#ifndef BOOTLOADER_STANDALONE
    // full headers (256 byte)
    case 0x61:
        for(int StepBackwards = 255; StepBackwards > -1; StepBackwards--)
        {
            logbook_getHeader(StepBackwards, &logbookHeader);
            plogbookHeaderOSTC3 = logbook_build_ostc3header(&logbookHeader);
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)plogbookHeaderOSTC3, 256,5000)!= HAL_OK)
                return 0;
        }
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

        // compact headers (16 byte)
    case 0x6D:
        for(int StepBackwards = 255; StepBackwards > -1; StepBackwards--)
        {
            logbook_getHeader(StepBackwards, &logbookHeader);
            plogbookHeaderOSTC3compact = logbook_build_ostc3header_compact(&logbookHeader);
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)plogbookHeaderOSTC3compact, 16,5000)!= HAL_OK)
                return 0;
        }
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    // set clock & date
    case 0x62:
// ToDo
        stimestructure.Hours = aRxBuffer[0];
        stimestructure.Minutes = aRxBuffer[1];
        stimestructure.Seconds = aRxBuffer[2];
        sdatestructure.Month = aRxBuffer[3];
        sdatestructure.Date = aRxBuffer[4];
        sdatestructure.Year = aRxBuffer[5]; // This parameter must be a number between Min_Data = 0 and Max_Data = 99
        setWeekday(&sdatestructure);

        if(		( stimestructure.Hours 		< 24 )
                &&(	stimestructure.Minutes 	< 60 )
                &&(	stimestructure.Seconds 	< 60 )
                &&(	sdatestructure.Month 		< 13 )
                &&(	sdatestructure.Date 		< 32 )
                &&(	sdatestructure.Year 		< 100 ))
        {
            setTime(stimestructure);
            setDate(sdatestructure);
            set_globalState(StUART_RTECONNECT);
            HAL_Delay(1);
            set_globalState(StUART_STANDARD);
        }
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    case 0x63:
        for(int i=0;i<60;i++)
            pSettings->customtext[i] = aRxBuffer[i];
        pSettings->customtext[59] = 0;
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    // get dive profile
    case 0x66:
        logbook_getHeader(255 - aRxBuffer[0], &logbookHeader);
        plogbookHeaderOSTC3 = logbook_build_ostc3header(&logbookHeader);
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)plogbookHeaderOSTC3, 256,5000)!= HAL_OK)
            return 0;

        OSTC3_profileLength = (plogbookHeaderOSTC3->profileLength[2] << 16) + (plogbookHeaderOSTC3->profileLength[1] << 8)
                								    + plogbookHeaderOSTC3->profileLength[0] -3;
        header_profileLength = (logbookHeader.profileLength[2] << 16) + (logbookHeader.profileLength[1] << 8) + logbookHeader.profileLength[0];
       
        if(OSTC3_profileLength != header_profileLength)			/* has headerdata been changed to dummy data? */
        {
        	sampleTotalLength = logbook_fillDummySampleBuffer(&logbookHeader);
			while(sampleTotalLength >= 128)
			{
				logbook_readDummySamples(aTxBuffer,128);
				sampleTotalLength -= 128;
				if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 128,5000)!= HAL_OK)
					return 0;
			}
			if(sampleTotalLength)
			{
				logbook_readDummySamples(aTxBuffer,sampleTotalLength);
				if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, sampleTotalLength,5000)!= HAL_OK)
					return 0;
			}
        }
        else
        {
			ext_flash_open_read_sample(255 - aRxBuffer[0], &sampleTotalLength);
			while(sampleTotalLength >= 128)
			{
				ext_flash_read_next_sample_part(aTxBuffer,128);
				sampleTotalLength -= 128;
				if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, 128,5000)!= HAL_OK)
					return 0;
			}
			if(sampleTotalLength)
			{
				ext_flash_read_next_sample_part(aTxBuffer,sampleTotalLength);
				if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, sampleTotalLength,5000)!= HAL_OK)
					return 0;
			}
        }
		aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

        // read min,default,max setting
    case 0x70:
    count += readDataLimits__8and16BitValues_4and7BytesOutput(aRxBuffer[0],&aTxBuffer[count]);
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    // read setting
    case 0x72:
        readData(aRxBuffer[0],&aTxBuffer[count]);
        count += 4;
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;

    // write setting
    case 0x77:
        writeData(aRxBuffer);
        updateSettingsAndMenuOnExit = 1;
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
#else
    /* bootloader dummies */
    // full headers (256 byte)
    case 0x61:
        for(int StepBackwards = 0;StepBackwards<256;StepBackwards++)
        {
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)dummyForBootloader, 256,5000)!= HAL_OK)
                return 0;
        }
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
    // compact headers (16 byte)
    case 0x6D:
        for(int StepBackwards = 0;StepBackwards<256;StepBackwards++)
        {
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)dummyForBootloader, 16,5000)!= HAL_OK)
                return 0;
        }
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
    // set clock & date
    case 0x62:
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
    // set custom text
    case 0x63:
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
    // get dive profile
    case 0x66:
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)dummyForBootloader, 256,5000)!= HAL_OK)
            return 0;
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
    // read min,default,max setting
    // read settings


    case 0x72:
        memcpy(&aTxBuffer[count], dummyForBootloader, 4);
  count += 4;
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
    // write settings
    case 0x77:
        aTxBuffer[count++] = prompt4D4C(receiveStartByteUart);
        break;
#endif
    }

    if(count)
    {
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBuffer, count,10000)!= HAL_OK)
            return 0;
        else
            return prompt4D4C(receiveStartByteUart);
    }
    return 0;
}

#define BLOCKSIZE 0x1000

HAL_StatusTypeDef receive_uart_large_size(UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size)
{
    uint16_t length_4k_blocks;
    uint16_t length_4k_remainder;
    uint32_t temp;
    HAL_StatusTypeDef result = HAL_OK;
    uint32_t pDataLocal;

    length_4k_blocks = (uint16_t) (Size / BLOCKSIZE);
    temp = length_4k_blocks;
    temp *= BLOCKSIZE;
    length_4k_remainder = (uint16_t) ( Size - temp);

    pDataLocal = (uint32_t)pData;


    while((result == HAL_OK) && length_4k_blocks)
    {
        result = HAL_UART_Receive(&UartHandle, (uint8_t *)pDataLocal, BLOCKSIZE , UART_TIMEOUT_LARGE_BLOCK);
        pDataLocal += BLOCKSIZE;
        length_4k_blocks--;
    }

    if((result == HAL_OK) && length_4k_remainder)
    {
        result = HAL_UART_Receive(&UartHandle, (uint8_t *)pDataLocal, length_4k_remainder , UART_TIMEOUT_LARGE_BLOCK);
    }
    return result;
}


/* for safety reason (memory blocking this code is main and sub */

#ifdef BOOTLOADER_STANDALONE

uint8_t receive_update_data_cpu2(void)
{
    uint8_t answer;

    uint8_t* pBuffer = (uint8_t*)getFrame(20);
    answer = receive_update_data_cpu2_sub(pBuffer);
    releaseFrame(20,(uint32_t)pBuffer);
    return answer;
}


uint8_t receive_update_data_cpu2_sub(uint8_t* pBuffer)
{
    uint8_t sBuffer[10];
    uint32_t length, offsetTotal, checksum, checksumCalc;
    uint8_t id;
    const uint8_t id_RTE = 0xFE;

    //Get length
    if(HAL_UART_Receive(&UartHandle, pBuffer, 4,5000)!= HAL_OK) // 58000
    {
        return 0;
    }
    length = 256  * 256 * 256 * (uint32_t)pBuffer[0] +  256 * 256 * (uint32_t)pBuffer[1] + 256 * (uint32_t)pBuffer[2] + pBuffer[3];

    //Get id
    if(HAL_UART_Receive(&UartHandle, pBuffer, 4,5000)!= HAL_OK) // 58000
    {
        return 0;
    }
    id = pBuffer[0];
    offsetTotal = 256  * 256 * 256 * (uint32_t)pBuffer[0] +  256 * 256 * (uint32_t)pBuffer[1] + 256 * (uint32_t)pBuffer[2] + pBuffer[3];

    // get checksum, bytes are in different order on Dev C++ code!!!
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
    {
        return 0;
    }
    checksum = 256  * 256 * 256 * (uint32_t)sBuffer[3] +  256 * 256 * (uint32_t)sBuffer[2] + 256 * (uint32_t)sBuffer[1] + sBuffer[0];
    checksumCalc = length + offsetTotal;

    // no need to get code if checksum == length is wrong
    if(checksumCalc != checksum)
    {
        return 0;
    }

    //get Code
    if(receive_uart_large_size(&UartHandle, pBuffer, length)!= HAL_OK)
    {
        return 0;
    }

    //get Checksum
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 580000
    {
        return 0;
    }
    checksum = 256  * 256 * 256 *(uint32_t)sBuffer[0] + 256 * 256 *  (uint32_t)sBuffer[1] + 256 *  (uint32_t)sBuffer[2] + sBuffer[3];
//  uint32_t checksumCalc = crc32c_checksum(pBuffer, length,0,0);
    checksumCalc = CRC_CalcBlockCRC((uint32_t*)pBuffer, length/4);

    if(checksum !=  checksumCalc)
    {
        return 0;
    }

    if(id != id_RTE)
    {
        strcpy(display_text,"wrong data.");
        display_text[255] = 32;
        return 0;
    }

    strcpy(display_text,"  RTE update.");
    display_text[255] = 32;

    return extCPU2bootloader(pBuffer,length,display_text);
}
#endif // BOOTLOADER_STANDALONE



uint8_t receive_update_flex(uint8_t isRTEupdateALLOWED)
{
    uint8_t answer;

    uint8_t* pBuffer1 = (uint8_t*)getFrame(20);
    uint8_t* pBuffer2 = (uint8_t*)getFrame(20);

    answer = receive_update_data_flex(pBuffer1, pBuffer2, isRTEupdateALLOWED);

    releaseFrame(20,(uint32_t)pBuffer1);
    releaseFrame(20,(uint32_t)pBuffer2);

    return answer;
}

uint8_t receive_update_data_mainCPU_firmware(void)
{
    uint8_t answer;

    uint8_t* pBuffer1 = (uint8_t*)getFrame(20);

    answer = receive_update_data_mainCPU_firmware_subroutine(1, pBuffer1, 0);

    releaseFrame(20,(uint32_t)pBuffer1);

    return answer;
}

/* multi buffer (long data) not tested yet */
uint8_t receive_update_data_mainCPU_variable_firmware(void)
{
    uint8_t answer;

    uint8_t* pBuffer1 = (uint8_t*)getFrame(20);
    uint8_t* pBuffer2 = (uint8_t*)getFrame(20);

    answer = receive_update_data_mainCPU_firmware_subroutine(2, pBuffer1, pBuffer2);

    releaseFrame(20,(uint32_t)pBuffer1);
    releaseFrame(20,(uint32_t)pBuffer2);

    return answer;
}

uint8_t receive_update_data_flex(uint8_t* pBuffer1, uint8_t* pBuffer2, uint8_t RTEupdateALLOWED)
{
    uint8_t sBuffer[10];
    uint8_t serialBuffer[10];
    uint32_t length1, length2, lengthCompare, offsetCompare, ByteCompareStatus;
    uint32_t lengthTotal, offsetTotal;
    uint32_t checksum, checksumCalc = 0;
    uint8_t id;
    const uint8_t id_Region1_firmware = 0xFF;
    const uint8_t id_RTE = 0xFE;
    uint8_t textpointer = 0;

    //Get length
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
    {
        return 0;
    }
    lengthTotal = 256 * 256 * 256 * (uint32_t)sBuffer[0] +  256 * 256 * (uint32_t)sBuffer[1] + 256 * (uint32_t)sBuffer[2] + sBuffer[3];

    //Get offset and/or id (id is 0xFF for RTE, 0xFE for firmware and offset if var)
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
    {
        return 0;
    }
    id = sBuffer[0];

    checksumCalc = 256 * 256 * 256 * (uint32_t)sBuffer[0] +  256 * 256 * (uint32_t)sBuffer[1] + 256 * (uint32_t)sBuffer[2] + sBuffer[3];
    checksumCalc += lengthTotal;
    //old, does no longer work because of the fonts: checksumCalc = lengthTotal + offsetTotal;

    if((id != id_Region1_firmware) && (id != id_RTE) && (id != id_FONT) && (id != id_FONT_OLD))
    {
        return 0;
    }

    // neu 110212
    if(id == id_FONT)
        offsetTotal = 256 * 256 * 256 * (uint32_t)sBuffer[1] +  256 * 256 * (uint32_t)sBuffer[2] + 256 * (uint32_t)sBuffer[3];
    else
        offsetTotal = 256 * 256 * 256 * (uint32_t)sBuffer[0] +  256 * 256 * (uint32_t)sBuffer[1] + 256 * (uint32_t)sBuffer[2] + sBuffer[3];

    // get checksum, bytes are in different order on Dev C++ code!!!
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
    {
        return 0;
    }
    checksum = 256 * 256 * 256 * (uint32_t)sBuffer[3] +  256 * 256 * (uint32_t)sBuffer[2] + 256 * (uint32_t)sBuffer[1] + sBuffer[0];


    if(checksumCalc != checksum)
    {
        uint8_t ptr = 0;
        strcpy(&display_text[ptr]," checksum error");
        ptr += 15;
        strcpy(&display_text[ptr],"\n\r");
        ptr += 2;
        ptr += gfx_number_to_string(10,0,&display_text[ptr],checksumCalc);
        display_text[ptr] = 0;
        display_text[255] = ptr + 1;
        return 0xFF;
    }

    //Get serial (new since 160211)
    if(HAL_UART_Receive(&UartHandle, serialBuffer, 4,5000)!= HAL_OK)
    {
        return 0;
    }

    if(lengthTotal > 768000)
    {
        length1 = 768000;
        length2 = lengthTotal - length1;
    }
    else
    {
        length1 = lengthTotal;
        length2 = 0;
    }

    if((pBuffer2 == 0) && (length2 != 0))
        return 0;

    //get Code
    if(receive_uart_large_size(&UartHandle, pBuffer1, length1)!= HAL_OK)
        return 0;

    if(length2)
        if(receive_uart_large_size(&UartHandle, pBuffer2, length2)!= HAL_OK)
            return 0;

    //get Checksum
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
        return 0;

    checksum = 256  * 256 * 256 *(uint32_t)sBuffer[0] + 256 * 256 *  (uint32_t)sBuffer[1] + 256 *  (uint32_t)sBuffer[2] + sBuffer[3];
//  uint32_t checksumCalc = crc32c_checksum(pBuffer1, length1, pBuffer2, length2);
    if(length2)
        checksumCalc = CRC_CalcBlockCRC_moreThan768000((uint32_t*)pBuffer1, (uint32_t*)pBuffer2, lengthTotal/4);
    else
        checksumCalc = CRC_CalcBlockCRC((uint32_t*)pBuffer1, length1/4);

    /* check id now */
    /*
    if(region == 2)
    {
        if((id == id_Region1_firmware) || (id == id_RTE))
        {
            strcpy(display_text,"wrong data.");
            display_text[255] = 32;
            return 0;
        }
    }
    else
    {
        if(id != id_Region1_firmware)
        {
            strcpy(display_text,"wrong data.");
            display_text[255] = 32;
            return 0;
        }
    }
    */
    /* test checksum */
    if(checksum !=  checksumCalc)
    {
        uint8_t ptr = 0;
        strcpy(&display_text[ptr]," checksum error");
        ptr += 15;
        strcpy(&display_text[ptr],"\n\r");
        display_text[ptr] = 0;
        display_text[255] = ptr + 1;
        return 0xFF;
    }

    if(id == id_Region1_firmware)
    {
        uint8_t ptr = 0;
        display_text[ptr++] = 'V';
        ptr += gfx_number_to_string(2,0,&display_text[ptr],pBuffer1[0x10000] & 0x1F);
        display_text[ptr++] = '.';
        ptr += gfx_number_to_string(2,0,&display_text[ptr],pBuffer1[0x10001] & 0x1F);
        display_text[ptr++] = '.';
        ptr += gfx_number_to_string(2,0,&display_text[ptr],pBuffer1[0x10002] & 0x1F);
        display_text[ptr++] = ' ';
        if(pBuffer1[0x10003])
        {
            strcpy(&display_text[ptr],"beta ");
            ptr +=5;
        }
        strcpy(&display_text[ptr],"\n\rpreparing for install.");
        ptr += 25;
        display_text[255] = ptr + 1;
    }
    else if(id == id_RTE)
    {
        if(RTEupdateALLOWED)
        {
            strcpy(display_text,"  RTE update.\n\r");
            textpointer = 0;
            while((display_text[textpointer] != 0) && (textpointer < 50))
                textpointer++;
#ifndef BOOTLOADER_STANDALONE
            if(textpointer < 50)
            {
//				display_text[textpointer++] =
                display_text[textpointer++] = '\025';
                display_text[textpointer++] = TXT_2BYTE;
                display_text[textpointer++] = TXT2BYTE_DecoDataLost;
                display_text[textpointer] = 0;
            }
#endif
            display_text[255] = textpointer+1;
            return extCPU2bootloader(pBuffer1,length1,display_text);
        }
        else
            return 0xFF;
    }
    else
    //if(region == 2)
    {
        uint8_t ptr = 0;
        ptr += gfx_number_to_string(7,0,&display_text[ptr],lengthTotal);
        strcpy(&display_text[ptr]," bytes with ");
        ptr += 12;
        ptr += gfx_number_to_string(7,0,&display_text[ptr],offsetTotal);
        strcpy(&display_text[ptr]," offset");
        ptr += 7;
        strcpy(&display_text[ptr],"\n\rpreparing for install.");
        ptr += 25;
        display_text[255] = ptr + 1;
    }


    // only non RTE !!
    uint8_t* pBufferCompare = (uint8_t*)getFrame(20);
    ByteCompareStatus = 0;

    if(id == id_Region1_firmware)
    {
        /* standard firmware limited to 768000 */
        if(ext_flash_read_firmware(pBufferCompare,4,0) != 0xFFFFFFFF)
            ext_flash_erase_firmware();
        ext_flash_write_firmware(pBuffer1, length1);
        lengthCompare = ext_flash_read_firmware(pBufferCompare,768000,0);

        if(lengthCompare != length1)
            ByteCompareStatus = 10000;
        for(int i = 0; i < length1; i++)
        {
            if(pBuffer1[0] != pBufferCompare[0])
                ByteCompareStatus++;
        }
    }
    else
    //if(region == 2)
    {
        /* upper region firmware can be larger (1MB) */
        if(ext_flash_read_firmware2(0, pBufferCompare,4, 0,0) != 0xFFFFFFFF)
            ext_flash_erase_firmware2();
        ext_flash_write_firmware2(offsetTotal, pBuffer1, length1, pBuffer2, length2);
        lengthCompare = ext_flash_read_firmware2(&offsetCompare, pBufferCompare,768000, 0,768000);

        if(lengthCompare != length1 + length2)
            ByteCompareStatus = 10000;
        if(offsetTotal != offsetCompare)
            ByteCompareStatus += 20000;
        for(int i = 0; i < length1; i++)
        {
            if(pBuffer1[0] != pBufferCompare[0])
                ByteCompareStatus++;
        }

        lengthCompare = ext_flash_read_firmware2(0, 0,768000, pBufferCompare,768000);
        for(int i = 0; i < length2; i++)
        {
            if(pBuffer2[0] != pBufferCompare[0])
                ByteCompareStatus++;
        }
    }

    releaseFrame(20,(uint32_t)pBufferCompare);

    if(ByteCompareStatus != 0)
    {
        strcpy(&display_text[0],"\n\rcopy error.");
        display_text[255] = 21;
        return 0;
    }
    else
    {
        strcpy(&display_text[0],"\n\rready to install.");
        display_text[255] = 21;
        return 1;
    }
}


uint8_t receive_update_data_mainCPU_firmware_subroutine(uint8_t region, uint8_t* pBuffer1, uint8_t* pBuffer2)
{
    uint8_t sBuffer[10];
    uint32_t length1, length2, lengthCompare, offsetCompare, ByteCompareStatus;
    uint32_t lengthTotal, offsetTotal, checksum, checksumCalc = 0;
    uint8_t id;

    //Get length
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
        return 0;

    lengthTotal = 256  * 256 * 256 * (uint32_t)sBuffer[0] +  256 * 256 * (uint32_t)sBuffer[1] + 256 * (uint32_t)sBuffer[2] + sBuffer[3];

    //Get offset and/or id (id is 0xFF for RTE, 0xFE for firmware and offset if var)
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
        return 0;

    id = sBuffer[0];

    checksumCalc = 256 * 256 * 256 * (uint32_t)sBuffer[0] +  256 * 256 * (uint32_t)sBuffer[1] + 256 * (uint32_t)sBuffer[2] + sBuffer[3];
    checksumCalc += lengthTotal;

    if((id != id_Region1_firmware) && (id != id_RTE) && (id != id_FONT) && (id != id_FONT_OLD))
        return 0;

    if(id == id_FONT)
        offsetTotal = 256 * 256 * 256 * (uint32_t)sBuffer[1] +  256 * 256 * (uint32_t)sBuffer[2] + 256 * (uint32_t)sBuffer[3];
        // alt, prior to id for font
    else
        offsetTotal = 256 * 256 * 256 * (uint32_t)sBuffer[0] +  256 * 256 * (uint32_t)sBuffer[1] + 256 * (uint32_t)sBuffer[2] + sBuffer[3];

    // get checksum, bytes are in different order on Dev C++ code!!!
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
        return 0;

    checksum = 256  * 256 * 256 * (uint32_t)sBuffer[3] +  256 * 256 * (uint32_t)sBuffer[2] + 256 * (uint32_t)sBuffer[1] + sBuffer[0];

    //old: checksumCalc = lengthTotal + offsetTotal;

    if(checksumCalc != checksum)
    {
        uint8_t ptr = 0;
        strcpy(&display_text[ptr]," checksum error");
        ptr += 15;
        strcpy(&display_text[ptr],"\n\r");
        ptr += 2;
        ptr += gfx_number_to_string(10,0,&display_text[ptr],checksumCalc);
        display_text[ptr] = 0;
        display_text[255] = ptr + 1;
        return 0xFF;
    }

    if(lengthTotal > 768000)
    {
        length1 = 768000;
        length2 = lengthTotal - length1;
    }
    else
    {
        length1 = lengthTotal;
        length2 = 0;
    }

    if((pBuffer2 == 0) && (length2 != 0))
        return 0;

    //get Code
    if(receive_uart_large_size(&UartHandle, pBuffer1, length1)!= HAL_OK)
        return 0;

    if(length2)
        if(receive_uart_large_size(&UartHandle, pBuffer2, length2)!= HAL_OK)
            return 0;

    //get Checksum
    if(HAL_UART_Receive(&UartHandle, sBuffer, 4,5000)!= HAL_OK) // 58000
        return 0;

    checksum = 256  * 256 * 256 *(uint32_t)sBuffer[0] + 256 * 256 *  (uint32_t)sBuffer[1] + 256 *  (uint32_t)sBuffer[2] + sBuffer[3];
//  uint32_t checksumCalc = crc32c_checksum(pBuffer1, length1, pBuffer2, length2);
    if(length2)
        checksumCalc = CRC_CalcBlockCRC_moreThan768000((uint32_t*)pBuffer1, (uint32_t*)pBuffer2, lengthTotal/4);
    else
        checksumCalc = CRC_CalcBlockCRC((uint32_t*)pBuffer1, length1/4);

    /* check id now */
    if(region == 2)
    {
        if((id == id_Region1_firmware) || (id == id_RTE))
        {
            strcpy(display_text,"wrong data.");
            display_text[255] = 32;
            return 0;
        }
    }
    else
    {
        if(id != id_Region1_firmware)
        {
            strcpy(display_text,"wrong data.");
            display_text[255] = 32;
            return 0;
        }
    }

    /* test checksum */
    if(checksum !=  checksumCalc)
    {
        uint8_t ptr = 0;
        strcpy(&display_text[ptr]," pruefsummen error");
        ptr += 15;
        strcpy(&display_text[ptr],"\n\r");
        display_text[ptr] = 0;
        display_text[255] = ptr + 1;
        return 0xFF;
    }

    if(region == 2)
    {
        uint8_t ptr = 0;
        ptr += gfx_number_to_string(7,0,&display_text[ptr],lengthTotal);
        strcpy(&display_text[ptr]," bytes with ");
        ptr += 12;
        ptr += gfx_number_to_string(7,0,&display_text[ptr],offsetTotal);
        strcpy(&display_text[ptr]," offset");
        ptr += 7;
        strcpy(&display_text[ptr],"\n\rpreparing for install.");
        ptr += 25;
        display_text[255] = ptr + 1;

    }
    else
    {
        uint8_t ptr = 0;
        display_text[ptr++] = 'V';
        ptr += gfx_number_to_string(2,0,&display_text[ptr],pBuffer1[0x10000] & 0x1F);
        display_text[ptr++] = '.';
        ptr += gfx_number_to_string(2,0,&display_text[ptr],pBuffer1[0x10001] & 0x1F);
        display_text[ptr++] = '.';
        ptr += gfx_number_to_string(2,0,&display_text[ptr],pBuffer1[0x10002] & 0x1F);
        display_text[ptr++] = ' ';
        if(pBuffer1[0x10003])
        {
            strcpy(&display_text[ptr],"beta ");
            ptr +=5;
        }
        strcpy(&display_text[ptr],"\n\rpreparing for install.");
        ptr += 25;
        display_text[255] = ptr + 1;
    }

    uint8_t* pBufferCompare = (uint8_t*)getFrame(20);
    ByteCompareStatus = 0;

    if(region == 2)
    {
        /* upper region firmware can be larger (1MB) */
        if(ext_flash_read_firmware2(0, pBufferCompare,4, 0,0) != 0xFFFFFFFF)
            ext_flash_erase_firmware2();
        ext_flash_write_firmware2(offsetTotal, pBuffer1, length1, pBuffer2, length2);
        lengthCompare = ext_flash_read_firmware2(&offsetCompare, pBufferCompare,768000, 0,768000);

        if(lengthCompare != length1 + length2)
            ByteCompareStatus = 10000;
        if(offsetTotal != offsetCompare)
            ByteCompareStatus += 20000;
        for(int i = 0; i < length1; i++)
        {
            if(pBuffer1[0] != pBufferCompare[0])
                ByteCompareStatus++;
        }

        lengthCompare = ext_flash_read_firmware2(0, 0,768000, pBufferCompare,768000);
        for(int i = 0; i < length2; i++)
        {
            if(pBuffer2[0] != pBufferCompare[0])
                ByteCompareStatus++;
        }
    }
    else
    {
        /* standard firmware limited to 768000 */
        if(ext_flash_read_firmware(pBufferCompare,4,0) != 0xFFFFFFFF)
            ext_flash_erase_firmware();
        ext_flash_write_firmware(pBuffer1, length1);
        lengthCompare = ext_flash_read_firmware(pBufferCompare,768000,0);

        if(lengthCompare != length1)
            ByteCompareStatus = 10000;
        for(int i = 0; i < length1; i++)
        {
            if(pBuffer1[0] != pBufferCompare[0])
                ByteCompareStatus++;
        }
    }

    releaseFrame(20,(uint32_t)pBufferCompare);

    if(ByteCompareStatus != 0)
    {
        strcpy(&display_text[0],"\n\rcopy error.");
        display_text[255] = 21;
        return 0;
    }
    else
    {
        strcpy(&display_text[0],"\n\rready to install.");
        display_text[255] = 21;
        return 1;
    }
}

void tComm_RequestBluetoothStrength(void)
{
	EvaluateBluetoothSignalStrength = 1;
}

/* read, validate the modul answer and flush rx que if necessary */
uint8_t tComm_CheckAnswerOK()
{
    char answerOkay[] = "\r\nOK\r\n";
    char aRxBuffer[UART_CMD_BUF_SIZE];
    uint8_t sizeAnswer = sizeof(answerOkay) -1;
	uint8_t result = HAL_OK;
	uint8_t indexRef = 0;
	uint8_t indexBuf = 0;
	uint8_t answer;

	memset(aRxBuffer,0,UART_CMD_BUF_SIZE);
	if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, sizeAnswer, UART_OPERATION_TIMEOUT) == HAL_OK)
	{
		do
		{
			if(answerOkay[indexRef] == aRxBuffer[indexBuf])
			{
				indexRef++;
			}
			else
			{
				if(indexRef != 0)
				{
					indexRef = 0;
				}
			}
			indexBuf++;
		}while(indexBuf < sizeAnswer);

		if(indexRef != sizeAnswer)		/* unexpected answer => there might be characters left in RX que => read and check all rx bytes */
		{
			do
			{
				answer = HAL_ERROR;
				if (indexBuf < UART_CMD_BUF_SIZE)
				{
					answer = HAL_UART_Receive(&UartHandle, (uint8_t*)&aRxBuffer[indexBuf], 1, 10);

					if(answerOkay[indexRef] == aRxBuffer[indexBuf])
					{
						indexRef++;
					}
					else
					{
						if(indexRef != 0)
						{
							indexRef = 0;
						}
					}
					indexBuf++;
				}
			}while((answer == HAL_OK) && (indexRef != sizeAnswer));
			if(indexRef != sizeAnswer)
			{
				result = HAL_ERROR;
			}
		}
	}
	else
	{
		result = HAL_ERROR;
	}
	return result;

}


void tComm_EvaluateBluetoothStrength(void)
{
	char aTxBufferBarSSI[] = "AT+BARSSI\r";
	char aTxBufferEscapeSequence[] = "+++";
	char aTxBufferEnd[] = "ATO\r";
    uint8_t sizeRequest = sizeof(aTxBufferBarSSI) -1;

    uint8_t answer = HAL_OK;
    char aRxBuffer[UART_CMD_BUF_SIZE];
	char SignalStr[UART_CMD_BUF_SIZE];
    uint8_t index = 0;
    uint8_t strindex = 0;
    int8_t sigqual = 0;

    HAL_Delay(200);
    if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferEscapeSequence, 3, 2000)== HAL_OK)
    {
    	if(tComm_CheckAnswerOK() == HAL_OK)
		{
			HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferBarSSI,sizeRequest , 2000);
			{
				index = 0;
				do						/* Answer is not the common one. Instead the signal strength is received => read all available bytes one by one*/
				{
					answer = HAL_UART_Receive(&UartHandle, (uint8_t*)&aRxBuffer[index], 1, 100);
					if(index < UART_CMD_BUF_SIZE) 
					{
						index++;
					}
				}while(answer == HAL_OK);

				if((aRxBuffer[index] != 'E') && (aRxBuffer[index] != 0))		/* E represents the first letter of the string ERROR */
				{
					index = 0;
					strindex = 0;
					do
					{
						SignalStr[strindex++] = aRxBuffer[index++];
					}while ((index < UART_CMD_BUF_SIZE - 1) && (aRxBuffer[index] != '\r'));
					SignalStr[strindex] = 0;	/* terminate String */
					sigqual = strtol(SignalStr,NULL,0);
					/* Map db to abstract Bargraph */
					if(sigqual > 0)
					{
						sprintf(SignalStr,"Bluetooth ||||||||");
					}
					else
					{
						sprintf(SignalStr,"Bluetooth |");
						strindex = strlen(SignalStr);
						sigqual *=-1;
						sigqual = 100 - sigqual;	/* invert because of negative db value */
						while(sigqual / 10 > 0 )
						{
							SignalStr[strindex++] = '|';
							sigqual -= 10;
						}
						SignalStr[strindex] = 0;
					}
					strcpy(display_text,SignalStr);
					display_text[255] = strlen(SignalStr);
					EvaluateBluetoothSignalStrength = 0;
				}
			}
			HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferEnd, 4, 2000);	/* exit terminal mode */
			index = 0;
			do	/* module will answer with current connection state */
			{
				answer = HAL_UART_Receive(&UartHandle, (uint8_t*)&aRxBuffer[index], 1, 100);
				if(index < UART_CMD_BUF_SIZE)
				{
					index++;
				}
			}while(answer == HAL_OK);
		}
    }
}

#ifdef BOOTLOADER_STANDALONE
void tComm_StartBlueModBaseInit()
{
	BmTmpConfig = BM_INIT_TRIGGER_ON;
}
#endif


uint8_t tComm_GetBTCmdStr(BTCmd cmdId, char* pCmdStr)
{
	uint8_t ret = 0;
	uint8_t oldModule = 1;

	if(isNewDisplay())
	{
		oldModule = 0;
	}

	switch (cmdId)
	{
		case BT_CMD_ECHO:	sprintf(pCmdStr,"ATE0\r");
							ret = 1;
			break;
		case BT_CMD_SILENCE:	if(oldModule)
								{
									strcpy(pCmdStr,"ATS30=0\r");
									ret = 1;
								}
			break;
		case BT_CMD_ESCAPE_DELAY:	if(oldModule)
									{
										strcpy(pCmdStr,"ATS12=10\r");
										ret = 1;
									}
			break;
		case BT_CMD_SIGNAL_POLL:	if(oldModule)
									{
										strcpy(pCmdStr,"AT+BSTPOLL=100\r");
										ret = 1;
									}
			break;
		case BT_CMD_BAUDRATE_115:	if(oldModule)
									{
										strcpy(pCmdStr,"AT%B8\r");
									}
									else
									{
										strcpy(pCmdStr,"AT+UMRS=115200,1,8,1,1,1\r");
									}
									ret = 1;
			break;

		case BT_CMD_BAUDRATE_460:	if(oldModule)
									{
										strcpy(pCmdStr,"AT%B22\r");
									}
									else
									{
										strcpy(pCmdStr,"AT+UMRS=460800,1,8,1,1,1\r");
									}
									ret = 1;
			break;
		case BT_CMD_NAME:			if(oldModule)
									{
										strcpy(pCmdStr,"AT+BNAME=OSTC4-12345\r");
									}
									else
									{
										strcpy(pCmdStr,"AT+UBTLN=OSTC5-12345\r");
									}
									ret = 1;
			break;
		case BT_CMD_EXIT_CMD:		if(oldModule)
									{
										strcpy(pCmdStr,"ATO\r");
									}
									else
									{
										strcpy(pCmdStr,"ATO1\r");
									}
									ret = 1;
			break;
		default:
			break;
	}
	return ret;
}

void tComm_StartBlueModConfig()
{
	uint8_t answer = HAL_OK;
	uint8_t RxBuffer[UART_CMD_BUF_SIZE];
	uint8_t index = 0;

	BmTmpConfig = BM_CONFIG_ECHO;
	do	/* flush RX buffer */
	{
		answer = HAL_UART_Receive(&UartHandle, (uint8_t*)&RxBuffer[index], 1, 10);
		if(index < UART_CMD_BUF_SIZE) index++;
	}while(answer == HAL_OK);
}

uint8_t tComm_HandleBlueModConfig()
{
	static uint8_t RestartModule = 1; 		/* used to do power off / on cycle */
	static uint8_t ConfigRetryCnt = 0;		/* Retry count without power cycle */

	char TxBuffer[UART_CMD_BUF_SIZE];
	uint8_t CmdSize = 0;

	uint8_t result = HAL_OK;

	memset(TxBuffer, 0, sizeof(TxBuffer));

	switch (BmTmpConfig)
	{
		case BM_CONFIG_ECHO: 			tComm_GetBTCmdStr (BT_CMD_ECHO, TxBuffer);
			break;
		case BM_CONFIG_SILENCE:			tComm_GetBTCmdStr (BT_CMD_SILENCE, TxBuffer);
			break;
		case BM_CONFIG_ESCAPE_DELAY:	tComm_GetBTCmdStr (BT_CMD_ESCAPE_DELAY, TxBuffer);
			break;
		case BM_CONFIG_SIGNAL_POLL:		tComm_GetBTCmdStr(BT_CMD_SIGNAL_POLL, TxBuffer);
			break;
		case BM_CONFIG_BAUD:
#ifdef ENABLE_FAST_COMM
										tComm_GetBTCmdStr(BT_CMD_BAUDRATE_460, TxBuffer);
#else
										BmTmpConfig = BM_CONFIG_DONE;
#endif
			break;
		case BM_CONFIG_RETRY:			ConfigRetryCnt--;
										HAL_Delay(1);
										if(ConfigRetryCnt == 0)
										{
											MX_Bluetooth_PowerOn();
											tComm_StartBlueModConfig();
										}
			break;
		case BM_CONFIG_DONE:
		case BM_CONFIG_OFF:
			ConfigRetryCnt = 0;
			RestartModule = 1;
			break;
#ifdef BOOTLOADER_STANDALONE		/* the procedure below is just needed for the initial bluetooth module initialization */
		case BM_INIT_TRIGGER_ON:	HAL_Delay(2000);
									HAL_GPIO_WritePin(BLE_UBLOX_DSR_GPIO_PORT,BLE_UBLOX_DSR_PIN,GPIO_PIN_RESET);
									BmTmpConfig++;
								break;
		case BM_INIT_TRIGGER_OFF:	HAL_Delay(1);
									HAL_GPIO_WritePin(BLE_UBLOX_DSR_GPIO_PORT,BLE_UBLOX_DSR_PIN,GPIO_PIN_SET);
									HAL_Delay(2000);
									BmTmpConfig++;
								break;
		case BM_INIT_ECHO: 			sprintf(TxBuffer,"ATE0\r");
			break;
		case BM_INIT_FACTORY:		sprintf(TxBuffer,"AT+UFACTORY\r");      /*Set to factory defined configuration */
								break;
		case BM_INIT_MODE:			sprintf(TxBuffer,"AT+UMSM=1\r");        /* start in Data mode */
								break;
		case BM_INIT_BLE:			sprintf(TxBuffer,"AT+UBTLE=2\r"); 		/* Bluetooth low energy Peripheral */
								break;
		case BM_INIT_NAME:			sprintf(TxBuffer,"AT+UBTLN=OSTC5-12345\r"); /* Bluetooth name */
									if(hardwareDataGetPointer()->primarySerial != 0xFFFF) /* module reinit? => restore old name */
									{
										gfx_number_to_string(5,1,&TxBuffer[15],hardwareDataGetPointer()->primarySerial);
									}
								break;
		case BM_INIT_SSP_IDO_OFF:	sprintf(TxBuffer,"AT+UDSC=0,0\r");    /* Disable SPP Server on ID0 */
								break;
		case BM_INIT_SSP_IDO_ON:	sprintf(TxBuffer,"AT+UDSC=0,3\r"); 	  /* SPP Server on ID0 */
								break;
		case BM_INIT_SSP_ID1_OFF:	sprintf(TxBuffer,"AT+UDSC=1,0\r");    /* Disable SPS Server on ID1 */
								break;
		case BM_INIT_SSP_ID1_ON:	sprintf(TxBuffer,"AT+UDSC=1,6\r");	  /* SPS Server on ID1 */
								break;
		case BM_INIT_STORE:			sprintf(TxBuffer,"AT&W\r");	          /* write settings into eeprom */
								break;
		case BM_INIT_RESTART:		sprintf(TxBuffer,"AT+CPWROFF\r");	  /* reboot module */
								break;
		case BM_INIT_DONE:			BmTmpConfig = BM_CONFIG_ECHO;
								break;
#endif
		default:
			break;
	}
	if(TxBuffer[0] != 0)		/* forward command to module */
	{
		CmdSize = strlen(TxBuffer);
		if(HAL_UART_Transmit(&UartHandle, (uint8_t*)TxBuffer,CmdSize, 2000) == HAL_OK)
		{
			result = tComm_CheckAnswerOK();

			if((BmTmpConfig == BM_CONFIG_BAUD) && (result == HAL_OK) && (UartHandle.Init.BaudRate != 460800)) /* is com already switched to fast speed? */
			{
				HAL_UART_DeInit(&UartHandle);
				HAL_Delay(1);
				UartHandle.Init.BaudRate   = 460800;
				HAL_UART_Init(&UartHandle);
			}
			else if((BmTmpConfig == BM_CONFIG_BAUD) && (result == HAL_OK) && (UartHandle.Init.BaudRate == 460800)) /* This shut not happen because default speed is 115200 => update module configuration */
			{
				tComm_GetBTCmdStr(BT_CMD_BAUDRATE_115, TxBuffer);

				CmdSize = strlen(TxBuffer);
				HAL_UART_Transmit(&UartHandle, (uint8_t*)TxBuffer,CmdSize, 2000);
				HAL_UART_DeInit(&UartHandle);
				HAL_Delay(10);
				UartHandle.Init.BaudRate   = 115200;
				HAL_UART_Init(&UartHandle);
				sprintf(TxBuffer,"AT&W\r");		/* write configuration */
				CmdSize = strlen(TxBuffer);
				HAL_UART_Transmit(&UartHandle, (uint8_t*)TxBuffer,CmdSize, 2000);
			}
			if(result == HAL_OK)
			{
				BmTmpConfig++;
				if(BmTmpConfig == BM_CONFIG_RETRY)
				{
					BmTmpConfig = BM_CONFIG_DONE;
				}
			}
			if(BmTmpConfig == BM_CONFIG_ECHO)
			{
				BmTmpConfig = BM_CONFIG_DONE;
				ConfigRetryCnt = 0;
				RestartModule = 1;
			}
		}
	}
	else		/* no command for the configuration step found => skip step */
	{
		if((BmTmpConfig > BM_CONFIG_OFF) && (BmTmpConfig < BM_CONFIG_DONE))
		{
			BmTmpConfig++;
		}
	}
	if(result != HAL_OK)
	{
		ConfigRetryCnt++;
		if(ConfigRetryCnt > 3)		/* Configuration failed => switch off module */
		{
			MX_Bluetooth_PowerOff();
			if(RestartModule)
			{
				RestartModule = 0;      /* only one try */
				ConfigRetryCnt = 200;	/* used for delay to startup module again */

				if((BmTmpConfig == BM_CONFIG_ECHO) || (BmTmpConfig == BM_INIT_ECHO))	/* the module did not answer even once => try again with alternative baud rate */
				{
					HAL_UART_DeInit(&UartHandle);
					HAL_Delay(1);
					UartHandle.Init.BaudRate   = 460800;
					HAL_UART_Init(&UartHandle);
				}
				BmTmpConfig = BM_CONFIG_RETRY;
			}
			else						/* even restarting module failed => switch bluetooth off */
			{
				ConfigRetryCnt = 0;
				BmTmpConfig = BM_CONFIG_OFF;
				settingsGetPointer()->bluetoothActive = 0;
			}
		}
	}
	return result;
}

static void tComm_Error_Handler(void)
{
  while(1)
  {}
}
