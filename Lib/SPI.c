#include "SPI.h"

void SPI_Initialize(void) {
  DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2); // SS, SCK, MOSI
  PORTB |= (1 << PB0);                             // Keep SS High

  // Start at 125kHz
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
  SPSR &= ~(1 << SPI2X);

  // Your actual SD CS pin
  DDRE |= (1 << PORTE6); // Assuming PE6 is your CS
  TC_SS_HIGH();
}

uint8_t SPI_Transfer(uint8_t data) {
  SPDR = data; // Load byte to send
  while (!(SPSR & (1 << SPIF)))
    ;          // Wait for transfer complete
  return SPDR; // Return received byte
}
