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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "BNO080.h"
#include "Quaternion.h"
#include "ICM20602.h"
#include "LPS22HH.h"
#include "M8N.h"
#include "FS-iA6B.h"
#include "AT24C08.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

int _write(int file, char *p, int len) {
	HAL_UART_Transmit_IT(&huart6, p, len);
	return len;
}

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t uart6_rxData;
uint8_t uart5_rxData;
uint8_t uart4_rxData;
uint8_t uart1_rxData;

uint8_t m8n_rx_buf[36]; //m8n 1frame data size
uint8_t m8n_rx_cplt_flag = 0;

uint8_t ibus_rx_buf[32];
uint8_t ibus_rx_cplt_flag = 0;

float batVolt;

float roll_in_kp;
float roll_in_ki;
float roll_in_kd;

float roll_out_kp;
float roll_out_ki;
float roll_out_kd;

float pitch_in_kp;
float pitch_in_ki;
float pitch_in_kd;

float pitch_out_kp;
float pitch_out_ki;
float pitch_out_kd;

float yaw_heading_kp;
float yaw_heading_ki;
float yaw_heading_kd;

float yaw_rate_kp;
float yaw_rate_ki;
float yaw_rate_kd;

uint8_t telemetry_tx_buf[40];
uint8_t telemetry_rx_buf[40];
uint8_t telemetry_rx_cplt_flag = 0;
uint8_t tim7_20ms_flag = 0;
uint8_t tim7_100ms_flag = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
int Is_iBus_Throttle_Min(void);
void ESC_Calibration(void);
int Is_iBus_Received();
void BNO080_Calibration(void);
void Encode_Msg_AHRS(uint8_t *telemetry_tx_buf);
void Encode_Msg_GPS(uint8_t *telemetry_tx_buf);
void Encode_Msg_PID_Gain(uint8_t *telemetry_tx_buf, uint8_t id, float p,
		float i, float d);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */
	float q[4];
	float quatRadianAccuracy;
	unsigned char buf_read[16] = { 0 };
	unsigned char buf_write[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
			14, 15, 16 };
	unsigned short adcVal;
	//find offset data and init
	short gyro_x_offset = 3, gyro_y_offset = 10, gyro_z_offset = -3;

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_TIM3_Init();
	MX_USART6_UART_Init();
	MX_SPI2_Init();
	MX_SPI1_Init();
	MX_SPI3_Init();
	MX_UART4_Init();
	MX_UART5_Init();
	MX_TIM5_Init();
	MX_I2C1_Init();
	MX_ADC1_Init();
	MX_USART1_UART_Init();
	MX_TIM7_Init();
	/* USER CODE BEGIN 2 */

	M8N_initialization();
	HAL_UART_Receive_IT(&huart1, &uart1_rxData, 1);
	HAL_UART_Receive_IT(&huart6, &uart6_rxData, 1);
	HAL_UART_Receive_IT(&huart4, &uart4_rxData, 1);
	HAL_UART_Receive_IT(&huart5, &uart5_rxData, 1);

	BNO080_Initialization();
	BNO080_enableRotationVector(2500);

	ICM20602_Initialization();
	LPS22HH_Initialization();

	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4);

	HAL_TIM_Base_Start_IT(&htim7);

	// pdata?�� ?��?��?���??? ???��?�� 주소
	HAL_ADC_Start_DMA(&hadc1, &adcVal, 1);

	ICM20602_Writebyte(0x13, (gyro_x_offset * -2) >> 8);
	ICM20602_Writebyte(0x14, (gyro_x_offset * -2));

	ICM20602_Writebyte(0x15, (gyro_y_offset * -2) >> 8);
	ICM20602_Writebyte(0x16, (gyro_y_offset * -2));

	ICM20602_Writebyte(0x17, (gyro_x_offset * -2) >> 8);
	ICM20602_Writebyte(0x18, (gyro_x_offset * -2));

	EP_PIDGain_Read(0, &roll_in_kp, &roll_in_ki, &roll_in_kd);
	Encode_Msg_PID_Gain(&telemetry_tx_buf[0], 0, roll_in_kp, roll_in_ki,
			roll_in_kd);
	HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20, 10);

	EP_PIDGain_Read(1, &roll_out_kp, &roll_out_ki, &roll_out_kd);
	Encode_Msg_PID_Gain(&telemetry_tx_buf[0], 1, roll_out_kp, roll_out_ki,
			roll_out_kd);
	HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20, 10);

	EP_PIDGain_Read(2, &pitch_in_kp, &pitch_in_ki, &pitch_in_kd);
	Encode_Msg_PID_Gain(&telemetry_tx_buf[0], 2, pitch_in_kp, pitch_in_ki,
			pitch_in_kd);
	HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20, 10);

	EP_PIDGain_Read(3, &pitch_out_kp, &pitch_out_ki, &pitch_out_kd);
	Encode_Msg_PID_Gain(&telemetry_tx_buf[0], 3, pitch_out_kp, pitch_out_ki,
			pitch_out_kd);
	HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20, 10);

	EP_PIDGain_Read(4, &yaw_heading_kp, &yaw_heading_ki, &yaw_heading_kd);
	Encode_Msg_PID_Gain(&telemetry_tx_buf[0], 4, yaw_heading_kp, yaw_heading_ki,
			yaw_heading_kd);
	HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20, 10);

	EP_PIDGain_Read(5, &yaw_rate_kp, &yaw_rate_ki, &yaw_rate_kd);
	Encode_Msg_PID_Gain(&telemetry_tx_buf[0], 5, yaw_rate_kp, yaw_rate_ki,
			yaw_rate_kd);
	HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20, 10);

	// adcVal = ADC1->DR; ?�� 코드�??? ?��?��?�� ?��?���??? ?��?���???�??? ?��?��.

