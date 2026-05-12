/*
 *  Created on: Feb 10, 2026
 *  Author: BalazsFarkas
 *  Project: STM32_DSI
 *  Processor: STM32U5G9
 *  Program version: 1.0
 *  Source file: SCR_screen_config_w_DSI.c
 *  Change history:
 */

#include "SCR_screen_config_w_DSI.h"


void CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize)
{

  uint32_t destination = (uint32_t)pDst + (y * 480 + x) * 4;			//480 is the resolution of the screen, here constant!
  uint32_t source      = (uint32_t)pSrc;

  RCC->AHB1ENR |= (1<<18);					//enable DMA2D clocking
  DMA2D->FGPFCCR = 0xff000000;				//we set 0xFF alpha on the input layer
  DMA2D->FGMAR = source;					//we set the source - ARGB888 data in the header/FLASH
  DMA2D->OMAR = destination;				//the designated frame buffer with the position where we want to update
  DMA2D->NLR |= (xsize<<16);				//x direction offset
  DMA2D->NLR |= (ysize<<0);					//y direction offset
  DMA2D->OOR = 480 - xsize;					//offset between image lines
  	  	  	  	  	  	  	  	  	  	  	//480 is the resolution of the screen in x direction
  DMA2D->CR |= (1<<0);						//enable

  //here we could do a polling or an interrupt, but since we refresh the images every 2 second, we don't care
  //enable will be pulled LOW once DMA2D is finished

}

void Copy_part_of_source(uint32_t *pSrc, uint32_t *pDst, uint16_t x_pos_in_source, uint16_t y_pos_in_source, uint16_t x_pos_in_dest, uint16_t y_pos_in_dest, uint16_t xsize, uint16_t ysize, uint32_t source_x_res, uint32_t dest_x_res)
{

	/*
	 * Copy a 2D section of the source into the destination
	 * Both source and destination is assumed to be ARGB888
	 * This function is there to transfer a 2D array to the frame buffer.
	 * x_pos_in_source and y_pos_in_source are the starting point within the source
	 * x_pos_in_dest and y_pos_in_dest are the starting point within the destination (frame buffer)
	 * source_x_res is the source array x resolution (needed for FGOR jump)
	 * dest_x_res is the destination array x resolution (needed for OOR jump)
	 * xsize and ysize is the size of the section we wish to transfer
	 */

  uint32_t destination = (uint32_t)pDst + (y_pos_in_dest * dest_x_res + x_pos_in_dest) * 4;			//480 is the resolution of the screen, here constant!
  uint32_t source      = (uint32_t)pSrc + (y_pos_in_source * source_x_res + x_pos_in_source) * 4;

  RCC->AHB1ENR |= (1<<18);					//enable DMA2D clocking
  DMA2D->FGPFCCR = 0xff000000;				//we set 0xFF alpha on the input layer
  DMA2D->FGMAR = source;					//we set the source - ARGB888 data in the header/FLASH
  DMA2D->OMAR = destination;				//the designated frame buffer with the position where we want to update
  DMA2D->NLR |= (xsize<<16);				//x direction offset
  DMA2D->NLR |= (ysize<<0);					//y direction offset
  DMA2D->OOR = dest_x_res - xsize;			//offset between image lines
  	  	  	  	  	  	  	  	  	  	  	//480 is the resolution of the screen in x direction
  DMA2D->FGOR = source_x_res - xsize;
  DMA2D->CR |= (1<<0);						//enable

  //here we could do a polling or an interrupt, but since we refresh the images every 2 second, we don't care
  //enable will be pulled LOW once DMA2D is finished

}

