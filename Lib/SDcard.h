#ifndef _SDCARD_H_
#define _SDCARD_H_
#include <stdint.h>
#define CM0 (0 | (1 << 6))
#define CM1 (1 | (1 << 6))
#define CM8 (8 | (1 << 6))
#define CM12 (12 | (1 << 6))
#define CM16 (16 | (1 << 6))
#define CM17 (17 | (1 << 6))
#define CM18 (18 | (1 << 6))
#define CM24 (24 | (1 << 6))
#define CM55 (55 | (1 << 6))
#define CM58 (58 | (1 << 6))
#define CM59 (59 | (1 << 6))
#define ACM41 (41 | (1 << 6))
#define BLOCK_LENGTH 512
#define NO_ERR 0
#define TIMEOUT 1
#define MID_TRANSFER_ERR 2
#define MID_TRANSFER_ERR_USB 3
extern uint8_t SD_ERROR;
extern uint8_t SD_IS_SDHC;
extern uint8_t ERROR_RESPONSE;
extern uint8_t SD_LIB_ERROR;
typedef struct {
  uint8_t cmd0;
  uint8_t cmd8;
  uint8_t cmd8_ocr[4];
  uint8_t acmd41;
  uint8_t acmd41_tries;
  uint8_t cmd58;
  uint8_t cmd58_ocr[4];
  uint8_t cmd16;
} SD_DebugInfo;

extern SD_DebugInfo SD_Debug;
uint8_t Send_SDC_CMD(uint8_t indexCMD, uint32_t argument,
                     uint8_t *additionalOutput);
uint8_t SD_Init(void);
uint8_t SDC_Read_Block(uint32_t address, uint8_t *readBuffer,
                       uint16_t *crc_checksum);
uint8_t SDC_Write_Block(uint32_t address, uint8_t *writeBuffer);

#endif
