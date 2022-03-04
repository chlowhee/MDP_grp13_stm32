/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *sudo sync && sudo systemctl poweroff
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "math.h"
#include "PID.h"
#include <time.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CMPERREV 0.01666666667f;
#define PID_KP 20.0f
#define PID_KI 0.000f
#define PID_KD 0.0f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;

UART_HandleTypeDef huart3;

/* Definitions for LED_Toggle */
osThreadId_t LED_ToggleHandle;
const osThreadAttr_t LED_Toggle_attributes = {
  .name = "LED_Toggle",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for MotorTask */
osThreadId_t MotorTaskHandle;
const osThreadAttr_t MotorTask_attributes = {
  .name = "MotorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh7,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM5_Init(void);
void StartDefaultTask(void *argument);
void motor(void *argument);

/* USER CODE BEGIN PFP */
void fastestCar();
void PIDmotor(float,int);
int motorCont(int speedL, int speedR, char dirL, char dirR, double dist);
void degTurn(int);
void forward(int);
void reverse(int);
void spotTurn(int);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int16_t currentLeft, currentRight;
float fLeft,fRight;
int16_t diffl = 0, diffr = 0, avg = 0;
int32_t tick = 0;
uint8_t display[20];
/* Ultrasonic sensor */
uint32_t IC_Val1 = 0;
uint32_t IC_Val2 = 0;
uint32_t Difference = 0;
int Is_First_Captured = 0;  // boolean function
uint16_t Distance = 0;
uint16_t uDistCheck1 = 0; uDistCheck2 = 0; uDistFinal = 0;
uint8_t ultra[20];
/* UART */
uint8_t aRxBuffer[1];
/* IR */
//uint32_t ir1Dist = 0;
uint32_t ir1Dist = 0;
uint32_t ir2Dist = 0;
uint32_t adc1;
uint32_t adc2;
float outLeft =0;
float outRight=0;
float error=0;
float currDist;
float velocityLeft=0;
float velocityRight=0;
float distanceLeft=0;
float distanceRight=0;
float pwmLeft = 1800;
float pwmRight = 1500;

/* Timer variables */
float startSec;
float currSec;

void delay(uint16_t time){  //provide us delay
	__HAL_TIM_SET_COUNTER(&htim4, 0);
	while(__HAL_TIM_GET_COUNTER(&htim4) < time);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)		//For HCSR04_Read();
{
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)  // if the interrupt source is channel1
	{
		if (Is_First_Captured==0) // if the first value is not captured
		{
			IC_Val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1); // read the first value
			Is_First_Captured = 1;  // set the first captured as true
			// Now change the polarity to falling edge
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
		}

		else if (Is_First_Captured==1)   // if the first is already captured
		{
			IC_Val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);  // read second value
			__HAL_TIM_SET_COUNTER(htim, 0);  // reset the counter

			if (IC_Val2 > IC_Val1) {
				Difference = IC_Val2-IC_Val1;
			}

			else if (IC_Val1 > IC_Val2) {
				Difference = (0xffff - IC_Val1) + IC_Val2;
			}

			Distance = Difference * .034/2;
			Is_First_Captured = 0; // set it back to false

			// set polarity to rising edge
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
			__HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC1);
		}
	}
}

uint16_t HCSR04_Read (void)		//Read Ultrasonic Distance
{
	HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);  // pull the TRIG pin HIGH
	delay(10);  // wait for 10 us
	HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);  // pull the TRIG pin low

	__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC1);
	return Distance;
}

void ultraDistCheck (void)
{
	uDistCheck1 = HCSR04_Read();
	HAL_Delay(100);
	uDistCheck2 = HCSR04_Read();
	HAL_Delay(100);
	while (abs(uDistCheck1 - uDistCheck2) >= 5) { // || uDistCheck2 - uDistCheck1 >= 5) {
		uDistCheck1 = HCSR04_Read();
		HAL_Delay(100);
		uDistCheck2 = HCSR04_Read();
		HAL_Delay(100);
	}
	uDistFinal = (uDistCheck1 + uDistCheck2)/2;
}

