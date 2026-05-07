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
 *  Main source file for the VirtualSerialMassStorage demo. This file contains
 * the main tasks of the demo and is responsible for the initial application
 * hardware configuration.
 */
#include "Lib/DataflashManager.h"

#include "Lib/SDcard.h"
#include "Lib/SPI.h"
#include "Lib/display.h"
#include "Lib/pffconf.h"
#include "VirtualSerialMassStorage.h"

#include <stdio.h>

/** LUFA CDC Class driver interface configuration and state information. This
 * structure is passed to all CDC Class driver functions, so that multiple
 * instances of the same class within a device can be differentiated from one
 * another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
    .Config =
        {
            .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
            .DataINEndpoint =
                {
                    .Address = CDC_TX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = CDC_RX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .NotificationEndpoint =
                {
                    .Address = CDC_NOTIFICATION_EPADDR,
                    .Size = CDC_NOTIFICATION_EPSIZE,
                    .Banks = 1,
                },
        },
};

/** LUFA Mass Storage Class driver interface configuration and state
 * information. This structure is passed to all Mass Storage Class driver
 * functions, so that multiple instances of the same class within a device can
 * be differentiated from one another.
 */
USB_ClassInfo_MS_Device_t Disk_MS_Interface = {
    .Config =
        {
            .InterfaceNumber = INTERFACE_ID_MassStorage,
            .DataINEndpoint =
                {
                    .Address = MASS_STORAGE_IN_EPADDR,
                    .Size = MASS_STORAGE_IO_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = MASS_STORAGE_OUT_EPADDR,
                    .Size = MASS_STORAGE_IO_EPSIZE,
                    .Banks = 1,
                },
            .TotalLUNs = TOTAL_LUNS,
        },
};

#define BLOCK_LENGTH 512 // if not already defined in SDcard.h
#define PRINT(fmt, ...)                                                        \
  do {                                                                         \
    printf(fmt, ##__VA_ARGS__);                                                \
    CDC_Device_Flush(&VirtualSerial_CDC_Interface);                            \
  } while (0)



/** Standard file stream for the CDC interface when set up, so that the virtual
 * CDC COM port can be used like any regular character stream in the C APIs.
 */
static FILE USBSerialStream;

/** Main program entry point. This routine contains the overall program flow,
 * including initial setup of all components and the main program loop.
 */
int main(void) {
  clock_prescale_set(clock_div_1);
  SetupHardware();

  CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);
  stdout = &USBSerialStream;
  // LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
  GlobalInterruptEnable();
  SD_Init();
  draw_image_from_sd("IMAGE.BIN");
  // Run SD init before USB — results stored in SD_Debug

  // Wait for USB to connect
  while (USB_DeviceState != DEVICE_STATE_Configured) {
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();
  }


  CDC_Device_Flush(&VirtualSerial_CDC_Interface);

  for (;;) {
    MS_Device_USBTask(&Disk_MS_Interface);
    uint8_t HostReady =
        (VirtualSerial_CDC_Interface.State.ControlLineStates.HostToDevice &
         CDC_CONTROL_LINE_OUT_DTR);

    if (HostReady && (SD_ERROR != 0)) {
      PRINT("SD Error = 0x%02X, SD Error Result = 0x%02X, SD Init = 0x%02X, "
            "SD_LIB_ERROR = 0x%02X, USB_READ_ERR = 0x%02X, IS SD Card = "
            "0x%02X\r\n",
            SD_ERROR, ERROR_RESPONSE, 0x00, SD_LIB_ERROR, USB_READ_ERR,
            SD_IS_SDHC);
    }

    SD_ERROR = 0;
    ERROR_RESPONSE = 0;
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();
  }
}

/** Configures the board hardware and chip peripherals for the demo's
 * functionality. */
void SetupHardware(void) {
#if (ARCH == ARCH_AVR8)
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  /* Disable clock division */
  clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
  /* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the
   * CPU core to run from it */
  XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
  XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

  /* Start the 32MHz internal RC oscillator and start the DFLL to increase it to
   * 48MHz using the USB SOF as a reference */
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

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void) {
  // LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void) {
  // LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
  bool ConfigSuccess = true;

  ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
  ConfigSuccess &= MS_Device_ConfigureEndpoints(&Disk_MS_Interface);

  // LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) {
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
  MS_Device_ProcessControlRequest(&Disk_MS_Interface);
}

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface
 * configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
  /* You can get changes to the virtual CDC lines in this callback; a common
     use-case is to use the Data Terminal Ready (DTR) flag to enable and
     disable CDC communications in your application when set to avoid the
     application blocking while waiting for a host to become ready and read
     in the pending data from the USB endpoints.
  */
  bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
                    CDC_CONTROL_LINE_OUT_DTR) != 0;

  (void)HostReady;
}

/** Mass Storage class driver callback function the reception of SCSI commands
 * from the host, which must be processed.
 *
 *  \param[in] MSInterfaceInfo  Pointer to the Mass Storage class interface
 * configuration structure being referenced
 */
bool CALLBACK_MS_Device_SCSICommandReceived(
    USB_ClassInfo_MS_Device_t *const MSInterfaceInfo) {
  bool CommandSuccess;

  // LEDs_SetAllLEDs(LEDMASK_USB_BUSY);
  CommandSuccess = SCSI_DecodeSCSICommand(MSInterfaceInfo);
  // LEDs_SetAllLEDs(LEDMASK_USB_READY);

  return CommandSuccess;
}
