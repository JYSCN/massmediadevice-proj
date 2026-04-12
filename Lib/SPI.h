#ifndef _SPI_H_
#define _SPI_H_
#include <avr/io.h>
#include <stdint.h>
#define DD_MOSI 2
#define DD_SCK 1
#define SS 6

#define TC_SS_LOW()   PORTE &= ~(1<<SS)
#define TC_SS_HIGH()  PORTE |= (1<<SS)
void SPI_Initialize(void);
uint8_t SPI_Transfer(uint8_t data);

#endif
