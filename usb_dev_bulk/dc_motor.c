/*
 * dc_motor.c
 *
 *  Created on: 16 déc. 2017
 *      Author: macload1
 */
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

#include "dc_motor.h"


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
// Initialises the timer PWM peripheral.
//
//*****************************************************************************
void initDCMotor(void)
{
	/**************************************************************************
	 *
	 * DC Motor Control
	 * Period: 1900 Hz <=> 526 microseconds
	 * Pulse width range: 0 ms - 0,526 ms
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
	TimerMatchSet(TIMER1_BASE, TIMER_A, MOTOR_SPEED_ZERO);
	TimerMatchSet(TIMER1_BASE, TIMER_B, MOTOR_SPEED_ZERO);
	TimerMatchSet(TIMER2_BASE, TIMER_A, MOTOR_SPEED_ZERO);
	TimerMatchSet(TIMER2_BASE, TIMER_B, MOTOR_SPEED_ZERO);

	//
	// Enable Timers.
	//
	TimerEnable(TIMER1_BASE, TIMER_A | TIMER_B);
	TimerEnable(TIMER2_BASE, TIMER_A | TIMER_B);

	LEFT_B(MOTOR_SPEED_MAX/2);
	LEFT_F(MOTOR_SPEED_ZERO);
	RIGHT_B(MOTOR_SPEED_MAX/2);
	RIGHT_F(MOTOR_SPEED_ZERO);

	return;
}
