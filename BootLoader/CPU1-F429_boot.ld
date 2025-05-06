/*****************************************************************************
 * -*- coding: UTF-8 -*-
 *
 * \file   Common/CPU1-F429.ld
 * \brief  Linker Script for CPU1 alias Discovery
 * \author Ac6 workbench, Heinrichs Weikamp, and JD Gascuel
 * \date   2018
 *
 * \details
 *  Linker script for STM32F429IIT6 Device from STM32F4 series
 *  - 192Kbytes RAM
 *  - 2048Kbytes ROM
 *
 *  Set heap size, stack size and stack location according
 *  to application requirements.
 *
 *  Set memory bank area and size if external memory is used.
 *
 * $Id$
 *****************************************************************************
 * \par Copyright (c) 2014-2018 Heinrichs Weikamp gmbh
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************
 * HISTORY
 *  2017-12-18 jDG: Mix between std AC6 file and chw stm32f429zi_flash.ld settings.
 */

/* Entry Point */
ENTRY(Reset_Handler)

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/****************************************************************************/
/***************************** Memory Definition ****************************/
MEMORY
{
  ROM_BOOT  (rx) : ORIGIN = 0x08000000, LENGTH = 0x00040000 /* 256K */
    ROM  (rx) : ORIGIN = 0x08040000, LENGTH = 0x00090000 /* 576K */
    UPPER(rx) : ORIGIN = 0x08132000, LENGTH = 0xAE270 /* 713K */
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 192K
  CCRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* Linker file shall be shared between Bootloader, Font and Firmware projects. Not used memory areas to be cut during bin generation */

/* Make sure ROM and UPPER are contiguous, and map 2MB */
/* TODO: At the moment the size of the defined memory will be directly reflected into the file size of resultion bin => Keep small until not used memory may be excluded from build */
/*ASSERT( ORIGIN(UPPER) - ORIGIN(ROM) == LENGTH(ROM), "ROM lower and upper region shall be continuous" )
/*ASSERT( LENGTH(ROM) + LENGTH(UPPER) == 2048K, "ROM lower + upper regions = 2MB" ) */

/* Highest address of the user mode stack */
/*_estack = ORIGIN(RAM) + LENGTH(RAM);*/


_Min_Heap_Size  = 0;      	/* no required heap  (malloc'ed space) */
_Min_Stack_Size = 0x08000; 	/* required 32K of stack */

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 * 
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *	 _sbss
 *   __bss_start__
 *	 _ebss
 *   __bss_end__
 *   _edata
 *   end
 *   _end
 *   _estack
 */

/****************************************************************************/
/*************************** Code and Data Sections *************************/
SECTIONS
{
  /******************* NVIC reset and interupt vectors **********************/
 
  .isr_vector 0x08000000 :
  {
  . = ALIGN(4);
    KEEP( *(.isr_vector) ) 
    KEEP(*(.init))
  } >ROM_BOOT


 .StdLibs 0x08000200:
  {
  . = ALIGN(4);
     *libm.a:*(*)
  } >ROM_BOOT

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >ROM_BOOT

 /********************** Constant data into ROM memory *********************/


  .ARM.extab   : { 
  	. = ALIGN(4);
  	*(.ARM.extab* .gnu.linkonce.armextab.*)
  	. = ALIGN(4);
  } >ROM_BOOT
  
  .ARM : {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >ROM_BOOT
  
   .preinit_array     :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN( __preinit_array_start = . );
    KEEP( *(.preinit_array*) )
    PROVIDE_HIDDEN( __preinit_array_end = . );
    . = ALIGN(4);
  } >ROM_BOOT
  
  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN( __init_array_start = . );
    KEEP( *(SORT(.init_array.*)) )
    KEEP( *(.init_array*) )
    PROVIDE_HIDDEN( __init_array_end = . );
    . = ALIGN(4);
  } >ROM_BOOT
  
  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN( __fini_array_start = . );
    KEEP( *(SORT(.fini_array.*)) )
    KEEP( *(.fini_array*) )
    PROVIDE_HIDDEN( __fini_array_end = . );
    . = ALIGN(4);
  } >ROM_BOOT
  
    /************* The program code and other data into ROM memory ************/

  
    /************* Data and Structures at absolute address in ROM *************/
 
 /* FIXME: this sets the SFirmwareData structure, not the code ... */
 .bootloader_firmware_data 0x0800A000 : {
 	PROVIDE( bootloader_FirmwareData = . );
 	KEEP( *(.bootloader_firmware_data) )
 } > ROM_BOOT
 
  .bootloader_hardware_data 0x0800A040 : {
  	PROVIDE( bootloader_HardwareData = . );
 	KEEP( *(.bootloader_hardware_data) )
 } > ROM_BOOT
 
  .text 0x08001000 :
  {
    . = ALIGN(4);
    *(.text.HAL*)
    . = ALIGN(4);
    *(.text*sprint*)
    *(.text.MX*)
  } >ROM_BOOT
 
 
  .text2 0x0800A100 :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)
    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >ROM_BOOT
 

 	
    /* Define Known Address for Each Font */
    /* Flash Sector 23 is protected (bootloader font + image) => use end of sector 22 */

.lower_fonts 	0x08016000  : {
	 *(.lower_fonts.image_data_*)
	 *(.lower_fonts.*)
	 *(.upper_fonts.image_data_FontT48*) 
	 *(.upper_fonts.FontT48*)
	 *(.upper_fonts.image_data_FontT24*) 
	 *(.upper_fonts.FontT24*)
} > ROM_BOOT

.lower_directory 0x08004300 : {
	*(.lower_font_directory*)
	*(.upper_font_directory.FontT48*)
	*(.upper_font_directory.FontT24*)
} > ROM_BOOT

.lower_image	0x08009000  : {
	 *(.lower_image*)
	  
} > ROM_BOOT

.lower_directory 0x08009500 : {
	*(.lower_image_directory*)
} > ROM_BOOT
  
  /* Used by the startup to initialize data */
_sidata = LOADADDR(.data);

  /************ Initialized Data Sections into RAM & ROM Memory **************/
  .data : 
  {
    . = ALIGN(4);
    
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
	*(vtable)

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT>ROM_BOOT
  
 

  /*************** Uninitialized Data Section into RAM Memory ****************/
  .bss :
  {
	. = ALIGN(4);
    
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

 /********************** User_heap_stack section ****************************/
  /* just to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    PROVIDE ( __end__ = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
    _estack = .;
  } >RAM

  /* Remove information from the compiler libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