//	while(Is_iBus_Received()==0)
//	{
//		HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
//		htim3.Instance->PSC = 3000;
//		HAL_Delay(200);
//		HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
//		HAL_Delay(200);
//	}

//	if(iBus.SwC==2000){
//		HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
//		htim3.Instance->PSC = 1500;
//		HAL_Delay(200);
//		htim3.Instance->PSC = 2000;
//		HAL_Delay(200);
//		htim3.Instance->PSC = 1500;
//		HAL_Delay(200);
//		htim3.Instance->PSC = 2000;
//		HAL_Delay(200);
//		HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
//
//		ESC_Calibration();
//		while(iBus.SwC != 1000)
//		{
//			HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
//			htim3.Instance->PSC = 1500;
//			HAL_Delay(200);
//			htim3.Instance->PSC = 2000;
//			HAL_Delay(200);
//			HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
//
//			Is_iBus_Received();
//		}
//	}
//
//	else if(iBus.SwC == 1500)
//	{
//		HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
//		htim3.Instance->PSC = 1500;
//		HAL_Delay(200);
//		htim3.Instance->PSC = 2000;
//		HAL_Delay(200);
//		htim3.Instance->PSC = 1500;
//		HAL_Delay(200);
//		htim3.Instance->PSC = 2000;
//		HAL_Delay(200);
//		HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
//
//		BNO080_Calibration();
//
//		while(iBus.SwC != 1000)
//		{
//			HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
//			htim3.Instance->PSC = 1500;
//			HAL_Delay(200);
//			htim3.Instance->PSC = 2000;
//			HAL_Delay(200);
//			HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
//
//			Is_iBus_Received();
//		}
//
//	}
//
//	while (Is_iBus_Throttle_Min() == 0){
//		HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
//		htim3.Instance->PSC = 1000;
//		HAL_Delay(70);
//		HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
//		HAL_Delay(70);
//	}

	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

	htim3.Instance->PSC = 2000;
	HAL_Delay(100);
	htim3.Instance->PSC = 1500;
	HAL_Delay(100);
	htim3.Instance->PSC = 1000;
	HAL_Delay(100);

	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
