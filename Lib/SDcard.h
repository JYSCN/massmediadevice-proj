#ifndef _SDCARD_H_
#define _SDCARD_H_
#include <stdint.h>
#define CM0 0 | (1<<6)
#define CM1 1 | (1<<6)
#define CM8 8 | (1<<6)
#define CM58 58 | (1<<6)
#define CM59 59 | (1<<6)
#define ACM41 41 | (1<<6)
#define CM0CRC (0x4A <<1)
#define CM55 (55 | (1<<6))
#define CM17 (17 | (1<<6))
#define CM24 (24 | (1<<6))
#define BLOCK_LENGTH 512
#define NO_ERR 0
#define TIMEOUT 1
#define MID_TRANSFER_ERR 2
uint8_t SD_ERROR = NO_ERR;

#endif
