/**
  ******************************************************************************
  * @file    firmwareEraseProgram.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    05-May-2015
  * @version V0.0.1
  * @since   05-May-2015
  * @brief   erase and program the STM32F4xx internal FLASH memory
  *
  @verbatim
  ==============================================================================
                        ##### How to use #####
  ==============================================================================
	ADDR_FLASH_SECTOR_0 to/with ADDR_FLASH_SECTOR_5 (256KB) is used for this bootloader

	ADDR_FLASH_SECTOR_23 is blocked and used for Font T48 and image_heinrichs_weikamp
	Font T24 for button text is not blocked / protected
	other fonts should not be used here


  ==============================================================================
                        ##### From  AN2557 #####
							STM32F10xxx In-Application programming CD00161640.pdf   2010
  ==============================================================================
User program conditions
The user application to be loaded into the Flash memory using IAP should be built with
these configuration settings:
1. Set the program load address at 0x08003000, using your toolchain linker file
2. Relocate the vector table at address 0x08003000, using the
"NVIC_SetVectorTable"function or the VECT_TAB_OFFSET definition inside the
"system_stm32f10x.c"

can be found here system_stm32f4xx.c


	@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "firmwareEraseProgram.h"
#include "settings.h" // to access SHardwareData

/* Exported variables --------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

/* Base address of the Flash sectors Bank 2 */
#define ADDR_FLASH_SECTOR_12     ((uint32_t)0x08100000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_13     ((uint32_t)0x08104000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_14     ((uint32_t)0x08108000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_15     ((uint32_t)0x0810C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_16     ((uint32_t)0x08110000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_17     ((uint32_t)0x08120000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_18     ((uint32_t)0x08140000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_19     ((uint32_t)0x08160000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_20     ((uint32_t)0x08180000) /* Base @ of Sector 8, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_21     ((uint32_t)0x081A0000) /* Base @ of Sector 9, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_22     ((uint32_t)0x081C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_23     ((uint32_t)0x081E0000) /* Base @ of Sector 11, 128 Kbytes */

#define SECTOR_SIZE_128KB     ((uint32_t)0x00020000) 

#define FLASH_FW_START_ADDR   ADDR_FLASH_SECTOR_6
#define FLASH_FW_END_ADDR     (ADDR_FLASH_SECTOR_12 - 1)

#define FLASH_FW2_START_ADDR   ADDR_FLASH_SECTOR_12
#define FLASH_FW2_END_ADDR     (ADDR_FLASH_SECTOR_22 + SECTOR_SIZE_128KB - 1)

/* Private variables ---------------------------------------------------------*/

static FLASH_EraseInitTypeDef EraseInitStruct; /*Variable used for Erase procedure*/

uint32_t FirstSector = 0, NbOfSectors = 0, Address = 0;
uint32_t SectorError = 0;
__IO uint32_t data32 = 0 , MemoryProgramStatus = 0;



/* Private function prototypes -----------------------------------------------*/
//static void firmware_Error_Handler(HAL_StatusTypeDef reason);
static uint32_t GetSector(uint32_t Address);
uint8_t hardware_programm_sub(uint8_t *buffer64, uint8_t length, uint32_t startAddress);

/* Exported functions --------------------------------------------------------*/

const SHardwareData* hardwareDataGetPointer(void)
{
	return (SHardwareData*)HARDWAREDATA_ADDRESS;
}

uint8_t hardware_programmPrimaryBluetoothNameSet(void)
{
	uint8_t data = 0xF0;
	return hardware_programm_sub(&data, 1, HARDWAREDATA_ADDRESS + 7);
}


uint8_t hardware_programmSecondaryBluetoothNameSet(void)
{
	uint8_t data = 0xF0;
	return hardware_programm_sub(&data, 1, HARDWAREDATA_ADDRESS + 52 + 7);
}


uint8_t hardware_programmProductionData(uint8_t *buffer52)
{
	buffer52[7] = 0xFF;// production_bluetooth_name_set
	return hardware_programm_sub(buffer52, 52, HARDWAREDATA_ADDRESS);// check base_bootloader.c of OSTC4bootloader code and settings.h
}


uint8_t hardware_programmSecondarySerial(uint8_t *buffer12)
{
	buffer12[7] = 0xFF;// secondary_bluetooth_name_set
	return hardware_programm_sub(buffer12, 12, HARDWAREDATA_ADDRESS + 52);
}