//	printf("start\n");
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		if (telemetry_rx_cplt_flag == 1) {
			telemetry_rx_cplt_flag = 0;

			if (iBus.SwA == 1000) {

				unsigned char chksum = 0xff;
				for (int i = 0; i < 19; i++)
					chksum = chksum - telemetry_rx_buf[i];
				if (chksum == telemetry_rx_buf[19]) {
						HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

	htim3.Instance->PSC = 1000;
	HAL_Delay(10);

	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);

					switch (telemetry_rx_buf[2]) {
					case 0:
						roll_in_kp = *(float*) &telemetry_rx_buf[3];
						roll_in_ki = *(float*) &telemetry_rx_buf[7];
						roll_in_kd = *(float*) &telemetry_rx_buf[11];
						EP_PIDGain_Write(telemetry_rx_buf[2], roll_in_kp,
								roll_in_ki, roll_in_kd);
						EP_PIDGain_Read(telemetry_rx_buf[2], &roll_in_kp,
								&roll_in_ki, &roll_in_kd);
						Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
								telemetry_rx_buf[2], roll_in_kp, roll_in_ki,
								roll_in_kd);
						HAL_UART_Transmit_IT(&huart1, &telemetry_tx_buf[0], 20);
						break;

					case 1:
						roll_out_kp = *(float*) &telemetry_rx_buf[3];
						roll_out_ki = *(float*) &telemetry_rx_buf[7];
						roll_out_kd = *(float*) &telemetry_rx_buf[11];
						EP_PIDGain_Write(telemetry_rx_buf[2], roll_out_kp,
								roll_out_ki, roll_out_kd);
						EP_PIDGain_Read(telemetry_rx_buf[2], &roll_out_kp,
								&roll_out_ki, &roll_out_kd);
						Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
								telemetry_rx_buf[2], roll_out_kp, roll_out_ki,
								roll_out_kd);
						HAL_UART_Transmit_IT(&huart1, &telemetry_tx_buf[0], 20);
						break;

					case 2:
						pitch_in_kp = *(float*) &telemetry_rx_buf[3];
						pitch_in_ki = *(float*) &telemetry_rx_buf[7];
						pitch_in_kd = *(float*) &telemetry_rx_buf[11];
						EP_PIDGain_Write(telemetry_rx_buf[2], pitch_in_kp,
								pitch_in_ki, pitch_in_kd);
						EP_PIDGain_Read(telemetry_rx_buf[2], &pitch_in_kp,
								&pitch_in_ki, &pitch_in_kd);
						Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
								telemetry_rx_buf[2], pitch_in_kp, pitch_in_ki,
								pitch_in_kd);
						HAL_UART_Transmit_IT(&huart1, &telemetry_tx_buf[0], 20);
						break;

					case 3:
						pitch_out_kp = *(float*) &telemetry_rx_buf[3];
						pitch_out_ki = *(float*) &telemetry_rx_buf[7];
						pitch_out_kd = *(float*) &telemetry_rx_buf[11];
						EP_PIDGain_Write(telemetry_rx_buf[2], pitch_out_kp,
								pitch_out_ki, pitch_out_kd);
						EP_PIDGain_Read(telemetry_rx_buf[2], &pitch_out_kp,
								&pitch_out_ki, &pitch_out_kd);
						Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
								telemetry_rx_buf[2], pitch_out_kp, pitch_out_ki,
								pitch_out_kd);
						HAL_UART_Transmit_IT(&huart1, &telemetry_tx_buf[0], 20);
						break;

					case 4:
						yaw_heading_kp = *(float*) &telemetry_rx_buf[3];
						yaw_heading_ki = *(float*) &telemetry_rx_buf[7];
						yaw_heading_kd = *(float*) &telemetry_rx_buf[11];
						EP_PIDGain_Write(telemetry_rx_buf[2], yaw_heading_kp,
								yaw_heading_ki, yaw_heading_kd);
						EP_PIDGain_Read(telemetry_rx_buf[2], &yaw_heading_kp,
								&yaw_heading_ki, &yaw_heading_kd);
						Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
								telemetry_rx_buf[2], yaw_heading_kp,
								yaw_heading_ki, yaw_heading_kd);
						HAL_UART_Transmit_IT(&huart1, &telemetry_tx_buf[0], 20);
						break;

					case 5:
						yaw_rate_kp = *(float*) &telemetry_rx_buf[3];
						yaw_rate_ki = *(float*) &telemetry_rx_buf[7];
						yaw_rate_kd = *(float*) &telemetry_rx_buf[11];
						EP_PIDGain_Write(telemetry_rx_buf[2], yaw_rate_kp,
								yaw_rate_ki, yaw_rate_kd);
						EP_PIDGain_Read(telemetry_rx_buf[2], &yaw_rate_kp,
								&yaw_rate_ki, &yaw_rate_kd);
						Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
								telemetry_rx_buf[2], yaw_rate_kp, yaw_rate_ki,
								yaw_rate_kd);
						HAL_UART_Transmit_IT(&huart1, &telemetry_tx_buf[0], 20);
						break;
					case 0x10:
						switch (telemetry_rx_buf[3]) {
						case 0:
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									telemetry_tx_buf[3], roll_in_kp, roll_in_ki,
									roll_in_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							break;

						case 1:
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									telemetry_tx_buf[3], roll_out_kp,
									roll_out_ki, roll_out_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							break;

						case 2:
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									telemetry_tx_buf[3], pitch_in_kp,
									pitch_in_ki, pitch_in_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							break;

						case 3:
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									telemetry_tx_buf[3], pitch_out_kp,
									pitch_out_ki, pitch_out_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							break;

						case 4:
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									telemetry_tx_buf[3], yaw_heading_kp,
									yaw_heading_ki, yaw_heading_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							break;

						case 5:
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									telemetry_tx_buf[3], yaw_rate_kp,
									yaw_rate_ki, yaw_rate_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							break;
						case 6:
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									0, roll_in_kp, roll_in_ki,
									roll_in_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									1, roll_out_kp,
									roll_out_ki, roll_out_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									2, pitch_in_kp,
									pitch_in_ki, pitch_in_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									3, pitch_out_kp,
									pitch_out_ki, pitch_out_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									4, yaw_heading_kp,
									yaw_heading_ki, yaw_heading_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);
							Encode_Msg_PID_Gain(&telemetry_tx_buf[0],
									5, yaw_rate_kp,
									yaw_rate_ki, yaw_rate_kd);
							HAL_UART_Transmit(&huart1, &telemetry_tx_buf[0], 20,
									10);

						break;
						}
						break;
					}
				}
			}
		}

		if (tim7_20ms_flag == 1 && tim7_100ms_flag != 1) {
			tim7_20ms_flag = 0;
			Encode_Msg_AHRS(&telemetry_tx_buf[0]);
			HAL_UART_Transmit_IT(&huart1, &telemetry_tx_buf[0], 20);
		} else if (tim7_20ms_flag == 1 && tim7_100ms_flag == 1) {
			tim7_100ms_flag = 0;
			tim7_20ms_flag = 0;
			Encode_Msg_AHRS(&telemetry_tx_buf[0]);
			Encode_Msg_GPS(&telemetry_tx_buf[20]);
			HAL_UART_Transmit_IT(&huart1, &telemetry_tx_buf[0], 40);
		}

		batVolt = adcVal * 0.003619f;
		//printf("%d\t%.2f\n",adcVal, batVolt);
		if (batVolt < 10.0f) {
			htim3.Instance->PSC = 1000;
			HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
		} else {
			HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
		}

		if (BNO080_dataAvailable() == 1) {
			q[0] = BNO080_getQuatI();
			q[1] = BNO080_getQuatJ();
			q[2] = BNO080_getQuatK();
			q[3] = BNO080_getQuatReal();
			quatRadianAccuracy = BNO080_getQuatAccuracy();

			Quaternion_Update(&q[0]);

//			printf("%.2f,%.2f,%.2f\n", BNO080_Roll, BNO080_Pitch, BNO080_Yaw);
		}

		if (ICM20602_DataReady() == 1) {
			ICM20602_Get3AxisGyroRawData(&ICM20602.gyro_x_raw);

			ICM20602.gyro_x = ICM20602.gyro_x_raw * 2000.f / 32768.f;
			ICM20602.gyro_y = ICM20602.gyro_y_raw * 2000.f / 32768.f;
			ICM20602.gyro_z = ICM20602.gyro_z_raw * 2000.f / 32768.f;

//		  printf("%.4f,%.4f,%.4f\n", ICM20602.gyro_x_raw, ICM20602.gyro_y_raw,ICM20602.gyro_z_raw);
		}
		if (LPS22HH_DataReady() == 1) {
			LPS22HH_GetPressure(&LPS22HH.pressure_raw);
			LPS22HH_GetTemperature(&LPS22HH.temperature_raw);

			LPS22HH.baroAlt = getAltitude2(LPS22HH.pressure_raw / 4096.f,
					LPS22HH.temperature_raw / 100.f);

#define X 0.90f

			LPS22HH.baroAltFilt = LPS22HH.baroAltFilt * X
					+ LPS22HH.baroAlt * (1.0f - X);

//		  printf("%d,%d\n",(int)(LPS22HH.baroAlt*100), (int)(LPS22HH.baroAltFilt*100));
		}

		if (m8n_rx_cplt_flag == 1) {
			m8n_rx_cplt_flag = 0;
			if (M8N_UBX_CHKSUM_check(&m8n_rx_buf[0], 36) == 1) //Checksum Same
					{
				HAL_GPIO_TogglePin(LED_Blue_GPIO_Port, LED_Blue_Pin);
				M8N_UBX_NAV_POSLLH_Parsing(&m8n_rx_buf[0], &posllh);

//				printf("LAT: %ld\tLON: %ld\tHeight: %ld\n", posllh.lat,	posllh.lon, posllh.height);
			}
		}

		if (ibus_rx_cplt_flag == 1) {
			ibus_rx_cplt_flag = 0;
			if (iBus_Check_CHKSUM(&ibus_rx_buf[0], sizeof(ibus_rx_buf))) {
				iBus_Parsing(&ibus_rx_buf[0], &iBus);
				if (iBus_isActiveFailsafe(&iBus) == 1) {
					HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
				} else {
					HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
				}
//				printf("%d\t%d\t%d\t%d\t%d\t%d\n",iBus.RH,iBus.RV,iBus.LH,iBus.LV,iBus.SwA,iBus.SwC);
//				HAL_Delay(100);
			}
		}
