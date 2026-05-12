/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ClockDriver_STM32U5G9.h"
#include "I2CDriver_STM32U5G9.h"
#include "SCR_LTDCDriver_STM32U5xx.h"
#include "SCR_screen_config_w_DSI.h"
#include "snake_graphics.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint32_t ImageIndex = 0;
static const uint32_t * Images[] =
{
  image_320x240_argb8888,
  life_augmented_argb8888,
};

static uint32_t x_pos = 0;
static uint32_t y_pos = 0;
static uint32_t x_pos_in_source = 0;
static uint32_t y_pos_in_source = 0;

static const uint8_t snake_bit_x_size = 20;
static const uint8_t snake_bit_y_size = 20;

static uint32_t snake_body_pos[272][2] =  {{240 ,220},					//x and y coordinates of the snake
										{220 ,220},
										{200 ,220},
										{0, 0}						//this is the copy dummy
};

static uint32_t   snake_body_length = 3;							//starting snake is always 3 bits long

static enum_Direction_Selector snake_move_direction = Right;
static uint32_t* image_ptr = (uint32_t*) snake_head_right;

static uint32_t fruit_pos[2] = {0,0};

static uint8_t had_fruit = 0;

static const uint32_t snake_crawl_speed = 500;							//crawl speed of the snake

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

static void place_fruit(void);
static void update_snake(void);
static void remove_tail(void);
static void  check_for_collision(void);

static void RNG_init(void);
static uint8_t CTP_I2C_Rx(uint8_t reg);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
 // SystemClock_Config();

  /* Configure the System Power */
//  SystemPower_Config();

  /* USER CODE BEGIN SysInit */

  SysClockConfig();

  TIM6Config();

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  I2C5_Config();

  RNG_init();

  LTDC_U5_Config(LCD_FRAME_BUFFER);

  DSI_config();

  //reset screen
  Delay_ms(11);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);
  Delay_ms(150);

  //configure screen
  Config_U5Disco_screen();

  //Clear LCD
  #define UTIL_LCD_COLOR_BLACK 0xFF000000UL					//carried over from stm32_lcd.h
  #define UTIL_LCD_COLOR_WHITE 0xFFFFFFFFUL

  LCD_FillRect(0, 0, 480, 480, UTIL_LCD_COLOR_BLACK);

#ifdef load_images_in_bulk
  x_pos = 67;
  y_pos = 140;
#endif


#ifdef roll_one_image_after_other
  x_pos = 67;
  y_pos = 140;
  x_pos_in_source = 0;
  y_pos_in_source = 0;
  image_ptr = (uint32_t*) Images[0];
#endif

//#ifdef snake
  //draw frame
  LCD_FillRect(78, 58, 2, 364, UTIL_LCD_COLOR_WHITE);
  LCD_FillRect(78, 58, 342, 2, UTIL_LCD_COLOR_WHITE);
  LCD_FillRect(400, 58, 2, 364, UTIL_LCD_COLOR_WHITE);
  LCD_FillRect(78, 420, 342, 2, UTIL_LCD_COLOR_WHITE);

  //we draw the starting snake body
  update_snake();

  //we draw the first fruit
  place_fruit();
//#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#ifdef load_images_in_bulk
	  //below we load the image
    CopyBuffer((uint32_t *)Images[ImageIndex ++], (uint32_t *)LCD_FRAME_BUFFER, 67, 140, IMAGE_WIDTH, IMAGE_HEIGHT);

    if(ImageIndex >= 2)
    {
      ImageIndex = 0;
    }

    /* Wait some time before switching to next stage */
    Delay_ms(2000);
#endif

#ifdef roll_one_image_after_other
    uint8_t x_size = 10;
    uint8_t y_size = 20;

    Copy_part_of_source(image_ptr, (uint32_t *)LCD_FRAME_BUFFER, x_pos_in_source, y_pos_in_source, x_pos, y_pos, x_size, y_size, 320, 480);

    if(x_pos < ((320 - x_size) + 67)){

    	x_pos += x_size;
    	x_pos_in_source += x_size;

    } else {

    	x_pos = 67;
    	x_pos_in_source = 0;
    	y_pos += y_size;
    	y_pos_in_source += y_size;

    }

    if(y_pos < ((240 - y_size) + 140)){

    	//do nothing

    } else {

    	y_pos = 140;
    	y_pos_in_source = 0;
    	image_ptr = (uint32_t*) Images[1];

    }

    Delay_ms(2);
#endif

//#ifdef snake
    //check the current length of the snake
    uint32_t snake_part_cnt = 0;

    for(uint32_t i = 0; i < (sizeof(snake_body_pos)/sizeof(snake_body_pos[0])); i++){

    	if(snake_body_pos[i][0] == 0){

    		break;

    	} else {

    		snake_part_cnt++;

    	}

    }

    snake_body_length = snake_part_cnt;

    //we "push" the snake down by 1 in the body matrix
    for(uint32_t i = snake_body_length; i > 0; i--){

    	snake_body_pos[i][0] = snake_body_pos[i - 1][0];
    	snake_body_pos[i][1] = snake_body_pos[i - 1][1];

    }

    //add CTP here for the screen
    //IC is ST1633I, write address is 0xE0, read address is 0xE1
    uint16_t X0_Y0_H;
    uint16_t X0_L;
    uint16_t Y0_L;

    X0_Y0_H = CTP_I2C_Rx(0x12);	//coord H
    X0_L = CTP_I2C_Rx(0x13);		//X L
    							//<60 is LEft
    							//
    Y0_L = CTP_I2C_Rx(0x14);		//Y L

    uint16_t X_coord = (uint16_t)(((X0_Y0_H >> 4) & 0x7) << 8) | X0_L;
    uint16_t Y_coord = (uint16_t)((X0_Y0_H & 0x7) << 8) | Y0_L;

    if (X_coord < 50){

    	if(snake_move_direction != Right){

    		snake_move_direction = Left;

    	} else {

    		//do nothing

    	}

    } else if (X_coord > 400){

    	if(snake_move_direction != Left){

    		snake_move_direction = Right;

    	} else {

    		//do nothing

    	}

    } else {

    	//do nothing

    }

    if (Y_coord < 50){

    	if(snake_move_direction != Down){

    		snake_move_direction = Up;

    	} else {

    		//do nothing

    	}

    } else if (Y_coord > 420){

    	if(snake_move_direction != Up){

    		snake_move_direction = Down;

    	} else {

    		//do nothing

    	}


    } else {

    	//do nothing

    }

    //define the next position of the snake's head
    switch(snake_move_direction){

		case Right:

		    //we update the place of the head
		    if(snake_body_pos[0][0] < ((320 - snake_bit_x_size) + 80)){

		    	snake_body_pos[0][0]  += snake_bit_x_size;

		    } else {

		    	snake_body_pos[0][0]  = 80;

		    }



		    //we select the graphics
		    image_ptr = (uint32_t*) snake_head_right;

			break;

		case Left:

		    //we update the place of the head
		    if(snake_body_pos[0][0] > 80){

		    	snake_body_pos[0][0]  -= snake_bit_x_size;

		    } else {

		    	snake_body_pos[0][0]  = (400 - snake_bit_x_size);

		    }

		    //we select the graphics
		    image_ptr = (uint32_t*) snake_head_left;

			break;

		case Down:

		    if(snake_body_pos[0][1] < ((360 - snake_bit_y_size) + 60)){

		    	snake_body_pos[0][1]  += snake_bit_y_size;;

		    } else {

		    	snake_body_pos[0][1]  = 60;

		    }

		    //we select the graphics
		    image_ptr = (uint32_t*) snake_head_down;

			break;

		case Up:

		    if(snake_body_pos[0][1] > 60){

		    	snake_body_pos[0][1]  -= snake_bit_y_size;

		    } else {

		    	snake_body_pos[0][1]  = (420 - snake_bit_y_size);

		    }


		    //we select the graphics
		    image_ptr = (uint32_t*) snake_head_down;

			break;

			break;

		default :
			  break;

    }

    //we check for collision with self or with a fruit
    check_for_collision();

    //we draw the head and the second bit of the snake (rest does not change)
    update_snake();

    if(had_fruit){

    	place_fruit();
    	had_fruit = 0;

    } else {

		//we remove the "tail"
		remove_tail();

    }

    Delay_ms(snake_crawl_speed);
//#endif

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = 8;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_0;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
   */
  HAL_PWREx_DisableUCPDDeadBattery();

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DSI_RESETn_GPIO_Port, DSI_RESETn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : DSI_RESETn_Pin */
  GPIO_InitStruct.Pin = DSI_RESETn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DSI_RESETn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PI6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void place_fruit(void){

	  uint8_t fruit_placed = 0;

	  //we place the fruit where there is no snake
	  while(!fruit_placed){

		  fruit_placed = 1;

		  uint32_t random_number;
		  random_number = RNG->DR;													//we take a random number

		  fruit_pos[0] = (random_number % 12) * snake_bit_x_size + 100;				//since we have 12 positions, we divide the random number by 12 and check the remnant
		  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//the +100 is the screen offset of the snake's movements field
		  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//we multiply by the x direction size of the snake to get the right coordinates
		  random_number = RNG->DR;													//we take another random number
		  fruit_pos[1] = (random_number % 12) * snake_bit_y_size + 60;

		  //we check if the fruit was placed to a position where there is no snake yet
		  for (uint32_t i = 0; i < snake_body_length; i++){

			  if(((snake_body_pos[i][0]) == fruit_pos[0]) & ((snake_body_pos[i][1]) == fruit_pos[1])){

				  fruit_placed = 0;

			  } else {

				  //do nothing

			  }

		  }

		  //we reset the RNG peripheral
		  //since the DR register is automatically refilled (plus the FIFO when empty), we don't need to manually reset
/*		  RNG->CR |= (1<<30);					//do CONDRST
		  Delay_ms(1);
		  RNG->CR &= ~(1<<30);					//reset CONDRST
		  Delay_ms(1);*/

	  }

	  image_ptr = (uint32_t*) fruit_body;

	  Copy_part_of_source(image_ptr, (uint32_t *)LCD_FRAME_BUFFER, x_pos_in_source, y_pos_in_source, fruit_pos[0], fruit_pos[1], snake_bit_x_size, snake_bit_y_size, 20, 480);

	  Delay_ms(2);

}

void update_snake(void){

	//place head
    Copy_part_of_source(image_ptr, (uint32_t *)LCD_FRAME_BUFFER, x_pos_in_source, y_pos_in_source, snake_body_pos[0][0], snake_body_pos[0][1], snake_bit_x_size, snake_bit_y_size, 20, 480);

    Delay_ms(2);

    //replace second bit with body
	image_ptr = (uint32_t*) snake_body;

	Copy_part_of_source(image_ptr, (uint32_t *)LCD_FRAME_BUFFER, x_pos_in_source, y_pos_in_source, snake_body_pos[1][0], snake_body_pos[1][1], snake_bit_x_size, snake_bit_y_size, 20, 480);

	Delay_ms(2);

}

void remove_tail(void){

	if((snake_body_pos[snake_body_length][0] == snake_body_pos[0][0]) & (snake_body_pos[snake_body_length][1] == snake_body_pos[0][1])){

		//if the head bites into the tail
		//do nothing, we don't remove the tail

	} else if((snake_body_pos[snake_body_length][0] == fruit_pos[0]) & (snake_body_pos[snake_body_length][1] == fruit_pos[1])){

		//if the fruit is placed exactly behind the snake
		//do nothing, we don't remove the "tail"/fruit

	} else {

		image_ptr = (uint32_t*) snake_body_remove;

		Copy_part_of_source(image_ptr, (uint32_t *)LCD_FRAME_BUFFER, x_pos_in_source, y_pos_in_source, snake_body_pos[snake_body_length][0], snake_body_pos[snake_body_length][1], snake_bit_x_size, snake_bit_y_size, 20, 480);

	}


/*	image_ptr = (uint32_t*) snake_body_remove;

	Copy_part_of_source(image_ptr, (uint32_t *)LCD_FRAME_BUFFER, x_pos_in_source, y_pos_in_source, snake_body_pos[snake_body_length][0], snake_body_pos[snake_body_length][1], snake_bit_x_size, snake_bit_y_size, 20, 480);
*/
	snake_body_pos[snake_body_length][0] = 0;
	snake_body_pos[snake_body_length][1] = 0;

}

void check_for_collision(void){

	//we check if we collided with a ourselves
	for(uint32_t i = 1; i < snake_body_length; i++){

		if(snake_body_pos[i][0] == snake_body_pos[0][0]){

			if(snake_body_pos[i][1] == snake_body_pos[0][1]){

				//game over
				while(1);

			} else {

				//do nothing

			}

		} else {

			//do nothing

		}

	}

	//we check if we found a fruit
	if((snake_body_pos[0][0] == fruit_pos[0]) & (snake_body_pos[0][1] == fruit_pos[1])){

		had_fruit = 1;

	} else {

		had_fruit = 0;

	}

}


void RNG_init(void){

	RCC->AHB2ENR1 |= (1<<18);						  	//enable RNG clocking
	RCC->CCIPR2 &= (3<<12);								//HSI48 selected as clock source

	RNG->CR |= (8<<20);									//RNG config 1
	RNG->CR &= ~(7<<13);								//RNG config 2
	RNG->CR |= (13<<8);									//RNG config 3
	RNG->CR |= (1<<2);									//RNGEN

}

uint8_t CTP_I2C_Rx(uint8_t reg)
{

	uint8_t buf[1];

	buf[0] = reg;

	I2C5_TX(0xE0, 1, buf);

	Delay_ms(1);

	I2C5_RX(0xE1, 1, buf);

    Delay_ms(1);

    return buf[0];
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
