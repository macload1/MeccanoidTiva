//*****************************************************************************
//
// usb_dev_bulk.c - Main routines for the generic bulk device example.
//
// Copyright (c) 2013-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.4.178 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/pwm.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdbulk.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "drivers/pinout.h"
#include "usb_bulk_structs.h"

#include "linked_list_dbl.h"
#include "delay.h"

#include "timer_handler.h"
#include "servo.h"
#include "dc_motor.h"
#include "Meccano.h"
//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Generic Bulk Device (usb_dev_bulk)</h1>
//!
//! This example provides a generic USB device offering simple bulk data
//! transfer to and from the host.  The device uses a vendor-specific class ID
//! and supports a single bulk IN endpoint and a single bulk OUT endpoint.
//! Data received from the host is assumed to be ASCII text and it is
//! echoed back with the case of all alphabetic characters swapped.
//!
//! A Windows INF file for the device is provided on the installation media and
//! in the C:/ti/TivaWare-C-Series-X.X/windows_drivers directory of TivaWare
//! releases.  This INF contains information required to install the WinUSB
//! subsystem on WindowsXP and Vista PCs.  WinUSB is a Windows subsystem
//! allowing user mode applications to access the USB device without the need
//! for a vendor-specific kernel mode driver.
//!
//! A sample Windows command-line application, usb_bulk_example, illustrating
//! how to connect to and communicate with the bulk device is also provided.
//! The application binary is installed as part of the ``TivaWare for C Series
//! PC Companion Utilities'' package (SW-TM4C-USB-WIN) on the installation CD
//! or via download from http://www.ti.com/tivaware .  Project files are
//! included to allow the examples to be built using
//! Microsoft Visual Studio 2008.  Source code for this application can be
//! found in directory ti/TivaWare_C_Series-x.x/tools/usb_bulk_example.
//
//*****************************************************************************


//*****************************************************************************
//
// Global variable to hold the system clock speed.
//
//*****************************************************************************
uint32_t ui32SysClock;
//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 100
#define SYSTICK_PERIOD_MS   (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// The global system tick counter.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount = 0;

//*****************************************************************************
//
// Variables tracking transmit and receive counts.
//
//*****************************************************************************
volatile uint32_t g_ui32TxCount = 0;
volatile uint32_t g_ui32RxCount = 0;

//*****************************************************************************
//
// Flags used to pass commands from interrupt context to the main loop.
//
//*****************************************************************************
#define COMMAND_PACKET_RECEIVED 0x00000001
#define COMMAND_STATUS_UPDATE   0x00000002

volatile uint32_t g_ui32Flags = 0;


//*****************************************************************************
//
// USB message types
//
//*****************************************************************************
#define	DC_DIRECT_CMD			0x00
#define	DC_MVMT_CMD				0x01
#define	DC_START_MVMT_CMD		0x02
#define	DC_CHARGE_MVMT_CMD		0x03
#define	DC_GET_POSITION_CMD		0x04
#define	SERVO_DIRECT_CMD		0x10
#define	SERVO_MVMT_CMD			0x11
#define	SERVO_START_MVMT_CMD	0x12
#define	SERVO_CHARGE_MVMT_CMD	0x13
#define	SERVO_GET_POSITION_CMD	0x14
#define	MECCANO_SERVO_POS_CMD	0x20
#define	MECCANO_SERVO_LED_CMD	0x21
#define	MECCANO_LED_CMD			0x22


//*****************************************************************************
//
// Global flag indicating that a USB configuration has been set.
//
//*****************************************************************************
static volatile bool g_bUSBConfigured = false;

//*****************************************************************************
//
// Linked list for servo motor movement.
//
//*****************************************************************************
struct list_s left_arm_list[4];		// list contains the different actions
struct list_s right_arm_list[4];	// list contains the different actions

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update our system tick counter.
    //
    g_ui32SysTickCount++;
}