//		tim5_ccr4 += 10;
//		if(tim5_ccr4>21000) tim5_ccr4 = 10500;
//		TIM5->CCR4 = tim5_ccr4;
//		HAL_Delay(1);
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */
int Is_iBus_Throttle_Min(void) {
	if (ibus_rx_cplt_flag == 1) {
		ibus_rx_cplt_flag = 0;
		if (iBus_Check_CHKSUM(&ibus_rx_buf[0], sizeof(ibus_rx_buf))) {

			iBus_Parsing(&ibus_rx_buf[0], &iBus);
			if (iBus.LV < 1010)
				return 1;
		}
	}
	return 0;
}

void ESC_Calibration(void) {
	TIM5->CCR1 = 21000;
	TIM5->CCR2 = 21000;
	TIM5->CCR3 = 21000;
	TIM5->CCR4 = 21000;
	HAL_Delay(7000);

	TIM5->CCR1 = 10500;
	TIM5->CCR2 = 10500;
	TIM5->CCR3 = 10500;
	TIM5->CCR4 = 10500;
	HAL_Delay(8000);
}

int Is_iBus_Received() {
	if (ibus_rx_cplt_flag == 1) {
		ibus_rx_cplt_flag = 0;
		if (iBus_Check_CHKSUM(&ibus_rx_buf[0], sizeof(ibus_rx_buf))) {

			iBus_Parsing(&ibus_rx_buf[0], &iBus);
			return 1;
		}
	}
	return 0;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

	static unsigned char cnt = 0;
	static unsigned char cnt_ibus = 0;
	static unsigned char cnt_Rxdata = 0;
	if (huart->Instance == USART1) {
		switch (cnt_Rxdata) {
		case 0:
			if (uart1_rxData == 0x47) {
				telemetry_rx_buf[cnt_Rxdata] = uart1_rxData;
				cnt_Rxdata++;
			}
			break;
		case 1:
			if (uart1_rxData == 0x53) {
				telemetry_rx_buf[cnt_Rxdata] = uart1_rxData;
				cnt_Rxdata++;
			} else
				cnt_Rxdata = 0;
			break;
		case 19:
			telemetry_rx_buf[cnt_Rxdata] = uart1_rxData;
			cnt_Rxdata = 0;
			telemetry_rx_cplt_flag = 1;
			break;
		default:
			telemetry_rx_buf[cnt_Rxdata] = uart1_rxData;
			cnt_Rxdata++;
			break;
		}
		HAL_UART_Receive_IT(&huart1, &uart1_rxData, 1);

	}

	if (huart->Instance == UART5) {
		HAL_UART_Receive_IT(&huart5, &uart5_rxData, 1);

		//HAL_UART_Transmit_IT(&huart6, uart5_rxData, 1);

		switch (cnt_ibus) {
		case 0:
			if (uart5_rxData == 0x20) {
				ibus_rx_buf[cnt_ibus] = uart5_rxData;
				cnt_ibus++;
			}
			break;

		case 1:
			if (uart5_rxData == 0x40) {
				ibus_rx_buf[cnt_ibus] = uart5_rxData;
				cnt_ibus++;
			} else {
				cnt_ibus = 0;
			}
			break;

		case 31:
			ibus_rx_buf[cnt_ibus] = uart5_rxData;
			cnt_ibus = 0;
			ibus_rx_cplt_flag = 1;
			break;

		default:
			ibus_rx_buf[cnt_ibus] = uart5_rxData;
			cnt_ibus++;
			break;
		}

	}
	if (huart->Instance == UART4) {

		HAL_UART_Receive_IT(&huart4, &uart4_rxData, 1);

		//GPS�???????? ?��?�� ?��?��받�? ?��?��?���???????? 바로 com?���???????? 보내�????????기에 주석처리?��?��.
		//HAL_UART_Transmit(&huart6, &uart4_rxData, 1, 0);

		switch (cnt) {
		case 0:
			if (uart4_rxData == 0xb5) {
				m8n_rx_buf[cnt] = uart4_rxData;
				cnt++;
			}
			break;
		case 1:
			if (uart4_rxData == 0x62) {
				m8n_rx_buf[cnt] = uart4_rxData;
				cnt++;
			} else
				cnt = 0;
			break;
		case 35:
			m8n_rx_buf[cnt] = uart4_rxData;
			cnt = 0;
			m8n_rx_cplt_flag = 1;
		default: //case?�� 만족?���???????? ?��?��?�� ?��?��?��?��.
			m8n_rx_buf[cnt] = uart4_rxData;
			cnt++;
			break;
		}
	}

	if (huart->Instance == USART6) {
		HAL_UART_Receive_IT(&huart6, &uart6_rxData, 1);
//		HAL_UART_Transmit_IT(&huart4, &uart6_rxData, 1);
	}
}

