/*
 * USART.c
 *
 *  Created on: Jan 3, 2024
 *      Author: Oski
 */

#include "USART.h"
#include "TCS.h"
#include <math.h>


extern UART_HandleTypeDef huart2;
extern uint8_t LIVE_TOGGLE;
extern uint8_t PREVIEW_TOGGLE;
extern uint16_t Delay;
extern uint8_t ARCHIVED_DATA[1200][3];


uint8_t USART_TxBuf[USART_TXBUF_LEN];
uint8_t USART_RxBuf[MAX_FRAME_SIZE];

__IO int USART_TX_Empty=0;
__IO int USART_TX_Busy=0;
__IO int USART_RX_Empty=0;
__IO int USART_RX_Busy=0;


void USART_start(){
	HAL_UART_Receive_IT(&huart2,&USART_RxBuf[0],1);
}

uint8_t BX_haveData(){
	if(USART_RX_Empty==USART_RX_Busy)
		return 0;
	else return 1;
}

//Funkcja pobierająca znak z buforu kołowego
uint8_t USART_getchar(){
uint8_t tmp;
	if(BX_haveData()){
		 tmp=USART_RxBuf[USART_RX_Busy];
		 USART_RX_Busy++;
		 if(USART_RX_Busy >= MAX_FRAME_SIZE)USART_RX_Busy=0;
		 return tmp;
	}else return -1;
}

uint8_t CALCULATE_CRC(char *data, uint16_t length) {
    const uint8_t WIELOMIAN = 0x9B;
    uint8_t MY_CRC = 0xFF;

    for (uint16_t i = 0; i < length; ++i) {
    	MY_CRC ^= data[i];
        for (int j = 0; j < 8; ++j){
            if (MY_CRC & 0x80)
            	MY_CRC = (MY_CRC << 1) ^ WIELOMIAN;
            else
            	MY_CRC <<= 1;
        }
    }
    return MY_CRC;
}

void USART_fsend(char* format,...){ // Funkcja odpowiedzialna za przesyłanie tekstu do komputera
	char tmp_rs[128];
	char FRAME[MAX_FRAME_SIZE];
	__IO int idx;
	va_list arglist; // Deklaracja listy argumentów
	va_start(arglist,format);
	vsprintf(tmp_rs,format,arglist); //Wpisanie do bufora tmp_rs sformatowanego tekstu
	va_end(arglist); // Koniec przetwarzania zmienych argumentow
	idx=USART_TX_Empty; // Przypisanie do idx wartości wskaźnika

	uint8_t CHECKSUM=CALCULATE_CRC(tmp_rs, strlen(tmp_rs));
	sprintf(FRAME,"%c%s%02x%c\r\n",STX,tmp_rs,CHECKSUM,ETB);

	for(int i=0;i<strlen(FRAME);i++){
		USART_TxBuf[idx]=FRAME[i]; //Przypisujemy do bufora znaki z tmp_rs
		idx++; // Zwiększamy wskaźnik
		if(idx >= USART_TXBUF_LEN)idx=0; // Jeżeli datarlśmy do końca bufora to zawijamy
	}
	__disable_irq();
	if((USART_TX_Empty==USART_TX_Busy)&&(__HAL_UART_GET_FLAG(&huart2,UART_FLAG_TXE)==SET)){
		USART_TX_Empty=idx; //
		uint8_t tmp=USART_TxBuf[USART_TX_Busy];
		USART_TX_Busy++;

		if(USART_TX_Busy >= USART_TXBUF_LEN)USART_TX_Busy=0;
			HAL_UART_Transmit_IT(&huart2, &tmp, 1);
	}else{
		USART_TX_Empty=idx;
	}
	__enable_irq();
}

uint8_t USART_GETFRAME(char *buf){
	static char FRAME[MAX_FRAME_SIZE];
	static uint8_t FRAME_STATE=0;
	static uint16_t FRAME_IDX=0;
	uint8_t ZNAK=USART_getchar();


	switch(ZNAK){
	case STX:
		FRAME_STATE=1;
		FRAME_IDX=0;
		FRAME[FRAME_IDX]=ZNAK;
		FRAME_IDX++;
	break;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	case ETB:
		if(FRAME_STATE==1){
			FRAME_STATE=0;
			FRAME[FRAME_IDX]=ZNAK;
			FRAME_IDX++;

			if (FRAME_IDX < 5){
				break;
			}

			if(FRAME_IDX>=MAX_FRAME_SIZE-4){
				USART_fsend("FRAME RANGE EXCEEDED;");
				break;
			}

			if(FRAME[FRAME_IDX-4]==SEPARATOR){
				char GIVEN_CRC[3]={""};
				char CALC_CRC[3]={""};

				sprintf(CALC_CRC,"%02x",CALCULATE_CRC(FRAME+1,FRAME_IDX-4));
				strncpy(GIVEN_CRC,FRAME+(FRAME_IDX-3),2);

				if(GIVEN_CRC[0]==CALC_CRC[0] && GIVEN_CRC[1]==CALC_CRC[1]){
					for(int i=1;i<FRAME_IDX-3;i++){
						buf[i-1]=FRAME[i];
					}
					buf[FRAME_IDX-4]=0;
				}
			}
		}
	break;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	default:
	if(FRAME_STATE==1){
		if (!(ZNAK > 0x21 && ZNAK < 0x7E)){
			FRAME_STATE = 0;
			break;
		}
		else{
			FRAME[FRAME_IDX] = ZNAK;
			FRAME_IDX++;
		}
	}
	break;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	}

	return 0;
}


