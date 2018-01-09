/*
 * dc_motor.h
 *
 *  Created on: 16 déc. 2017
 *      Author: macload1
 */

#ifndef DC_MOTOR_H_
#define DC_MOTOR_H_

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

//*****************************************************************************
//
// Switching frequency.
//
//*****************************************************************************
#define	SWITCHING_FREQ			(unsigned long) 1900		// in Hz (1832 minimum)
#define SYS_CLOCK				(unsigned long) 120000000	// in Hz


#define MOTOR_SPEED_ZERO			SYS_CLOCK/SWITCHING_FREQ-1
#define MOTOR_SPEED_MAX				0


//*****************************************************************************
//
// Motor corresponding timers.
//
//*****************************************************************************
#define	LEFT_B(x)					TimerMatchSet(TIMER1_BASE, TIMER_B, x);
#define	LEFT_F(x)					TimerMatchSet(TIMER1_BASE, TIMER_A, x);
#define	RIGHT_B(x)					TimerMatchSet(TIMER2_BASE, TIMER_A, x);
#define	RIGHT_F(x)					TimerMatchSet(TIMER2_BASE, TIMER_B, x);



// Initialise PWM peripheral
void initDCMotor(void);

#endif /* DC_MOTOR_H_ */