void BNO080_Calibration(void) {
	//Resets BNO080 to disable All output
	BNO080_Initialization();

	//BNO080/BNO085 Configuration
	//Enable dynamic calibration for accelerometer, gyroscope, and magnetometer
	//Enable Game Rotation Vector output
	//Enable Magnetic Field output
	BNO080_calibrateAll(); //Turn on cal for Accel, Gyro, and Mag
	BNO080_enableGameRotationVector(20000); //Send data update every 20ms (50Hz)
	BNO080_enableMagnetometer(20000); //Send data update every 20ms (50Hz)

	//Once magnetic field is 2 or 3, run the Save DCD Now command
	printf(
			"Calibrating BNO080. Pull up FS-i6 SWC to end calibration and save to flash\n");
	printf("Output in form x, y, z, in uTesla\n\n");

	//while loop for calibration procedure
	//Iterates until iBus.SwC is mid point (1500)
	//Calibration procedure should be done while this loop is in iteration.
	while (iBus.SwC == 1500) {
		if (BNO080_dataAvailable() == 1) {
			//Observing the status bit of the magnetic field output
			float x = BNO080_getMagX();
			float y = BNO080_getMagY();
			float z = BNO080_getMagZ();
			unsigned char accuracy = BNO080_getMagAccuracy();

			float quatI = BNO080_getQuatI();
			float quatJ = BNO080_getQuatJ();
			float quatK = BNO080_getQuatK();
			float quatReal = BNO080_getQuatReal();
			unsigned char sensorAccuracy = BNO080_getQuatAccuracy();

			printf("%f,%f,%f,", x, y, z);
			if (accuracy == 0)
				printf("Unreliable\t");
			else if (accuracy == 1)
				printf("Low\t");
			else if (accuracy == 2)
				printf("Medium\t");
			else if (accuracy == 3)
				printf("High\t");

			printf("\t%f,%f,%f,%f,", quatI, quatI, quatI, quatReal);
			if (sensorAccuracy == 0)
				printf("Unreliable\n");
			else if (sensorAccuracy == 1)
				printf("Low\n");
			else if (sensorAccuracy == 2)
				printf("Medium\n");
			else if (sensorAccuracy == 3)
				printf("High\n");

			//Turn the LED and buzzer on when both accuracy and sensorAccuracy is high
			if (accuracy == 3 && sensorAccuracy == 3) {
				HAL_GPIO_WritePin(GPIOC,
				LED_Blue_Pin | LED_Green_Pin | LED_Red_Pin, 1);
				htim3.Instance->PSC = 65000; //Very low frequency
				HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

			} else {
				HAL_GPIO_WritePin(GPIOC,
				LED_Blue_Pin | LED_Green_Pin | LED_Red_Pin, 0);

				HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
			}
		}

		Is_iBus_Received(); //Refreshes iBus Data for iBus.SwC
		HAL_Delay(100);
	}

	//Ends the loop when iBus.SwC is not mid point
	//Turn the LED and buzzer off
	HAL_GPIO_WritePin(GPIOC, LED_Blue_Pin | LED_Green_Pin | LED_Red_Pin, 0);

	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);

	//Saves the current dynamic calibration data (DCD) to memory
	//Sends command to get the latest calibration status
	BNO080_saveCalibration();
	BNO080_requestCalibrationStatus();

	//Wait for calibration response, timeout if no response
	int counter = 100;
	while (1) {
		if (--counter == 0)
			break;
		if (BNO080_dataAvailable()) {
			//The IMU can report many different things. We must wait
			//for the ME Calibration Response Status byte to go to zero
			if (BNO080_calibrationComplete() == 1) {
				printf("\nCalibration data successfully stored\n");
				HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
				TIM3->PSC = 2000;
				HAL_Delay(300);
				TIM3->PSC = 1500;
				HAL_Delay(300);
				HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
				HAL_Delay(1000);
				break;
			}
		}
		HAL_Delay(10);
	}
	if (counter == 0) {
		printf("\nCalibration data failed to store. Please try again.\n");
		HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
		TIM3->PSC = 1500;
		HAL_Delay(300);
		TIM3->PSC = 2000;
		HAL_Delay(300);
		HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
		HAL_Delay(1000);
	}

	//BNO080_endCalibration(); //Turns off all calibration
	//In general, calibration should be left on at all times. The BNO080
	//auto-calibrates and auto-records cal data roughly every 5 minutes

	//Resets BNO080 to disable Game Rotation Vector and Magnetometer
	//Enables Rotation Vector
	BNO080_Initialization();
	BNO080_enableRotationVector(2500); //Send data update every 2.5ms (400Hz)
}

