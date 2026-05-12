/*
 *  Created on: Feb 10, 2026
 *  Author: BalazsFarkas
 *  Project: STM32_DSI
 *  Processor: STM32U5G9
 *  Program version: 1.0
 *  Source file: SCR_DSIDriver_STM32U5xx.c
 *  Change history:
 */

#include "SCR_DSIDriver_STM32U5xx.h"


void DSI_config(void){

/*
 * We are calibrating the DSI below
 * We follow refman page 1792 with the exception of step 10 which is moved later
 * */

	//we are going through the refman page 1792
	//note: clocking should be done here since if we use HAL drivers, they might reset the PLL config to 0x0

	//DSI PHY PLL clocking and DSI enable - 2
	RCC->CCIPR2 &= ~(1<<15);													//PLL3P as DSI clock
	RCC->APB2ENR |= (1<<27);													//enable DSI clocking
//	RCC->CCIPR2 |= (1<<15);														//PLL DSI PHY as DSI clock

	//steps 3,4 and 5 are optional

	//we turn on the DSI bias - 6
	DSI->BCFGR |= (1<<6);

	Delay_ms(2);																//needs 2 ms delay according to HAL

	//DSI PLL clock setup - 7
	DSI->WRPCR = 0x0;
	DSI->WRPCR |= (125<<2);														//NDIV 125
	DSI->WRPCR |= (2<<20);														//ODF is 2
	DSI->WRPCR |= (4<<11);														//IDF is 4
	DSI->WRPCR &= ~(1<<29);														//band control 500 to 800 MHz
	DSI->WRPCR |= (1<<0);														//PLL DSI PHY enabled

	Delay_ms(2);

	while ((DSI->WISR & (1<<9)) != (1<<9));										//we wait for the DSI PLL to activate

	//enable DSI - 8
	DSI->CR |= (1<<0);															//enable DSI

	//internal clock config - 9
	DSI->CCR = 0;
	DSI->CCR |= (1<<8);															//timeout clock division
	DSI->CCR |= (4<<0);															//TX escape clock division (needed to configure the DSI PHY)

	//despite what is written in page 1792, we actually need to go with a different sequence to calibrate the DSI here

	//enable DSI PHY EN - 11
	DSI->PCTLR |= (1<<1);

	//set skew rates and band control (44.14.2) - 12
	DSI->DPCBCR = 64;						//clock band control 450 MHz - 510 MHz
	DSI->DPCSRCR = 14;						//clock skew rate
	DSI->DPDL0HSOCR = 32;					//D0 HS offset
	DSI->DPDL0LPXOCR = 0;					//D0 LP offset
	DSI->DPDL0BCR = 8;						//D0 band control
	DSI->DPDL0SRCR = 14;					//D0 skew rate
	DSI->DPDL1HSOCR = 32;					//D1 HS offset
	DSI->DPDL1LPXOCR = 0;					//D1 LP offset
	DSI->DPDL1BCR = 8;						//D1 band control
	DSI->DPDL1SRCR = 14;					//D1 skew rate

	//enable DSI PHY clock - 13
	DSI->PCTLR |= (1<<2);

	//configure DSI PHY (section 44.14.2) - 10
	//this section must be done later compared to what the refman suggests, otherwise the clocking is not set
	DSI->PCONFR	|= (1<<0);					//number of data lines is 2
	DSI->PCONFR	|= (7<<8);					//stop wait time is 7
	DSI->CLTCR |= (40<<16);					//clock HS2LP transit time is 40
	DSI->CLTCR |= (40<<0);					//clock LP2HS transit time is 40
	DSI->DLTCR = 0;
	DSI->DLTCR |= (12<<16);					//data HS2LP transit time is 12
	DSI->DLTCR |= (23<<0);					//data LP2HS transit time is 23

	//clock lane config
	DSI->CLCR |= (1<<0);														//HS
	DSI->CLCR |= (1<<1);														//yes auto

	//check for DSI PHY to be in stop state - 15

	while ((DSI->PSR & (1<<2)) != (1<<2));			//clock lane stop
	while ((DSI->PSR & (1<<4)) != (1<<4));			//D0 lane stop
	while ((DSI->PSR & (1<<7)) != (1<<7));			//D1 lane stop


/*	//enable DSI PHY EN - 11
	DSI->PCTLR |= (1<<1);

	//set slew rates and band control (44.14.2) - 12
	DSI->DPCBCR = 64;						//clock band control 450 MHz - 510 MHz
	DSI->DPCSRCR = 14;						//clock skew rate
	DSI->DPDL0HSOCR = 32;					//D0 HS offset
	DSI->DPDL0LPXOCR = 0;					//D0 LP offset
	DSI->DPDL0BCR = 8;						//D0 band control
	DSI->DPDL0SRCR = 14;					//D0 skew rate
	DSI->DPDL1HSOCR = 32;					//D1 HS offset
	DSI->DPDL1LPXOCR = 0;					//D1 LP offset
	DSI->DPDL1BCR = 8;						//D1 band control
	DSI->DPDL1SRCR = 14;					//D1 skew rate

	//enable DSI PHY clock and EN - 13
	DSI->PCTLR |= (1<<2);*/

	//switch to DSI PHY PLL - 14
	RCC->CCIPR2 |= (1<<15);														//???

	//check for DSI PHY to be in stop state - 15
//	while ((DSI->PSR & (1<<2)) != (1<<2));			//clock lane stop
//	while ((DSI->PSR & (1<<4)) != (1<<4));			//D0 lane stop
//	while ((DSI->PSR & (1<<7)) != (1<<7));			//D1 lane stop

	//disable DSI Host - 16
	DSI->CR &= ~(1<<0);															//disable DSI

	//clock lane config high speed, no automatic control - 17
	DSI->CLCR |= (1<<0);														//HS
	DSI->CLCR &= ~(1<<1);														//no auto

	//DSI Host timing config (44.14.3) - 18
	//set the timeouts, if used (here all 0)
	DSI->TCCR[0] = 0x0;
	DSI->TCCR[1] = 0x0;
	DSI->TCCR[2] = 0x0;
	DSI->TCCR[3] = 0x0;
	DSI->TCCR[4] = 0x0;
	DSI->TCCR[5] = 0x0;

	//DSI Host flow control and DBI (44.14.4) - 19
	DSI->PCR |= (1<<2);												//BTA bus turn around enabled
	DSI->GVCIDR = 0;												//no virtual channels
	DSI->CMCR = 0;													//no command ACK or tearing effect

	//DSI Host LTDC interface(44.14.5) - 20
	DSI->LCVCIDR = 0;												//no virtual channels
	DSI->LCOLCR |= (5<<0);											//24 bit
	DSI->WCFGR |= (5<<1);											//COLMUX 24-bit
	DSI->LPCR = 0;													//active HIGH DATA ENABLE, HSYCNH and VSYCH

	//DSI Host video mode (44.14.6) - 21
	DSI->MCR = 0;													//set video mode
	DSI->VMCR |= (1<<1);											//burst mode
	DSI->VMCR |= (1<<8);											//LP VSYNCH enabled
	DSI->VMCR |= (1<<9);											//LP vertical back porch enabled
	DSI->VMCR |= (1<<10);											//LP vertical front porch enabled
	DSI->VMCR |= (1<<11);											//LP vertical active enabled
	DSI->VMCR |= (1<<12);											//LP horizontal back porch enabled
	DSI->VMCR |= (1<<13);											//LP horizontal front porch enabled
	DSI->VMCR |= (1<<14);											//BTA enabled
	DSI->VMCR &= ~(1<<16);											//pattern generator disabled
	DSI->VPCR = 480;												//video packet size
	DSI->VCCR = 0;													//chunk number - if 0 or 1, the video line is bursted
	DSI->VNPCR = 0;													//null packet size
	DSI->VLCR = 1452;												//line duration
	DSI->VHSACR = 6;												//hsychn duration
	DSI->VHBPCR = 3;												//horizontal back proch
	DSI->VVSACR = 1;												//vysnch duration
	DSI->VVBPCR = 12;												//vertical back porch
	DSI->VVFPCR = 50;												//vertical front porch
	DSI->VVACR = 481;												//vertical active duration
//	DSI->GHCR |= (5<<0);											//packet data type???
//	DSI->GHCR |= (41<<8);											//word count LSB???

	//reset display - 22

	//start DSI - 23
//	DSI->WCR |= (1<<3);															//DSI wrapper enable
//	DSI->CR |= (1<<0);															//enable DSI

	//enable LTDC - 25
	//can be done later, LTDC will be master anyway
	LTDC->GCR |= (1<<0);

	//start LTDC flow through the DSI wrapper - 26
	DSI->WCR |= (1<<2);

}


