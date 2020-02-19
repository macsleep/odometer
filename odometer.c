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
    // enable atomic writes, disable interrupts
    EECR = (0 << EEPM1) | (0 << EEPM0) | (0 << EERIE);
}

void eeprom_write(uint16_t addr, uint8_t data) {
    // wait for completion of write
    while (EECR & (1 << EEPE));

    // set up address
    EEAR = addr;

    // set up data
    EEDR = data;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // write logical one to EEMPE
        EECR |= (1 << EEMPE);

        // start EEPROM write
        EECR |= (1 << EEPE);
    }
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

uint8_t eeprom_busy(void) {
    // eeprom program enable
    return (EECR & (1 << EEPE));
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
    TIFR |= (1 << OCF0A);

    // enable interrupt
    TIMSK |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
    wheel_turned = true;
#ifndef NOLED
    led_strobe = true;
#endif
}

/*
  analog comparator
 */

void ac_disable(void) {
    // disable interrupt
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
    // internal oscillator calibration
    // OSCCAL += -5;

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

int usi_putchar(char c) {
    uint8_t i, data;

    // bit swap
    data = usi_reverse(c);

    // disable pin change interrupt
    PCMSK &= ~(1 << PCINT0);

    // reset USI counter
    USISR = (1 << USIOIF) | 8;

    // start bit
    USIDR = (0 << 7);

    // enable TX/start bit
    DDRB |= (1 << PB1);
    _delay_us(USI_BAUD_DELAY - 2);

    // load data
    USIDR = data;
    _delay_us(USI_BAUD_DELAY - 4);

    for (i = 0; i < 7; i++) {
        // software strobe USI
        USICR |= (1 << USICLK);
        _delay_us(USI_BAUD_DELAY - 8);
    }

    // stop bit
    USIDR = (1 << 7);
    _delay_us(USI_BAUD_DELAY);

    // disable TX
    DDRB &= ~(1 << PB1);

    // enable pin change interrupt
    PCMSK |= (1 << PCINT0);

    return 0;
}

int usi_getchar(void) {
    int data = -1;

    if (usi_rx_head != usi_rx_tail) {
        // move tail
        usi_rx_tail++;
        if (usi_rx_tail >= USI_RX_BUFFER_SIZE) usi_rx_tail = 0;

        // save data
        data = usi_rx_data[usi_rx_tail];
    }

    return data;
}

ISR(PCINT0_vect) {
    uint8_t i;

    // disable pin change interrupt
    PCMSK &= ~(1 << PCINT0);

    // reset USI counter
    USISR = (1 << USIOIF) | 8;

    // wait start bit
    _delay_us(USI_BAUD_DELAY - 2);

    for (i = 0; i < 8; i++) {
        // software strobe USI
        USICR |= (1 << USICLK);
        _delay_us(USI_BAUD_DELAY - 8);
    }

    // move head
    usi_rx_head++;
    if (usi_rx_head >= USI_RX_BUFFER_SIZE) usi_rx_head = 0;

    // save data
    usi_rx_data[usi_rx_head] = usi_reverse(USIBR);

    // overflow
    if (usi_rx_tail == usi_rx_head) usi_rx_tail++;
    if (usi_rx_tail >= USI_RX_BUFFER_SIZE) usi_rx_tail = 0;

    // enable pin change interrupt
    PCMSK |= (1 << PCINT0);
}

/*
  odometer functions
 */

void odometer_init(void) {
    uint8_t a, b, exec_high = true, exec_low = true;
    int16_t i, i_high = 0, i_low = -1;

    a = eeprom_read(EEPROM_SIZE - 1);
    for (i = 0; i < EEPROM_SIZE; i++) {
        b = eeprom_read(i);

        // step in high nibbles
        if ((((a >> 4) + 1) & 0x0f) == (b >> 4)) {
            a = (b & 0xf0) | (a & 0x0f);
        }
        if (exec_high && ((a >> 4) == (((b >> 4) + 1) & 0x0f))) {
            a = (b & 0xf0) | (a & 0x0f);
            exec_high = false;
            i_high = i;
        }

        // step in low nibbles
        if ((((a & 0x0f) + 1) & 0x0f) == (b & 0x0f)) {
            a = (a & 0xf0) | (b & 0x0f);
        }
        if (exec_low && ((a & 0x0f) == (((b & 0x0f) + 1) & 0x0f))) {
            a = (a & 0xf0) | (b & 0x0f);
            exec_low = false;
            i_low = i;
        }

        // consistency check
        if (a != b) eeprom_ok = false;

        a = b;
    }

    // index low
    eeprom_index_low = (i_low < 0) ? i_high + 1 : i_low;
    if (eeprom_index_low >= EEPROM_SIZE) eeprom_index_low = 0;

    // index high
    eeprom_index_high = i_high;
}

void odometer_increment(void) {
    uint8_t a, b;

    // calc data
    a = eeprom_read(eeprom_index_low);
    b = (a & 0xf0) | ((a + 1) & 0x0f);
    if ((eeprom_index_low == eeprom_index_high) && ((a & 0x0f) == 0x0f)) b = b + 0x10;

    // atomic write
    eeprom_write(eeprom_index_low, b);

    // index high increment
    if ((eeprom_index_low == eeprom_index_high) && ((a & 0x0f) == 0x0f)) {
        eeprom_index_high++;
        if (eeprom_index_high >= EEPROM_SIZE) eeprom_index_high = 0;
        eeprom_index_low = eeprom_index_high;
    }

    // index low increment
    eeprom_index_low++;
    if (eeprom_index_low >= EEPROM_SIZE) eeprom_index_low = 0;
}

uint32_t odometer_getValue(void) {
    uint8_t data, carry_high, carry_low;
    uint16_t i, n_high = 0, n_low = 0;

    // carry high
    data = eeprom_read(EEPROM_SIZE - 1);
    carry_high = ((data & 0xf0) == 0xf0) ? 1 : 0;

    // carry low
    data = eeprom_read(eeprom_index_high);
    carry_low = ((data & 0x0f) == 0x0f) ? 1 : 0;

    for (i = 0; i < EEPROM_SIZE; i++) {
        data = eeprom_read(i);

        // sum nibbles
        n_high = n_high + (data >> 4);
        n_low = n_low + (data & 0x0f);

        // add carry
        if (carry_high && ((data & 0xf0) == 0)) n_high = n_high + 16;
        if (carry_low && ((data & 0x0f) == 0)) n_low = n_low + 16;
    }

    return (((uint32_t) EEPROM_SIZE * 16 * n_high) + n_low);
}

void odometer_setValue(uint32_t value) {
    uint8_t a, b, data;
    uint16_t i, v_high, v_low, quotient, modulo, addr;

    // high chain
    v_high = value / (uint16_t) (EEPROM_SIZE * 16);
    quotient = v_high / (uint16_t) EEPROM_SIZE;
    eeprom_index_high = v_high % (uint16_t) EEPROM_SIZE;

    a = ((quotient + 1) << 4) & 0xf0;
    b = (quotient << 4) & 0xf0;
    for (i = 0; i < EEPROM_SIZE; i++) {
        (i < eeprom_index_high) ? eeprom_write(i, a) : eeprom_write(i, b);
    }

    // low chain
    v_low = value % (uint16_t) (EEPROM_SIZE * 16);
    quotient = v_low / (uint16_t) EEPROM_SIZE;
    modulo = v_low % (uint16_t) EEPROM_SIZE;
    eeprom_index_low = (eeprom_index_high + modulo + 1) % EEPROM_SIZE;

    a = (quotient + 1) & 0x0f;
    b = quotient & 0x0f;
    for (i = 0; i < EEPROM_SIZE; i++) {
        addr = (i + eeprom_index_high + 1) % EEPROM_SIZE;
        data = eeprom_read(addr);
        (i < modulo) ? eeprom_write(addr, (data | a)) : eeprom_write(addr, (data | b));
    }
}

void odometer_terminal(void) {
    char c;
    uint8_t i;
    uint32_t value, n;

    while ((c = usi_getchar()) >= 0) {
        odometer_line[odometer_line_index] = c;

        if (c == '\r') {
            // print command
            if (odometer_line[0] == 'p' && odometer_line_index == 1) {
                value = odometer_getValue();

                usi_putchar('\r');
                usi_putchar('\n');

                // convert to ASCII
                i = 0;
                do {
                    odometer_line[i++] = ((value % 10) + 48) & 0x7f;
                } while ((value = value / 10));

                // write to serial
                for (; i > 0; i--) {
                    usi_putchar(odometer_line[i - 1]);
                }
            }

            // set command
            if (odometer_line[0] == 's' && odometer_line_index > 1) {
                n = 1;
                value = 0;
                for (i = (odometer_line_index - 1); i > 0; i--) {
                    // check ASCII
                    if (odometer_line[i] < 48 || odometer_line[i] > 58) break;

                    value = value + ((odometer_line[i] - 48) * n);

                    // calculate power
                    n = n * 10;
                }

                if (value < ODOMETER_MAX_VALUE) {
                    led(ON);
                    odometer_setValue(value);
                    led(OFF);
                }
            }

            // increment command
            if (odometer_line[0] == 'i' && odometer_line_index == 1) {
                odometer_increment();
            }

            // version command
            if (odometer_line[0] == 'v' && odometer_line_index == 1) {
                usi_putchar('\r');
                usi_putchar('\n');

                i = 0;
                while (VERSION[i] != 0) {
                    usi_putchar(VERSION[i]);
                    i++;
                }
            }
        }

        // echo
        usi_putchar(c);
        if (c == '\r') usi_putchar('\n');

        odometer_line_index++;
        if (odometer_line_index >= ODOMETER_LINE_SIZE || c == '\r') odometer_line_index = 0;
    }
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
        odometer_terminal();

        // wait for eeprom
        while (eeprom_busy());

        // save power
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_enable();
        sleep_cpu();
        sleep_disable();
    }
}