//*****************************************************************************
//
// Receive new data and echo it back to the host.
//
// \param psDevice points to the instance data for the device whose data is to
// be processed.
// \param pi8Data points to the newly received data in the USB receive buffer.
// \param ui32NumBytes is the number of bytes of data available to be
// processed.
//
// This function is called whenever we receive a notification that data is
// available from the host. We read the data, byte-by-byte and swap the case
// of any alphabetical characters found then write it back out to be
// transmitted back to the host.
//
// \return Returns the number of bytes of data processed.
//
//*****************************************************************************
static uint32_t
EchoNewDataToHost(tUSBDBulkDevice *psDevice, uint8_t *pi8Data,
                  uint_fast32_t ui32NumBytes)
{
    uint_fast32_t ui32Loop, ui32Space, ui32Count;
    uint_fast32_t ui32ReadIndex;
    uint_fast32_t ui32WriteIndex;
    tUSBRingBufObject sTxRing;

    //
    // Get the current buffer information to allow us to write directly to
    // the transmit buffer (we already have enough information from the
    // parameters to access the receive buffer directly).
    //
    USBBufferInfoGet(&g_sTxBuffer, &sTxRing);

    //
    // How much space is there in the transmit buffer?
    //
    ui32Space = USBBufferSpaceAvailable(&g_sTxBuffer);

    //
    // How many characters can we process this time round?
    //
    ui32Loop = (ui32Space < ui32NumBytes) ? ui32Space : ui32NumBytes;
    ui32Count = ui32Loop;

    //
    // Update our receive counter.
    //
    g_ui32RxCount += ui32NumBytes;

    //
    // Set up to process the characters by directly accessing the USB buffers.
    //
    ui32ReadIndex = (uint32_t)(pi8Data - g_pui8USBRxBuffer);
    ui32WriteIndex = sTxRing.ui32WriteIndex;

    while(ui32Loop)
    {
        //
        // Copy from the receive buffer to the transmit buffer converting
        // character case on the way.
        //

        //
        // Is this a lower case character?
        //
        if((g_pui8USBRxBuffer[ui32ReadIndex] >= 'a') &&
           (g_pui8USBRxBuffer[ui32ReadIndex] <= 'z'))
        {
            //
            // Convert to upper case and write to the transmit buffer.
            //
            g_pui8USBTxBuffer[ui32WriteIndex] =
                (g_pui8USBRxBuffer[ui32ReadIndex] - 'a') + 'A';
        }
        else
        {
            //
            // Is this an upper case character?
            //
            if((g_pui8USBRxBuffer[ui32ReadIndex] >= 'A') &&
               (g_pui8USBRxBuffer[ui32ReadIndex] <= 'Z'))
            {
                //
                // Convert to lower case and write to the transmit buffer.
                //
                g_pui8USBTxBuffer[ui32WriteIndex] =
                    (g_pui8USBRxBuffer[ui32ReadIndex] - 'Z') + 'z';
            }
            else
            {
                //
                // Copy the received character to the transmit buffer.
                //
                g_pui8USBTxBuffer[ui32WriteIndex] =
                    g_pui8USBRxBuffer[ui32ReadIndex];
            }
        }

        //
        // Move to the next character taking care to adjust the pointer for
        // the buffer wrap if necessary.
        //
        ui32WriteIndex++;
        ui32WriteIndex =
            (ui32WriteIndex == BULK_BUFFER_SIZE) ? 0 : ui32WriteIndex;

        ui32ReadIndex++;

        ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
                         0 : ui32ReadIndex);

        ui32Loop--;
    }

    //
    // We've processed the data in place so now send the processed data
    // back to the host.
    //
    USBBufferDataWritten(&g_sTxBuffer, ui32Count);

    //
    // We processed as much data as we can directly from the receive buffer so
    // we need to return the number of bytes to allow the lower layer to
    // update its read pointer appropriately.
    //
    return(ui32Count);
}

//*****************************************************************************
//
// Handles bulk driver notifications related to the transmit channel (data to
// the USB host).
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the bulk driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t
TxHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
          void *pvMsgData)
{
    //
    // We are not required to do anything in response to any transmit event
    // in this example. All we do is update our transmit counter.
    //
    if(ui32Event == USB_EVENT_TX_COMPLETE)
    {
        g_ui32TxCount += ui32MsgValue;
    }
    return(0);
}