uint32_t irLeft (void) { //ADC1 (a bit more wonky)
	adc1 = 0;
	float V = 0;
	HAL_ADC_Start(&hadc1);
	adc1 = HAL_ADC_GetValue(&hadc1);
	V = (float)adc1/1000;

	if (V <= 0.5) V = 0.5; //cap at 80 cm
	else if (V >= 2.84) V = 2.84; //cap at 10 cm


	ir1Dist = 34.96332 * pow(V, -1.19878);
	return ir1Dist;
}

uint32_t irRight (void) { //ADC2
	adc2 = 0;
	float V = 0;
	HAL_ADC_Start(&hadc2);
	adc2 = HAL_ADC_GetValue(&hadc2);
	V = (float)adc2/1000;

	if (V <= 0.42) V = 0.44; //cap at 80 cm
	else if (V >= 2.9) V = 2.95; //cap at 10 cm


	ir2Dist = 32.6167 * pow(V, -1.0928);
	return ir2Dist;
}

void waitCmd (void) {	//not complete
	//HAL_UART_Transmit_IT(&huart3,(uint8_t *)"OK",2);
	while (*aRxBuffer == 'Z') {
		//HAL_UART_Transmit_IT(&huart3,(uint8_t *)"OK",2);
		HAL_UART_Receive_IT(&huart3, (uint8_t *)aRxBuffer, 1);
		HAL_Delay(100);
	}
}

void fastestCar(){
	/* (1) Start straight line loop */
	while(1){
		/* Increase RPM until ??? */


		/* Break to new loop for slowing down */
		ultraDistCheck();
		if(uDistFinal<=40){
			break;
		}
	}

	/* (2) Slow down car to prepare for turning */
	while(1){
		//Slow down code

		ultraDistCheck();
		if(uDistFinal<=20){
			htim1.Instance->CCR4 = 104;
			HAL_Delay(1000);
			htim1.Instance->CCR4 = 73;
			break;
		}
	}

	/* (3) Move until near end of obstacle's corner */
	int x=0;
	while(1){
		/* Constant Speed */


		if(irLeft()>=30){
			htim1.Instance->CCR4 = 56;
			HAL_Delay(1000);
			htim1.Instance->CCR4 = 75;
			HAL_Delay(1000);
			htim1.Instance->CCR4 = 56;
			HAL_Delay(1000);
			htim1.Instance->CCR4 = 75;
			break;
		}

	}
	/* (4) Move 50cm */
	//Move

	/* (5) Move straight until centre of obstacle */
	//Calculate Dist to move
	//Move Dist

	//Turn Right

	/* (6) Move back to parking spot */
	while(1){
		//Move straight

		ultraDistCheck();
		if(uDistFinal<=20){
			//Stop
			break;
		}
	}

}

void preCorr(){
		while(uDistFinal>15 && uDistFinal<40){
			HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
			__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 600);
			__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 600);
			ultraDistCheck();
		}
		__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 0);
		__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 0);

}

void corrMotor(int mode){
	if(mode==1){
		while(uDistFinal>22 || uDistFinal>40){
			HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
			__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 600);
			__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 600);
			ultraDistCheck();

		}
		__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 0);
		__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 0);
	}
	else if(mode==2){
		while(uDistFinal<24){
			HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_SET);
			__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 600);
			__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 600);
			ultraDistCheck();
		}
		__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 0);
		__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 0);
	}
}

