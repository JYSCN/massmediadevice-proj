/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2014      */
/*-----------------------------------------------------------------------*/
#include "diskio.h"
#include "../../Lib/SDcard.h"
#include <string.h>
uint8_t DISK_STATUS = 0;
DSTATUS disk_initialize(void) {
  if (SD_Init() == 0) {
    DISK_STATUS = 0;
    return 0;
  }

  DISK_STATUS = STA_NOINIT;

  return STA_NOINIT;
}

DRESULT disk_readp(BYTE *buff, DWORD sector, UINT offset, UINT count) {
  if (DISK_STATUS == STA_NOINIT)
    return RES_NOTRDY;

  uint16_t crc;
  uint8_t response = SDC_Read_Block(sector, block_buf, &crc);

  if (response != 0xFE)
    return RES_ERROR;

  if (buff) {
    memcpy(buff, block_buf + offset, count);
  }
  return RES_OK;
}

DRESULT disk_writep(const BYTE *buff, DWORD sc) { return RES_ERROR; }
