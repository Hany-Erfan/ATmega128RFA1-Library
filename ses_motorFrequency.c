/*INCLUDES ************************************************************/

#include "ses_timer.h"
#include "ses_scheduler.h"
#include "util/atomic.h"
#include"ses_led.h"
#include"util/atomic.h"
#include <stdbool.h>
#include <stdlib.h>
#include <avr/io.h>
#include"ses_motorFrequency.h"
#include"ses_timer.h"

/* PRIVATE VARIABLES **************************************************/

/* macro for the size of array to store frequency values */

#define N  11

/* variable to count number of current spikes as motor rotates*/

int spikes = 1;

/* time taken for one revolution (every 6 spikes) */

static uint16_t timeOfRevolution = 0;

/* flag to control how timer 5 detects a stopped motor*/

int flag = 0;

/* temporary variable that stores the first comparison frequency in timer 5 ISR */

uint16_t temp1 = 1;

/* temporary variable that stores the second comparison frequency in timer 5 ISR */

uint16_t temp2 = 2;

/* boolean flag to identify the motor is on or off */

bool motorOff = false;

/* array for storing frequencies of motors*/

uint16_t frequenciesArray[N];

/* global variable i to access the frequency array in External interrupt 0 ISR*/

int i = 0;

/* FUNCTION DEFINITION *******************************************************/

/* initialise the Motor*/

void motorFrequency_init() {

	timer5_start();

	/* initialise external interrupt 0*/

	EICRA |= (1 << ISC00);
	EICRA |= (1 << ISC01);
	EIMSK |= (1 << PORTD0);
	sei();

	/* initialise frequencies Array by filling it with 0s*/

	for (int i = 0; i < N; i++) {
		frequenciesArray[i] = 0;
	}

}

/* check if motor is turned off output frequency of 0 Hz
 *  else output  the most recent frequency calculated from the time of one revolution */

uint16_t motorFrequency_getRecent() {

	uint16_t frequency;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{

		if (motorOff) {
			frequency = 0;

		} else
			frequency = 1 / ((timeOfRevolution * 0.1) / 25000);
	}

	return frequency;
}

/* check if motor is turned off output median frequency of 0 Hz
 *  else output  the median calculated from the copy of the frequencies Array */

uint16_t motorFrequency_getMedian() {

	/* initialise an array to copy the frequenciesArray */

	uint16_t frequenciesArrayCopy[N];

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		for (int i = 0; i < N; i++) {
			frequenciesArrayCopy[i] = frequenciesArray[i];
		}
	}

	if (motorOff) {
		return 0;
	} else {

		/* sorting the copy of frequencies` array in ascending order */

		int i = 0, j = 0;
		uint16_t temp = 0;
		for (i = 0; i < N; i++) {
			for (j = 0; j < N - 1; j++) {
				if (frequenciesArrayCopy[j] > frequenciesArrayCopy[j + 1]) {
					temp = frequenciesArrayCopy[j];
					frequenciesArrayCopy[j] = frequenciesArrayCopy[j + 1];
					frequenciesArrayCopy[j + 1] = temp;
				}
			}
		}

		/* calculate the median of the sorted array */

		uint16_t median = 0;

		/* if number of elements are even*/

		if (N % 2 == 0)
			median = (frequenciesArrayCopy[(N - 1) / 2]
					+ frequenciesArrayCopy[N / 2]) / 2.0;

		/* if number of elements are odd */

		else
			median = frequenciesArrayCopy[N / 2];

		return median;
	}
}

/* external interrupt 0 service routine to calculate the time of one revolution
 *  and record the frequency of the motor in the frequency Array */

ISR(INT0_vect) {

	/* turn off green led as motor is running */

	led_greenOff();

	/* set motor off flag to false because motor is now running */

	motorOff = false;

	/* check if we reached last position in frequencies array and start from the beginning */

	if (i == N) {
		i = 0;
	}

	/* check if current spikes is 1 then reset timer 5 */

	if (spikes == 1) {
		TCNT5 = 0;
	}

	/* check if current spikes is 6 then copy timer 5 value and use it to calculate frequency and store in frequencies array */

	if (spikes == 6) {
		frequenciesArray[i] = 1 / ((TCNT5 * 0.1) / 25000);
		timeOfRevolution = TCNT5;
		i++;
		spikes = 0;

	}

	spikes++;

	/* toggle yellow led with each spike */

	led_yellowToggle();

}

/* timer 5 interrupt serice routine to identify if the motor is turned off or not every 0.1s */

ISR(TIMER5_COMPA_vect) {

	/* first point of comparison of frequency */

	if (flag == 0) {
		temp1 = motorFrequency_getRecent();
		flag = 1;
	}

	/* second point of comparison of frequency */

	else if (flag == 1) {
		temp2 = motorFrequency_getRecent();
		flag = 0;
	}

	/* if both temps are equal then motor is stopped and green led is turned on */

	if (temp1 == temp2) {
		led_greenOn();
		motorOff = true;
	}
}
