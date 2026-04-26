#include "pff.h"
#include "u8x8_avr.h"
#include <avr/io.h>
#include <stdint.h>
#include <u8x8.h>

// For software I2C - adjust pins for your ATmega32u4 wiring
#define I2C_CLK_DIR DDRD
#define I2C_CLK_PORT PORTD
#define I2C_CLK_PIN 0

#define I2C_DATA_DIR DDRD
#define I2C_DATA_PORT PORTD
#define I2C_DATA_PIN 1

uint8_t u8x8_gpio_and_delay_atmega32u4(u8x8_t *u8x8, uint8_t msg,
                                       uint8_t arg_int, void *arg_ptr) {

  if (u8x8_avr_delay(u8x8, msg, arg_int, arg_ptr))
    return 1;

  switch (msg) {

  default:
    u8x8_SetGPIOResult(u8x8, 1);
    break;
  }
  return 1;
}

uint8_t u8x8_byte_hw_i2c_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                            void *arg_ptr) {

  static uint8_t buffer[32];
  static uint8_t buf_idx;
  uint16_t timeout = 0;
  uint8_t *data;

  switch (msg) {
  case U8X8_MSG_BYTE_INIT:

    TWSR = 0;
    TWBR = ((F_CPU / 400000UL) - 16) / 2;
    break;

  case U8X8_MSG_BYTE_START_TRANSFER:
    buf_idx = 0;

    buffer[buf_idx++] = u8x8_GetI2CAddress(u8x8);
    break;

  case U8X8_MSG_BYTE_SEND:
    data = (uint8_t *)arg_ptr;
    while (arg_int > 0) {
      buffer[buf_idx++] = *data++;
      arg_int--;
    }
    break;

  case U8X8_MSG_BYTE_END_TRANSFER:

    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)) && timeout < 4096) {
      timeout++;
    }
    timeout = 0;
    for (uint8_t i = 0; i < buf_idx; i++) {
      timeout = 0;
      TWDR = buffer[i];
      TWCR = (1 << TWINT) | (1 << TWEN);
      while (!(TWCR & (1 << TWINT)) && timeout < 4096) {
        timeout++;
      }
    }

    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    break;

  case U8X8_MSG_BYTE_SET_DC:
    break;

  default:
    return 0;
  }
  return 1;
}

void draw_image_from_sd(const char *filename) {
  FATFS fs;
  UINT br;
  uint8_t tile[8] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
  u8x8_t u8x8;

  u8x8_Setup(&u8x8, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_i2c,
             u8x8_byte_hw_i2c_cb, u8x8_gpio_and_delay_atmega32u4);

  u8x8_InitDisplay(&u8x8);
  u8x8_SetPowerSave(&u8x8, 0);
  u8x8_ClearDisplay(&u8x8);

  u8x8_SetFont(&u8x8, u8x8_font_chroma48medium8_r);

  if (pf_mount(&fs)) {
    u8x8_DrawString(&u8x8, 0, 2, "Mount fail");
    return;
  }
  if (pf_open(filename)) {
    u8x8_DrawString(&u8x8, 0, 2, "Open fail");
    return;
  }
  u8x8_DrawString(&u8x8, 0, 2, "SD OK");

  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t col = 0; col < 16; col++) {
      pf_read(tile, 8, &br);
      u8x8_DrawTile(&u8x8, col, row, 1, tile);
    }
  }
}
