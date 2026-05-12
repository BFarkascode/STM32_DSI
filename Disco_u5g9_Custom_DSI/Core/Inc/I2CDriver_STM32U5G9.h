/*
 *  Created on: Feb 26, 2026
 *  Author: BalazsFarkas
 *  Project: STM32_DSI
 *  Processor: STM32U5G9
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Header version: 1.0
 *  File: I2CDriver_STM32U5G9.h
 *  Change history: N/A
 */

#ifndef INC_I2CDRIVER_STM32U5G9_H_
#define INC_I2CDRIVER_STM32U5G9_H_

#include "stdint.h"
#include "stm32u5xx.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE

//FUNCTION PROTOTYPES
void I2C5_Config(void);
void I2C5_TX (uint8_t slave_addr, uint8_t number_of_bytes, uint8_t *bytes_to_send);
void I2C5_RX (uint8_t slave_addr, uint8_t number_of_bytes, uint8_t* bytes_received);

#endif /* INC_I2CDRIVER_STM32U5G9_H_ */
