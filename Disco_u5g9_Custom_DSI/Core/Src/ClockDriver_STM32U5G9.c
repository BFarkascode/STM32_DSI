/*
 *  Created on: Jan 30, 2026
 *  Author: BalazsFarkas
 *  Project: STM32_DSI
 *  Processor: STM32U5G9
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Program version: 1.0
 *  File: ClockDriver_STM32U5G9.c
 *  Change history: N/A
 */

#include "ClockDriver_STM32U5G9.h"

//1)We set up the core clock and the peripheral prescalers/dividers
void SysClockConfig(void) {
	/**
	 * What happens here?
	 * Firstly, we choose a clocking input, which is going to be HSI16 for us (and internal resonator).
	 * Then we set the power interface's clocking and the FLASH NVM clocking, just in case they were not set before (we assume they haven't been). We touch upon the NVM in the NVMDriver project.
	 * Both the NVM and the PWR are set as the standard recommended values. They can be both played with to optimise the power consumption of the mcu, albeit that will be for another project.
	 * We set the AHB, APB1 and APB2 clocks. These are various periphery clocks that will define at what clocking the peripheries will, well, clock.
	 * Knowing what values are set in the AHB/APB1/APB2 is extremely important to have the right clocking on the peripheries (failing to do so will not allow the periphery to work).
	 * Lastly, we set the phased-locked loop or PLL, which allows us to have a base clock of 32 MHz - the maximum for the L0x3 series. In the end, this will be our system clock that will feed the AHB/APB1/APB2
	 *
	 *
	 *
	 * 1)Enable - future - system clock and wait until it becomes available. Originally we are running on MSI.
	 * 2)Set PWREN clock and the VOLTAGE REGULATOR
	 * 3)FLASH prefetch and LATENCY
	 * 4)Set PRESCALER HCLK, PCLK1, PCLK2
	 * 5)Configure PLL
	 * 6)Enable PLL and wait until available
	 * 7)Select clock source for system and wait until available
	 *
	 **/

	//1)
	//MSIS on
	RCC->CR |= (1<<0);															//we turn on MSIS
	while (!(RCC->CR & (1<<2)));												//and wait until it becomes stable. Bit 2 should be 1.

	//set MSIS 4 MHz
	RCC->ICSCR1 &= ~(15<<28);
	RCC->ICSCR1 |= (4<<28);														//set MSIS 4 MHz range (should be reset value)
	RCC->ICSCR1 |= (1<<23);														//clock range selection for MSIS activated

	//HSI16 on, wait for ready flag
	RCC->CR |= (1<<8);															//we turn on HSI16
	while (!(RCC->CR & (1<<10)));												//and wait until it becomes stable. Bit 10 should be 1.

	//HSE on, wait for ready flag
	RCC->CR |= (1<<16);															//we turn on HSE
	while (!(RCC->CR & (1<<17)));												//and wait until it becomes stable. Bit 17 should be 1.

	//2)
	//power control enabled
	//select SMPS as power regulator
	//set range 1 (range 4 not allowed when using PLL), range 1 is needed for 160 MHz
	//flash latency to 4
	//out of the box, we have range 4 and latency 0

	RCC->AHB3ENR |= (1<<2);
	PWR->CR3 |= (1<<1);															//SMPS enabled
	PWR->VOSR |= (3<<16);														//range 1
	PWR->VOSR |= (1<<18);														//BOOST EN - must be set for range 1

	//3)
	//Flash access control register - prefetch, 4 WS latency
	FLASH->ACR |= (1<<2);														//4 WS
	FLASH->ACR |= (1<<8);														//prefetch

	//4)Setting up the clocks
	//Note: this part is always specific to the usecase!
	//Here 32 MHz full speed, HSI16, PLL_mul 4, plldiv 2, pllclk, AHB presclae 1, hclk 32, ahb prescale 1, apb1 clock divider 4, apb2 clockdiv 1, pclk1 2, pclk2 1

	//AHB 1

	RCC->CFGR2 &= ~(15<<0);														//no AHB prescale

	//APB1, APB2, APB3
	RCC->CFGR2 &= ~(7<<4);														//APB1 not divided
	RCC->CFGR2 &= ~(7<<8);														//APB2 not divided
	RCC->CFGR3 &= ~(7<<4);														//APB3 not divided
																				//Note: in the U031, there is only one APB clock

	//5) PLL
	//set PLL1
	RCC->PLL1CFGR = 0x0;														//we wipe the PLLCFGR register

	//PLL1 source MSIS
	RCC->PLL1CFGR |= (1<<0);
	RCC->PLL1CFGR &= ~(1<<1);

	//PLL1 "M" division is 1
	RCC->PLL1CFGR &= ~(15<<8);

	//wipe divider register
	RCC->PLL1DIVR = 0x0;

	//PLL1 "N" multiplier is 80
	RCC->PLL1DIVR |= (79<<0);

	//PLL1 "R" divider is 2
	RCC->PLL1DIVR |= (1<<24);

	//PLL1 "P" multiplier is 8
	RCC->PLL1DIVR |= (7<<9);

	//PLL1 "Q" divider is 2
	RCC->PLL1DIVR |= (1<<16);

	//Enable PLL1
	//enable R output and ready flag for PLL
	RCC->PLL1CFGR |= (1<<18);													//we turn on the PLL R output
																				//PLL1 P and Q are not activated

	RCC->CR |= (1<<24);															//turn on PLL1
	while (!(RCC->CR & (1<<25)));												//and wait until it becomes available

	//we aren't using PLL2
	RCC->CR &= ~(1<<26);														//turn off PLL2

	//set PLL3
	RCC->PLL3CFGR = 0x0;														//we wipe the PLLCFGR register

	//PLL3 source HSE
	RCC->PLL3CFGR |= (3<<0);

	//PLL3 "M" division is 4
	RCC->PLL3CFGR &= ~(15<<8);
	RCC->PLL3CFGR |= (3<<8);

	//PLL3 fractional latch
	RCC->PLL3CFGR |= (1<<4);

	//wipe divider register
	RCC->PLL3DIVR = 0x0;

	//PLL3 "N" multiplier is 125
	RCC->PLL3DIVR |= (124<<0);

	//PLL3 "Q" divider is 2
	RCC->PLL3DIVR |= (1<<16);

	//PLL3 "R" divider is 24
	RCC->PLL3DIVR |= (23<<24);

	//PLL3 "P" divider is 8
	RCC->PLL3DIVR |= (7<<9);

	//Enable PLL3
	//enable R output and ready flag for PLL
	RCC->PLL3CFGR |= (1<<18);													//we turn on the PLL R output
	RCC->PLL3CFGR |= (1<<16);													//we turn on the PLL P output
																				//PLL3 Q is not activated

	RCC->CR |= (1<<28);															//turn on PLL3
	while (!(RCC->CR & (1<<29)));												//and wait until it becomes available

	//6)DSI PHY PLL clocking - 2
	//below we follow page 1792 in the refman
	RCC->CCIPR2 &= ~(1<<15);													//PLL3P as DSI clock
	RCC->APB2ENR |= (1<<27);													//enable DSI
//	RCC->CCIPR2 |= (1<<15);														//PLL DSI PHY as DSI clock

	//we turn on the DSI bias - 6
	DSI->BCFGR |= (1<<6);

	//DSI PLL clock - 7
	DSI->WRPCR |= (125<<2);														//NDIV 125
	DSI->WRPCR |= (2<<20);														//ODF is 2
	DSI->WRPCR |= (4<<11);														//IDF is 4
	DSI->WRPCR &= ~(1<<29);														//band control 500 to 800 MHz
	DSI->WRPCR |= (1<<0);														//PLL DSI PHY enabled

	while ((DSI->WISR & (1<<9)) != (1<<9));										//we wait for the DSI PLL to activate

	//enable DSI - 8
	DSI->CR |= (1<<0);															//enable DSI

	//clock config - 9
	DSI->CCR |= (1<<8);															//timeout clock division
	DSI->CCR |= (4<<0);															//TX escape clock division (needed to configure the DSI PHY)

	//this is until point 9 in the refman page 1792

	//7)Set PLL1 as system source
	RCC->CFGR1 |= (1<<4);														//wake up is on HSI16
	RCC->CFGR1 |= (3<<0);														//PLL1R as tick source
	while ((RCC->CFGR1 & (3<<2)) != (3<<2));									//system clock status (set by hardware in bits [3:2]) should be matching the PLL source status set in bits [1:0]


	SystemCoreClockUpdate();													//This CMSIS function must be called to update the system clock! If not done, we will remain in the original clocking (likely MSI).

}

