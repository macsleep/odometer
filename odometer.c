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
    led_strobe = true;
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
	uint8_t a, b, once_high = true, once_low = true;
	int16_t i, i_high = 0, i_low = -1;

	a = eeprom_read(EEPROM_SIZE - 1);
	for (i = 0; i < EEPROM_SIZE; i++) {
		b = eeprom_read(i);

		// step high
		if ((((a >> 4) + 1) & 0x0f) == (b >> 4)) {
			a = (b & 0xf0) | (a & 0x0f);
		}
		if (once_high && ((a >> 4) == (((b >> 4) + 1) & 0x0f))) {
			i_high = i;
			once_high = false;
			a = (b & 0xf0) | (a & 0x0f);
		}

		// step low
		if ((((a & 0x0f) + 1) & 0x0f) == (b & 0x0f)) {
			a = (a & 0xf0) | (b & 0x0f);
		}
		if (once_low && ((a & 0x0f) == (((b & 0x0f) + 1) & 0x0f))) {
			i_low = i;
			once_low = false;
			a = (a & 0xf0) | (b & 0x0f);
		}

		// consistency check
		if (a != b) eeprom_ok = false;

		a = b;
	}

	// index low
	eeprom_index_low = i_high + 1;
	if (i_low != -1) eeprom_index_low = i_low;
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

        // wait for eeprom
        while (eeprom_busy());

        // save power
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_enable();
        sleep_cpu();
        sleep_disable();
    }
}