//*****************************************************************************
//
// Handles bulk driver notifications related to the receive channel (data from
// the USB host).
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the bulk driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t
RxHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
          void *pvMsgData)
{
    //
    // Which event are we being sent?
    //
    switch(ui32Event)
    {
        //
        // We are connected to a host and communication is now possible.
        //
        case USB_EVENT_CONNECTED:
        {
            g_bUSBConfigured = true;
            g_ui32Flags |= COMMAND_STATUS_UPDATE;

            //
            // Flush our buffers.
            //
            USBBufferFlush(&g_sTxBuffer);
            USBBufferFlush(&g_sRxBuffer);

            break;
        }

        //
        // The host has disconnected.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_bUSBConfigured = false;
            g_ui32Flags |= COMMAND_STATUS_UPDATE;
            break;
        }

        //
        // A new packet has been received.
        //
        case USB_EVENT_RX_AVAILABLE:
        {
        	int i;
        	int left_direction;
        	int right_direction;
			int servo;
			int position = 0;
			int colour;
			int red, green, blue, time;
			int speed;
			int return_nbr;
            tUSBDBulkDevice *psDevice;
            uint_fast32_t ui32ReadIndex;
            //
            // Get a pointer to our instance data from the callback data
            // parameter.
            //
            psDevice = (tUSBDBulkDevice *)pvCBData;

            //
            // Treat the new data
            //

            //
			// Set up to process the characters by directly accessing the USB buffers.
			//
			ui32ReadIndex = (uint32_t)((uint8_t *)pvMsgData - g_pui8USBRxBuffer);

            //
            // Check the message type
            //
            switch(g_pui8USBRxBuffer[ui32ReadIndex])
            {
            case DC_DIRECT_CMD:
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				right_direction = g_pui8USBRxBuffer[ui32ReadIndex];
				// Right DC Motor
				if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x01)
				{
					ui32ReadIndex++;
					ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
									 0 : ui32ReadIndex);
					// Move forward
					RIGHT_B(MOTOR_SPEED_ZERO);
					speed = 0;
					for(i = 1; i <= 4; i++)
					{
						speed <<= 8;
						speed += g_pui8USBRxBuffer[ui32ReadIndex];
						ui32ReadIndex++;
						ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
										 0 : ui32ReadIndex);
					}
					RIGHT_F(speed);
				}
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x02)
				{
					ui32ReadIndex++;
					ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
									 0 : ui32ReadIndex);
					// Move backward
					RIGHT_F(MOTOR_SPEED_ZERO);
					speed = 0;
					for(i = 1; i <= 4; i++)
					{
						speed <<= 8;
						speed += g_pui8USBRxBuffer[ui32ReadIndex];
						ui32ReadIndex++;
						ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
										 0 : ui32ReadIndex);
					}
					RIGHT_B(speed);
				}
//				UARTprintf("Right: fw -> %d  speed -> %d \r\n",
//										right_direction,
//										speed);
				// Left DC Motor
				left_direction = g_pui8USBRxBuffer[ui32ReadIndex];
				if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x01)
				{
					ui32ReadIndex++;
					ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
									 0 : ui32ReadIndex);
					// Move forward
					LEFT_B(MOTOR_SPEED_ZERO);
					speed = 0;
					for(i = 6; i <= 9; i++)
					{
						speed <<= 8;
						speed += g_pui8USBRxBuffer[ui32ReadIndex];
						ui32ReadIndex++;
						ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
										 0 : ui32ReadIndex);
					}
					LEFT_F(speed);
				}
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x02)
				{
					ui32ReadIndex++;
					ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
									 0 : ui32ReadIndex);
					// Move backward
					LEFT_F(MOTOR_SPEED_ZERO);
					speed = 0;
					for(i = 6; i <= 9; i++)
					{
						speed <<= 8;
						speed += g_pui8USBRxBuffer[ui32ReadIndex];
						ui32ReadIndex++;
						ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
										 0 : ui32ReadIndex);
					}
					LEFT_B(speed);
				}
