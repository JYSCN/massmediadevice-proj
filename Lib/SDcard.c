#include "Lib/SDcard.h"
#include "SPI.h"
#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>
#define NEWMACRO 1
SD_DebugInfo SD_Debug = {0};
#define UNINITIALIZED 0
uint8_t SD_ERROR = UNINITIALIZED;
uint8_t SD_IS_SDHC = 0;
uint8_t ERROR_RESPONSE = 0;
uint8_t SD_LIB_ERROR = 0;

uint16_t crc16(uint8_t *data, int len) {
  uint16_t crc = 0;
  for (int i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021; // Polynomial 0x1021
      else
        crc <<= 1;
    }
  }
  return crc;
}

uint8_t crc7(uint8_t *data, int len) {
  uint8_t crc = 0;
  for (int i = 0; i < len; i++) {
    uint8_t byte = data[i];
    for (int j = 0; j < 8; j++) {
      // Check if the most significant bit (bit 6 of 7-bit CRC) will be shifted
      // out
      uint8_t bit = (byte >> (7 - j)) & 0x01;
      if ((crc >> 6) ^ bit) {
        crc = ((crc << 1) ^ 0x09) & 0x7F;
      } else {
        crc = (crc << 1) & 0x7F;
      }
    }
  }
  return crc;
}

uint8_t Send_SDC_CMD(uint8_t indexCMD, uint32_t argument,
                     uint8_t *additionalOutput) {
  SPI_Transfer(indexCMD);
  for (int index = 0; index < 4; index++) {
    SPI_Transfer(argument >> (8 * (3 - index)));
  }
  uint8_t packet[5];
  packet[0] = indexCMD;
  packet[1] = (uint8_t)(argument >> 24);
  packet[2] = (uint8_t)(argument >> 16);
  packet[3] = (uint8_t)(argument >> 8);
  packet[4] = (uint8_t)(argument);
  uint8_t crc_value = (crc7(packet, 5) << 1) | 1;

  uint8_t response = SPI_Transfer(crc_value);
  for (uint16_t i = 0; i < 255; i++) {
    response = SPI_Transfer(0xFF);
    if (response != 0xFF)
      break;
  }
  if (indexCMD == CM8 && additionalOutput != NULL) {
    for (int index = 0; index < 4; index++) {
      additionalOutput[index] = SPI_Transfer(0xFF);
    }
  }

  // Also handle CMD58 additional output
  if (indexCMD == CM58 && additionalOutput != NULL) {
    for (int index = 0; index < 4; index++) {
      additionalOutput[index] = SPI_Transfer(0xFF);
    }
  }
  return response;
}

uint8_t SD_Init(void) {
  SPI_Initialize();
  SPSR |= (1 << SPI2X);
  SPCR |= (1 << SPR1);

  TC_SS_HIGH();
  for (int index = 0; index < 80; index++)
    SPI_Transfer(0xFF);
  TC_SS_LOW();
  SPI_Transfer(0xFF);

  uint8_t response = Send_SDC_CMD(CM0, 0x0, NULL);
  SD_Debug.cmd0 = response;
  if (response != 0x01) {
    TC_SS_HIGH();
    return response;
  }

  uint8_t fourbyte_response[4] = {0};
  response = Send_SDC_CMD(CM8, 0x000001AA, fourbyte_response);
  SD_Debug.cmd8 = response;
  // 0x01 = v2 card, 0x05 = v1 card (illegal cmd), both ok
  if (response != 0x01 && response != 0x05) {
    TC_SS_HIGH();
    return response;
  }

  uint8_t timeout_counter = 0;
  do {
    TC_SS_HIGH();
    SPI_Transfer(0xFF);
    TC_SS_LOW();
    Send_SDC_CMD(CM55, 0, NULL);
    TC_SS_HIGH();
    SPI_Transfer(0xFF);
    TC_SS_LOW();
    response = Send_SDC_CMD(ACM41, 0x40000000, NULL);
    _delay_ms(10);
    if (timeout_counter++ > 100) {
      TC_SS_HIGH();
      return 0xFF;
    }
  } while (response == 0x01);
  SD_Debug.acmd41 = response;
  SD_Debug.acmd41_tries = timeout_counter;
  if (response != 0x00) {
    TC_SS_HIGH();
    return response;
  }

  uint8_t ocr[4] = {0};
  response = Send_SDC_CMD(CM58, 0, SD_Debug.cmd58_ocr);
  SD_Debug.cmd58 = response;
  SD_IS_SDHC = (SD_Debug.cmd58_ocr[0] & 0x40) ? 1 : 0;
  SD_IS_SDHC = 1;

  response = Send_SDC_CMD(CM16, 0x00000200, NULL);
  SD_Debug.cmd16 = response;

  TC_SS_HIGH();

  for (uint8_t i = 0; i < 10; i++)
    SPI_Transfer(0xFF);

  SPCR = (1 << SPE) | (1 << MSTR);
  SPSR |= (1 << SPI2X);

  for (uint8_t i = 0; i < 10; i++)
    SPI_Transfer(0xFF);

  return 0x00;
}

