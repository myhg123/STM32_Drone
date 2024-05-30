/*
 * FS-iA6B.h
 *
 *  Created on: Apr 6, 2024
 *      Author: myhg1
 */
#ifndef __FSIA6B_H__
#define __FSIA6B_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"


typedef struct _FSiA6B_iBus{
	unsigned short RH;
	unsigned short RV;
	unsigned short LH;
	unsigned short LV;
	unsigned short SwA;
	unsigned short SwB;
	unsigned short SwC;
	unsigned short SwD;
	unsigned short VrA;
	unsigned short VrB;

	unsigned char Failsafe;
}FSiA6B_iBus;

extern FSiA6B_iBus iBus;

unsigned char iBus_Check_CHKSUM(unsigned char* data, unsigned char len);
void iBus_Parsing(unsigned char* data, FSiA6B_iBus* iBus);
void FSiA6B_UART5_Initialization(void);
unsigned char iBus_isActiveFailsafe(FSiA6B_iBus* iBus);

#ifdef __cplusplus
}
#endif

#endif /* __FSIA6B_H__ */
