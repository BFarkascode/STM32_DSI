/*
 *  Created on: Feb 10, 2026
 *  Author: BalazsFarkas
 *  Project: STM32_DSI
 *  Processor: STM32U5G9
 *  Program version: 1.0
 *  Header file: SCR_screen_config_w_DSI.h
 *  Change history:
 */

#ifndef INC_SCR_SCREEN_CONFIG_W_DSI_H_
#define INC_SCR_SCREEN_CONFIG_W_DSI_H_

#include <stdint.h>
#include "stm32u5xx.h"
#include "main.h"
#include "SCR_DSIDriver_STM32U5xx.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE

//FUNCTION PROTOTYPES

uint32_t Config_U5Disco_screen(void);
void CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize);
void Copy_part_of_source(uint32_t *pSrc, uint32_t *pDst, uint16_t x_pos_in_source, uint16_t y_pos_in_source, uint16_t x_pos_in_dest, uint16_t y_pos_in_dest, uint16_t xsize, uint16_t ysize, uint32_t source_x_res, uint32_t dest_x_res);
int32_t LCD_FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color);

#endif /* INC_SCR_SCREEN_CONFIG_W_DSI_H_ */
