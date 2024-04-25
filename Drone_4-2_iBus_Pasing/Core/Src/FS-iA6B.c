/*
 * FS-iA6B.c
 *
 *  Created on: Apr 6, 2024
 *      Author: myhg1
 */

#include "FS-iA6B.h"

unsigned char iBus_Check_CHKSUM(unsigned char* data, unsigned char len)
{
	unsigned short Chksum=0xffff;
	for(int i=0; i<len-2 ;i++)
	{
		Chksum -= data[i];
	}
	return(((Chksum&0x00ff)==data[30]) && ((Chksum>>8)==data[31]));
}
