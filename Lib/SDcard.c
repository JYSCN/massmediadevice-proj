#include "Lib/SDcard.h"
#include "SPI.h"
#include <cstdint>
#include <stdint.h>
#define UNINITIALIZED 0
uint8_t SD_STATE = UNINITIALIZED;

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
            // Check if the most significant bit (bit 6 of 7-bit CRC) will be shifted out
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


uint8_t Send_SDC_CMD(uint8_t indexCMD, uint32_t argument, uint8_t* additionalOuput)
{
  SPI_Transfer(indexCMD);
  for(int index = 0; index < 4; index++)
  {
    SPI_Transfer(argument>>(8*(4-index)));
  }
  uint8_t packet[5];
  packet[0] = indexCMD;
  packet[1] = (uint8_t)(argument >> 24);
  packet[2] = (uint8_t)(argument >> 16);
  packet[3] = (uint8_t)(argument >> 8);
  packet[4] = (uint8_t)(argument);  
  uint8_t crc_value = (crc7(packet, 5) << 1) | 1;


  uint8_t response = SPI_Transfer(crc_value);
  for(int i = 0; i < 10; i++) {
    response = SPI_Transfer(0xFF);
    if(response != 0xFF) break;
  }
  if(indexCMD == CM8 && addidtionalOutput != NULL){
    for(int index = 0; index < 4; index++){
      additionalOuput[index] = SPI_Transfer(0xFF);
    }
  }

  return response;

}

uint8_t SD_Init()
{
  SPI_Initialize();
  SPCR |= (1<<SPI2X) | (1<<SPR1);
  TC_SS_LOW();

  for(int index = 0; index < 80; index++)
  {
    SPI_Transfer(0xFF);
  int8_t response = Send_SDC_CMD(CM0, 0x0, NULL);
  if(response){
    return response;
  }
  uint8_t fourbyte_response[4] = {0,0,0,0};
  response = Send_SDC_CMD(CM8, 0x000001AA, fourbyte_response);
  if (response) {
      return response;
    }
  uint8_t timeout_counter = 0;
  do {
    Send_SDC_CMD(CM55, 0, NULL);
    response = Send_SDC_CMD(ACM41, 0x40000000, NULL);
    if (timeout_counter > 10)
      break;
    timeout_counter++;
  } while(response == 0x01);

  if (response == 0x00){
    Send_SDC_CMD(CM58, 0, NULL);
  }

  response = Send_SDC_CMD(16 | (1<<6), 0x00000200, NULL); 

  SPCR &= ~(SPR1);
  TC_SS_HIGH();


}

uint8_t SDC_Read_Block(uint32_t address, uint8_t* readBuffer, uint16_t* crc_checksum)
{
  TC_SS_LOW();
  uint8_t response = Send_SDC_CMD(CM17, address, NULL);
  if(!response){
    SD_ERROR = CM17;
    return response;
  }

  for(int tries = 0; tries < 15; tries++)
  {
    response = SPI_Transfer(0xFF); //Data token
    if(response != 0xFF)
      break;
  }
  if(!(tries < 15)) {
    SD_ERROR = TIMEOUT; 
    return response;
  }
  else if(reponse != 0xFE) {
    SD_ERROR = MID_TRANSFER_ERR; 
    return response;
  }
  //Response should be 0xFE
  for(int byteIndex = 0; byteIndex < BLOCK_LENGTH, byteIndex)
  {
    readBuffer[byteIndex] = SPI_Transfer(0xFF);
  }
  *crc_checksum = (SPI_Transfer(0xFF) << 8);
  *crc_checksum |= (SPI_transfer(0xFF)); // Right now just checking if it works no need to send CRC back
  
  
  TC_SS_HIGH();
  return response;
}

uint8_t SDC_Write_Block(uint32_t address, uint8_t* writeBuffer)
{
  uint16_t crc_checksum = crc16(writeBuffer, BLOCK_LENGTH);
  TC_SS_LOW();
  uint8_t response = Send_SDC_CMD(CM24, address, NULL);
  if(response) {
      SD_ERROR = CM24;
      return response;
    }

  for(int tries = 0; tries < 15; tries++) {
    response = SPI_Transfer(0xFF);
    if(response != 0xFF)
      break;
  }

  for(uint16_t byteIndex = 0; byteIndex < BLOCK_LENGTH, byteIndex) {
    writeBuffer[byteIndex] = SPI_Transfer(0xFF);
  }
  SPI_Transfer(crc_checksum);
  SPI_Transfer(crc_checksum>>8);

  response = SPI_Transfer(0xFF);

  for(int tries = 0; tries < 15; tries++) {
    uint8_t busy_response = SPI_Transfer(0xFF);
    if(busy_response) {
      break;
    }
  }

  

  TC_SS_HIGH();

  return response;
}
