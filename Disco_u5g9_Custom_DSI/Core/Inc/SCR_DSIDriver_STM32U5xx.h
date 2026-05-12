/*
 *  Created on: Feb 10, 2026
 *  Author: BalazsFarkas
 *  Project: STM32_DSI
 *  Processor: STM32U5G9
 *  Program version: 1.0
 *  Header file: SCR_DSIDriver_STM32U5xx.h
 *  Change history:
 */

#ifndef INC_SCR_DSIDRIVER_STM32U5XX_H_
#define INC_SCR_DSIDRIVER_STM32U5XX_H_

#include <stdint.h>
#include "stm32u5xx.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE

//FUNCTION PROTOTYPES

void DSI_config(void);
void DSI_single_Tx(uint32_t cmd, uint32_t parameter);
void DSI_multi_Tx(uint32_t DCS_code, uint32_t len_param_array, uint8_t *param_array);

#endif /* INC_SCR_DSIDRIVER_STM32U5XX_H_ */
