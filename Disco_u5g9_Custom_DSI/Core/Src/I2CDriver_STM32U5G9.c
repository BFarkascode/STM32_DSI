/*
 *  Created on: Feb 26, 2026
 *  Author: BalazsFarkas
 *  Project: STM32_DSI
 *  Processor: STM32U5G9
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Header version: 1.0
 *  File: I2CDriver_STM32U5G9.c
 *  Change history: N/A
 */

#include "I2CDriver_STM32U5G9.h"


void I2C5_Config(void) {

	/**Below is the configuration for the I2C for the STM32L0 series
	 * The setting up the I2C is very different compared to M4 devices (what one can easily find on youtube)
	 * No DMA, no CRC and no interrupts are included currently.
	 * Main differences are:
	 * -we need to set the source for the I2C5 in the RCC
	 * -alternative function register values are in the STM32L0 datasheet, not the reference manual
	 * -there is no SWRST bit, we need to use the PE to enable/de-enable the entire peripheral instead
	 * -we don't set standard/fast/fastest mode separate, we use the TIMINGR register
	 * -master/slave switch is automatic: if a START condition is generated, we are in master mode, otherwise slave mode**/

	//1)Enable clocking in the RCC, set I2CCLK source clock, enable GPIO clocking - PB5 SCL, PH4 SDA

	RCC->APB1ENR2 |= (1<<6);					//enable I2C5 clock
	RCC->CCIPR2 &= ~(1<<24);					//APB1 is selected as I2C source clock
	RCC->CCIPR2 &= ~(1<<25);					//APB1 is selected as I2C source clock
	RCC->AHB2ENR1 |= (1<<7);					//PORTH clocking

	//2)Set GPIO parameters (mode, speed, pullup) - PH5 SCL, PH4 SDA
	GPIOH->MODER &= ~(1<<8);					//alternate function for PH4
	GPIOH->MODER &= ~(1<<10);					//alternate function for PH5

	GPIOH->OTYPER |= (1<<4);					//open drain for PH4
	GPIOH->OTYPER |= (1<<5);					//open drain for PH5
	GPIOH->OSPEEDR |= (3<<8);					//very high speed PH4
	GPIOH->OSPEEDR |= (3<<10);					//very high speed PH5
	GPIOH->PUPDR |= (1<<8);						//pullup PH4
	GPIOH->PUPDR |= (1<<10);					//pullup PH5

	//Note: AFR values are in the device datasheet for L0. For I2C5, they will be AF4 as seen on page 45 of the datasheet.
	GPIOH->AFR[0] |= (2<<16);					//PH4 AF2 setup
	GPIOH->AFR[0] |= (2<<20);					//PH5 AF2 setup

	//3)Set a clock source for the internal clock of the I2C
	I2C5->CR1 &= ~(1<<0);						//disable I2C, used as software reset here - no designated SWRST bit in registers
	Delay_ms(1);								//PE should be LOW for at least 3 cycles to take effect! 3 cycles at 8 MHz is 375 ns.
												//config registers are not impacted by the PE reset, only START, STOP, NACK, and various ISR registers

	//4) Set timing - standard mode selected with 8 MHz I2CCLK

	//Standard timing
//	I2C5->TIMINGR |= (19<<0);					//SCLL value shall be 0x13 for 8 MHz I2CCLK, Standard mode (see page 727, ref manual)
//	I2C5->TIMINGR |= (15<<8);					//SCLH value shall be 0xF for 8 MHz I2CCLK, Standard mode
//	I2C5->TIMINGR |= (2<<16);					//SDADEL value shall be 0x2 for 8 MHz I2CCLK, Standard mode
//	I2C5->TIMINGR |= (4<<20);					//SCLDEL value shall be 0x4 for 8 MHz I2CCLK, Standard mode
//	I2C5->TIMINGR |= (1<<28);					//PRESC value shall be 0x1 for 8 MHz I2CCLK, Standard mode

	//Fast timing
/*	I2C5->TIMINGR |= (9<<0);					//SCLL value shall be 0x9 for 8 MHz I2CCLK, Fast mode (see page 727, ref manual)
	I2C5->TIMINGR |= (3<<8);					//SCLH value shall be 0x3 for 8 MHz I2CCLK, Fast mode
	I2C5->TIMINGR |= (1<<16);					//SDADEL value shall be 0x1 for 8 MHz I2CCLK, Fast mode
	I2C5->TIMINGR |= (3<<20);					//SCLDEL value shall be 0x3 for 8 MHz I2CCLK, Fast mode
	I2C5->TIMINGR &= ~(15<<28);					//PRESC value shall be 0x0 for 8 MHz I2CCLK, Fast mode*/

	I2C5->TIMINGR = 0x30909dec;					//taken from CubeMx

	//5) Set own address
	I2C5->OAR1 &= ~(1<<15);						//we disable the own address
	I2C5->OAR1 &= ~(1<<10);						//we choose 7-bit address
//	I2C5->OAR1 |= (dev_own_addr<<1);			//we define the 7-bit address
	I2C5->OAR1 |= (1<<15);						//we enable the own address

	//6)Clean up the ISR register
	I2C5->ISR |= (1<<0);						//we flush the transmit register

	//7)Enable I2C
	I2C5->CR1 &= ~(1<<12);						//analog filter enabled
	I2C5->CR1 &= ~(1<<17);						//clock stretch enabled
												//this must be kept as such for MASTER mode
	I2C5->CR1 |= (1<<0);						//enable I2C
	Delay_ms(1);							//We wait for the setup to take effect
}


