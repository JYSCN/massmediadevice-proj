#include "SPI.h"


void SPI_Initialize(void)
{
    DDRB = (1 << DD_MOSI) | (1 << DD_SCK) | (1 << PB0);
    PORTB |= (1 << PB0);

    // Start at 125kHz for init: SPR1=1, SPR0=1, SPI2X=0
    // 16MHz / 128 = 125kHz
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
    SPSR &= ~(1 << SPI2X);

    DDRE = (1 << SS);
    TC_SS_HIGH();
}

uint8_t SPI_Transfer(uint8_t data) {
    SPDR = data;                        // Load byte to send
    while (!(SPSR & (1 << SPIF)));      // Wait for transfer complete
    return SPDR;                        // Return received byte
}
