
#include "stm32f4xx_hal.h" /* for HAL_Delay() */
#include "ostc.h"
#include "display.h"

#define TFT_ENABLE_EXTENDED_COMMANDS	0xB9
#define TFT_SET_POWER					0xB1
#define TFT_SLEEP_OUT					0x11
#define TFT_DISPLAY_INVERSION_OFF		0x20
#define TFT_MEMORY_ACCESS_ONTROL		0x36
#define TFT_INTERFACE_PIXEL_FORMAT		0x3A
#define TFT_SET_RGB_INTERFACE_RELATED	0xB3
#define TFT_SET_DISPLAY_WAVEFORM		0xB4
#define TFT_SET_PANEL					0xCC
#define TFT_SET_GAMMA_CURVE_RELATED		0xE0
#define TFT_DISPLAY_ON					0x29
#define TFT_DISPLAY_OFF					0x28
#define TFT_SLEEP_IN					0x10

#define OLED_SCTE_SET_31h				0x31		// 0x0008
#define OLED_WCWE_SET_32h				0x32		// 0x0014
#define OLED_GATELESS1_30h				0x30		// 0x0002
#define OLED_GATELESS2_27h				0x27		// 0x0000
#define	OLED_OSCILLATOR					0x11		// 0x00A1
#define OLED_VBP_SET_12h				0x12		// 0x0008
#define OLED_VFP_SET_13h				0x13		// 0x0008
#define OLED_DISPLAY_CON_15h			0x15		// 0x0000
#define OLED_COLOR_DEPTH_SET_16h		0x16		// 0x0000
#define OLED_PENTILE_KEY_EFh			0xEF		// 0x00D0 or 0x00E8
#define	OLED_PENTILE1_A0h				0xA0		// 0x0063
#define	OLED_PENTILE2_A1h				0xA1		// 0x00C0
#define	OLED_PENTILE3_A2h				0xA2		// 0x0032
#define	OLED_PENTILE4_A3h				0xA3		// 0x0002
#define	OLED_BRIGHTNESS_CTRL_39h		0x39		// 0044h
// gamma table 0x40 - 0x66
#define OLED_BOOSTING_FREQ				0x17		// 0x0022
#define OLED_AMP_SET_18h				0x18		// 0x0033
#define OLED_GAMMA_AMP_19h				0x19		// 0x0003
#define OLED_POWER_CONTROL2_1Ah			0x1A		// 0x0001
#define	OLED_POWER_CONTROL2_1Bh			0x1B		//
#define	OLED_POWER_CONTROL2_1Ch			0x1C		//
#define OLED_INTERNAL_LOGIC_VOLTAGE		0x22		// VCC*0,65 = 3,3V * 0,55 = 0x00A2
#define OLED_POWER_SET					0x23		// VC1OUT = VCI X 0.98 (default) = 0x00
#define OLED_POWER_SET2					0x24		// VREG2OUT = 5,4V, VREG1OUT = 4,2V =0x77
#define OLED_DISPLAY_CONDITION_SET_26h	0x26		// 0x00A0
#define	OLED_STB_BY_OFF_1Dh				0x1D		// 00A0 + 300ms wait
#define	OLED_DDISP_ON_14h				0x14		// 0003

static void Display_Error_Handler(void);
static void display_power_on__2_of_2__post_RGB_display0(void);
static void display_power_on__2_of_2__post_RGB_display1(void);
void display_1_brightness_max(void);
void display_1_brightness_high(void);
void display_1_brightness_std(void);
void display_1_brightness_eco(void);
void display_1_brightness_cave(void);

static uint8_t receive_screen();
uint8_t brightness_screen1;

