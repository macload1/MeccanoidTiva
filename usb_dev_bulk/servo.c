/*
 * servo.c
 *
 *  Created on: 15 juin 2017
 *      Author: macload1
 */
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/timer.h"

#include "servo.h"
#include "linked_list_dbl.h"

//*****************************************************************************
//
// Switching frequency.
//
//*****************************************************************************
#define	SWITCHING_FREQ			(unsigned long) 1900		// in Hz (1832 minimum)
#define SYS_CLOCK				(unsigned long) 120000000	// in Hz

//*****************************************************************************
//
// Global variable to hold the system clock speed.
//
//*****************************************************************************
extern uint32_t ui32SysClock;


extern struct list_s left_arm_list[4];		// list contains the different actions
extern struct list_s right_arm_list[4];		// list contains the different actions

//*****************************************************************************
//
// Servo corresponding PWM outputs.
//
//*****************************************************************************
#define	RIGHT_BASE_PWM			PWM_OUT_0
#define	RIGHT_UPPER_BASE_PWM	PWM_OUT_5
#define RIGHT_WRIST_PWM			PWM_OUT_1
#define RIGHT_HAND_PWM			PWM_OUT_3
#define	LEFT_BASE_PWM			PWM_OUT_6
#define	LEFT_UPPER_BASE_PWM		PWM_OUT_4
#define LEFT_WRIST_PWM			PWM_OUT_2
#define LEFT_HAND_PWM			PWM_OUT_7

uint32_t left_arm_servos[4] = {LEFT_BASE_PWM,
							   LEFT_UPPER_BASE_PWM,
							   LEFT_WRIST_PWM,
							   LEFT_HAND_PWM};
uint32_t right_arm_servos[4] = {RIGHT_BASE_PWM,
							    RIGHT_UPPER_BASE_PWM,
							    RIGHT_WRIST_PWM,
							    RIGHT_HAND_PWM};

struct list_s* servo_list[8] = {&right_arm_list[0],
								&right_arm_list[2],
								&left_arm_list[2],
								&right_arm_list[3],
								&left_arm_list[1],
								&right_arm_list[1],
								&left_arm_list[0],
								&left_arm_list[3]};

//*****************************************************************************
//
// Servo movement variables.
//
//*****************************************************************************
bool left_is_moving = false;
bool right_is_moving = false;

uint32_t left_mvmt_start_time;
uint32_t right_mvmt_start_time;



//*****************************************************************************
//
// Initialises the PWM peripheral.
//
//*****************************************************************************
void initPWM(void)
{
	/**************************************************************************
	 *
	 * PWM Control
	 * Period: 50 Hz <=> 20 ms
	 * Pulse width: 600 µs - 2400 µs
	 * 				0° <=> 600 µs
	 * 				90° <=> 1500 µs
	 * 				180° <=> 2400 µs
	 *
	 *************************************************************************/
	unsigned long ulPeriod;

	// Enable the peripherals used by this program.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

	// Configure PWM frequency
	ulPeriod = ui32SysClock / (64 * 50);					//PWM frequency 50HZ

	// Configure PF0, PF1, PF2, PF3, PG0, PG1, PK4 & PK5 Pins as PWM
	GPIOPinConfigure(GPIO_PF0_M0PWM0);
	GPIOPinConfigure(GPIO_PF1_M0PWM1);
	GPIOPinConfigure(GPIO_PF2_M0PWM2);
	GPIOPinConfigure(GPIO_PF3_M0PWM3);
	GPIOPinConfigure(GPIO_PG0_M0PWM4);
	GPIOPinConfigure(GPIO_PG1_M0PWM5);
	GPIOPinConfigure(GPIO_PK4_M0PWM6);
	GPIOPinConfigure(GPIO_PK5_M0PWM7);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	GPIOPinTypePWM(GPIO_PORTK_BASE, GPIO_PIN_4 | GPIO_PIN_5);

	// Configure PWM Options
	// PWM_GEN_0 Covers M1PWM0 and M1PWM1
	// PWM_GEN_1 Covers M1PWM2 and M1PWM3
	// PWM_GEN_2 Covers M1PWM4 and M1PWM5
	// PWM_GEN_3 Covers M1PWM6 and M1PWM7
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);

	// Set the Period (expressed in clock ticks)
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ulPeriod);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, ulPeriod);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, ulPeriod);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, ulPeriod);

	// Set PWM duty cycle: 3% - 12%
	// Right Arm Base
	// lowest value: 1150
	// highest value: 4000
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 2575);
	// Right Arm Upper Base
	// lowest value: 1150 ==> nach hinten genickt
	// highest value: 4000 ==> nach vorne genickt
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 2575);
	// Right Arm Handgelenk
	// lowest value: 1150
	// highest value: 4000
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 2575);
	// Right Hand
	// lowest value: 1150 ==> nach vorne genickt
	// highest value: 4000 ==> nach hinten genickt
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 2000);
	// Left Arm Base
	// lowest value: 1150 ==> Hand vertikal zom Roboter, Motor links
	// middle value: 2575 ==> Hand parallel zom Roboter
	// highest value: 4000 ==> Hand vertikal zum Roboter, Motor rechts
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 2575);
	// Left Arm Upper Base
	// lowest value: 1150 ==> nach vorne genickt
	// highest value: 4000 ==> nach hinten genickt
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 2500);
	// Left Arm Handgelenk
	// lowest value: 1150
	// highest value: 4000
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 2575);
	// Left Hand
	// lowest value: 1250 => Hand geöffnet
	// highest value: 2200 => Hand geschlossen
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 2000);

	// Enable the PWM generator
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMGenEnable(PWM0_BASE, PWM_GEN_1);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
	PWMGenEnable(PWM0_BASE, PWM_GEN_3);


	// Configure PWM Clock to match system/64
	PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_64);

	// Turn on the Output pins
	PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT
			  	  	  	  	  | PWM_OUT_1_BIT
							  | PWM_OUT_2_BIT
							  | PWM_OUT_3_BIT
							  | PWM_OUT_4_BIT
							  | PWM_OUT_5_BIT
							  | PWM_OUT_6_BIT
							  | PWM_OUT_7_BIT, true);

	return;
}


void setServoPosition(uint32_t servo, uint32_t position)
{
	PWMPulseWidthSet(PWM0_BASE, servo, position);
	return;
}

uint32_t getServoPosition(uint32_t servo, bool left)
{
	if(left)
		return PWMPulseWidthGet(PWM0_BASE, left_arm_servos[servo]);
	else
		return PWMPulseWidthGet(PWM0_BASE, right_arm_servos[servo]);
}