void correction(){
	*aRxBuffer = 'Z';
	irLeft();
	irRight();
	ultraDistCheck();
	if(uDistFinal>40){
		while(irLeft()<=35 && irRight()<=35){
			__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 600);
			__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 600);
		}
		__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 0);
		__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 0);
	}
	int mode = 0;
	if(irLeft()<=35 || irRight()<=35){
		if(irLeft<irRight)mode =1;
		else mode=2;
	}
	if(mode==1){
		preCorr();
		while(irLeft()<=35){
			htim1.Instance->CCR4 = 104;
			HAL_Delay(100);
			motorCont(1000, 1000, 'R', 'R', 1);
			htim1.Instance->CCR4 = 56;
			HAL_Delay(100);
			motorCont(1000, 1000, 'F', 'F', 1);
			irLeft();
			htim1.Instance->CCR4 = 76;
			HAL_Delay(50);
			ultraDistCheck();
			if(uDistFinal<15){
				motorCont(1000, 1000, 'R', 'R', 2);
				ultraDistCheck();
			}
			if(uDistFinal>15 && uDistFinal<40){
				motorCont(1000, 1000, 'F', 'F', 2);
			}
		}
		htim1.Instance->CCR4 = 76;
	}
	else if(mode==2){
		preCorr();
		while(irRight()<=35){
			htim1.Instance->CCR4 =56;

			HAL_Delay(100);
			motorCont(1000, 1000, 'R', 'R', 1);
			htim1.Instance->CCR4 = 104;
			HAL_Delay(100);
			motorCont(1000, 1000, 'F', 'F', 1);
			irRight();
			htim1.Instance->CCR4 = 73;
			HAL_Delay(50);
			ultraDistCheck();
			if(uDistFinal<15){
				motorCont(1000, 1000, 'R', 'R', 2);
				ultraDistCheck();
			}
			if(uDistFinal>15 && uDistFinal<40){
				motorCont(1000, 1000, 'F', 'F', 2);
			}
		}
		htim1.Instance->CCR4 = 73;
		HAL_Delay(50);
	}
	HAL_Delay(1000);
	if(uDistFinal>23 && uDistFinal<40){//Forward until 25
		corrMotor(1);
	}
	else if(uDistFinal<23){
		corrMotor(2);
	}
}


void motorSpeed(float left, float right){
	if(left<0){
		HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_SET);
		left = fabs(left);
	}
	else if(left>0){
		HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
	}
	if(right<0){
		HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_SET);
		right = fabs(right);
	}
	else if(right>0){
		HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
	}
	__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, left);
	__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, right);
}

void readEncoder(){
	if(__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim2)){
		currentLeft = __HAL_TIM_GET_COUNTER(&htim2);
		fLeft = currentLeft *-1*CMPERREV;
	}
	else{
		currentLeft = __HAL_TIM_GET_COUNTER(&htim2);
		fLeft = currentLeft *CMPERREV;
	}
	if(__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim3)){
		currentRight = __HAL_TIM_GET_COUNTER(&htim3);
		fRight = currentRight * CMPERREV;
	}
	else{
		currentRight = __HAL_TIM_GET_COUNTER(&htim2);
		fRight = currentRight * -1 *CMPERREV;
	}
}

float getTime(){
	return __HAL_TIM_GET_COUNTER(&htim5)/1000000;
}

void timeStart(){
	startSec = getTime();
}

float timeNow(){
	currSec = getTime() - startSec;
}

//Master function for motor with PID control
void PIDmotor(float setDist, int time){
	/* Set UARTBuffer to default value */
	*aRxBuffer = 'Z';
	velocityLeft=velocityRight = setDist/time;

	/* Initialise PID Controllers */
	PIDController pidLeft = {PID_KP,PID_KI,PID_KD,0,-3000,3000,-1000,1000,1.0f};
	PIDController pidRight ={PID_KP,PID_KI,PID_KD,0,-3000,3000,-1000,1000,1.0f};

//	/* Initialise Encoder */
	HAL_TIM_Base_Start(&htim5);
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);		//left encoder(MotorA) start
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);		//right encoder(MotorB) start

	PIDController_Init(&pidLeft);
	PIDController_Init(&pidRight);
	timeStart();
	while(1){
		/* Take current encoder values */
		readEncoder();
		distanceLeft = velocityLeft*timeNow();
		distanceRight = velocityRight * currSec;

		/* Compute new control signal */
		outLeft = PIDController_Update(&pidLeft, distanceLeft, fLeft);
		outRight = PIDController_Update(&pidRight, distanceRight, fRight);

		/* Update new values to motors */
		motorSpeed(outLeft, outRight);
		error = pidLeft.prevError;
		/* Preventive measures for errors */
		//if(pidLeft.out>=4000 || pidRight.out>=4000 || pidLeft.out<=4000 || pidRight.out<=4000)break;
	}

	motorSpeed(0,0);
	/* Reset Encoders value */
	__HAL_TIM_SET_COUNTER(&htim2,0);
	__HAL_TIM_SET_COUNTER(&htim3,0);
}

void pidForward(float velocity, int time){
	startSec = getTime();
	while(1){
		currSec= getTime()-startSec;
	}

}