void display_power_on__1_of_2__pre_RGB(void)
{
	uint8_t aTxBuffer[3];
	/* reset system */
	HAL_GPIO_WritePin(DISPLAY_CSB_GPIO_PORT,DISPLAY_CSB_PIN,GPIO_PIN_SET); // chip select

	HAL_GPIO_WritePin(DISPLAY_RESETB_GPIO_PORT,DISPLAY_RESETB_PIN,GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(DISPLAY_RESETB_GPIO_PORT,DISPLAY_RESETB_PIN,GPIO_PIN_SET);
	HAL_Delay(25);
	// check for new screen
	hardwareDisplay=0;		// default is old screen
	aTxBuffer[0] = 0x71;	// Read internal register
	if (receive_screen((uint8_t*)aTxBuffer) == 0x27)		// chip Index (=0x27 for new screen)
		{
		hardwareDisplay=1;
		}
	else
	{	// re-reset the screen to be sure the 0x71 command did nothing
		HAL_GPIO_WritePin(DISPLAY_RESETB_GPIO_PORT,DISPLAY_RESETB_PIN,GPIO_PIN_RESET);
		HAL_Delay(10);
		HAL_GPIO_WritePin(DISPLAY_RESETB_GPIO_PORT,DISPLAY_RESETB_PIN,GPIO_PIN_SET);
		HAL_Delay(25);

	}

	/* RGB signals should be now for 2 frames or more (datasheet) */
}


static void send(uint8_t *pData, uint16_t inputlength)
{
	HAL_GPIO_WritePin(DISPLAY_CSB_GPIO_PORT,DISPLAY_CSB_PIN,GPIO_PIN_RESET); // chip select

	if(HAL_SPI_Transmit(&hspiDisplay,(uint8_t*)pData, inputlength, 10000) != HAL_OK)
		Display_Error_Handler();

	while (HAL_SPI_GetState(&hspiDisplay) != HAL_SPI_STATE_READY)
  {
  }
	HAL_GPIO_WritePin(DISPLAY_CSB_GPIO_PORT,DISPLAY_CSB_PIN,GPIO_PIN_SET); // chip select
}

static uint8_t receive_screen(uint8_t *pData)
{
	uint8_t byte;
	HAL_GPIO_WritePin(DISPLAY_CSB_GPIO_PORT,DISPLAY_CSB_PIN,GPIO_PIN_RESET); // chip select
	if(HAL_SPI_Transmit(&hspiDisplay,(uint8_t*)pData, 1, 10000) != HAL_OK)
		Display_Error_Handler();
	while (HAL_SPI_GetState(&hspiDisplay) != HAL_SPI_STATE_READY)
  {
  }
	if(HAL_SPI_Receive(&hspiDisplay, &byte, 1, 10000) != HAL_OK)
		Display_Error_Handler();
	while (HAL_SPI_GetState(&hspiDisplay) != HAL_SPI_STATE_READY)
  {
  }
	HAL_GPIO_WritePin(DISPLAY_CSB_GPIO_PORT,DISPLAY_CSB_PIN,GPIO_PIN_SET); // chip select
	return byte;
}


static uint16_t convert8to9to8(uint8_t *pInput, uint8_t *pOutput,uint16_t inputlength)
{
	uint16_t outputlength;
	uint8_t readbit =  0x80;//0b1000000;
	uint8_t writebit = 0x40;//0b0100000;
	uint16_t i,j,k;

	outputlength = ((inputlength+7)/8)*9;

	for(i=0;i<outputlength;i++)
		pOutput[i] = 0;

	k = 0;
	for(i=0;i<inputlength;i++)
	{
		if(i != 0)
		{
			pOutput[k] |= writebit; // 9. bit
			writebit = writebit >> 1;
			if(writebit == 0)
			{
				writebit = 0x80;
				k++;
			}
		}
		for(j=0;j<8;j++)
		{
			if((pInput[i] & readbit) != 0)
			{
				pOutput[k] |= writebit;
			}
			readbit = readbit >> 1;
			if(readbit == 0)
				readbit = 0x80;
			writebit = writebit >> 1;
			if(writebit == 0)
			{
				writebit = 0x80;
				k++;
			}
		}
	}
	return outputlength;
}

void display_power_off(void)
{
	if (hardwareDisplay == 1)
		{
		uint8_t aTxBuffer[3];

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = OLED_DDISP_ON_14h;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x00;
		send((uint8_t*)aTxBuffer, 2);
		HAL_Delay(25);
		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = OLED_STB_BY_OFF_1Dh;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0xA1;
		send((uint8_t*)aTxBuffer, 2);
		HAL_Delay(200);
		HAL_GPIO_WritePin(DISPLAY_RESETB_GPIO_PORT,DISPLAY_RESETB_PIN,GPIO_PIN_RESET);
		}
	else
	{
		// display 0
	}
}


void display_power_on__2_of_2__post_RGB(void)
{
	if (hardwareDisplay == 1)
		{
		display_power_on__2_of_2__post_RGB_display1();
		}
		else
		{
		display_power_on__2_of_2__post_RGB_display0();
		}
}

 void display_power_on__2_of_2__post_RGB_display0(void)
{

	uint8_t aTxBuffer[32];
	uint8_t bTxBuffer[36];
	uint16_t i,length;

	for(i=0;i<32;i++)
		aTxBuffer[i] = 0;
	for(i=0;i<36;i++)
		bTxBuffer[i] = 0;

	aTxBuffer[0] = TFT_ENABLE_EXTENDED_COMMANDS;
	aTxBuffer[1] = 0xFF;
	aTxBuffer[2] = 0x83;
	aTxBuffer[3] = 0x63;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,4);
	send((uint8_t*)bTxBuffer, length);

	aTxBuffer[0] = TFT_SET_POWER;
	aTxBuffer[1] = 0x81;
	aTxBuffer[2] = 0x24;
	aTxBuffer[3] = 0x04;
	aTxBuffer[4] = 0x02;
	aTxBuffer[5] = 0x02;
	aTxBuffer[6] = 0x03;
	aTxBuffer[7] = 0x10;
	aTxBuffer[8] = 0x10;
	aTxBuffer[9] = 0x34;
	aTxBuffer[10] = 0x3C;
	aTxBuffer[11] = 0x3F;
	aTxBuffer[12] = 0x3F;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,13);
	send((uint8_t*)bTxBuffer, length);

	aTxBuffer[0] = TFT_SLEEP_OUT;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,1);
	send((uint8_t*)bTxBuffer, length);
	HAL_Delay(5+1);

	aTxBuffer[0] = TFT_DISPLAY_INVERSION_OFF;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,1);
	send((uint8_t*)bTxBuffer, length);

	aTxBuffer[0] = TFT_MEMORY_ACCESS_ONTROL;
	aTxBuffer[1] = 0x00;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,2);
	send((uint8_t*)bTxBuffer, length);

	aTxBuffer[0] = TFT_INTERFACE_PIXEL_FORMAT;
	aTxBuffer[1] = 0x70;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,2);
	send((uint8_t*)bTxBuffer, length);
	HAL_Delay(120+20);

	aTxBuffer[0] = TFT_SET_POWER;
	aTxBuffer[1] = 0x78;
	aTxBuffer[2] = 0x24;
	aTxBuffer[3] = 0x04,
	aTxBuffer[4] = 0x02;
	aTxBuffer[5] = 0x02;
	aTxBuffer[6] = 0x03;
	aTxBuffer[7] = 0x10;
	aTxBuffer[8] = 0x10;
	aTxBuffer[9] = 0x34;
	aTxBuffer[10] = 0x3C;
	aTxBuffer[11] = 0x3F;
	aTxBuffer[12] = 0x3F;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,13);
	send((uint8_t*)bTxBuffer, length);

	aTxBuffer[0] = TFT_SET_RGB_INTERFACE_RELATED;
	aTxBuffer[1] = 0x01;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,2);
	send((uint8_t*)bTxBuffer, length);

	aTxBuffer[0] = TFT_SET_DISPLAY_WAVEFORM;
	aTxBuffer[1] = 0x00;
	aTxBuffer[2] = 0x08;
	aTxBuffer[3] = 0x56;
	aTxBuffer[4] = 0x07;
	aTxBuffer[5] = 0x01;
	aTxBuffer[6] = 0x01;
	aTxBuffer[7] = 0x4D;
	aTxBuffer[8] = 0x01;
	aTxBuffer[9] = 0x42;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,10);
	send((uint8_t*)bTxBuffer, length);

	aTxBuffer[0] = TFT_SET_PANEL;
	aTxBuffer[1] = 0x0B;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,2);
	send((uint8_t*)bTxBuffer, length);

	aTxBuffer[0] = TFT_SET_GAMMA_CURVE_RELATED;
	aTxBuffer[1] = 0x01;
	aTxBuffer[2] = 0x48;
	aTxBuffer[3] = 0x4D;
	aTxBuffer[4] = 0x4E;
	aTxBuffer[5] = 0x58;
	aTxBuffer[6] = 0xF6;
	aTxBuffer[7] = 0x0B;
	aTxBuffer[8] = 0x4E;
	aTxBuffer[9] = 0x12;
	aTxBuffer[10] = 0xD5;
	aTxBuffer[11] = 0x15;
	aTxBuffer[12] = 0x95;
	aTxBuffer[13] = 0x55;
	aTxBuffer[14] = 0x8E;
	aTxBuffer[15] = 0x11;
	aTxBuffer[16] = 0x01;
	aTxBuffer[17] = 0x48;
	aTxBuffer[18] = 0x4D;
	aTxBuffer[19] = 0x55;
	aTxBuffer[20] = 0x5F;
	aTxBuffer[21] = 0xFD;
	aTxBuffer[22] = 0x0A;
	aTxBuffer[23] = 0x4E;
	aTxBuffer[24] = 0x51;
	aTxBuffer[25] = 0xD3;
	aTxBuffer[26] = 0x17;
	aTxBuffer[27] = 0x95;
	aTxBuffer[28] = 0x96;
	aTxBuffer[29] = 0x4E;
	aTxBuffer[30] = 0x11;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,31);
	send((uint8_t*)bTxBuffer, length);
	HAL_Delay(5+1);

	aTxBuffer[0] = TFT_DISPLAY_ON;
	length = convert8to9to8((uint8_t*)aTxBuffer,(uint8_t*)bTxBuffer,1);
	send((uint8_t*)bTxBuffer, length);
}


