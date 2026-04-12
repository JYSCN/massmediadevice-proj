
#include <avr/io.h>
#include <avr/wdt.h>

void wdt_init(void) __attribute__((naked, used, section(".init3")));
void wdt_init(void) {
    MCUSR = 0;
    wdt_disable();
}
