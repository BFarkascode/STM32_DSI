/*
 *  Created on: Feb 10, 2026
 *  Author: BalazsFarkas
 *  Project: STM32_DSI
 *  Processor: STM32U5G9
 *  Program version: 1.0
 *  Source file: SCR_LTDCDriver_STM32U5xx.c
 *  Change history:
 */

#include "SCR_LTDCDriver_STM32U5xx.h"

void LTDC_U5_Config(uint32_t frame_buf_address){

	/*
	 * Configure the window (frame plus porches) and the layer we intend to feed into the window
	 */

	RCC->APB2ENR |= (1<<26);									//enable LTDC clocking
	RCC->CCIPR2 &= ~(1<<18);									//PPL3 as PLL source

	//LTDC config
	LTDC->SSCR = 0b10000000000000000;							//horizontal synch height 1, vertical synch height 0 (these are 1 below the actual value
	LTDC->BPCR = 0b100000000000001100;							//horizontal back porch 2, vertical back porch 12
	LTDC->AWCR = 0b1111000100000000111101101;					//accumulated active width 482, active height 493
	LTDC->TWCR = 0b1111000110000001000011111;					//total width in clock periods 483 , total height in clocks 543
	LTDC->IER  = 0b110;											//FIFO underrun and Transfer error IRS enabled
	LTDC->LIPCR = 0b0;											//line interrupt position (we aren't using line interruptr in the ISR)

	//layer config
	LTDC_Layer1->CR    = 0b1;									//layer enable
	LTDC_Layer1->WHPCR = 0b1111000100000000000000011;			//horizontal stop (482) and start (3) positions
	LTDC_Layer1->WVPCR = 0b1111011010000000000001101;			//vertical stop (493) and start (13) positions
	LTDC_Layer1->PFCR  = 0b0;									//ARGB888 for the layer input
	LTDC_Layer1->CFBAR = frame_buf_address;						//address of the frame buffer
	LTDC_Layer1->CFBLR = 0b111100000000000011110000011;			//pixel pitch (780) and line length (783)
	LTDC_Layer1->CFBLNR = 0b111100001;							//how many lines we have in the layer frame buffer - 481
	LTDC_Layer1->BFCR = 0b10000000101;							//blending factor constant alpha, constant alpha for the layer
	LTDC_Layer1->CACR = 0b11111111;								//constant alpha for the layer

	LTDC->SRCR = 0x1;											//update LTDC parameters
																	//Note: this bit is cleared once the update is done
	LTDC->GCR  = 0b11000000000000000010001000100000;			//HSPOL active HIGH, VSPOL active HIGH,LTDc disabled (rest is read only)
	LTDC->GCR |= (1<<0);										//LTDC enabled
}