uint8_t SDC_Read_Block(uint32_t address, uint8_t *readBuffer,
                       uint16_t *crc_checksum) {

  TC_SS_HIGH();
  for (int i = 0; i < 8; i++)
    SPI_Transfer(0xFF);
  TC_SS_LOW();
  SPI_Transfer(0xFF); // dummy byte after CS goes low
  uint32_t send_address = SD_IS_SDHC ? address : address * BLOCK_LENGTH;
  uint8_t response = Send_SDC_CMD(CM17, send_address, NULL);
  if (response) {
    SD_ERROR = CM17;
    ERROR_RESPONSE = response;
    return response;
  }
  uint32_t tries = 0;
  for (; tries < 200000; tries++) {
    response = SPI_Transfer(0xFF); // Data token
    if (response == 0xFE)
      break;
  }
  if (!(tries < 200000)) {
    SD_ERROR = CM17;
    SD_LIB_ERROR = TIMEOUT;
    ERROR_RESPONSE = response;
    return response;
  } else if (response != 0xFE) {
    SD_ERROR = CM17;
    SD_LIB_ERROR = MID_TRANSFER_ERR;
    ERROR_RESPONSE = response;
    return response;
  }
  // Response should be 0xFE
  for (int byteIndex = 0; byteIndex < BLOCK_LENGTH; byteIndex++) {
    readBuffer[byteIndex] = SPI_Transfer(0xFF);
  }
  *crc_checksum = (SPI_Transfer(0xFF) << 8);
  *crc_checksum |= (SPI_Transfer(
      0xFF)); // Right now just checking if it works no need to send CRC back

  TC_SS_HIGH();
  // ← 8 trailing clocks
  return response;
}

uint8_t SDC_Write_Block(uint32_t address, uint8_t *writeBuffer) {
  uint16_t crc_checksum = crc16(writeBuffer, BLOCK_LENGTH);

  TC_SS_HIGH();
  SPI_Transfer(0xFF);
  TC_SS_LOW();
  SPI_Transfer(0xFF);
  uint32_t send_address = SD_IS_SDHC ? address : address * 512;
  uint8_t response = Send_SDC_CMD(CM24, send_address, NULL);
  if (response) {
    SD_ERROR = CM24;
    return response;
  }
  SPI_Transfer(0xFF);
  SPI_Transfer(0xFE);

  for (uint16_t byteIndex = 0; byteIndex < BLOCK_LENGTH; byteIndex++) {
    SPI_Transfer(writeBuffer[byteIndex]);
  }
  SPI_Transfer(crc_checksum >> 8);
  SPI_Transfer(crc_checksum);

  response = SPI_Transfer(0xFF);

  for (int tries = 0; tries < 15; tries++) {
    response = SPI_Transfer(0xFF);
    if (response != 0x00)
      break;
  }

  for (int tries = 0; tries < 15; tries++) {
    uint8_t busy_response = SPI_Transfer(0xFF);
    if (busy_response) {
      break;
    }
  }

  TC_SS_HIGH();
  for (int i = 0; i < 8; i++)
    SPI_Transfer(0xFF);

  return response;
}
