/*
             LUFA Library
     Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2021  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the VirtualSerialMassStorage demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */
#include "VirtualSerialMassStorage.h"
#include "Lib/SDcard.h"
#include "Lib/SPI.h"
#include <stdio.h>


/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber         = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint                 =
					{
						.Address                = CDC_TX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.DataOUTEndpoint                =
					{
						.Address                = CDC_RX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.NotificationEndpoint           =
					{
						.Address                = CDC_NOTIFICATION_EPADDR,
						.Size                   = CDC_NOTIFICATION_EPSIZE,
						.Banks                  = 1,
					},
			},
	};

/** LUFA Mass Storage Class driver interface configuration and state information. This structure is
 *  passed to all Mass Storage Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MS_Device_t Disk_MS_Interface =
	{
		.Config =
			{
				.InterfaceNumber                = INTERFACE_ID_MassStorage,
				.DataINEndpoint                 =
					{
						.Address                = MASS_STORAGE_IN_EPADDR,
						.Size                   = MASS_STORAGE_IO_EPSIZE,
						.Banks                  = 1,
					},
				.DataOUTEndpoint                =
					{
						.Address                = MASS_STORAGE_OUT_EPADDR,
						.Size                   = MASS_STORAGE_IO_EPSIZE,
						.Banks                  = 1,
					},
				.TotalLUNs                      = TOTAL_LUNS,
			},
	};

#define BLOCK_LENGTH 512  // if not already defined in SDcard.h
#define PRINT(fmt, ...) do { printf(fmt, ##__VA_ARGS__); CDC_Device_Flush(&VirtualSerial_CDC_Interface); } while(0)
static uint8_t sd_block_buffer[BLOCK_LENGTH];

void Test_Read_Block(uint32_t address) {

    uint16_t crc = 0;

    printf("Reading block at address 0x%08lX...\r\n", address);

    uint8_t response = SDC_Read_Block(address, sd_block_buffer, &crc);

    if (response != 0xFE) {
        printf("Read failed! Response: 0x%02X  SD_ERROR: 0x%02X\r\n", response, SD_ERROR);
        return;
    }

    printf("CRC: 0x%04X\r\n", crc);
    printf("Data:\r\n");

    // Print hex dump — 16 bytes per row
    for (int i = 0; i < BLOCK_LENGTH; i++) {
        if (i % 16 == 0)
            printf("\r\n%04X: ", i);   // row address
        printf("%02X ", sd_block_buffer[i]);
    }

    printf("\r\n--- End of Block ---\r\n");

    // Flush so data actually goes out
    CDC_Device_Flush(&VirtualSerial_CDC_Interface);
}

// Read a string from USB serial until newline
static void USB_ReadLine(char *buf, uint8_t maxlen) {
    uint8_t i = 0;
    while (i < maxlen - 1) {
        // Wait for a byte
        while (!CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface)) {
            CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
            USB_USBTask();
        }
        char c = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

        // Echo character back so user can see what they type
        printf("%c", c);
        CDC_Device_Flush(&VirtualSerial_CDC_Interface);

        if (c == '\r' || c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
}

void SD_CommandLoop(void) {
    char input[16];

    // Reset card state before every command
    TC_SS_HIGH();
    for (int i = 0; i < 8; i++)
        SPI_Transfer(0xFF);   // 8 dummy bytes to fully flush card state

    printf("\r\nEnter block address (hex, e.g. 0A3F): ");
    CDC_Device_Flush(&VirtualSerial_CDC_Interface);

    USB_ReadLine(input, sizeof(input));

    uint32_t address = 0;
    if (sscanf(input, "%lx", &address) != 1) {
        printf("\r\nInvalid address.\r\n");
        CDC_Device_Flush(&VirtualSerial_CDC_Interface);
        return;
    }

    printf("\r\n");
    Test_Read_Block(address);
}



/** Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be
 *  used like any regular character stream in the C APIs.
 */
