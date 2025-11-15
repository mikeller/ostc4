/**
  ******************************************************************************
	* @copyright heinrichs weikamp
  * @file   		data_central_mini.c   - bootloader only -
  * @author 		heinrichs weikamp gmbh
  * @date   		10-November-2014
  * @version		V1.0.3
  * @since			10-Nov-2014
  * @brief	
	* @bug
	* @warning
  @verbatim
  
	 @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "data_central.h"
#include "stm32f4xx_hal.h"
#include "crcmodel.h"

void translateDate(uint32_t datetmpreg, RTC_DateTypeDef *sDate)
{
  datetmpreg = (uint32_t)(datetmpreg & RTC_DR_RESERVED_MASK);

  /* Fill the structure fields with the read parameters */
  sDate->Year = (uint8_t)((datetmpreg & (RTC_DR_YT | RTC_DR_YU)) >> 16);
  sDate->Month = (uint8_t)((datetmpreg & (RTC_DR_MT | RTC_DR_MU)) >> 8);
  sDate->Date = (uint8_t)(datetmpreg & (RTC_DR_DT | RTC_DR_DU));
  sDate->WeekDay = (uint8_t)((datetmpreg & (RTC_DR_WDU)) >> 13);

	/* Convert the date structure parameters to Binary format */
	sDate->Year = (uint8_t)RTC_Bcd2ToByte(sDate->Year);
	sDate->Month = (uint8_t)RTC_Bcd2ToByte(sDate->Month);
	sDate->Date = (uint8_t)RTC_Bcd2ToByte(sDate->Date);
}

void translateTime(uint32_t tmpreg, RTC_TimeTypeDef *sTime)
{
  tmpreg = (uint32_t)(tmpreg & RTC_TR_RESERVED_MASK);

  /* Fill the structure fields with the read parameters */
  sTime->Hours = (uint8_t)((tmpreg & (RTC_TR_HT | RTC_TR_HU)) >> 16);
  sTime->Minutes = (uint8_t)((tmpreg & (RTC_TR_MNT | RTC_TR_MNU)) >>8);
  sTime->Seconds = (uint8_t)(tmpreg & (RTC_TR_ST | RTC_TR_SU));
  sTime->TimeFormat = (uint8_t)((tmpreg & (RTC_TR_PM)) >> 16);

	/* Convert the time structure parameters to Binary format */
	sTime->Hours = (uint8_t)RTC_Bcd2ToByte(sTime->Hours);
	sTime->Minutes = (uint8_t)RTC_Bcd2ToByte(sTime->Minutes);
	sTime->Seconds = (uint8_t)RTC_Bcd2ToByte(sTime->Seconds);
  sTime->SubSeconds = 0;
}


/* This is derived from crc32b but does table lookup. First the table
itself is calculated, if it has not yet been set up.
Not counting the table setup (which would probably be a separate
function), when compiled to Cyclops with GCC, this function executes in
7 + 13n instructions, where n is the number of bytes in the input
message. It should be doable in 4 + 9n instructions. In any case, two
of the 13 or 9 instrucions are load byte.
   This is Figure 14-7 in the text. */

/* http://www.hackersdelight.org/ i guess ;-)  *hw */

uint32_t crc32c_checksum(uint8_t* message, uint16_t length, uint8_t* message2, uint16_t length2) {
	int i, j;
	uint32_t byte, crc, mask;
	static unsigned int table[256] = {0};

	/* Set up the table, if necessary. */
	if (table[1] == 0) {
		for (byte = 0; byte <= 255; byte++) {
			 crc = byte;
			 for (j = 7; j >= 0; j--) {    // Do eight times.
					mask = -(crc & 1);
					crc = (crc >> 1) ^ (0xEDB88320 & mask);
			 }
			 table[byte] = crc;
		}
	}

	/* Through with table setup, now calculate the CRC. */
	i = 0;
	crc = 0xFFFFFFFF;
	while (length--) {
		byte = message[i];
		crc = (crc >> 8) ^ table[(crc ^ byte) & 0xFF];
		i = i + 1;
	}
	if(length2)
	{
	 i = 0;
	 while (length2--) {
			byte = message2[i];
			crc = (crc >> 8) ^ table[(crc ^ byte) & 0xFF];
			i = i + 1;
	 }
	}
	return ~crc;
}


uint32_t	CRC_CalcBlockCRC_moreThan768000(uint32_t *buffer1, uint32_t *buffer2, uint32_t words)
{
	 cm_t        crc_model;
	 uint32_t      word_to_do;
	 uint8_t       byte_to_do;
	 int         i;

	 uint32_t wordCnt = 0;
 
     // Values for the STM32F generator.
 
     crc_model.cm_width = 32;            // 32-bit CRC
     crc_model.cm_poly  = 0x04C11DB7;    // CRC-32 polynomial
     crc_model.cm_init  = 0xFFFFFFFF;    // CRC initialized to 1's
     crc_model.cm_refin = FALSE;         // CRC calculated MSB first
     crc_model.cm_refot = FALSE;         // Final result is not bit-reversed
     crc_model.cm_xorot = 0x00000000;    // Final result XOR'ed with this
 
     cm_ini(&crc_model);
 
     do
     {
         // The STM32F10x hardware does 32-bit words at a time!!!
				if(wordCnt >= (768000/4))
					word_to_do = *buffer2++;
				else
					word_to_do = *buffer1++;
 
         // Do all bytes in the 32-bit word.
 
         for (i = 0; i < sizeof(word_to_do); i++)
         {
             // We calculate a *byte* at a time. If the CRC is MSB first we
             // do the next MS byte and vica-versa.
 
             if (crc_model.cm_refin == FALSE)
             {
                 // MSB first. Do the next MS byte.
 
                 byte_to_do = (uint8_t) ((word_to_do & 0xFF000000) >> 24);
                 word_to_do <<= 8;
             }
             else
             {
                 // LSB first. Do the next LS byte.
 
                 byte_to_do = (uint8_t) (word_to_do & 0x000000FF);
                 word_to_do >>= 8;
             }
 
             cm_nxt(&crc_model, byte_to_do);
         }
         wordCnt++;
     } while (wordCnt != words);
 
     // Return the final result.
 
     return (cm_crc(&crc_model));
}
 

uint32_t	CRC_CalcBlockCRC(uint32_t *buffer, uint32_t words)
{
 cm_t        crc_model;
 uint32_t      word_to_do;
 uint8_t       byte_to_do;
 int         i;
 
     // Values for the STM32F generator.
 
     crc_model.cm_width = 32;            // 32-bit CRC
     crc_model.cm_poly  = 0x04C11DB7;    // CRC-32 polynomial
     crc_model.cm_init  = 0xFFFFFFFF;    // CRC initialized to 1's
     crc_model.cm_refin = FALSE;         // CRC calculated MSB first
     crc_model.cm_refot = FALSE;         // Final result is not bit-reversed
     crc_model.cm_xorot = 0x00000000;    // Final result XOR'ed with this
 
     cm_ini(&crc_model);
 
     while (words--)
     {
         // The STM32F10x hardware does 32-bit words at a time!!!
 
         word_to_do = *buffer++;
 
         // Do all bytes in the 32-bit word.
 
         for (i = 0; i < sizeof(word_to_do); i++)
         {
             // We calculate a *byte* at a time. If the CRC is MSB first we
             // do the next MS byte and vica-versa.
 
             if (crc_model.cm_refin == FALSE)
             {
                 // MSB first. Do the next MS byte.
 
                 byte_to_do = (uint8_t) ((word_to_do & 0xFF000000) >> 24);
                 word_to_do <<= 8;
             }
             else
             {
                 // LSB first. Do the next LS byte.
 
                 byte_to_do = (uint8_t) (word_to_do & 0x000000FF);
                 word_to_do >>= 8;
             }
 
             cm_nxt(&crc_model, byte_to_do);
         }
     }
 
     // Return the final result.
 
     return (cm_crc(&crc_model));
}
 
