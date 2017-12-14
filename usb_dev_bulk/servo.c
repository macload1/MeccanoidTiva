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

//*****************************************************************************
//
// Motor corresponding timers.
//
//*****************************************************************************
#define	LEFT_B(x)					TimerMatchSet(TIMER1_BASE, TIMER_A, x);
#define	LEFT_F(x)					TimerMatchSet(TIMER1_BASE, TIMER_B, x);
#define	RIGHT_B(x)					TimerMatchSet(TIMER2_BASE, TIMER_A, x);
#define	RIGHT_F(x)					TimerMatchSet(TIMER2_BASE, TIMER_B, x);

#define MOTOR_SPEED_ZERO			SYS_CLOCK/SWITCHING_FREQ



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

	//
	// Initialise the GPIOs and Timers that drive the PWM outputs.
	//

	/**************************************************************************
	 *
	 * PWM Control
	 * Period: 500 Hz <=> 2 ms
	 * Pulse width range: 0 ms - 2 ms
	 *
	 *************************************************************************/

	//
	// The Timer1 and Timer2 peripheral must be enabled.
	//
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);

	//
	// GPIO port A needs to be enabled so their pins can be used.
	//
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	//
	// Configure the GPIO pin muxing for the Timer/CCP function.
	// This is only necessary if your part supports GPIO pin function muxing.
	// Study the data sheet to see which functions are allocated per pin.
	//
	GPIOPinConfigure(GPIO_PA2_T1CCP0);
	GPIOPinConfigure(GPIO_PA3_T1CCP1);
	GPIOPinConfigure(GPIO_PA4_T2CCP0);
	GPIOPinConfigure(GPIO_PA5_T2CCP1);

	//
	// Configure the ccp settings for CCP pin. This function also gives
	// control of these pins to the timer hardware. Consult the data sheet to
	// see which functions are allocated per pin.
	//
	GPIOPinTypeTimer(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);

	//
	// Configure Timer as a 32-bit periodic timer.
	//
	TimerConfigure(TIMER1_BASE, TIMER_CFG_SPLIT_PAIR |
				   TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);
	TimerConfigure(TIMER2_BASE, TIMER_CFG_SPLIT_PAIR |
				   TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);

	//
	// Set the Timers load value to 100000 (for 500 Hz period). From the
	// load value (i.e. 100000) down to match value (set below) the signal
	// will be high.  From the match value to 0 the timer will be low.
	//
	TimerLoadSet(TIMER1_BASE, TIMER_A, (SYS_CLOCK/SWITCHING_FREQ));
	TimerLoadSet(TIMER1_BASE, TIMER_B, (SYS_CLOCK/SWITCHING_FREQ));
	TimerLoadSet(TIMER2_BASE, TIMER_A, (SYS_CLOCK/SWITCHING_FREQ));
	TimerLoadSet(TIMER2_BASE, TIMER_B, (SYS_CLOCK/SWITCHING_FREQ));

	//
	// Set the WTimers match value to maximum (pulse width = 0).
	//
//	TimerMatchSet(TIMER0_BASE, TIMER_A, ((SYS_CLOCK/SWITCHING_FREQ)-1));
//	TimerMatchSet(TIMER0_BASE, TIMER_B, ((SYS_CLOCK/SWITCHING_FREQ)-1));
//	TimerMatchSet(TIMER4_BASE, TIMER_A, ((SYS_CLOCK/SWITCHING_FREQ)-1));
//	TimerMatchSet(TIMER4_BASE, TIMER_B, ((SYS_CLOCK/SWITCHING_FREQ)-1));
	TimerMatchSet(TIMER1_BASE, TIMER_A, MOTOR_SPEED_ZERO);
	TimerMatchSet(TIMER1_BASE, TIMER_B, MOTOR_SPEED_ZERO);
	TimerMatchSet(TIMER2_BASE, TIMER_A, MOTOR_SPEED_ZERO);
	TimerMatchSet(TIMER2_BASE, TIMER_B, MOTOR_SPEED_ZERO);

	//
	// Enable Timers.
	//
	TimerEnable(TIMER1_BASE, TIMER_A | TIMER_B);
	TimerEnable(TIMER2_BASE, TIMER_A | TIMER_B);

	LEFT_B(1000);
	//LEFT_F(5000);
	RIGHT_B(2000);
	//RIGHT_F(4000);

	return;
}