//				UARTprintf("Left: fw -> %d  speed -> %d \r\n",
//						left_direction,
//						speed);
				return 11;
            case DC_MVMT_CMD:

            	break;
            case DC_START_MVMT_CMD:

            	break;
            case DC_CHARGE_MVMT_CMD:

            	break;
            case DC_GET_POSITION_CMD:

            	break;
            case SERVO_DIRECT_CMD:
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x00)
					servo = PWM_OUT_0;
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x01)
					servo = PWM_OUT_1;
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x02)
					servo = PWM_OUT_2;
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x03)
					servo = PWM_OUT_3;
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x04)
					servo = PWM_OUT_4;
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x05)
					servo = PWM_OUT_5;
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x06)
					servo = PWM_OUT_6;
				else if(g_pui8USBRxBuffer[ui32ReadIndex] == 0x07)
					servo = PWM_OUT_7;
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				for(i = 1; i <= ui32MsgValue; i++)
				{
					position <<= 8;
					position += g_pui8USBRxBuffer[ui32ReadIndex];
					ui32ReadIndex++;

					ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
									 0 : ui32ReadIndex);
				}

				PWMPulseWidthSet(PWM0_BASE, servo, position);
				// Return number of bytes received
				return 6;
            case SERVO_MVMT_CMD:

            	break;
            case SERVO_START_MVMT_CMD:
            	// Retrieve actual Servo position
            	actual_pos[0] = getServoPosition(0, false);
            	actual_pos[1] = getServoPosition(2, false);
            	actual_pos[2] = getServoPosition(2, true);
            	actual_pos[3] = getServoPosition(3, false);
            	actual_pos[4] = getServoPosition(1, true);
            	actual_pos[5] = getServoPosition(1, false);
            	actual_pos[6] = getServoPosition(0, true);
            	actual_pos[7] = getServoPosition(3, true);

				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				uint8_t arm = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				// Start left arm movement
				if((arm & 0x01) == 0x01)
				{
					left_mvmt_start_time = milli_second;
					left_is_moving = true;
				}
				// Start right arm movement
				if((arm & 0x02) == 0x02)
				{
					right_mvmt_start_time = milli_second;
					right_is_moving = true;
				}
            	return 2;
            case SERVO_CHARGE_MVMT_CMD:
            	return_nbr = 2;
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				uint32_t packet_nbr = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);

				while(packet_nbr--)
				{
					uint32_t servo_nbr = g_pui8USBRxBuffer[ui32ReadIndex];
					ui32ReadIndex++;
					ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
									 0 : ui32ReadIndex);
					uint32_t start_time = 0;
					for(i = 1; i <= 4; i++)
					{
						start_time <<= 8;
						start_time += g_pui8USBRxBuffer[ui32ReadIndex];
						ui32ReadIndex++;
						ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
										 0 : ui32ReadIndex);
					}
					uint32_t stop_time = 0;
					for(i = 1; i <= 4; i++)
					{
						stop_time <<= 8;
						stop_time += g_pui8USBRxBuffer[ui32ReadIndex];
						ui32ReadIndex++;
						ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
										 0 : ui32ReadIndex);
					}
					uint32_t stop_position = 0;
					for(i = 1; i <= 4; i++)
					{
						stop_position <<= 8;
						stop_position += g_pui8USBRxBuffer[ui32ReadIndex];
						ui32ReadIndex++;
						ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
										 0 : ui32ReadIndex);
					}
					Insert(servo_list[servo_nbr],
							start_time,
							stop_time,
							stop_position);
					return_nbr += 13;
				}
				return return_nbr;
            case SERVO_GET_POSITION_CMD:
                //
                // Read the new packet and echo it back to the host.
                //
                //return(EchoNewDataToHost(psDevice, pvMsgData, ui32MsgValue));
            	break;
            case MECCANO_SERVO_POS_CMD:
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				servo = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				position = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);

				setMeccanoServoPosition(servo, position);
				// Return number of bytes received
				return 3;
            case MECCANO_SERVO_LED_CMD:
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				servo = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				colour = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);

				setMeccanoServoColor(servo, colour);
				// Return number of bytes received
				return 3;
            case MECCANO_LED_CMD:
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				red = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				green = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				blue = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);
				time = g_pui8USBRxBuffer[ui32ReadIndex];
				ui32ReadIndex++;
				ui32ReadIndex = ((ui32ReadIndex == BULK_BUFFER_SIZE) ?
								 0 : ui32ReadIndex);

				setMeccanoLEDColor(red, green, blue, time);
				// Return number of bytes received
				return 5;

            default:
            	break;
            }



            return 5;
        }

        //
        // Ignore SUSPEND and RESUME for now.
        //
        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
            break;

        //
        // Ignore all other events and return 0.
        //
        default:
            break;
    }

    return(0);
}

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
int
main(void)
{
    uint_fast32_t ui32TxCount;
    uint_fast32_t ui32RxCount;
    uint32_t ui32PLLRate;

    //
    // Run from the PLL at 120 MHz.
    //
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Configure the device pins.
    //
    PinoutSet(false, true);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, ui32SysClock);

    //
    // Not configured initially.
    //
    g_bUSBConfigured = false;

    //
    // Enable the system tick.
    //
    ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Show the application name on the display and UART output.
    //
    UARTprintf("\033[2J\nTiva C Series USB bulk device example\n");
    UARTprintf("---------------------------------\n\n");

    //
    // Tell the user what we are up to.
    //
    UARTprintf("Configuring USB... \n");

    //
    // Initialize the transmit and receive buffers.
    //
    USBBufferInit(&g_sTxBuffer);
    USBBufferInit(&g_sRxBuffer);

    //
    // Tell the USB library the CPU clock and the PLL frequency.  This is a
    // new requirement for TM4C129 devices.
    //
    SysCtlVCOGet(SYSCTL_XTAL_25MHZ, &ui32PLLRate);
    USBDCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
    USBDCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    //
    // Initialize the USB stack for device mode.
    //
    USBStackModeSet(0, eUSBModeDevice, 0);

    //
    // Pass our device information to the USB library and place the device
    // on the bus.
    //
    USBDBulkInit(0, &g_sBulkDevice);

    //
    // Wait for initial configuration to complete.
    //
    UARTprintf("Waiting for host...\r");

    //
    // Clear our local byte counters.
    //
    ui32RxCount = 0;
    ui32TxCount = 0;


    /* Initialise action list */
    left_arm_list[0].h_p = left_arm_list[0].t_p = NULL;
    left_arm_list[1].h_p = left_arm_list[1].t_p = NULL;
    left_arm_list[2].h_p = left_arm_list[2].t_p = NULL;
    left_arm_list[3].h_p = left_arm_list[3].t_p = NULL;
    right_arm_list[0].h_p = right_arm_list[0].t_p = NULL;
    right_arm_list[1].h_p = right_arm_list[1].t_p = NULL;
    right_arm_list[2].h_p = right_arm_list[2].t_p = NULL;
    right_arm_list[3].h_p = right_arm_list[3].t_p = NULL;

    //
    // Initialise millisecond timer
    //
    timerInit();

    //
    // Initialise delay functions
    //
    delay_init();

    //
    // Initialise Servo Control
    //
    initPWM();

    //
    // Initialise DC Motor Control
    //
    initDCMotor();

    //
    // Initialise the Meccano Servos and LEDs
    //
    MeccanoInit();

    //
    // Main application loop.
    //
    while(1)
    {
        //
        // Have we been asked to update the status display?
        //
        if(g_ui32Flags & COMMAND_STATUS_UPDATE)
        {
            g_ui32Flags &= ~COMMAND_STATUS_UPDATE;

            if(g_bUSBConfigured)
            {
                UARTprintf("Host Connected.            \n\n");
                UARTprintf("Data transferred:\n");
                UARTprintf("TX: %d  RX: %d                    \r",
                           g_ui32TxCount,
                           g_ui32RxCount);
            }
            else
            {
                UARTprintf("\n\nHost Disconnected.\n\n");
            }
        }

        //
        // Has there been any transmit traffic since we last checked?
        //
        if(ui32TxCount != g_ui32TxCount)
        {
            //
            // Take a snapshot of the latest transmit count.
            //
            ui32TxCount = g_ui32TxCount;

            //
            // Update the displayed buffer count information.
            //
            UARTprintf("TX: %d  RX: %d                    \r",
                       g_ui32TxCount,
                       g_ui32RxCount);
        }

        //
        // Has there been any receive traffic since we last checked?
        //
        if(ui32RxCount != g_ui32RxCount)
        {
            //
            // Take a snapshot of the latest receive count.
            //
            ui32RxCount = g_ui32RxCount;

            //
            // Update the displayed buffer count information.
            //
            UARTprintf("TX: %d  RX: %d                    \r",
                       g_ui32TxCount,
                       g_ui32RxCount);
        }
    }
}
