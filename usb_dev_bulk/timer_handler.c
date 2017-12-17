/*
 * timer_handler.c
 *
 *  Created on: 17 déc. 2017
 *      Author: macload1
 */
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "linked_list_dbl.h"

#include "servo.h"
//*****************************************************************************
//
// Global variable to hold the system clock speed.
//
//*****************************************************************************
extern uint32_t ui32SysClock;

uint32_t milli_second = 0;

//*****************************************************************************
//
// The interrupt handler for the first timer interrupt.
//
//*****************************************************************************
void
Timer0AIntHandler(void)
{
	int i;
    //
    // Clear the timer interrupt.
    //
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Increment millisecond counter.
    //
    milli_second++;

    //
    // Check servo lists for pending actions.
    //
    for(i = 0; i < 4; i++)
    {
    	if(!listIsEmpty(left_arm_list[i]))
    	{
    		uint32_t ms;
    		uint32_t pos;
    		ms = left_arm_list[i].h_p->ms_time_stamp - milli_second;
    		pos = getServoPosition(i, true) + (left_arm_list[i].h_p->position - getServoPosition(i, true))/ms;
    		setServo(left_arm_servos[i], pos);
    		if(left_arm_list[i].h_p->ms_time_stamp == milli_second)
    		{
    			listDelete(left_arm_list[i], true);
    		}
    	}
    	if(!listIsEmpty(right_arm_list[i]))
    	{
    		uint32_t ms;
    		uint32_t pos;
    		ms = right_arm_list[i].h_p->ms_time_stamp - milli_second;
    		pos = getServoPosition(i, false) + (right_arm_list[i].h_p->position - getServoPosition(i, false))/ms;
    		setServo(right_arm_servos[i], pos);
    		if(right_arm_list[i].h_p->ms_time_stamp == milli_second)
    		{
    			listDelete(right_arm_list[i], true);
    		}
    	}
    }
}


void timerInit(void)
{
    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Configure the two 32-bit periodic timers.
    //
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ui32SysClock/1000);	// 1ms tick

    //
    // Setup the interrupts for the timer timeouts.
    //
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Enable the timers.
    //
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);

	return;
}