static FILE USBSerialStream;


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
  clock_prescale_set(clock_div_1);
    SetupHardware();

    CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);
    stdout = &USBSerialStream;
    LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
    GlobalInterruptEnable();

    // Run SD init before USB — results stored in SD_Debug
    uint8_t sd_result = SD_Init();

    // Wait for USB to connect
    while (USB_DeviceState != DEVICE_STATE_Configured) {
        CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
        USB_USBTask();
    }

    // Small delay to let the terminal open
    _delay_ms(500);

    // Now dump everything we captured
    PRINT("=== SD Init Debug ===\r\n");
    PRINT("CMD0:   0x%02X\r\n", SD_Debug.cmd0);
    PRINT("CMD8:   0x%02X  OCR: %02X %02X %02X %02X\r\n",
        SD_Debug.cmd8,
        SD_Debug.cmd8_ocr[0], SD_Debug.cmd8_ocr[1],
        SD_Debug.cmd8_ocr[2], SD_Debug.cmd8_ocr[3]);
    PRINT("ACMD41: 0x%02X  tries: %d\r\n", SD_Debug.acmd41, SD_Debug.acmd41_tries);
    PRINT("CMD58:  0x%02X  SDHC: %d  OCR: %02X %02X %02X %02X\r\n",
        SD_Debug.cmd58, SD_IS_SDHC,
        SD_Debug.cmd58_ocr[0], SD_Debug.cmd58_ocr[1],
        SD_Debug.cmd58_ocr[2], SD_Debug.cmd58_ocr[3]);
    PRINT("CMD16:  0x%02X\r\n", SD_Debug.cmd16);
    PRINT("Init result: 0x%02X\r\n", sd_result);
    PRINT("=====================\r\n");
    CDC_Device_Flush(&VirtualSerial_CDC_Interface);

    for (;;) {
        SD_CommandLoop();
        CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
        USB_USBTask();
    }
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();

	/* Check if the Dataflash is working, abort if not */

	/* Clear Dataflash sector protections, if enabled */

}

/** Checks for changes in the position of the board joystick, sending strings to the host upon each change. */
void CheckJoystickMovement(void)
{
	uint8_t     JoyStatus_LCL = Joystick_GetStatus();
	char*       ReportString  = NULL;
	static bool ActionSent    = false;

	if (JoyStatus_LCL & JOY_UP)
	  ReportString = "Joystick Up\r\n";
	else if (JoyStatus_LCL & JOY_DOWN)
	  ReportString = "Joystick Down\r\n";
	else if (JoyStatus_LCL & JOY_LEFT)
	  ReportString = "Joystick Left\r\n";
	else if (JoyStatus_LCL & JOY_RIGHT)
	  ReportString = "Joystick Right\r\n";
	else if (JoyStatus_LCL & JOY_PRESS)
	  ReportString = "Joystick Pressed\r\n";
	else
	  ActionSent = false;

	if ((ReportString != NULL) && (ActionSent == false))
	{
		ActionSent = true;

		/* Write the string to the virtual COM port via the created character stream */
		fputs(ReportString, &USBSerialStream);

		/* Alternatively, without the stream: */
		// CDC_Device_SendString(&VirtualSerial_CDC_Interface, ReportString);
	}
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	ConfigSuccess &= MS_Device_ConfigureEndpoints(&Disk_MS_Interface);

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
	MS_Device_ProcessControlRequest(&Disk_MS_Interface);
}

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
	/* You can get changes to the virtual CDC lines in this callback; a common
	   use-case is to use the Data Terminal Ready (DTR) flag to enable and
	   disable CDC communications in your application when set to avoid the
	   application blocking while waiting for a host to become ready and read
	   in the pending data from the USB endpoints.
	*/
	bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) != 0;

	(void)HostReady;
}

/** Mass Storage class driver callback function the reception of SCSI commands from the host, which must be processed.
 *
 *  \param[in] MSInterfaceInfo  Pointer to the Mass Storage class interface configuration structure being referenced
 */
bool CALLBACK_MS_Device_SCSICommandReceived(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
	bool CommandSuccess;

	LEDs_SetAllLEDs(LEDMASK_USB_BUSY);
	CommandSuccess = SCSI_DecodeSCSICommand(MSInterfaceInfo);
	LEDs_SetAllLEDs(LEDMASK_USB_READY);

	return CommandSuccess;
}

