/*
 * RelayWatchdog.c
 *
 * Created: 2017-04-09 20:05:47
 *  Author: mikael
 */

/*
 * Hardware setup:
 * ATmega328 or similar at about 1MHz
 * PD4-PD7 watchdog input signals. Toggle.
 * PC0-PC3 relay output signals. Active high
 * PB0-PB3 led indicators. Active high
 */

#define F_CPU   1000000L

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <stdbool.h>

// Timers are incremented every 2 seconds, since that makes
// 60*60*24/2=43200 ticks per day. That fits in uint16_t.
static uint16_t _wdtTimers[4];      // The time when each channel is reset
static uint16_t _wdtTimeouts[4];    // The timeout time for each channel that'll trigger a relay break.
static uint16_t _wdtWarning[4];     // The leds will start flashing rappidly at this time
static uint16_t _wdtNotify[4];      // The leds will indicate at this threashold
static volatile uint16_t _wdtTimer; // Global watchdog timer
static volatile uint8_t _partialTimer;  // Runs at 16 ticks per second, or 32 ticks per wdt timer tick

ISR(TIMER0_OVF_vect)
{
    _partialTimer ++;
    if (_partialTimer >= 32) {
        _wdtTimer ++;   // Increment every 2 seconds
        _partialTimer = 0;
    }
    TCNT0 = 12;     // 256 - 1MHz/256/16
}

// Interrupt on any pin change on PD4-7.
// Such pin change resets the corresponding watchdog timer.
static volatile uint8_t _wdtLastState;
ISR(PCINT2_vect)
{
    // PCINT20->PCINT23
    uint8_t newState = PIND;
    for (uint8_t i=0 ; i < 4 ; i ++) {
        uint8_t msk = 1 << (i+4);
        if ( (newState&msk) != (_wdtLastState&msk) ) {
            _wdtTimers[i] = _wdtTimer;
        }
    }
    _wdtLastState = newState;
}


void setup()
{
    // LED indicators
    PORTB = 0x00;
    DDRB = 0x0F;

    // Relay drivers
    PORTC = 0x00;
    //DDRC = 0x0F;

    // Watchdog inputs
    PCMSK2 |= _BV(PCINT20) | _BV(PCINT21) | _BV(PCINT22) | _BV(PCINT23);
    PCICR |= _BV(PCIE2);
    DDRD &= ~(0xF0);

    // Timer setup
    TCCR0A = 0; // Normal operation
    TCCR0B = 0x04; // prescaler (0100 -> clk/256)
    TIMSK0 = _BV(TOIE0);
    _wdtTimer = 0;
    _partialTimer = 0;

    // Watchdog timeouts
    for (uint8_t i=0 ; i < 4 ; i ++) {
        _wdtTimeouts[i] = 30;     // 60/2*60*12
        _wdtWarning[i] = 15;
        _wdtNotify[i] = 10;
        _wdtTimers[i] = 0;
    }

    // Power saving options
    PRR = _BV(PRTWI) | _BV(PRADC);   // Power reduction: TWI, ADC

    sei();
}

void loop()
{
    for (uint8_t i=0 ; i < 4 ; i ++)
    {
        uint16_t timespan = _wdtTimer - _wdtTimers[i];
        if (timespan > _wdtTimeouts[i]) {
            // reset relays
            PORTB |= 1 << i;
            PORTC |= 1 << i;
            _delay_ms(250);
            PORTC = 0x00;
            PORTB &= ~(1 << i);
            _wdtTimers[i] = _wdtTimer;
        }
        else if (timespan > _wdtWarning[i]) {
            // Close to timeout. Fast blink led.
            if (_partialTimer & 0x01) {
                PORTB |= 1 << i;
            }
            else {
                PORTB &= ~(1 << i);
            }
        }
        else if (timespan > _wdtNotify[i]) {
            // Half timeout. Slow led blink.
            if (_partialTimer & 0x08) {
                PORTB |= 1 << i;
            }
            else {
                PORTB &= ~(1 << i);
            }
        }
        else {
            PORTB &= ~(1 << i);
        }
    }
}

int main(void)
{
    setup();
    while(1)
    {
        loop();
    }

}