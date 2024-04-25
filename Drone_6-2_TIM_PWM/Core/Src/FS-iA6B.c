/*
 * FS-iA6B.c
 *
 *  Created on: Apr 6, 2024
 *      Author: myhg1
 */

#include "FS-iA6B.h"

FSiA6B_iBus iBus;

unsigned char iBus_Check_CHKSUM(unsigned char* data, unsigned char len)
{
	unsigned short Chksum=0xffff;
	for(int i=0; i<len-2 ;i++)
	{
		Chksum -= data[i];
	}
	return(((Chksum&0x00ff)==data[30]) && ((Chksum>>8)==data[31]));
}

void iBus_Parsing(unsigned char* data, FSiA6B_iBus* iBus){
	iBus->RH = (data[2] | data[3]<<8)& 0x0fff;
	iBus->RV = (data[4] | data[5]<<8) & 0x0fff;
	iBus->LV = (data[6] | data[7]<<8) & 0x0fff;
	iBus->LH = (data[8] | data[9]<<8) & 0x0fff;
	iBus->SwA = (data[10] | data[11]<<8) & 0x0fff;
	iBus->SwC = (data[12] | data[13]<<8) & 0x0fff;

	//상위 4비트는 fail-safe에 저장할것이다.
	iBus->Failsafe = (data[13]>>4);
}

unsigned char iBus_isActiveFailsafe(FSiA6B_iBus* iBus)
{

}

void FSiA6B_UART5_Initialization(void){

	  huart5.Instance = UART5;
	  huart5.Init.BaudRate = 115200;
	  huart5.Init.WordLength = UART_WORDLENGTH_8B;
	  huart5.Init.StopBits = UART_STOPBITS_1;
	  huart5.Init.Parity = UART_PARITY_NONE;
	  huart5.Init.Mode = UART_MODE_RX;
	  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
	  if (HAL_UART_Init(&huart5) != HAL_OK)
	  {
	    Error_Handler();
	  }
}