uint32_t Config_U5Disco_screen(void)
{

 // if(HAL_DSI_Start(&hdsi) != HAL_OK) return 1;

  DSI->WCR |= (1<<3);															//DSI wrapper enable
  DSI->CR |= (1<<0);															//enable DSI


  /* CMD Mode */
  uint8_t InitParam1[3] = {0xFF ,0x83 , 0x79};
  DSI_multi_Tx(0xB9, 3, InitParam1);

  /* SETPOWER */
  uint8_t InitParam3[16] = {0x44,0x1C,0x1C,0x37,0x57,0x90,0xD0,0xE2,0x58,0x80,0x38,0x38,0xF8,0x33,0x34,0x42};
  DSI_multi_Tx(0xB1, 16, InitParam3);

  /* SETDISP */
  uint8_t InitParam4[9] = {0x80,0x14,0x0C,0x30,0x20,0x50,0x11,0x42,0x1D};
  DSI_multi_Tx(0xB2, 9, InitParam4);

  /* Set display cycle timing */
  uint8_t InitParam5[10] = {0x01,0xAA,0x01,0xAF,0x01,0xAF,0x10,0xEA,0x1C,0xEA};
  DSI_multi_Tx(0xB4, 10, InitParam5);

  /* SETVCOM */
  uint8_t InitParam60[4] = {00,00,00,0xC0};
  DSI_multi_Tx(0xC7, 4, InitParam60);

  /* Set Panel Related Registers */
  DSI_single_Tx(0xCC, 0x02);

  DSI_single_Tx(0xD2, 0x77);

  uint8_t InitParam50[37] = {0x00,0x07,0x00,0x00,0x00,0x08,0x08,0x32,0x10,0x01,0x00,0x01,0x03,0x72,0x03,0x72,0x00,0x08,0x00,0x08,0x33,0x33,0x05,0x05,0x37,0x05,0x05,0x37,0x0A,0x00,0x00,0x00,0x0A,0x00,0x01,0x00,0x0E};
  DSI_multi_Tx(0xD3, 37, InitParam50);

  uint8_t InitParam51[34] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x19,0x19,0x18,0x18,0x18,0x18,0x19,0x19,0x01,0x00,0x03,0x02,0x05,0x04,0x07,0x06,0x23,0x22,0x21,0x20,0x18,0x18,0x18,0x18,0x00,0x00};
  DSI_multi_Tx(0xD5, 34, InitParam51);

  uint8_t InitParam52[35] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x19,0x19,0x18,0x18,0x19,0x19,0x18,0x18,0x06,0x07,0x04,0x05,0x02,0x03,0x00,0x01,0x20,0x21,0x22,0x23,0x18,0x18,0x18,0x18};
  DSI_multi_Tx(0xD6, 35, InitParam52);

  /* SET GAMMA */
  uint8_t InitParam8[42] = {0x00,0x16,0x1B,0x30,0x36,0x3F,0x24,0x40,0x09,0x0D,0x0F,0x18,0x0E,0x11,0x12,0x11,0x14,0x07,0x12,0x13,0x18,0x00,0x17,0x1C,0x30,0x36,0x3F,0x24,0x40,0x09,0x0C,0x0F,0x18,0x0E,0x11,0x14,0x11,0x12,0x07,0x12,0x14,0x18};
  DSI_multi_Tx(0xE0, 42, InitParam8);

  uint8_t InitParam44[3] = {0x2C,0x2C,00};
  DSI_multi_Tx(0xB6, 3, InitParam44);

  DSI_single_Tx(0xBD, 0x00);

  uint8_t InitParam14[] = {0x01,0x00,0x07,0x0F,0x16,0x1F,0x27,0x30,0x38,0x40,0x47,0x4E,0x56,0x5D,0x65,0x6D,0x74,0x7D,0x84,0x8A,0x90,0x99,0xA1,0xA9,0xB0,0xB6,0xBD,0xC4,0xCD,0xD4,0xDD,0xE5,0xEC,0xF3,0x36,0x07,0x1C,0xC0,0x1B,0x01,0xF1,0x34,0x00};
  DSI_multi_Tx(0xC1, 42, InitParam14);

  DSI_single_Tx(0xBD, 0x01);

  uint8_t InitParam15[] = {0x00,0x08,0x0F,0x16,0x1F,0x28,0x31,0x39,0x41,0x48,0x51,0x59,0x60,0x68,0x70,0x78,0x7F,0x87,0x8D,0x94,0x9C,0xA3,0xAB,0xB3,0xB9,0xC1,0xC8,0xD0,0xD8,0xE0,0xE8,0xEE,0xF5,0x3B,0x1A,0xB6,0xA0,0x07,0x45,0xC5,0x37,0x00};
  DSI_multi_Tx(0xC1, 42, InitParam15);

  DSI_single_Tx(0xBD, 0x02);

  uint8_t InitParam20[42] = {0x00,0x09,0x0F,0x18,0x21,0x2A,0x34,0x3C,0x45,0x4C,0x56,0x5E,0x66,0x6E,0x76,0x7E,0x87,0x8E,0x95,0x9D,0xA6,0xAF,0xB7,0xBD,0xC5,0xCE,0xD5,0xDF,0xE7,0xEE,0xF4,0xFA,0xFF,0x0C,0x31,0x83,0x3C,0x5B,0x56,0x1E,0x5A,0xFF};
  DSI_multi_Tx(0xC1, 42, InitParam20);

  DSI_single_Tx(0xBD, 0x00);

  /* Exit Sleep Mode*/
  DSI_single_Tx(0x11, 0x00);

  Delay_ms(120);

  /* Clear LCD_FRAME_BUFFER */
  memset((uint32_t *)LCD_FRAME_BUFFER,0x00, 0xFFFFF);

  /* Display On */
  DSI_single_Tx(0x29, 0x00);

  Delay_ms(120);

  /* All setting OK */
  return 0;
}


int32_t LCD_FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  uint32_t  Xaddress = 0;
  uint32_t  Startaddress = 0;
  uint32_t  i;
  uint32_t  j;

  /* Get the rectangle start address */
  Startaddress = LCD_FRAME_BUFFER + (4 * (Ypos * 480 + Xpos));

  /* Fill the rectangle */
  for (i = 0; i < Height; i++)
  {
    Xaddress = Startaddress + (480 * 4 * i);
    for (j = 0; j < Width; j++)
    {
      *(__IO uint32_t *)(Xaddress) = Color;
      Xaddress += 4;
    }
  }

  return 0;
}