void display_power_on__2_of_2__post_RGB_display1(void)
{
	uint8_t aTxBuffer[3];

	aTxBuffer[0] = 0x71;	// Read chip Index & revision number
	aTxBuffer[1] = 0x00;	// Dummy write - reads out 0x27
	aTxBuffer[1] = 0x00;	// Dummy write - reads out 0x96
	send((uint8_t*)aTxBuffer, 3);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_OSCILLATOR;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0xA4;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_SCTE_SET_31h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x08;//8
	send((uint8_t*)aTxBuffer, 2);

	/*//debug read
	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_SCTE_SET_31h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x73;	// Read internal register
	aTxBuffer[1] = 0x00;	// Dummy write - reads out 0x08 (The just-set OLED_SCTE_SET value)
	send((uint8_t*)aTxBuffer, 2);
	*/

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_WCWE_SET_32h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x14;//14
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_GATELESS1_30h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x02;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_GATELESS2_27h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x01;
	send((uint8_t*)aTxBuffer, 2);


	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_VBP_SET_12h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x08;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_VFP_SET_13h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x08;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_DISPLAY_CON_15h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x01;		//SS=0
	//aTxBuffer[1] = 0x11;		//SS=1
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_COLOR_DEPTH_SET_16h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_PENTILE_KEY_EFh;	// write-only register...
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0xD0;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0xE8;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_PENTILE1_A0h;	// write-only register...
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x63;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_PENTILE2_A1h;	// write-only register...
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0xC0;				// SID1&SID0=00
//	aTxBuffer[1] = 0xC4;				// SID1&SID0=01    CC C8 C4 C0
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_PENTILE3_A2h;	// write-only register...
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x32;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_PENTILE4_A3h;	// write-only register...
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x02;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_BRIGHTNESS_CTRL_39h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x44;//44
	send((uint8_t*)aTxBuffer, 2);


	display_1_brightness_std();			// boot brightness


	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_BOOSTING_FREQ;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x22;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_AMP_SET_18h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x22;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_GAMMA_AMP_19h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x02;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_POWER_CONTROL2_1Ah;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	/*
	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_POWER_CONTROL2_1Bh;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x4B;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_POWER_CONTROL2_1Ch;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x05;
	send((uint8_t*)aTxBuffer, 2);
	*/

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_INTERNAL_LOGIC_VOLTAGE;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0xA2;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_POWER_SET;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_POWER_SET2;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x77;
	send((uint8_t*)aTxBuffer, 2);


	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_DISPLAY_CONDITION_SET_26h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0xA0;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_STB_BY_OFF_1Dh;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0xA0;
	send((uint8_t*)aTxBuffer, 2);

	HAL_Delay(250);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = OLED_DDISP_ON_14h;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x03;
	send((uint8_t*)aTxBuffer, 2);
}

