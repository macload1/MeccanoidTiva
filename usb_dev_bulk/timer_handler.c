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

//*****************************************************************************
//
// Linked list for servo motor movement.
//
//*****************************************************************************
extern struct list_s left_arm_list[4];		// list contains the different actions
extern struct list_s right_arm_list[4];		// list contains the different actions

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

	bool left_empty = true;
	bool right_empty = true;
    //
    // Check servo lists for pending actions.
    //
    for(i = 0; i < 4; i++)
    {
    	if(left_is_moving)
    	{
			if(!listIsEmpty(&left_arm_list[i]))
			{
				left_empty = false;
				if((left_mvmt_start_time + left_arm_list[i].h_p->ms_time_start) <= milli_second)
				{
					uint32_t ms;
					uint32_t duration;
					uint32_t pos;
					uint32_t initial_position = actual_pos[left_servo_not[i]];
					duration = left_arm_list[i].h_p->ms_time_stop - left_arm_list[i].h_p->ms_time_start;
					ms = left_mvmt_start_time + left_arm_list[i].h_p->ms_time_stop - milli_second;
					if(initial_position <= left_arm_list[i].h_p->position)
					{
						pos = initial_position + (left_arm_list[i].h_p->position - initial_position) * (duration - ms) / (duration);
					}
					else
					{
						pos = initial_position - (initial_position - left_arm_list[i].h_p->position) * (duration - ms) / (duration);
					}
					if(ms == 0)
					{
						setServoPosition(left_arm_servos[i], left_arm_list[i].h_p->position);
						actual_pos[left_servo_not[i]] = left_arm_list[i].h_p->position;
					}
					else
						setServoPosition(left_arm_servos[i], pos);
					if((left_mvmt_start_time + left_arm_list[i].h_p->ms_time_stop) <= milli_second)
					{
						listDelete(&left_arm_list[i], true);
					}
				}
			}
    	}
    	if(right_is_moving)
    	{
			if(!listIsEmpty(&right_arm_list[i]))
			{
				right_empty = false;
				if((right_mvmt_start_time + right_arm_list[i].h_p->ms_time_start) <= milli_second)
				{
					uint32_t ms;
					uint32_t duration;
					uint32_t pos;
					uint32_t initial_position = actual_pos[right_servo_not[i]];
					duration = right_arm_list[i].h_p->ms_time_stop - right_arm_list[i].h_p->ms_time_start;
					ms = right_mvmt_start_time + right_arm_list[i].h_p->ms_time_stop - milli_second;
					if(initial_position <= right_arm_list[i].h_p->position)
					{
						pos = initial_position + (right_arm_list[i].h_p->position - initial_position) * (duration - ms) / (duration);
					}
					else
					{
						pos = initial_position - (initial_position - right_arm_list[i].h_p->position) * (duration - ms) / (duration);
					}
					if(ms == 0)
					{
						setServoPosition(right_arm_servos[i], right_arm_list[i].h_p->position);
						actual_pos[right_servo_not[i]] = right_arm_list[i].h_p->position;
					}
					else
						setServoPosition(right_arm_servos[i], pos);
					if((right_mvmt_start_time + right_arm_list[i].h_p->ms_time_stop) <= milli_second)
					{
						listDelete(&right_arm_list[i], true);
					}
				}
			}
    	}
    }
    if(left_empty)
    	left_is_moving = false;
    if(right_empty)
    	right_is_moving = false;
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
