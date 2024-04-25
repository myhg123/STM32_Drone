/*
 * AT24C08.h
 *
 *  Created on: Apr 10, 2024
 *      Author: myhg1
 */

#ifndef INC_AT24C08_H_
#define INC_AT24C08_H_

#include "i2c.h"

typedef union _Parser{
	unsigned char byte[4];
	float f;
}Parser;

void AT24C08_Page_Write(unsigned char page, unsigned char* data, unsigned char len);
void AT24C08_Page_Read(unsigned char page, unsigned char* data, unsigned char len);
unsigned char EP_PIDGain_Read(unsigned char id,float* PGain, float* IGain, float* DGain);
void EP_PIDGain_Write(unsigned char id,float PGain, float IGain, float DGain);

#endif /* INC_AT24C08_H_ */
