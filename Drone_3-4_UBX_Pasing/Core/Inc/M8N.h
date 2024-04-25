/*
 * M8N.h
 *
 *  Created on: Apr 4, 2024
 *      Author: myhg1
 */

#ifndef INC_M8N_H_
#define INC_M8N_H_

typedef struct _M8N_UBX_NAV_POSLLH{
	unsigned char CLASS;
	unsigned char ID;
	unsigned short LENGTH;

	unsigned int iTOW;
	signed int lon;
	signed int lat;
	signed int height;
	signed int hMSL;
	unsigned int hAcc;
	unsigned int vAcc;

	double lon_f64;
	double lat_f64;

}M8N_UBX_NAV_POSLLH;

unsigned char M8N_UBX_CHKSUM_check(unsigned char* data, unsigned char len);
void M8N_UBX_NAV_POSLLH_Parsing(unsigned char* data, M8N_UBX_NAV_POSLLH* posllh);

//구조체 변수선언 main에서 쓰기위해 extern사용
extern M8N_UBX_NAV_POSLLH posllh;


#endif /* INC_M8N_H_ */
