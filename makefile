#
#             LUFA Library
#     Copyright (C) Dean Camera, 2021.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = USBKEY
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = main
SRC          = $(TARGET).c Descriptors.c wdt_fix.c Lib/SDcard.c Lib/SPI.c Lib/DataflashManager.c Lib/SCSI.c Lib/display.c PetitFs/source/diskio.c PetitFs/source/pff.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
U8G2_PATH = ./u8g2/csrc
SRC += $(filter-out $(wildcard $(U8G2_PATH)/u8x8_d_*.c) $(wildcard $(U8G2_PATH)/u8g2_d_*.c), \
       $(wildcard $(U8G2_PATH)/*.c))
SRC += $(U8G2_PATH)/u8x8_d_ssd1306_128x64_noname.c
SRC += ./u8g2/sys/avr/avr-libc/lib/u8x8_avr.c
LUFA_PATH    = ./lufa/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/ -IPetitFs/source/ -Iu8g2/csrc/ -Iu8g2/sys/avr/avr-libc/lib/
LD_FLAGS     =


# Default target
all:

# Include LUFA-specific DMBS extension modules
DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

# Include common DMBS build system modules
DMBS_PATH      ?= $(LUFA_PATH)/Build/DMBS/DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk
