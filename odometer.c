/*
  Copyright 2019 Jan Schlieper

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
 */

#include "odometer.h"

/*
  LED functions
 */

void led_init(void) {
    // pin 3 output
    DDRB |= (1 << PB3);
}

void led(uint8_t mode) {
    switch (mode) {
        case ON:
            PORTB |= (1 << PB3);
            break;
        case OFF:
            PORTB &= ~(1 << PB3);
            break;
        case TOGGLE:
            PORTB ^= (1 << PB3);
            break;
    }
}

/*
  EEPROM functions
 */

void eeprom_init(void) {
    // disable interrupts, enable atomic writes
    EECR = (0 << EERIE) | (0 << EEPM1) | (0 << EEPM0);
}

void eeprom_write(uint16_t addr, uint8_t data) {
    // wait for completion of write
    while (EECR & (1 << EEPE));

    // set up address
    EEAR = addr;

    // set up data
    EEDR = data;

    // write logical one to EEMPE
    EECR |= (1 << EEMPE);

    // start EEPROM write
    EECR |= (1 << EEPE);
}

uint8_t eeprom_read(uint16_t addr) {
    // wait for completion of write
    while (EECR & (1 << EEPE));

    // set up address register
    EEAR = addr;

    // start EEPROM read
    EECR |= (1 << EERE);

    return EEDR;
}

/*
  timer0 functions
 */

void timer0_init(void) {
    // Compare Timer Counter mode
    TCCR0A = (1 << WGM01) | (0 << WGM00);

    // get clock from falling edge of t0 pin
    TCCR0B = (0 << WGM02) | (1 << CS02) | (1 << CS01) | (0 << CS00);

    // initial counter value
    TCNT0 = 0;

    // set counter
    OCR0A = PULSES_PER_WHEEL_TURN - 1;

    // clear interrupt flag
    TIFR = (1 << OCF0A);

    // enable interrupt
    TIMSK |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
    wheel_turned = true;
    led_strobe = true;
}

/*
  analog comparator
 */

void ac_disable(void) {
    // clear interrupt
    ACSR &= ~(1 << ACIE);

    // turn off analog comparator
    ACSR |= (1 << ACD);
}

/*
  analog digital converter
 */

void adc_disable(void) {
    // turn off adc
    ADCSRA &= ~(1 << ADEN);

    // save power
    PRR |= (1 << PRADC);
}

/*
  Universal Serial Interface functions
 */

void usi_init(void) {
    // define RX as input
    DDRB &= ~(1 << PB0);

    // disable USI interrupt
    USICR &= ~(1 << USIOIE);

    // USI three wire mode
    USICR |= (0 << USIWM1) | (1 << USIWM0) | (0 << USICS1) | (0 << USICS0);

    // enable pin change on pin 0
    PCMSK |= (1 << PCINT0);

    // clear interrupt flag
    GIFR = (1 << PCIF);

    // enable pin change interrupt
    GIMSK |= (1 << PCIE);
}

uint8_t usi_reverse(uint8_t b) {
    b = ((b >> 1) & 0x55) | ((b << 1) & 0xaa);
    b = ((b >> 2) & 0x33) | ((b << 2) & 0xcc);
    b = ((b >> 4) & 0x0f) | ((b << 4) & 0xf0);

    return b;
}

void usi_write(uint8_t c) {
    uint8_t i;

    // disable pin change interrupt
    PCMSK &= ~(1 << PCINT0);

    // reset USI counter
    USISR = (1 << USIOIF) | 8;

    // start bit
    USIDR = (0 << 7);

    // enable TX
    DDRB |= (1 << PB1);

    // start bit delay
    _delay_us(BAUD_DELAY - 17);

    // load data
    USIDR = usi_reverse(c);
    _delay_us(BAUD_DELAY);

    for (i = 0; i < 7; i++) {
        // software strobe USI
        USICR |= (1 << USICLK);
        _delay_us(BAUD_DELAY);
    }

    // stop bit
    USIDR = (1 << 7);
    _delay_us(BAUD_DELAY);

    // disable TX
    DDRB &= ~(1 << PB1);

    // enable pin change interrupt
    PCMSK |= (1 << PCINT0);
}

int8_t usi_getchar(void) {
    int8_t data = -1;

    if (usi_rx_avail) {
        usi_rx_avail = false;
        data = usi_rx_data;
    }

    return data;
}

void usi_print(uint32_t value) {
    uint8_t i = 0;
    char buffer[16];

    // convert to ASCII
    do {
        buffer[i++] = ((value % 10) + 48) & 0x7f;
    } while ((value = value / 10));

    // write to serial
    for (; i > 0; i--) {
        usi_write(buffer[i - 1]);
    }

    usi_write('\r');
    usi_write('\n');
}

ISR(PCINT0_vect) {
    uint8_t i;

    // disable pin change interrupt
    PCMSK &= ~(1 << PCINT0);

    // reset USI counter
    USISR = (1 << USIOIF) | 8;

    // wait start bit
    _delay_us(BAUD_DELAY + 10);

    for (i = 0; i < 8; i++) {
        // software strobe USI
        USICR |= (1 << USICLK);
        _delay_us(BAUD_DELAY);
    }

    // save data
    usi_rx_data = usi_reverse(USIBR);
    usi_rx_avail = true;

    // enable pin change interrupt
    PCMSK |= (1 << PCINT0);
}

/*
  odometer functions
 */

void odometer_init(void) {
    uint8_t a, b, step = 1;
    uint16_t i;

    a = eeprom_read(0);
    for (i = 1; i < (EEPROM_SIZE - 2); i++) {
        b = eeprom_read(i);

        // find start
        if (step && (a == ((b + 1) & 0xff))) {
            eeprom_index = i;
            step = 0;
            a = b;
        }

        // eeprom inconsistent
        if (b != a) {
            eeprom_ok = false;
            break;
        }
    }
}

void odometer_increment(void) {
    uint8_t value;

    // first byte
    value = eeprom_read(eeprom_index) + 1;
    eeprom_write(eeprom_index, value);

    // second byte
    if (value == 0 && eeprom_index == (EEPROM_SIZE - 3)) {
        value = eeprom_read(EEPROM_SIZE - 2) + 1;
        eeprom_write((EEPROM_SIZE - 2), value);

        // third byte
        if (value == 0) {
            value = eeprom_read(EEPROM_SIZE - 1) + 1;
            eeprom_write((EEPROM_SIZE - 1), value);
        }
    }

    // increment pointer
    eeprom_index = (eeprom_index >= (EEPROM_SIZE - 3)) ? 0 : eeprom_index + 1;
}

uint32_t odometer_getValue(void) {
    uint8_t data, carry;
    uint16_t i;
    uint32_t value = 0;

    // carry flag
    carry = (eeprom_read(EEPROM_SIZE - 3) == 0xff) ? 1 : 0;

    // first byte chain
    for (i = 0; i < (EEPROM_SIZE - 2); i++) {
        data = eeprom_read(i);
        value = value + data;

        // apply carry if needed
        if (data == 0 && carry) value = value + 256;
    }

    // second byte
    value = value + ((uint32_t) eeprom_read(EEPROM_SIZE - 2) * (EEPROM_SIZE - 2) * 256);

    // third byte
    value = value + ((uint32_t) eeprom_read(EEPROM_SIZE - 1) * 256 * (EEPROM_SIZE - 2) * 256);

    return value;
}

void odometer_setValue(uint32_t value) {
    uint8_t divisor;
    uint16_t i;

    // third byte
    divisor = value / ((uint32_t) (EEPROM_SIZE - 2) * 256 * 256);
    eeprom_write((EEPROM_SIZE - 1), divisor);

    // second byte
    value = value % ((uint32_t) (EEPROM_SIZE - 2) * 256 * 256);
    divisor = value / ((uint32_t) (EEPROM_SIZE - 2) * 256);
    eeprom_write((EEPROM_SIZE - 2), divisor);

    // first byte chain
    value = value % ((uint32_t) (EEPROM_SIZE - 2) * 256);
    divisor = value / (uint32_t) (EEPROM_SIZE - 2);
    for (i = 0; i < (EEPROM_SIZE - 2); i++) {
        eeprom_write(i, divisor);
    }

    // first byte chain remainder
    value = value % (uint32_t) (EEPROM_SIZE - 2);
    for (i = 0; i < value; i++) {
        eeprom_write(i, divisor + 1);
    }

    // set index
    eeprom_index = value;
}

/*
  main program
 */

int main(void) {

    // save power
    wdt_disable();
    ac_disable();
    adc_disable();

    // initialize functions
    led_init();
    eeprom_init();
    odometer_init();
    usi_init();
    timer0_init();

    // enable global interrupts
    sei();

    while (eeprom_ok) {
        // update odometer
        if (wheel_turned) {
            wheel_turned = false;
            odometer_increment();
        }

        // flash led
        if (led_strobe) {
            led_strobe = false;
            led(ON);
            _delay_ms(1);
            led(OFF);
        }

        // process UART
        switch (usi_getchar()) {
            case '\r':
                usi_print(odometer_getValue());
                break;
        }

        // save power
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_enable();
        sleep_cpu();
        sleep_disable();
    }
}
