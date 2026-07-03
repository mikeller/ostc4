///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/data_exchange_spi_real.c
/// \brief  Production dispatch impl: real SPI-DMA transfer to CPU2.
/// \author heinrichs weikamp gmbh
///
/// One half of the link-time mock-or-real dispatch for data_exchange.
/// data_exchange_main.c calls data_exchange_perform_spi_dispatch()
/// unconditionally; production builds link this file (real DMA), Renode
/// sim builds link cpu2_mock.c instead. Result: data_exchange_main.c is
/// bit-identical between production and RENODE_BUILD.
///
/// Design rationale: see
/// docs/superpowers/specs/2026-05-06-mock-mode-decision-design.md §5.1.
/// Mirrors the Small_CPU/Src/device_time_hooks.c relocation pattern from
/// foundation-A.
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2026 Heinrichs Weikamp gmbh
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

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "ostc.h"
#include "data_exchange.h"
#include "data_exchange_main.h"

static uint8_t count_DataEX_Error_Handler = 0;
static uint8_t last_error_DataEX_Error_Handler = 0;

static void DataEX_Error_Handler(uint8_t answer)
{
	count_DataEX_Error_Handler++;
	last_error_DataEX_Error_Handler = answer;

	/* A wrong footer indicates a communication interrupt. State machine is waiting for new data which is not received because no new transmission is triggered */
	/* ==> Abort data exchange to enable a new RX / TX cycle */
	if(answer == HAL_BUSY)
	{
		HAL_SPI_Abort_IT(&cpu2DmaSpi);
	}

	return;
}

void data_exchange_perform_spi_dispatch(SDataReceiveFromMaster *out, SDataExchangeSlaveToMaster *in)
{
	HAL_GPIO_WritePin(SMALLCPU_CSB_GPIO_PORT, SMALLCPU_CSB_PIN, GPIO_PIN_RESET);

	HAL_StatusTypeDef ans = HAL_SPI_TransmitReceive_DMA(&cpu2DmaSpi, (uint8_t *)out, (uint8_t *)in, EXCHANGE_BUFFERSIZE);
	if(ans != HAL_OK)
	{
		DataEX_Error_Handler(ans);
	}
}
