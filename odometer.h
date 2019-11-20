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

#ifndef _ODOMETER_H_
#define _ODOMETER_H_

/* includes */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

/* macros */

#define true 1
#define false 0
#define PULSES_PER_WHEEL_TURN 13
#define BAUD_DELAY 98
#define EEPROM_SIZE (E2END+1)

enum {
    ON, OFF, TOGGLE
};

/* global variables */

volatile uint16_t eeprom_index = 0;
volatile uint8_t led_strobe = 0;
volatile uint8_t wheel_turned = 0;
volatile uint8_t usi_rx_data = 0;
volatile uint8_t usi_rx_avail = 0;

/* function prototypes */

void led_init(void);
void led(uint8_t mode);
void eeprom_init(void);
void eeprom_write(uint16_t addr, uint8_t data);
uint8_t eeprom_read(uint16_t addr);
void timer0_init(void);
void ac_disable(void);
void adc_disable(void);
void usi_init(void);
uint8_t usi_reverse(uint8_t b);
void usi_write(uint8_t c);
void usi_print(uint32_t value);
int8_t usi_getchar(void);
void odometer_init(void);
void odometer_increment(void);
uint32_t odometer_getValue(void);
void odometer_setValue(uint32_t value);

#endif