//2) TIM6 setup for precise delay generation
void TIM6Config (void) {
	/**
	 * What happens here?
	 * We first enable the timer, paying VERY close attention on which APB it is connected to (APB1).
	 * We then prescale the (automatically x2 multiplied!) APB clock to have a nice round frequency.
	 * Since we will play with how far this timer counts, we put the automatic maximum value of count value to maximum. This means that this timer can only count 65535 cycles.
	 * We tgeb enable the timers and wait until it is engaged.
	 *
	 * TIM6 is a basic clock that is configured to provide a counter for a simple delay function (see below).
	 * It is connected to AP1B which is clocking at 16 MHz currently (see above the clock config for the PLL setup to generate 32 MHz and then the APB1 clock divider left at DIV4 and a PCLK multiplier of 2 - not possible to change)
	 * 1)Enable TIM6 clocking
	 * 2)Set prescaler and ARR
	 * 3)Enable timer and wait for update flag
	 **/

	//1)
	RCC->APB1ENR1 |= (1<<4);													//enable TIM6 clocking

	//2)

	TIM6->PSC = 160 - 1;														// 160 MHz/160 = 1 MHz -- 1 us delay
																				// Note: the timer has a prescaler, but so does APB1!
																				// Note: the timer has a x2 multiplier on the APB clock
																				// Here APB1 PCLK is 8 MHz

	TIM6->ARR = 0xFFFF;															//Maximum ARR value - how far can the timer count?

	//3)
	TIM6->CR1 |= (1<<0);														//timer counter enable bit
	while(!(TIM6->SR & (1<<0)));												//wait for the register update flag - UIF update interrupt flag
																				//update the timer if we are overflown/underflow with the counter was reinitialised.
																				//This part is necessary since we can update on the fly. We just need to wait until we are done with a counting cycle and thus an update event has been generated.
																				//also, almost everything is preloaded before it takes effect
																				//update events can be disabled by writing to the UDIS bits in CR1. UDIS as LOW is UDIS ENABLED!!!s
}


//3) Delay function for microseconds
void Delay_us(int micro_sec) {
	/**
	 * Since we can use TIM6 only to count up to maximum 65535 cycles (65535 us), we need to up-scale out counter.
	 * We do that by counting first us seconds...
	 *
	 *
	 * 1)Reset counter for TIM6
	 * 2)Wait until micro_sec
	 **/
	TIM6->CNT = 0;
	while(TIM6->CNT < micro_sec);												//Note: this is a blocking timer counter!
}


//4) Delay function for milliseconds
void Delay_ms(int milli_sec) {
	/*
	 * ...and then, ms seconds.
	 * This function will be equivalent to HAL_Delay().
	 *
	 * */

	for (uint32_t i = 0; i < milli_sec; i++){
		Delay_us(1000);															//we call the custom microsecond delay for 1000 to generate a delay of 1 millisecond
	}
}