//Funkcja odpowiedzialna za rozpoznawanie komend
void ParseCommand(char *buf){
	char * commBuff = strtok(buf,";");
	while(commBuff!=NULL){
		if(strncmp(commBuff,"START",5)==0){
			LIVE_TOGGLE=1;
			USART_fsend("STARTED!;");
		}
		else if(strncmp(commBuff,"STOP",4)==0){
			LIVE_TOGGLE=0;
			USART_fsend("STOPPED!;");
		}
		else if(strncmp(commBuff,"PREVIEW[ON]",12)==0){
			PREVIEW_TOGGLE=1;
			USART_fsend("PREVIEW TURNED ON!;");
		}
		else if(strncmp(commBuff,"PREVIEW[OFF]",12)==0){
			PREVIEW_TOGGLE=0;
			USART_fsend("PREVIEW TURNED OFF!;");
		}
		else if (strncmp(commBuff, "SETINTERVAL[", 12) == 0 && buf[strlen(buf) - 1] == ']'){
			if(atoi(buf + 12)==0){
				USART_fsend("WRONG INTERVAL VALUE;");
			}
			else if(atoi(buf + 12)<3){
				USART_fsend("GIVEN VALUE IS TOO LOW: %d ;", atoi(buf + 12));
			}
			else if(atoi(buf + 12)>65535){
				USART_fsend("INTERVAL LIMIT EXCEEDED;");
			}
			else{
				Delay = atoi(buf + 12);
				USART_fsend("INTERVAL SET TO %d MS;",Delay);
				//I2C_RGBCTime(Delay);
			}
		}
		else if(strncmp(commBuff,"SHOWINT",7)==0){
			USART_fsend("INTERVAL= %dms",Delay);
		}
		else if (strncmp(commBuff, "SHOWARCHIVAL[", 13) == 0 && buf[strlen(buf) - 1] == ']'){
			uint16_t arg1 = atoi(commBuff+13);
			uint16_t arg2 = atoi(commBuff+14+((int)log10(arg1)+1));
			ShowArchivalData(arg1,arg2);
		}
		else{
			USART_fsend("UNRECOGNIZED COMMAND!;");
		}
		commBuff = strtok( NULL,";");
	}
}


void ShowArchivalData(uint16_t x,uint16_t y){
	if((x > 0 && x <= 1200) && (y > 0 && y <= 1200)){
		if(x>y){
			for(int i=x-1;i>=y-1;i--){
				if(ARCHIVED_DATA[i][0]>=0 && ARCHIVED_DATA[i][0]<257)
					USART_fsend("DATA NOT AVAIBLE AT INDEX=%d",i);
				else
					USART_fsend("%d| \tR= %d\tG= %d\tB= %d;",i,ARCHIVED_DATA[i][0],ARCHIVED_DATA[i][1],ARCHIVED_DATA[i][2]);
			}
		}
		else{
			for(int i=x-1;i<=y-1;i++){
				if(ARCHIVED_DATA[i][0]>=0 && ARCHIVED_DATA[i][0]<257)
					USART_fsend("DATA NOT AVAIBLE AT INDEX=%d",i);
				else
					USART_fsend("%d| \tR= %d\tG= %d\tB= %d;",i,ARCHIVED_DATA[i][0],ARCHIVED_DATA[i][1],ARCHIVED_DATA[i][2]);
			}
		}
	}else{USART_fsend("WRONG PARAMETER!");}
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
   if(huart==&huart2){
	   if(USART_TX_Empty!=USART_TX_Busy){
		   uint8_t tmp=USART_TxBuf[USART_TX_Busy];
		   USART_TX_Busy++;
		   if(USART_TX_Busy >= USART_TXBUF_LEN)USART_TX_Busy=0;
		   HAL_UART_Transmit_IT(&huart2, &tmp, 1);
	   }
   }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	 if(huart==&huart2){
		 USART_RX_Empty++;
		 if(USART_RX_Empty>=MAX_FRAME_SIZE)USART_RX_Empty=0;
		 HAL_UART_Receive_IT(&huart2,&USART_RxBuf[USART_RX_Empty],1);

	 }
}