void DSI_single_Tx(uint32_t cmd, uint32_t parameter){

	/*
	 * We are setting up the DSI packet header here
	 * For a short write DCS with 1 parameter, that header mode and type will be 0x15. I assume we know we need that from the screen's datasheet (which we don't have)
	 * We wait until the write FIFO is empty in the DSI and then put the new header on the bus
	 * Note: the "while" loop could be improved by adding a timeout to it.
	 *
	 */

	//we wait until the Write FIFO becomes empty
	while ((DSI->GPSR & (1<<0)) != (1<<0));

	//we update the DSI packet header
	DSI->GHCR = (0x15 | (0 << 6U) | (cmd << 8U) | (parameter << 16U));
							//DT header data type is 0x15 for CMD + 1 parameter, see "DSI_SHORT_WRITE_PKT_Data_Type"
							//virtual channel ID is 0
							//WCLSB - LSB word, so the first word/4-byte to be sent over the DSI - is the cmd
							//WCMSB - MSB word, so the second word/4-byte to be sent over the DSI - is the parameter

}


void DSI_multi_Tx(uint32_t DCS_code, uint32_t len_param_array, uint8_t *param_array){

	/*
	 * We are setting up the DSI packet header here
	 * For a long write DCS, that header mode and type will be 0x39.
	 * virtual channel is still 0
	 * the parameter is replaced by the number of parameters (+ 1 in the header constructor)
	 * We wait until the write FIFO is empty in the DSI and then put the new header on the bus
	 * Note: the "while" loop could be improved by adding a timeout to it.
	 *
	 */

	uint8_t *param_array_byte_ptr = param_array;		//we define a secondary 8-bit pointer to move along the 8-bit parameter array

	//we wait until the Write FIFO becomes empty
	while ((DSI->GPSR & (1<<0)) != (1<<0));

	//if we have less than 3 bytes to send over after the DCS_code, we round it up to 3 so we will have a full word fifo to send
	uint32_t byte_number_to_send_in_packet;
	byte_number_to_send_in_packet = (len_param_array < 3U) ? len_param_array : 3U;

	//we construct a word buffer for the bytes, turning them into 32-bit packages
	uint32_t word_buf;
	word_buf = DCS_code;					//we put the DCS code on the buffer

	//we build up the entire word buffer
	uint32_t count;
	for (count = 0U; count < byte_number_to_send_in_packet; count++){

		word_buf |= (((uint32_t)(*(param_array_byte_ptr + count))) << (8U + (8U * count)));

	}

	//we place the word on the bus as payload
	DSI->GPDR = word_buf;

	//we count downwards
	uint32_t tx_bytes_remain_cnt;
	tx_bytes_remain_cnt = len_param_array - byte_number_to_send_in_packet;

	//we step the array pointer
	param_array_byte_ptr += byte_number_to_send_in_packet;

	while(tx_bytes_remain_cnt != 0){																//as long as we have bytes to send

		byte_number_to_send_in_packet = (tx_bytes_remain_cnt < 4U) ? tx_bytes_remain_cnt : 4U;		//we check how many bytes are left to send
		word_buf = 0U;																				//wipe the word buffer
	    for (count = 0U; count < byte_number_to_send_in_packet; count++)											//construct the buffer
	    {
	    	word_buf |= (((uint32_t)(*(param_array_byte_ptr + count))) << (8U * count));
	    }
	    DSI->GPDR = word_buf;

	    tx_bytes_remain_cnt -= byte_number_to_send_in_packet;
	    param_array_byte_ptr += byte_number_to_send_in_packet;

	}

	//we construct the header as configuration
	DSI->GHCR = (0x39 | (0 << 6U) | (((len_param_array + 1U) & 0x00FFU) << 8U) | ((((len_param_array + 1U) & 0xFF00U) >> 8U)) << 16U);

}