void Encode_Msg_AHRS(uint8_t *telemetry_tx_buf) {

	telemetry_tx_buf[0] = 0x46;
	telemetry_tx_buf[1] = 0x43;

	telemetry_tx_buf[2] = 0x10;

	telemetry_tx_buf[3] = (short) (BNO080_Roll * 100);
	telemetry_tx_buf[4] = ((short) (BNO080_Roll * 100)) >> 8;

	telemetry_tx_buf[5] = (short) (BNO080_Pitch * 100);
	telemetry_tx_buf[6] = ((short) (BNO080_Pitch * 100)) >> 8;

	telemetry_tx_buf[7] = (unsigned short) (BNO080_Yaw * 100);
	telemetry_tx_buf[8] = ((unsigned short) (BNO080_Yaw * 100)) >> 8;

	telemetry_tx_buf[9] = (short) (LPS22HH.baroAltFilt * 10);
	telemetry_tx_buf[10] = ((short) (LPS22HH.baroAltFilt * 10)) >> 8;

	telemetry_tx_buf[11] = (short) ((iBus.RH - 1500) * 0.1f * 100);
	telemetry_tx_buf[12] = ((short) ((iBus.RH - 1500) * 0.1f * 100)) >> 8;

	telemetry_tx_buf[13] = (short) ((iBus.RV - 1500) * 0.1f * 100);
	telemetry_tx_buf[14] = ((short) ((iBus.RV - 1500) * 0.1f * 100)) >> 8;

	telemetry_tx_buf[15] = (unsigned short) ((iBus.LH - 1000) * 0.36f * 100);
	telemetry_tx_buf[16] = ((unsigned short) ((iBus.LH - 1000) * 0.36f * 100))
			>> 8;

	telemetry_tx_buf[17] = (short) iBus.LV*10;
	telemetry_tx_buf[18] = ((short)(iBus.LV*10))>>8;


	telemetry_tx_buf[19] = 0xff;

	for (int i = 0; i < 19; i++)
		telemetry_tx_buf[19] = telemetry_tx_buf[19] - telemetry_tx_buf[i];

}