//Master function for image recognition motor control
int motorCont(int speedL, int speedR, char dirL, char dirR, double dist){
	*aRxBuffer = 'Z';
	//declaration
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);		//left encoder(MotorA) start
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);		//right encoder(MotorB) start
	currentLeft = __HAL_TIM_GET_COUNTER(&htim2);
	currentRight = __HAL_TIM_GET_COUNTER(&htim3);
	tick = HAL_GetTick();
	double encDist = dist * 72;

	//Select direction of motor//
	switch(dirL){
		case 'F':
			HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
			break;

		case 'R':
			HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_SET);
			break;
	}

	switch(dirR){
		case 'F':
			HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
			break;

		case 'R':
			HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_SET);
			break;
	}
	//End of motor direction selection//

	__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, speedL);
	__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, speedR);


	while(1){
			currentLeft = __HAL_TIM_GET_COUNTER(&htim2);
			currentRight = __HAL_TIM_GET_COUNTER(&htim3);
			diffl = abs(currentLeft);
			diffr =abs(currentRight);
			avg = abs((diffl+diffr)/2);
			sprintf(display,"Left:%5d\0", diffl/68);
			OLED_ShowString(10,35,display);
			sprintf(display,"Right:%5d\0", diffr/68);
			OLED_ShowString(10,50,display);
			OLED_Refresh_Gram();

			if(avg>=encDist){
				__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_1, 0);
				__HAL_TIM_SetCompare(&htim8,TIM_CHANNEL_2, 0);
				HAL_Delay(500);
				break;
			}

		}
		__HAL_TIM_SET_COUNTER(&htim2,0);
		__HAL_TIM_SET_COUNTER(&htim3,0);

		speedL=speedR=tick=diffl=diffr=0;
		OLED_Refresh_Gram();
		*aRxBuffer = 'Z';
}

void degTurn(int mode){
	switch(mode){
	case 1: //Turn left
			htim1.Instance->CCR4 = 61;
			HAL_Delay(100);
			motorCont(500,2200,'F','F',105*0.57);
			htim1.Instance->CCR4 = 76;
			break;
	case 2:
			htim1.Instance->CCR4 = 96;
			HAL_Delay(100);
			motorCont(2200,500,'F','F',100*0.57);
			HAL_Delay(50);
			htim1.Instance->CCR4 = 73;
			break;
	}
}

void forward(int mode){ //Forward for image recognition
	HAL_Delay(100);
	switch(mode){
	case 0:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 100);break;
	case 1:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 8);break;
	case 2:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 18);break;
	case 3:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 28);break;
	case 4:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 38);break;
	case 5:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 49);break;
	case 6:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 60);break;
	case 7:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 70);break;
	case 8:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 80);break;
	case 9:
			motorCont(pwmLeft, pwmRight, 'F', 'F', 90);break;
	}
}

void reverse(int mode){//Reverse for image recognition
	HAL_Delay(100);
	switch(mode){
	case 0:
			motorCont(pwmLeft, pwmRight, 'R', 'R', 100);break;
	case 1:
			motorCont(pwmLeft, pwmRight,'R', 'R', 6);break;
	case 2:
			motorCont(pwmLeft, pwmRight,'R', 'R', 18);break;
	case 3:
			motorCont(pwmLeft, pwmRight, 'R', 'R', 28);break;
	case 4:
			motorCont(pwmLeft, pwmRight, 'R', 'R', 38);break;
	case 5:
			motorCont(pwmLeft, pwmRight,'R', 'R', 49);break;
	case 6:
			motorCont(pwmLeft, pwmRight, 'R', 'R', 60);break;
	case 7:
			motorCont(pwmLeft, pwmRight, 'R', 'R', 70);break;
	case 8:
			motorCont(pwmLeft, pwmRight, 'R', 'R', 80);break;
	case 9:
			motorCont(pwmLeft, pwmRight, 'R', 'R', 90);break;

	}
}

