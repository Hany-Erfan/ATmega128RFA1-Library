/*
 * ses_rotary.c
 *
 *  Created on: Jul 4, 2018
 *      Author: Hany Erfan
 */
/*INCLUDES-------------------------------------------------------------------*/

#include  "ses_led.h"
#include  "ses_button.h"
#include <stdbool.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "ses_adc.h"
#include "ses_lcd.h"
#include <stdint.h>
#include"ses_uart.h"
#include"ses_timer.h"
#include"ses_scheduler.h"
#include"util/atomic.h"
#include "ses_common.h"
#include "ses_fsm.h"

/* TYPES and DEFINES********************************************************************/

#define A_ROTARY_PORT   PORTB
#define B_ROTARY_PORT   PORTG
#define A_ROTARY_PIN       5
#define B_ROTARY_PIN       2
typedef void (*pTypeRotaryCallback)();
pTypeRotaryCallback clockwise = NULL;
pTypeRotaryCallback counterclockwise = NULL;
#define BUTTON_NUM_DEBOUNCE_CHECKS2 5

/*FUNCTION DEFINITION *************************************************/

void rotary_init() {

	DDR_REGISTER(A_ROTARY_PORT) &= ~(1 << A_ROTARY_PIN);
	DDR_REGISTER(B_ROTARY_PORT) &= ~(1 << B_ROTARY_PIN);
}

void rotary_setClockwiseCallback(void (*cw)(void)) {
	if (cw != NULL)
		clockwise = cw;
	else
		return;

}

void rotary_setCounterClockwiseCallback(void (*ccw)(void)) {

	if (ccw != NULL)
		counterclockwise = ccw;
	else
		return;
}

void rotary_checkState() {

	static uint8_t state[BUTTON_NUM_DEBOUNCE_CHECKS2] = { };
	static uint8_t index = 0;
	static uint8_t debouncedState = 0;
	uint8_t lastDebouncedState = debouncedState;

	/* each bit in every state byte represents one button*/

	state[index] = 0;
	if (!(PIN_REGISTER(A_ROTARY_PORT) & (1 << A_ROTARY_PIN))) {
		state[index] |= 1;
	}
	if (!(PIN_REGISTER(B_ROTARY_PORT) & (1 << B_ROTARY_PIN))) {
		state[index] |= 2;
	}
	index++;
	if (index == BUTTON_NUM_DEBOUNCE_CHECKS2) {
		index = 0;
	}

	/* init compare value and compare with ALL reads */

	uint8_t j = 0xFF;
	for (uint8_t i = 0; i < BUTTON_NUM_DEBOUNCE_CHECKS2; i++) {
		j = j & state[i];
	}
	debouncedState = j;

	if ((lastDebouncedState == 0 && debouncedState == 1)
			&& (lastDebouncedState != debouncedState))
		clockwise();
	if ((lastDebouncedState == 0 && debouncedState == 2)
			&& (lastDebouncedState != debouncedState))
		counterclockwise();

}

