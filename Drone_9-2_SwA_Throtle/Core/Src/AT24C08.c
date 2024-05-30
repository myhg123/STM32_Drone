/*
 * AT24C08.c
 *
 *  Created on: Apr 10, 2024
 *      Author: myhg1
 */
#include "AT24C08.h"

void AT24C08_Page_Write(unsigned char page, unsigned char* data, unsigned char len){
	//비트 연산을 잘계산 해보면
	unsigned char devAddress = ((page*16)>>8)<<1|0xA0;
	unsigned char wordAddress = (page*16)&0xff;

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	HAL_I2C_Mem_Write(&hi2c1, devAddress, wordAddress, I2C_MEMADD_SIZE_8BIT, &data[0], 16,
			1);
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void AT24C08_Page_Read(unsigned char page, unsigned char* data, unsigned char len){
	//비트 연산을 잘계산 해보면
	unsigned char devAddress = ((page*16)>>8)<<1|0xA0;
	unsigned char wordAddress = (page*16)&0xff;

	HAL_I2C_Mem_Read(&hi2c1, devAddress, wordAddress, I2C_MEMADD_SIZE_8BIT, &data[0], 16,
			1);
}

//EP 프로토콜을 이용한 함수
void EP_PIDGain_Write(unsigned char id,float PGain, float IGain, float DGain)
{
	unsigned char buf_write[16];
	//공용체 변수 선언
	Parser parser;

	//sync char와 id
	buf_write[0] = 0x45;
	buf_write[1] = 0x50;
	buf_write[2] = id;
	//Pgain 값
	parser.f = PGain;
	buf_write[3] = parser.byte[0];
	buf_write[4] = parser.byte[1];
	buf_write[5] = parser.byte[2];
	buf_write[6] = parser.byte[3];
	//Igain 값
	parser.f = IGain;
	buf_write[7] = parser.byte[0];
	buf_write[8] = parser.byte[1];
	buf_write[9] = parser.byte[2];
	buf_write[10] = parser.byte[3];
	//Dgain 값
	parser.f = DGain;
	buf_write[11] = parser.byte[0];
	buf_write[12] = parser.byte[1];
	buf_write[13] = parser.byte[2];
	buf_write[14] = parser.byte[3];

	//chksum
	unsigned char chksum = 0xff;
	for(int i=0; i<15; i++) chksum -= buf_write[i];

	buf_write[15] = chksum;

	//id에 따라 저장할 페이지가 바뀐다.

	switch(id)
	{
	case 0:
		AT24C08_Page_Write(0,&buf_write[0], 16);
		break;
	case 1:
		AT24C08_Page_Write(1,&buf_write[0], 16);
		break;
	case 2:
		AT24C08_Page_Write(2,&buf_write[0], 16);
		break;
	case 3:
		AT24C08_Page_Write(3,&buf_write[0], 16);
		break;
	case 4:
		AT24C08_Page_Write(4,&buf_write[0], 16);
		break;
	case 5:
		AT24C08_Page_Write(5,&buf_write[0], 16);
		break;
	}
}

//콜 바이 레퍼런스로 주소를 받아와서 해당 주소의 데이터를 저장해야한다. 그러므로 포인터로 선언
//그리고 잘 저장 됬는지 확인하기 위해 반환값을 주자
unsigned char EP_PIDGain_Read(unsigned char id,float* PGain, float* IGain, float* DGain){
	unsigned char buf_read[16];
	Parser parser;

	//먼저 역순으로 받아온다.
	switch(id)
	{
	case 0:
		AT24C08_Page_Read(0,&buf_read[0], 16);
		break;
	case 1:
		AT24C08_Page_Read(1,&buf_read[0], 16);
		break;
	case 2:
		AT24C08_Page_Read(2,&buf_read[0], 16);
		break;
	case 3:
		AT24C08_Page_Read(3,&buf_read[0], 16);
		break;
	case 4:
		AT24C08_Page_Read(4,&buf_read[0], 16);
		break;
	case 5:
		AT24C08_Page_Read(5,&buf_read[0], 16);
		break;
	}

	unsigned char chksum = 0xff;
	for(int i=0; i<15; i++) chksum -= buf_read[i];

	if(buf_read[15] == chksum && buf_read[0] == 0x45 && buf_read[1] == 0x50)
	{
		//공용체 멤버변수 f에 든값을 gain의 주소에 넣어주자.
		//Pgain
		parser.byte[0] = buf_read[3];
		parser.byte[1] = buf_read[4];
		parser.byte[2] = buf_read[5];
		parser.byte[3] = buf_read[6];
		*PGain = parser.f;
		parser.byte[0] = buf_read[7];
		parser.byte[1] = buf_read[8];
		parser.byte[2] = buf_read[9];
		parser.byte[3] = buf_read[10];
		*IGain = parser.f;
		parser.byte[0] = buf_read[11];
		parser.byte[1] = buf_read[12];
		parser.byte[2] = buf_read[13];
		parser.byte[3] = buf_read[14];
		*DGain = parser.f;

		//제대로 데이터가 읽힘
		return 0;
	}
	//데이터가 제대로 안 읽혔을때
	return 1;
}