void Encode_Msg_GPS(uint8_t *telemetry_tx_buf) {
	telemetry_tx_buf[0] = 0x46;
	telemetry_tx_buf[1] = 0x43;

	telemetry_tx_buf[2] = 0x11;

	telemetry_tx_buf[3] = posllh.lat;
	telemetry_tx_buf[4] = posllh.lat >> 8;

	telemetry_tx_buf[5] = posllh.lat >> 16;
	telemetry_tx_buf[6] = posllh.lat >> 24;

	telemetry_tx_buf[7] = posllh.lon;
	telemetry_tx_buf[8] = posllh.lon >> 8;

	telemetry_tx_buf[9] = posllh.lon >> 16;
	telemetry_tx_buf[10] = posllh.lon >> 24;

	telemetry_tx_buf[11] = (unsigned short) (batVolt * 100);
	telemetry_tx_buf[12] = ((unsigned short) (batVolt * 100)) >> 8;

	telemetry_tx_buf[13] = iBus.SwA == 1000 ? 0 : 1;
	telemetry_tx_buf[14] = iBus.SwC == 1000 ? 0 : iBus.SwC == 1500 ? 1 : 2;

	telemetry_tx_buf[15] = iBus_isActiveFailsafe(&iBus);

	telemetry_tx_buf[16] = 0x00;
	telemetry_tx_buf[17] = 0x00;
	telemetry_tx_buf[18] = 0x00;

	telemetry_tx_buf[19] = 0xff;

	for (int i = 0; i < 19; i++)
		telemetry_tx_buf[19] = telemetry_tx_buf[19] - telemetry_tx_buf[i];

}

void Encode_Msg_PID_Gain(uint8_t *telemetry_tx_buf, uint8_t id, float p,
		float i, float d) {
	telemetry_tx_buf[0] = 0x46;
	telemetry_tx_buf[1] = 0x43;

	telemetry_tx_buf[2] = id;

//	memcpy(&telemetry_tx_buf[3], &p, 4);
//	memcpy(&telemetry_tx_buf[7], &i, 4);
//	memcpy(&telemetry_tx_buf[11], &d, 4);

	*(float*) &telemetry_tx_buf[3] = p;
	*(float*) &telemetry_tx_buf[7] = i;
	*(float*) &telemetry_tx_buf[11] = d;

	telemetry_tx_buf[15] = 0x00;
	telemetry_tx_buf[16] = 0x00;
	telemetry_tx_buf[17] = 0x00;
	telemetry_tx_buf[18] = 0x00;

	telemetry_tx_buf[19] = 0xff;

	for (int i = 0; i < 19; i++)
		telemetry_tx_buf[19] = telemetry_tx_buf[19] - telemetry_tx_buf[i];

}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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