uint8_t hardware_programm_sub(uint8_t *buffer, uint8_t length, uint32_t startAddress)
{
	HAL_StatusTypeDef answer;

	uint32_t ptr = 0;
	uint8_t data8;

	// test empty
	Address = startAddress; 
	for(int i=0;i<length;i++)
	{
		if((*(uint8_t *)Address != 0xFF) && (buffer[i] != 0xFF))
			return 0xE0;
		Address = Address + 1;
	}
	
	// start programming
	HAL_FLASH_Unlock();
	
	Address = startAddress; 
	ptr = 0;
	answer = HAL_OK;
  while (ptr < length)
  {
		if(buffer[ptr] != 0xFF)
		{
			answer = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address, buffer[ptr]);
		}
    if (answer == HAL_OK)
    {
      Address = Address + 1;
			ptr++;
    }
    else
    { 
			HAL_FLASH_Lock(); 
			return answer;
    }
  }
	HAL_FLASH_Lock(); 

  /* Check if the programmed data is OK 
      MemoryProgramStatus = 0: data programmed correctly
      MemoryProgramStatus != 0: number of words not programmed correctly ******/
	Address = startAddress; // check base_bootloader.c of OSTC4bootloader code
  MemoryProgramStatus = 0x0;
  
	ptr = 0;
  while(ptr < length)
  {
    data8 = *(__IO uint8_t*)Address;

    if((buffer[ptr] != 0xFF) && (data8 !=  buffer[ptr]))
    {
      MemoryProgramStatus++;  
    }

    Address = Address + 1;
		ptr++;
  }  

  /* Check if there is an issue to program data */
  if (MemoryProgramStatus == 0)
  {
		return HAL_OK;
  }
  else
  {
		return 0xEE;
  }
}


uint8_t firmware2_variable_upperpart_eraseFlashMemory(uint32_t length, uint32_t offset)
{
	uint32_t startAddress, endAddress;

//	HAL_StatusTypeDef answer;
	HAL_FLASH_Unlock();
  
	startAddress = FLASH_FW2_START_ADDR + offset;
	endAddress = startAddress + length;
	
	if(endAddress > FLASH_FW2_END_ADDR)
		endAddress = FLASH_FW2_END_ADDR;
	
  FirstSector = GetSector(startAddress);
  NbOfSectors = GetSector(endAddress) - FirstSector + 1;

  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_1;
  EraseInitStruct.Sector = FirstSector;
  EraseInitStruct.NbSectors = NbOfSectors;
  
	return HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
}


uint8_t firmware2_variable_upperpart_programFlashMemory(uint32_t length, uint32_t offset, uint8_t *pBuffer1, uint32_t pBuffer1Size, uint8_t *pBuffer2)
{
	HAL_StatusTypeDef answer;
	uint32_t ptr = 0;
	uint32_t length1, length2;
	
	if((pBuffer2) && (length > pBuffer1Size))
	{
		length1 = pBuffer1Size;
		length2 = length - length1;
	}
	else
	{
		length1 = length;
		length2 = 0;
	}
		
	Address = FLASH_FW2_START_ADDR + offset;

	ptr = 0;
  while ((Address <= FLASH_FW2_END_ADDR) && (ptr < length1))
  {
		answer = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address, pBuffer1[ptr]);
    if (answer == HAL_OK)
    {
      Address = Address + 1;
			ptr++;
    }
    else
    { 
			return answer;
    }
  }
	ptr = 0;
  while ((Address <= FLASH_FW2_END_ADDR) && (ptr < length2))
  {
		answer = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address, pBuffer2[ptr]);
    if (answer == HAL_OK)
    {
      Address = Address + 1;
			ptr++;
    }
    else
    { 
			return answer;
    }
  }
  HAL_FLASH_Lock(); 

	Address = FLASH_FW2_START_ADDR + offset;;
  MemoryProgramStatus = 0x0;
  
	ptr = 0;
  while ((Address <= FLASH_FW2_END_ADDR) && (ptr < length1))
  {
    data32 = *(__IO uint8_t*)Address;

    if (data32 !=  pBuffer1[ptr])
    {
      MemoryProgramStatus++;  
    }

    Address = Address + 1;
		ptr++;
  }  
	ptr = 0;
  while ((Address <= FLASH_FW2_END_ADDR) && (ptr < length2))
  {
    data32 = *(__IO uint8_t*)Address;

    if (data32 !=  pBuffer2[ptr])
    {
      MemoryProgramStatus++;  
    }

    Address = Address + 1;
		ptr++;
  }  

  if (MemoryProgramStatus == 0)
  {
		return HAL_OK;
  }
  else
  {
		return 0xEE;
  }
}

uint8_t firmware_eraseFlashMemory(void)
{
//	HAL_StatusTypeDef answer;
  /* Unlock the Flash to enable the flash control register access *************/ 
  HAL_FLASH_Unlock();

  /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  /* Get the 1st sector to erase */
  FirstSector = GetSector(FLASH_FW_START_ADDR);
  /* Get the number of sector to erase from 1st sector*/
  NbOfSectors = GetSector(FLASH_FW_END_ADDR) - FirstSector + 1;

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_1;
  EraseInitStruct.Sector = FirstSector;
  EraseInitStruct.NbSectors = NbOfSectors;
  
  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
	return HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
}

