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
#define USI_BAUD_DELAY 96
#define USI_RX_BUFFER_SIZE 4
#define EEPROM_SIZE (E2END+1)
#define ODOMETER_MAX_VALUE ((uint32_t)(EEPROM_SIZE*16)*(EEPROM_SIZE*16))
#define ODOMETER_LINE_SIZE 16

enum {
    ON, OFF, TOGGLE
};

/* global variables */

volatile uint8_t eeprom_ok = true;
volatile uint16_t eeprom_index_high;
volatile uint16_t eeprom_index_low;
volatile uint8_t led_strobe = false;
volatile uint8_t wheel_turned = false;
volatile uint8_t usi_rx_head = 0;
volatile uint8_t usi_rx_tail = 0;
volatile uint8_t usi_rx_data[USI_RX_BUFFER_SIZE];
volatile uint8_t odometer_line_index = 0;
volatile uint8_t odometer_line[ODOMETER_LINE_SIZE];

/* function prototypes */

void led_init(void);
void led(uint8_t mode);
void eeprom_init(void);
void eeprom_write(uint16_t addr, uint8_t data);
uint8_t eeprom_read(uint16_t addr);
uint8_t eeprom_busy(void);
void timer0_init(void);
void ac_disable(void);
void adc_disable(void);
void usi_init(void);
uint8_t usi_reverse(uint8_t b);
int usi_getchar(void);
int usi_putchar(char c);
void odometer_init(void);
void odometer_increment(void);
uint32_t odometer_getValue(void);
void odometer_setValue(uint32_t value);
void odometer_terminal(void);

#endif

