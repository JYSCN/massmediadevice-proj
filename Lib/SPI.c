#include "SPI.h"

void SPI_Initialize(void) {
  DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2); 
  PORTB |= (1 << PB0);                             

  
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
  SPSR &= ~(1 << SPI2X);

  
  DDRE |= (1 << PORTE6); 
  TC_SS_HIGH();
}