void I2C5_TX (uint8_t slave_addr, uint8_t number_of_bytes, uint8_t *bytes_to_send) {
	/**
	 * START bit is automatically generated when we set the appropriate bit. START bit is pulled LOW by the hardware once address is sent.
	 * RD_WRN bit, SADD and NBYTES change is not allowed when START bit is set
	 * All communications are set to be automatic with no focus on specific flag generation. This may need to be changed if we want a good interrupt control.
	 * Writing LOW to the START bit has no effect
	 * "slave_addr" is a 7-bit integer without the write/read bit
	 * START bit is set to LOW after each TX, no matter the ACK/NACK of the address
	 * STOP is generated automatically due to AUTOEND
	 * **/

	//1)We reset the transmission control register
	I2C5->CR2 = 0x0;								//we reset the CR2 register and rebuild it completely to avoid an address being stuck in there

	//2)We set the slave address and the addressing mode
	I2C5->CR2 = slave_addr;					//we write the slave address to the SADD register
													//Note: we are in 7-bit address mode, as it was set above in the I2C config
	I2C5->CR2 &= ~(1<<11);							//7 bits

	//3)We set NBYTES
	I2C5->CR2 |= (number_of_bytes << 16);

	//4)AUTOEND with write as direction, no RELOAD
	I2C5->CR2 |= (1 << 25);
	I2C5->CR2 &= ~(1 << 24);
	I2C5->CR2 &= ~(1<<10);

	//5)We enable PE and set the START bit

	I2C5->CR1 |= (1<<0);							//enable
	I2C5->CR2 |= (1<<13);							//start

	//6)We wait for a reply
	Delay_ms(1);										//25 us wait. This is to allow the byte and the ACK to cross the bus at 100 kHz

	uint8_t reply = 0;

	if ((I2C5->ISR & (1<<4)) == (1<<4)) {													//if the NACKF bit is 1, it means that we had a NO ACKNOWLEDGE from the slave
		reply = 0;
	} else {																	//if NACK is 0, the slave has been ACKed
		I2C5->CR2 |= (1<< 13);													//we use the START bit to send over the byte
		while (number_of_bytes)
		{
			while(!(I2C5->ISR & (1<<1))) {
				for (volatile uint8_t i = 0x0 ; i < 255 ; i++) {
					//timeout control
					//Note:
					i = i;
				};
				break;

			};										//wait for the TXDR flag to be set
			I2C5->TXDR = (volatile uint8_t) *bytes_to_send++;					//we load the byte and thus will have TXIS LOW until the data byte is cleared
																				//difference between TXIS and TXE is that the TXE is just a state register, not en interrupt
			number_of_bytes--;
//			while((I2C5->ISR & (1<<4)) == (1<<4));								//we wait for the NACK to go LOW and thus the device acknowledge the command
																				//if there is a loss of coms, the system will just freeze for now
																				//this should be interrupted!!!!!
			Delay_ms(1);								//25 us wait to transmit the byte and receive an ACK. This is for the 100kHz standard clocking
																				//Note: delay is necessary to avoid clashing of registers: we need to wait until transmission is done before we send a new byte
																				//Note: unfortunately, the NACK bit is active LOW and won't trigger unless there is a NACK. No other flag is available when reload is active.
		}
		reply = 1;
	}

//8)We reset the bus and all the registers in ISR
	I2C5->CR1 &= ~(1<<0);														//we turn off PE and reset the I2C bus
	Delay_ms(1);															//we need to wait 3 APB cycles after PE is cleared. We wait for 1 us instead.

}

//4) Reception
//Expects a certain number of bytes over the bus from the slave
//Each byte is automatically ACKed by the master, except the last one - NACK will go HIGH!
		//NOTE: there is no interrupt in this TX!!!!!!!
//Data input is an array pointer - pass the function either the array, or the address of the array

void I2C5_RX (uint8_t slave_addr, uint8_t number_of_bytes, uint8_t* bytes_received) {
	/**
	 *STOP is generated automatically due to AUTOEND
	 * **/

		//1)We reset the transmission control register
		I2C5->CR2 = 0x0;														//we reset the CR2 register and rebuild it completely to avoid an address being stuck in there

		//2)We set the slave address and the addressing mode
		I2C5->CR2 = slave_addr;											//we write the slave address to the SADD register
																				//Note: we are in 7-bit address mode, as it was set above in the I2C config
		I2C5->CR2 &= ~(1<<11);													//7 bits

		//3)We set NBYTES
		I2C5->CR2 |= (number_of_bytes << 16);

		//4)AUTOEND with write as direction, no RELOAD
		I2C5->CR2 |= (1 << 25);
		I2C5->CR2 &= ~(1 << 24);

		//5)We read now!
		I2C5->CR2 |= (1<<10);

		//6)We enable PE and set the START bit

		I2C5->CR1 |= (1<<0);													//enable
		I2C5->CR2 |= (1<<13);													//start

		//7)We wait for a reply
		Delay_ms(1);									//latency wait

		uint8_t reply = 0;

		if ((I2C5->ISR & (1<<4)) == (1<<4)) {												//if the NACKF bit is 1, it means that we had a NO ACKNOWLEDGE from the slave
			reply = 0;
		} else {																//if NACK is 0, the slave has been ACKed
			while (number_of_bytes) {
				while (!(I2C5->ISR & (1<<2)));									//we wait for the RXNE flag to go HIGH, indicating that we have something in the RXDN register
				*bytes_received = I2C5->RXDR;									//we dereference the pointer, thus we can give it a value
				bytes_received++;												//this is technically an address. A pointer is technically a memory address. Here we step through the array at the address.
				number_of_bytes--;
			}
			reply = 1;
		}

		//8)We reset the bus and all the registers in ISR
		I2C5->CR1 &= ~(1<<0);													//we turn off PE and reset the I2C bus
		Delay_ms(1);

}