uint8_t firmware_programFlashMemory(uint8_t *pBuffer1, uint32_t length1)//, uint8_t *pBuffer2, uint32_t length2)
{
	HAL_StatusTypeDef answer;
	
  /* Program the user Flash area word by word
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	uint32_t ptr = 0;
	
	Address = FLASH_FW_START_ADDR;

	ptr = 0;
  while ((Address <= FLASH_FW_END_ADDR) && (ptr < length1))
  {
		answer = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address, pBuffer1[ptr]);
    if (answer == HAL_OK)
//    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, DATA_32) == HAL_OK)
    {
      Address = Address + 1;//4;
			ptr++;
    }
    else
    { 
			return answer;
    }
  }
	/* same for pBuffer2
	ptr = 0;
  while ((Address < FLASH_FW_END_ADDR) && (ptr < length2))
  {
		
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address, pBuffer2[ptr]) == HAL_OK)
    {
      Address = Address + 1;
			ptr++;
    }
    else
    { 
      firmware_Error_Handler();
    }
  }
*/
  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock(); 

  /* Check if the programmed data is OK 
      MemoryProgramStatus = 0: data programmed correctly
      MemoryProgramStatus != 0: number of words not programmed correctly ******/
  Address = FLASH_FW_START_ADDR;
  MemoryProgramStatus = 0x0;
  
	ptr = 0;
  while ((Address <= FLASH_FW_END_ADDR) && (ptr < length1))
  {
    data32 = *(__IO uint8_t*)Address;

    if (data32 !=  pBuffer1[ptr])
    {
      MemoryProgramStatus++;  
    }

    Address = Address + 1;//4;
		ptr++;
  }  
	/* same for pBuffer2 
	ptr = 0;
  while ((Address < FLASH_FW_END_ADDR) && (ptr < length2))
  {
    data32 = *(__IO uint32_t*)Address;

    if (data32 !=  pBuffer2[ptr])
    {
      MemoryProgramStatus++;  
    }

    Address = Address + 1;//4;
		ptr++;
  }  
*/
  /* Check if there is an issue to program data */
  if (MemoryProgramStatus == 0)
  {
		return HAL_OK;
    /* No error detected. Switch on LED3 */
  }
  else
  {
		return 0xEE;
  }

	
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;  
  }
  else if((Address < ADDR_FLASH_SECTOR_12) && (Address >= ADDR_FLASH_SECTOR_11))
  {
    sector = FLASH_SECTOR_11;  
  }
  else if((Address < ADDR_FLASH_SECTOR_13) && (Address >= ADDR_FLASH_SECTOR_12))
  {
    sector = FLASH_SECTOR_12;  
  }
  else if((Address < ADDR_FLASH_SECTOR_14) && (Address >= ADDR_FLASH_SECTOR_13))
  {
    sector = FLASH_SECTOR_13;  
  }
  else if((Address < ADDR_FLASH_SECTOR_15) && (Address >= ADDR_FLASH_SECTOR_14))
  {
    sector = FLASH_SECTOR_14;  
  }
  else if((Address < ADDR_FLASH_SECTOR_16) && (Address >= ADDR_FLASH_SECTOR_15))
  {
    sector = FLASH_SECTOR_15;  
  }
  else if((Address < ADDR_FLASH_SECTOR_17) && (Address >= ADDR_FLASH_SECTOR_16))
  {
    sector = FLASH_SECTOR_16;  
  }
  else if((Address < ADDR_FLASH_SECTOR_18) && (Address >= ADDR_FLASH_SECTOR_17))
  {
    sector = FLASH_SECTOR_17;  
  }
  else if((Address < ADDR_FLASH_SECTOR_19) && (Address >= ADDR_FLASH_SECTOR_18))
  {
    sector = FLASH_SECTOR_18;  
  }
  else if((Address < ADDR_FLASH_SECTOR_20) && (Address >= ADDR_FLASH_SECTOR_19))
  {
    sector = FLASH_SECTOR_19;  
  }
  else if((Address < ADDR_FLASH_SECTOR_21) && (Address >= ADDR_FLASH_SECTOR_20))
  {
    sector = FLASH_SECTOR_20;  
  } 
  else if((Address < ADDR_FLASH_SECTOR_22) && (Address >= ADDR_FLASH_SECTOR_21))
  {
    sector = FLASH_SECTOR_21;  
  }
  else if((Address < ADDR_FLASH_SECTOR_23) && (Address >= ADDR_FLASH_SECTOR_22))
  {
    sector = FLASH_SECTOR_22;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_23))*/
  {
    sector = FLASH_SECTOR_23;  
  }

  return sector;
}

/*
static void firmware_Error_Handler(HAL_StatusTypeDef reason)
{
	static HAL_StatusTypeDef last_reason = HAL_OK;
	
	last_reason = reason;
  while(1)
  {
  }
}
*/
