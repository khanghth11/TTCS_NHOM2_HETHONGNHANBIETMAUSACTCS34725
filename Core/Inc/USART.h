/*
 * USART.h
 *
 *  Created on: Jan 3, 2024
 *      Author: Oski
 */

#ifndef INC_USART_H_
#define INC_USART_H_

#define USART_TXBUF_LEN 1512
#define MAX_FRAME_SIZE 261
#define SEPARATOR 0x3b
#define STX 0x02
#define ETB 0x17

#include "stm32f4xx.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

void USART_start();
void ParseCommand(char *buf);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
uint8_t USART_GETFRAME(char *buf);
void USART_fsend(char* format,...);
uint8_t CALCULATE_CRC(char *data, uint16_t length);
void ShowArchivalData(uint16_t x,uint16_t y);
uint8_t USART_getchar();
uint8_t BX_haveData();

#endif /* INC_USART_H_ */
