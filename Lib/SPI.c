#include "SPI.h"


void SPI_Initialize()
{
  DDRB = (1<<DD_MOSI)|(1<<DD_SCK);
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<DORD);
  DDRE = (1<< SS);
}

uint8_t spi_transfer(uint8_t data) {
    SPDR = data;                        // Load byte to send
    while (!(SPSR & (1 << SPIF)));      // Wait for transfer complete
    return SPDR;                        // Return received byte
}
