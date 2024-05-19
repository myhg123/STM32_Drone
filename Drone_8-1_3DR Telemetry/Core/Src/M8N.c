/*
 * M8N.c
 *
 *  Created on: Apr 4, 2024
 *      Author: myhg1
 */
#include "M8N.h"

M8N_UBX_NAV_POSLLH posllh;

//초기 설정코드의 배열
const unsigned char UBX_CFG_PRT[] = {

 0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00,

 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x01, 0x00,

 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A, 0x79

}; //UBX Protocol In, Out, UART1, 8N1-9600


const unsigned char UBX_CFG_MSG[] = {

 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x02, 0x00, 0x01,

 0x00, 0x00, 0x00, 0x00, 0x13, 0xBE

}; //NAV-POSLLH(01-02), UART1


const unsigned char UBX_CFG_RATE[] = {

 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00,

 0x01, 0x00, 0xDE, 0x6A

}; //GPS Time, 5Hz Navigation Frequency


const unsigned char UBX_CFG_CFG[] = {

 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00,

 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x31,

 0xBF

}; //Save current configuration, Devices: BBR, FLASH, I2C-EEPROM, SPI-FLASH,

void M8N_UART4_initialization(void)
{
	  huart4.Instance = UART4;
	  huart4.Init.BaudRate = 9600;
	  huart4.Init.WordLength = UART_WORDLENGTH_8B;
	  huart4.Init.StopBits = UART_STOPBITS_1;
	  huart4.Init.Parity = UART_PARITY_NONE;
	  huart4.Init.Mode = UART_MODE_TX_RX;
	  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
	  if (HAL_UART_Init(&huart4) != HAL_OK)
	  {
	    Error_Handler();
	  }
}

void M8N_TransmitData(unsigned char* data, unsigned char len){
	//위의 배열을 보면 프로토콜에 맞게 정확히 잘 되어있다. 그대로 전달해주면된다.
	for(int i=0; i<len; i++){
		HAL_UART_Transmit(&huart4, *(data+i), sizeof(data[i]),10);
	}
}

void M8N_initialization(void)
{
	M8N_UART4_initialization();

	M8N_TransmitData(&UBX_CFG_PRT[0], sizeof(UBX_CFG_PRT));
	HAL_Delay(100);
	M8N_TransmitData(&UBX_CFG_MSG[0], sizeof(UBX_CFG_MSG));
	HAL_Delay(100);
	M8N_TransmitData(&UBX_CFG_RATE[0], sizeof(UBX_CFG_RATE));
	HAL_Delay(100);
	M8N_TransmitData(&UBX_CFG_CFG[0], sizeof(UBX_CFG_CFG));
}

//체크섬 검사이후 체크섬이 맞으면 1 틀리면 0을 호출하는 함수를 구현할 것이다.
unsigned char M8N_UBX_CHKSUM_check(unsigned char* data, unsigned char len)
{
	//체크섬을 진행할 변수 두가지
	unsigned char CK_A=0, CK_B =0;

	//받은 데이터의 앞의 ub는 체크섬 영역이 아니고 마지막 2바이트의 데이터도 체크섬이기에 i를 다음과 설정한다.
	for(int i=2; i<len-2; i++)
	{
		CK_A = CK_A + data[i];
		CK_B = CK_B + CK_A;
	}

	//계산된 체크섬이 data 포인터가 가리키는 임시버퍼 변수의 35번과 36번 변수와 같은지 체크하면된다.
	return ((CK_A==data[len-2])&&(CK_B==data[len-1]));//둘다 맞다면 1이 틀리면 0이 반환된다.
}

void M8N_UBX_NAV_POSLLH_Parsing(unsigned char* data, M8N_UBX_NAV_POSLLH* posllh)
{
	//Sync char를 건너뛴 2부터 받아야한다.
	posllh->CLASS = data[2];
	posllh->ID =  data[3];
	//Length는 little endian 으로 들어오기에 먼저 들어온 데이터가 낮은 자리를 가리킨다.
	posllh->LENGTH = data[4] | data[5]<<8;//받은 데이터의 비트를 밀고 or로 합친다.

	posllh->iTOW = data[6] | data[7]<<8 | data[8]<<16 | data[9]<<24;
	posllh->lon = data[10] | data[11]<<8 | data[12]<<16 | data[13]<<24;
	posllh->lat = data[14] | data[15]<<8 | data[16]<<16 | data[17]<<24;
	posllh->height = data[18] | data[19]<<8 | data[20]<<16 | data[21]<<24;
	posllh->hMSL = data[22] | data[23]<<8 | data[24]<<16 | data[25]<<24;
	posllh->hAcc = data[26] | data[27]<<8 | data[28]<<16 | data[29]<<24;
	posllh->vAcc = data[30] | data[31]<<8 | data[32]<<16 | data[33]<<24;

//	posllh->lon_f64 = posllh->lon / 10000000.;//.을 반드시 붙여서 실수로 저장해야한다.
//	posllh->lat_f64 = posllh->lat / 10000000.;//.을 반드시 붙여서 실수로 저장해야한다.

}
