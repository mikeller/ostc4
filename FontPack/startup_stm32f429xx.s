/*****************************************************************************
*** -*- coding: UTF-8 -*-
***
*** \file   Discovery/Src/startup_stm32f429xx.s
*** \brief  STM32F427xx Devices vector table
*** \author Heinrichs Weikamp gmbh
*** \date   15-December-2014
***
*** \details
***         STM32F427xx Devices vector table
***         This module performs:
***             - Set the initial SP
***             - Set the initial PC == Reset_Handler,
***             - Set the vector table entries with the exceptions ISR address
***             - Branches to main in the C library (which eventually
***               calls main()).
***         After Reset the Cortex-M4 processor is in Thread mode,
***         priority is Privileged, and the Stack is set to Main.
***
*** $Id$
******************************************************************************
*** \par Copyright (c) 2014-2018 Heinrichs Weikamp gmbh
***
***     This program is free software: you can redistribute it and/or modify
***     it under the terms of the GNU General Public License as published by
***     the Free Software Foundation, either version 3 of the License, or
***     (at your option) any later version.
***
***     This program is distributed in the hope that it will be useful,
***     but WITHOUT ANY WARRANTY; without even the implied warranty of
***     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
***     GNU General Public License for more details.
***
***     You should have received a copy of the GNU General Public License
***     along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************
*** \par Copyright (c) 2014 STMicroelectronics
***
***     Redistribution and use in source and binary forms, with or without modification,
***     are permitted provided that the following conditions are met:
***       1. Redistributions of source code must retain the above copyright notice,
***          this list of conditions and the following disclaimer.
***       2. Redistributions in binary form must reproduce the above copyright notice,
***          this list of conditions and the following disclaimer in the documentation
***          and/or other materials provided with the distribution.
***       3. Neither the name of STMicroelectronics nor the names of its contributors
***          may be used to endorse or promote products derived from this software
***          without specific prior written permission.
***
***     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
***     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
***     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
***     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
***     FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
***     DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
***     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
***     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
***     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
***     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
.word  _sidata
/* start address for the .data section. defined in linker script */
.word  _sdata
/* end address for the .data section. defined in linker script */
.word  _edata
/* start address for the .bss section. defined in linker script */
.word  _sbss
/* end address for the .bss section. defined in linker script */
.word  _ebss
/* stack used for SystemInit_ExtMemCtl; always internal RAM used */

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/

    .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:
