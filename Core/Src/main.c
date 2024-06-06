/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "TCS.h"
#include "USART.h"
#include "ssd1306.h"
#include "fonts.h"

TIM_HandleTypeDef htim3; // Thêm biến này để điều khiển PWM cho đèn LED
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;
UART_HandleTypeDef huart2;
uint8_t LIVE_TOGGLE=0;
uint8_t PREVIEW_TOGGLE=0;
uint16_t Delay=500;

uint8_t ARCHIVED_DATA[1200][3]={{-1,-1,-1}};
__IO int ARCHIVE_RS=0;

static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C3_Init(void);
static void LED_GPIO_Init(void);
static void MX_TIM3_Init(void);

void delay1() {
	for (uint32_t i = 0; i < 300000; i++)
		;
}

static void MX_TIM3_Init(void) {
    TIM_OC_InitTypeDef sConfigOC ={0} ;

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 83; // Tần số 1 MHz (84 MHz / (Prescaler + 1))
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 155; // Độ phân giải 8-bit
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim3);
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    // Cấu hình kênh PWM cho các chân LED RGB
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) {
            Error_Handler();
        }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_MspInit(&htim3);
}




int main(void)
{
int cuctac=1;
  HAL_Init();
  SysTick_Config(84000000/1000);
  SystemClock_Config();
  MX_TIM3_Init();
  LED_GPIO_Init();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  if (ssd1306_Init(&hi2c3) != 0) {
      Error_Handler();
    }
    char buffer[20];
    char dev[20];
  I2C_Write8BIT(ATIME_REG,0); // ham dieu chinh intergration time cua cam bien
  I2C_Write8BIT(CONTROL_REG,0x03); // ham dieu chinh do nhay gain cam bien
  I2C_Write8BIT(ENABLE_REG,3); // Ham turn on/off cam bien


  while (1)
  {
	  	  HAL_GPIO_WritePin(TCSLED_GPIO_Port, TCSLED_Pin, GPIO_PIN_RESET); //Ham bat tat led tren cam bien tcs
	  	uint8_t  	   red = I2C_GetColor(RED);
	    uint8_t       green = I2C_GetColor(GREEN);
	       uint8_t    blue = I2C_GetColor(BLUE); // gan cac gia tri r,g,b
	       uint8_t    rw = red/5; // vi de giam do sang cua led nen /5
	       uint8_t    gw = green/5;
	       uint8_t    bw = blue/5;
	        adjustLED(rw,gw,bw);
	        if ((I2C_GetColor(RED) >= 240 && I2C_GetColor(RED) <= 256) &&
	        	      (I2C_GetColor(GREEN) >= 230 && I2C_GetColor(GREEN) <= 256) &&
	        	      (I2C_GetColor(BLUE) >= 230 && I2C_GetColor(BLUE) <= 256))
	        	  {
	        	      HAL_GPIO_WritePin(BUZZER_PORT,BUZZER_PIN, GPIO_PIN_RESET);// buzzer se kich hoat khi thoa dieu kien tren
	        	      //dieu kien tren la gia tri mac dinh cua sensor khi khong quet vat nao, co the xac dinh = cach de sensor trong hộp tối
	        	  }
	        	  	else {
	        	   delay1();
	        	   cuctac++;
	        	  if(cuctac >=2) {
	        	  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);

	        	   	delay1();
	        	   	HAL_GPIO_WritePin(BUZZER_PORT,BUZZER_PIN, GPIO_PIN_RESET);// Lenh de buzzer keu bip bip bip ( ngat quang) khi khong thoa dieu kien tren
	        	  cuctac--;

	        	  }
	        	  	}



    /* USER CODE END WHILE */
	    HAL_Delay(100);
	    ssd1306_Fill(Black);
	    HAL_Delay(100);
	    sprintf(buffer, "Red=%d,Green=%d", I2C_GetColor(RED),I2C_GetColor(GREEN));
	    sprintf(dev, "Blue=%d",I2C_GetColor(BLUE));
	    ssd1306_SetCursor(0, 0);
	    ssd1306_WriteString(buffer, Font_7x10, White);
	    ssd1306_SetCursor(0, 36);
	    ssd1306_WriteString(dev, Font_7x10, White);
	    ssd1306_UpdateScreen(&hi2c3);
// Cac ham o tren day la de xuat gia tri r,g,b ra OLED

  }
  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}


static void MX_I2C1_Init(void) // cau hinh I2C1
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void) //Cau hinh I2C3
{

	/* USER CODE BEGIN I2C3_Init 0 */

	  /* USER CODE END I2C3_Init 0 */

	  /* USER CODE BEGIN I2C3_Init 1 */

	  /* USER CODE END I2C3_Init 1 */
	  hi2c3.Instance = I2C3;
	  hi2c3.Init.ClockSpeed = 100000;
	  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
	  hi2c3.Init.OwnAddress1 = 0;
	  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	  hi2c3.Init.OwnAddress2 = 0;
	  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  /* USER CODE BEGIN I2C1_Init 2 */

	  /* USER CODE END I2C1_Init 2 */


}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void) // cau hinh GPIO
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();


  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TCSLED_GPIO_Port, TCSLED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : TCSLED_Pin */
  GPIO_InitStruct.Pin = TCSLED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TCSLED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}



void adjustLED(uint8_t red, uint8_t green, uint8_t blue) { // cau hinh pwm cho cac kenh mau

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, red); // Đặt giá trị PWM cho kênh LED đỏ
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, green); // Đặt giá trị PWM cho kênh LED xanh lục
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, blue); // Đặt giá trị PWM cho kênh LED xanh

}


static void LED_GPIO_Init(void) { //cau hinh gpio pwm cho led rgb module
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Cấu hình chân cho đèn LED xanh lục
    GPIO_InitStruct.Pin = LEDGREEN_RGB_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(LEDGREEN_RGB_PORT, &GPIO_InitStruct);

    // Cấu hình chân cho đèn LED đỏ
    GPIO_InitStruct.Pin = LEDRED_RGB_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(LEDRED_RGB_PORT, &GPIO_InitStruct);


    // Cấu hình chân cho đèn LED xanh
    GPIO_InitStruct.Pin = LEDBLUE_RGB_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(LEDBLUE_RGB_PORT, &GPIO_InitStruct);
}



/* USER CODE BEGIN 4 */

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