void display_1_brightness_max(void)
{
	uint8_t aTxBuffer[3];

	if (brightness_screen1 == 4)
	{
		// do nothing
	}
	else
	{
		// GAMMA L=250
		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x40;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x00;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x41;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x3F;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x42;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x2A;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x43;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x27;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x44;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x27;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x45;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x1F;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x46;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x44;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x50;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x00;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x51;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x00;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x52;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x17;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x53;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x24;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x54;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x26;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x55;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x1F;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x56;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x43;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x60;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x00;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x61;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x3F;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x62;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x2A;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x63;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x25;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x64;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x24;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x65;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x1B;
		send((uint8_t*)aTxBuffer, 2);

		aTxBuffer[0] = 0x70;
		aTxBuffer[1] = 0x66;
		send((uint8_t*)aTxBuffer, 2);
		aTxBuffer[0] = 0x72;
		aTxBuffer[1] = 0x5C;
		send((uint8_t*)aTxBuffer, 2);

		brightness_screen1=4;
	}
}

void display_1_brightness_high(void)
{
	uint8_t aTxBuffer[3];
	if (brightness_screen1 == 3)
	{
		// do nothing
	}
	else
	{
	// GAMMA L=200
	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x40;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x41;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x42;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2A;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x43;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x27;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x44;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x27;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x45;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x1F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x46;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x44;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x50;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x51;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x52;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x17;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x53;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x24;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x54;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x26;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x55;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x1F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x56;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x43;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x60;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x61;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x62;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2A;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x63;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x25;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x64;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x24;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x65;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x1B;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x66;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x5C;
	send((uint8_t*)aTxBuffer, 2);

	brightness_screen1=3;
	}
}