void spotTurn(int mode){
	float var = 1;
	float left = 1.0;
	float turn1 = 20*var;
	float turn2 = 21*var;
	float turn3 = 3*var;
	float turn4 = turn3;
	switch(mode){
	case 1: //Turn left
		htim1.Instance->CCR4 = 56;
		HAL_Delay(500);
		motorCont(700, 2100, 'F', 'F', turn1*left);
		htim1.Instance->CCR4 = 104;
		HAL_Delay(500);
		motorCont(2100, 700, 'R', 'R', turn2*left);
		htim1.Instance->CCR4 = 56;
		HAL_Delay(500);
		motorCont(700, 2100, 'F', 'F', turn3);
		HAL_Delay(50);
		htim1.Instance->CCR4 = 76;
		HAL_Delay(500);
		motorCont(1500, 1500, 'F', 'F', 5.5);
		break;
	case 2: //Turn right
		htim1.Instance->CCR4 = 104;
		HAL_Delay(500);
		motorCont(2100, 700, 'F', 'F', turn1);
		htim1.Instance->CCR4 = 56;
		HAL_Delay(500);
		motorCont(700, 2100, 'R', 'R', turn2);
		htim1.Instance->CCR4 = 104;
		HAL_Delay(500);
		motorCont(2100, 700, 'F', 'F', turn4);
		HAL_Delay(50);
		htim1.Instance->CCR4 = 73;
		HAL_Delay(500);
		motorCont(1500, 1500, 'F', 'F', 2);
		break;
	}
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

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
  MX_TIM8_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_RTC_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_1);
  HAL_UART_Receive_IT(&huart3, (uint8_t *) aRxBuffer, 1);
  OLED_Init();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of LED_Toggle */
  LED_ToggleHandle = osThreadNew(StartDefaultTask, NULL, &LED_Toggle_attributes);

  /* creation of MotorTask */
  MotorTaskHandle = osThreadNew(motor, NULL, &MotorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DMAContinuousRequests = DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable Calibrartion
  */
  if (HAL_RTCEx_SetCalibrationOutPut(&hrtc, RTC_CALIBOUTPUT_1HZ) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 320;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 10;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 10;
  if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 16-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 0xffff-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 16-1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 7199;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, OLED_SCL_Pin|OLED_SDA_Pin|OLED_RST_Pin|OLED_DC_Pin
                          |LED3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, AIN2_Pin|AIN1_Pin|BIN1_Pin|BIN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : OLED_SCL_Pin OLED_SDA_Pin OLED_RST_Pin OLED_DC_Pin
                           LED3_Pin */
  GPIO_InitStruct.Pin = OLED_SCL_Pin|OLED_SDA_Pin|OLED_RST_Pin|OLED_DC_Pin
                          |LED3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : AIN2_Pin AIN1_Pin BIN1_Pin BIN2_Pin */
  GPIO_InitStruct.Pin = AIN2_Pin|AIN1_Pin|BIN1_Pin|BIN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : USRBUT_Pin */
  GPIO_InitStruct.Pin = USRBUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USRBUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TRIG_Pin */
  GPIO_InitStruct.Pin = TRIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TRIG_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	/*Prevent unused argument(s) compilation warning*/
	UNUSED(huart);
	HAL_UART_Transmit(&huart3,(uint8_t *)aRxBuffer,10,0xFFFF); //might not nd
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	uint8_t test[20] = "Testing Pi";
	uint8_t ultra[20];
	uint8_t checkPi[1];

	/* Infinite loop */
	for(;;)
	{
		HAL_UART_Receive_IT(&huart3, (uint8_t *) aRxBuffer, 1);
		OLED_ShowString(5,5,test);
//		sprintf(checkPi, "Pi cmd: %s\0", aRxBuffer);
//		OLED_ShowString(10, 20, checkPi);
		ultraDistCheck();
		HAL_Delay(200);
		sprintf(ultra, "uDistF: %u\0", uDistFinal);
		OLED_ShowString(10, 50, ultra);

//		sprintf(ultra, "uDist1: %u\0", uDistCheck1);
//		OLED_ShowString(10, 25, ultra);
//
//		sprintf(ultra, "uDist2: %u\0", uDistCheck2);
//		OLED_ShowString(10, 35, ultra);

		irLeft();
		HAL_Delay(100);
		sprintf(ultra, "IR left: %u\0", ir1Dist);
		OLED_ShowString(10, 30, ultra);

		irRight();
		HAL_Delay(100);
		sprintf(ultra, "IR right: %u\0", ir2Dist);
		OLED_ShowString(10, 40, ultra);


		OLED_Refresh_Gram();
		//	  HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
		osDelay(100);
	}
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_motor */
/**
* @brief Function implementing the MotorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_motor */
void motor(void *argument)
{
  /* USER CODE BEGIN motor */
	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	htim1.Instance->CCR4 = 56;
	HAL_Delay(500);
	htim1.Instance->CCR4 = 104;
	HAL_Delay(500);
	htim1.Instance->CCR4 = 73;
	*aRxBuffer = '\0';
	uint8_t toRpiTest[6] = "NiHao";
		for(;;)
		  {
			switch (*aRxBuffer)
			{
			case '\0': // initialize
				HAL_UART_Receive_IT(&huart3, (uint8_t *)aRxBuffer, 1);
				break;
			case 'H':
				HAL_UART_Transmit_IT(&huart3,(uint8_t *)&toRpiTest,6);
				osDelay(50);
				*aRxBuffer = 'Z';
				break;
			case 'K':
				HAL_UART_Transmit_IT(&huart3,(uint8_t *)"OK?\n",4);
				osDelay(50);
				*aRxBuffer = 'Z';
				break;
			//========================Forward========================
			case '9':
				forward(0);break;
			case '0':
				forward(1);break;
			case '1':
				forward(2);break;
			case '2':
				forward(3);break;
			case '3':
				forward(4);break;
			case '4':
				forward(5);break;
			case '5':
				forward(6);break;
			case '6':
				forward(7);break;
			case '7':
				forward(8);break;
			case '8':
				forward(9);break;
			//========================Reverse========================
			case 33:
				reverse(1);break;
			case 34:
				reverse(2);break;
			case 35:
				reverse(3);break;
			case 36:
				reverse(4);break;
			case 37:
				reverse(5);break;
			case 38:
				reverse(6);break;
			case 39:
				reverse(7);break;
			case 40:
				reverse(8);break;
			case 41:
				reverse(9);break;
			case 42:
				reverse(0);break;
			//========================Turn========================
			case 'L':
				spotTurn(1);break;
			case 'R':
				spotTurn(2);break;
			case 'Q':
				degTurn(1);break;
			case 'E':
				degTurn(2);break;
			case 'U':
				ultraDistCheck();
				HAL_Delay(200);
				char reply[] = "000\n";
				reply[0] += uDistFinal / 100 % 10;
				reply[1] += uDistFinal / 10 % 10;
				reply[2] += uDistFinal % 10;
				if (uDistFinal > 999)
					reply[0] = '9';
				HAL_UART_Transmit_IT(&huart3, (uint8_t *)reply, strlen(reply));
				osDelay(50);
				*aRxBuffer = 'Z';
				break;
//			case 'S':
////				irRight();
//				char reply2[] = "00\n";
//				reply2[0] += ir2Dist / 10 % 10;
//				reply2[1] += ir2Dist % 10;
//				HAL_UART_Transmit_IT(&huart3, (uint8_t *)reply2, strlen(reply2));
//				osDelay(50);
//				*aRxBuffer = 'R';
//				break;
//			case 'M':
////				irLeft();
//				char reply3[] = "00\n";
//				reply3[0] += ir2Dist / 10 % 10;
//				reply3[1] += ir2Dist % 10;
//				HAL_UART_Transmit_IT(&huart3, (uint8_t *)reply2, strlen(reply3));
//				osDelay(50);
//				*aRxBuffer = 'R';
//				break;
			/* Test Cases */
			case 'V':
				correction();
				break;
			case 'W':
				PIDmotor(100,10);
				break;
			case 'X':
				ultraDistCheck();
				irLeft();
				irRight();
				break;
			case 'Y':
				HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);		//left encoder(MotorA) start
					HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);		//right encoder(MotorB) start
				while(1){
				readEncoder();}
				break;
			case 'Z':
				waitCmd();
				break;
			default:
//				*aRxBuffer = 'R';
				HAL_UART_Receive_IT(&huart3, (uint8_t *)aRxBuffer, 1);
				break;
			}
			HAL_Delay(100);
			HAL_UART_Transmit_IT(&huart3,(uint8_t *)"OK",2);
		  }

	osDelay(1000);
  /* USER CODE END motor */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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

