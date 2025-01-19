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

/*
 .firmware_FirmwareData 0x08050000 : {
 	KEEP( *(.firmware_firmware_data) )
  } >ROM
  
  .font_firmware_data 0x08132000 : {
  	KEEP( *(.font_firmware_data) ) 
  	  
  } >UPPER
  */
  
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

  .upper_fonts 	0x08132040  : {
  	__upper_font_data = .;
  	 . = 0x13E; 
  	 *(.upper_fonts.image_data_FontT144_0x0030)
  	 . = 0x02515; 
  	 *(.upper_fonts.image_data_FontT144_0x0031)
  	 . = 0x03469; 
  	 *(.upper_fonts.image_data_FontT144_0x0032)
  	 . = 0x4FDC; 
  	 *(.upper_fonts.image_data_FontT144_0x0033)
  	 . = 0x6862; 
  	 *(.upper_fonts.image_data_FontT144_0x0034)
  	 . = 0x86C2; 
  	 *(.upper_fonts.image_data_FontT144_0x0035)
  	 . = 0x9FB3; 
  	 *(.upper_fonts.image_data_FontT144_0x0036)
  	 . = 0xBB26; 
  	 *(.upper_fonts.image_data_FontT144_0x0037)
  	 . = 0xD845; 
  	 *(.upper_fonts.image_data_FontT144_0x0038)
  	 . = 0xF34D;
  	 *(.upper_fonts.image_data_FontT144_0x0039)
  	 . = 0x10EC0;
  	 *(.upper_fonts.image_data_FontT24_0x0020)
  	 *(.upper_fonts.image_data_FontT24_0x0021)
  	 *(.upper_fonts.image_data_FontT24_0x0022)
  	 *(.upper_fonts.image_data_FontT24_0x0023)
  	 *(.upper_fonts.image_data_FontT24_0x0024)
  	 *(.upper_fonts.image_data_FontT24_0x0025)
  	 *(.upper_fonts.image_data_FontT24_0x0026)
  	 *(.upper_fonts.image_data_FontT24_0x0027)
  	 *(.upper_fonts.image_data_FontT24_0x0028)
  	 *(.upper_fonts.image_data_FontT24_0x0029)
  	 *(.upper_fonts.image_data_FontT24_0x002a)
  	 *(.upper_fonts.image_data_FontT24_0x003f)
  	 *(.upper_fonts.image_data_FontT24_0x002c)
  	 *(.upper_fonts.image_data_FontT24_0x002d)
  	 *(.upper_fonts.image_data_FontT24_0x002e)
  	 *(.upper_fonts.image_data_FontT24_0x002f)
     *(.upper_fonts.image_data_FontT24_0x0030)
  	 *(.upper_fonts.image_data_FontT24_0x0031)
  	 *(.upper_fonts.image_data_FontT24_0x0032)
  	 *(.upper_fonts.image_data_FontT24_0x0033)
  	 *(.upper_fonts.image_data_FontT24_0x0034)
  	 *(.upper_fonts.image_data_FontT24_0x0035)
  	 *(.upper_fonts.image_data_FontT24_0x0036)
  	 *(.upper_fonts.image_data_FontT24_0x0037)
  	 *(.upper_fonts.image_data_FontT24_0x0038)
  	 *(.upper_fonts.image_data_FontT24_0x0039)
  	 *(.upper_fonts.image_data_FontT24_0x003a)
  	 *(.upper_fonts.image_data_FontT24_0x003b)
  	 *(.upper_fonts.image_data_FontT24_0x003c)
  	 *(.upper_fonts.image_data_FontT24_0x003d)
  	 *(.upper_fonts.image_data_FontT24_0x003e)
  	 . = . + 0x8c;
     *(.upper_fonts.image_data_FontT24_0x0040)
  	 *(.upper_fonts.image_data_FontT24_0x0041)
  	 *(.upper_fonts.image_data_FontT24_0x0042)
  	 *(.upper_fonts.image_data_FontT24_0x0043)
  	 *(.upper_fonts.image_data_FontT24_0x0044)
  	 *(.upper_fonts.image_data_FontT24_0x0045)
  	 *(.upper_fonts.image_data_FontT24_0x0046)
  	 *(.upper_fonts.image_data_FontT24_0x0047)
  	 *(.upper_fonts.image_data_FontT24_0x0048)
  	 *(.upper_fonts.image_data_FontT24_0x0049)
  	 *(.upper_fonts.image_data_FontT24_0x004a)
  	 *(.upper_fonts.image_data_FontT24_0x004b)
  	 *(.upper_fonts.image_data_FontT24_0x004c)
  	 *(.upper_fonts.image_data_FontT24_0x004d)
  	 *(.upper_fonts.image_data_FontT24_0x004e)  	
  	 *(.upper_fonts.image_data_FontT24_0x004f)   
     *(.upper_fonts.image_data_FontT24_0x0050)
  	 *(.upper_fonts.image_data_FontT24_0x0051)
  	 *(.upper_fonts.image_data_FontT24_0x0052)
  	 *(.upper_fonts.image_data_FontT24_0x0053)
  	 *(.upper_fonts.image_data_FontT24_0x0054)
  	 *(.upper_fonts.image_data_FontT24_0x0055)
  	 *(.upper_fonts.image_data_FontT24_0x0056)
  	 *(.upper_fonts.image_data_FontT24_0x0057)
  	 *(.upper_fonts.image_data_FontT24_0x0058)
  	 *(.upper_fonts.image_data_FontT24_0x0059)
  	 *(.upper_fonts.image_data_FontT24_0x005a)
  	 *(.upper_fonts.image_data_FontT24_0x005b)
  	 *(.upper_fonts.image_data_FontT24_0x005c)
  	 *(.upper_fonts.image_data_FontT24_0x005d)
  	 *(.upper_fonts.image_data_FontT24_0x005e)  	
  	 *(.upper_fonts.image_data_FontT24_0x005f)    
  	 *(.upper_fonts.image_data_FontT24_0x0060)
  	 *(.upper_fonts.image_data_FontT24_0x0061)
  	 *(.upper_fonts.image_data_FontT24_0x0062)
  	 *(.upper_fonts.image_data_FontT24_0x0063)
  	 . = . + 0x1c;
  	 *(.upper_fonts.image_data_FontT24_0x0064)
  	 *(.upper_fonts.image_data_FontT24_0x0065)
  	 *(.upper_fonts.image_data_FontT24_0x0066)
  	 *(.upper_fonts.image_data_FontT24_0x0067)
  	 *(.upper_fonts.image_data_FontT24_0x0068)
  	 *(.upper_fonts.image_data_FontT24_0x0069)
  	 *(.upper_fonts.image_data_FontT24_0x006a)
  	 *(.upper_fonts.image_data_FontT24_0x006b)
  	 *(.upper_fonts.image_data_FontT24_0x006c)
  	 *(.upper_fonts.image_data_FontT24_0x006d)
  	 *(.upper_fonts.image_data_FontT24_0x006e)  	
  	 *(.upper_fonts.image_data_FontT24_0x006f)  	 
  	 *(.upper_fonts.image_data_FontT24_0x0070)
  	 *(.upper_fonts.image_data_FontT24_0x0071)
  	 *(.upper_fonts.image_data_FontT24_0x0072)
  	 *(.upper_fonts.image_data_FontT24_0x0073)
  	 *(.upper_fonts.image_data_FontT24_0x0074)
  	 *(.upper_fonts.image_data_FontT24_0x0075)
  	 *(.upper_fonts.image_data_FontT24_0x0076)
  	 *(.upper_fonts.image_data_FontT24_0x0077)
  	 *(.upper_fonts.image_data_FontT24_0x0078)
  	 *(.upper_fonts.image_data_FontT24_0x0079)
  	 *(.upper_fonts.image_data_FontT24_0x007a)
  	 *(.upper_fonts.image_data_FontT24_0x007b)
  	 *(.upper_fonts.image_data_FontT24_0x007c)
  	 . = . + 0x1c;
  	 *(.upper_fonts.image_data_FontT24_0x007d)
  	 *(.upper_fonts.image_data_FontT24_0x007e)  	
  	 *(.upper_fonts.image_data_FontT24_0x00a1)
  	 *(.upper_fonts.image_data_FontT24_0x00bf)    
  	 *(.upper_fonts.image_data_FontT24_0x00c1)
  	 *(.upper_fonts.image_data_FontT24_0x00c4)  
  	 *(.upper_fonts.image_data_FontT24_0x00c6) 
  	 *(.upper_fonts.image_data_FontT24_0x00c7)
  	 *(.upper_fonts.image_data_FontT24_0x00c9)
  	 *(.upper_fonts.image_data_FontT24_0x00cd)
  	 *(.upper_fonts.image_data_FontT24_0x00d1)
  	 *(.upper_fonts.image_data_FontT24_0x00d3)
  	 *(.upper_fonts.image_data_FontT24_0x00d6)
  	 *(.upper_fonts.image_data_FontT24_0x00da)
  	 *(.upper_fonts.image_data_FontT24_0x00dc)
  	 *(.upper_fonts.image_data_FontT24_0x00df) 
  	 *(.upper_fonts.image_data_FontT24_0x00e0)
  	 *(.upper_fonts.image_data_FontT24_0x00e1)
  	 *(.upper_fonts.image_data_FontT24_0x00e2)
  	 *(.upper_fonts.image_data_FontT24_0x00e4)
  	 *(.upper_fonts.image_data_FontT24_0x00e6)
  	 *(.upper_fonts.image_data_FontT24_0x00e7)
  	 *(.upper_fonts.image_data_FontT24_0x00e8)
  	 *(.upper_fonts.image_data_FontT24_0x00e9)         
  	 *(.upper_fonts.image_data_FontT24_0x00ec)
  	 *(.upper_fonts.image_data_FontT24_0x00ed)
  	 *(.upper_fonts.image_data_FontT24_0x00f0)
  	 *(.upper_fonts.image_data_FontT24_0x00f1)
  	 *(.upper_fonts.image_data_FontT24_0x00f2)
  	 *(.upper_fonts.image_data_FontT24_0x00f3)
  	 *(.upper_fonts.image_data_FontT24_0x00f6)
  	 *(.upper_fonts.image_data_FontT24_0x00f9)
  	 *(.upper_fonts.image_data_FontT24_0x00fa)
  	 *(.upper_fonts.image_data_FontT24_0x00fc)         
  	 *(.upper_fonts.image_data_FontT24_0x00ff)
  	 *(.upper_fonts.image_data_FontT24_0x0152)
  	 *(.upper_fonts.image_data_FontT24_0x0153)
  	 *(.upper_fonts.image_data_FontT24_0x0178)
  	 *(.upper_fonts.image_data_FontT24_0x002b)
  	 . =  . + 0x1d642;
  	  *(.upper_images.image_data_ostc_fuer_Tauchcomputer_240px)
	 *(.upper_fonts.image_data_FontT84_0x002e)
	  . =  . + 0x9f;
	 *(.upper_fonts.image_data_FontT84_0x0030)
	  . =  . + 0x0d;
	 *(.upper_fonts.image_data_FontT84_0x0031)
	  . =  . + 0xaa;
	 *(.upper_fonts.image_data_FontT84_0x0032)
	  . =  . + 0x7b;
	 *(.upper_fonts.image_data_FontT84_0x0033)
	  . =  . + 0x38;
	 *(.upper_fonts.image_data_FontT84_0x0034)
	  . =  . + 0x7f;
	 *(.upper_fonts.image_data_FontT84_0x0035)
	  . =  . + 0x77;
	 *(.upper_fonts.image_data_FontT84_0x0036)
	  . =  . + 0x7b;
	 *(.upper_fonts.image_data_FontT84_0x0037)
	 . =  . + 0x0d;
	 *(.upper_fonts.image_data_FontT84_0x0038)
	 . =  . + 0x2b;
	 *(.upper_fonts.image_data_FontT84_0x0039)
	 . =  . + 0xe8;
  	 *(.upper_fonts.image_data_FontT54_0x0022)
  	 *(.upper_fonts.image_data_FontT54_0x0025)
  	 . =  . + 0x112;
  	 *(.upper_fonts.image_data_FontT54_0x0027)
  	 . =  . + 0xd8;
  	 *(.upper_fonts.image_data_FontT54_0x002a)
  	 . =  . + 0x133;
     *(.upper_fonts.image_data_FontT54_0x002d)
  	 . =  . + 0x1d7;
  	 *(.upper_fonts.image_data_FontT54_0x002e)
  	 . =  . + 0x17b;
  	 *(.upper_fonts.image_data_FontT54_0x002f)
  	 . =  . + 0x02;
  	 *(.upper_fonts.image_data_FontT54_0x0030)
  	 . =  . + 0x0ba;
  	 *(.upper_fonts.image_data_FontT54_0x0031)
  	 . =  . + 0x27d;
  	 *(.upper_fonts.image_data_FontT54_0x0032)
  	 . =  . + 0xe3;
  	 *(.upper_fonts.image_data_FontT54_0x0033)
  	 . =  . + 0x35;
  	 *(.upper_fonts.image_data_FontT54_0x0034)
  	 . =  . + 0x68;
  	 *(.upper_fonts.image_data_FontT54_0x0035)
  	 . =  . + 0x35;
  	 *(.upper_fonts.image_data_FontT54_0x0036)
  	 . =  . + 0xba;
  	 *(.upper_fonts.image_data_FontT54_0x0037)
  	 . =  . + 0x191;
  	 *(.upper_fonts.image_data_FontT54_0x0038)
  	 . =  . + 0xe3;
  	 *(.upper_fonts.image_data_FontT54_0x0039)
  	 . =  . + 0xe3;
  	 *(.upper_fonts.image_data_FontT54_0x003a)
  	 . =  . + 0x7b;
  	 *(.upper_fonts.image_data_FontT54_0x0043)
  	 . =  . + 0x3b;
  	 *(.upper_fonts.image_data_FontT54_0x006c)
  	 . =  . + 0x2d;
  	 *(.upper_fonts.image_data_FontT54_0x006d)
  	 . =  . + 0x94;
  	 *(.upper_fonts.image_data_FontT54_0x00ba)
  	 . =  . + 0x0a;
  	 
  	 *(.upper_fonts.image_data_FontT105_0x002*)
  	 *(.upper_fonts.image_data_FontT105_0x003*)
  	 *(.upper_fonts.image_data_FontT105_0x0043)
  	 . =  . + 0x17c8;
  	 *(.upper_fonts.image_data_FontT105_0x007c)
  	 *(.upper_fonts.image_data_FontT105_0x00ba)
  	 *(.upper_fonts.image_data_FontT105_54_0x006d)
  	 
	 *(.upper_fonts.image_data_FontT54_0x0068)
	 . =  . + 0x1195;
	 
	 *(.upper_fonts.image_data_FontT42_0x00b0)
	 *(.upper_fonts.image_data_FontT42_0x0020)
	  . =  . + 0x23;
	 *(.upper_fonts.image_data_FontT42_0x002*)
	 *(.upper_fonts.image_data_FontT42_0x003*)
	 *(.upper_fonts.image_data_FontT42_0x004*)
	 *(.upper_fonts.image_data_FontT42_0x005*)
	 . =  . + 0x420;
	 *(.upper_fonts.image_data_FontT42_0x006*)
	 *(.upper_fonts.image_data_FontT42_0x0070)
	 *(.upper_fonts.image_data_FontT42_0x0071)
	 *(.upper_fonts.image_data_FontT42_0x0072)
	 *(.upper_fonts.image_data_FontT42_0x0073)
	 *(.upper_fonts.image_data_FontT42_0x0074)
	 *(.upper_fonts.image_data_FontT42_0x0075)
	 *(.upper_fonts.image_data_FontT42_0x0076)
	 *(.upper_fonts.image_data_FontT42_0x0077)
	 *(.upper_fonts.image_data_FontT42_0x0078)
	 *(.upper_fonts.image_data_FontT42_0x0079)
	 . =  . + 0x12c0;
  	 *(.upper_fonts.image_data_FontT42_*)
  	 *(.upper_fonts.image_data_T54addon_0x002b_PLUS*)
  	 *(.upper_fonts.image_data_T54addon_0x0040_AT*)
  	 *(.upper_fonts.image_data_FontT48addon*)
  	 *(.upper_fonts.image_data_FontT24plus_0x00fb*)
  	 *(.upper_fonts.image_data_FontT24plus_0x00fd*)
  	 *(.upper_fonts.image_data_FontT48_*) 	 
  	 *(.upper_fonts.image_data_awe48_*)
  
/* moving of fonts from lower to upper */  	 
  	 
	 *(.upper_fonts.*) 
	 
  	__upper_font_data_end = .;
  } >UPPER

 

  .upper_directory 0x081DEF00 : {
      __font_directory = .;
	*(.upper_font_directory.Awe48)
	*(.upper_font_directory.FontT24)
	*(.upper_font_directory.FontT42)
	*(.upper_font_directory.FontT48)
	*(.upper_font_directory.FontT54)
	*(.upper_font_directory.FontT84)
	*(.upper_font_directory.FontT105)
	*(.upper_font_directory.FontT144)
	*(.upper_font_directory.Batt24*)
		
	__font_directory_end = .;
  	LONG( -1 );	/* Magic End Marker */

  } >UPPER  




  
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