void display_1_brightness_std(void)
{
	uint8_t aTxBuffer[3];

	if (brightness_screen1 == 2)
	{
		// do nothing
	}
	else
	{
	// GAMMA L=150
	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x40;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x41;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x42;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2D;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x43;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x29;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x44;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x28;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x45;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x23;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x46;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x37;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x50;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x51;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x52;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x0B;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x53;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x25;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x54;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x28;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x55;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x22;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x56;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x36;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x60;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x61;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x62;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2B;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x63;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x28;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x64;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x26;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x65;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x1F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x66;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x4A;
	send((uint8_t*)aTxBuffer, 2);

	brightness_screen1=2;
	}
}

void display_1_brightness_eco(void)
{
	uint8_t aTxBuffer[3];

	if (brightness_screen1 == 1)
	{
		// do nothing
	}
	else
	{
	// GAMMA L=100
	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x40;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x41;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x42;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x30;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x43;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2A;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x44;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2B;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x45;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x24;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x46;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x50;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x51;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x52;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x53;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x25;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x54;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x29;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x55;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x24;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x56;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2E;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x60;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x61;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x62;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x63;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x29;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x64;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x29;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x65;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x21;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x66;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	brightness_screen1=1;
	}
}

void display_1_brightness_cave(void)
{
	uint8_t aTxBuffer[3];

	if (brightness_screen1 == 0)
	{
		// do nothing
	}
	else
	{
	// GAMMA L=50
	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x40;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x41;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x42;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3C;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x43;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2C;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x44;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2D;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x45;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x27;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x46;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x24;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x50;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x51;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x52;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x53;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x22;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x54;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2A;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x55;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x27;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x56;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x23;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x60;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x00;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x61;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3F;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x62;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x3B;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x63;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2C;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x64;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x2B;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x65;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x24;
	send((uint8_t*)aTxBuffer, 2);

	aTxBuffer[0] = 0x70;
	aTxBuffer[1] = 0x66;
	send((uint8_t*)aTxBuffer, 2);
	aTxBuffer[0] = 0x72;
	aTxBuffer[1] = 0x31;
	send((uint8_t*)aTxBuffer, 2);

	brightness_screen1=0;
	}
}

static void Display_Error_Handler(void)
{
  //while(1)
  {
  }
}
