/*
 * task5.c
 *
 *  Created on: Jun 24, 2018
 *      Author: Hany Erfan
 */


/*INCLUDES ************************************************************/

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
#include"ses_pwm.h"
#include"ses_motorFrequency.h"

/* PRIVATE VARIABLES **************************************************/

/* pointer to pass functions as pointers to call back setter functions */

void (*callBack)(void);

/* flag to control wether the motor is on or off */

bool motorOn = false;

/* FUNCTION DEFINITION *******************************************************/

/* function to switch my motor on or off */

void motorControl() {

	if (!motorOn) {

		/* set motor pwm duty cycle to 170 to start it */

		pwm_setDutyCycle(170);
		motorOn = true;

	} else {

		/* set motor pwm duty cycle to 255 to stop it */

		pwm_setDutyCycle(255);
		motorOn = false;
	}

}

/*function to display the recent frequency as well as the median of frequencies of the motor on the LCD
 it is added as task to the taskDescriptor  motorDisplay which is added to the scheduler to call the function every 1s
 */

void display_motorFrequency(void*x) {

	lcd_clear();
	lcd_setCursor(0, 0);

	/* print motor frequency on LCD as rpm */

	fprintf(lcdout, "Recent=%u", motorFrequency_getRecent() * 60);
	lcd_setCursor(0, 1);

	/* print the median of motor frequencies on LCD as rpm */

	fprintf(lcdout, "Median=%u", (motorFrequency_getMedian() * 60));

}

/* declare motorDisplay to be added to scheduler to display functions needed on LCD */

taskDescriptor motorDisplay = { display_motorFrequency, NULL, 1000, 1000, 0, 0,
NULL };

/* main function :) */

int main(void) {

	/* initialisations */

	sei();
	scheduler_init();
	led_redInit();
	led_yellowInit();
	led_greenInit();
	lcd_init();
	button_init(true);
	pwm_init();
	motorFrequency_init();

	/* make sure motor is off at start of program after initialisation */

	pwm_setDutyCycle(255);

	DDR_REGISTER(PORTG) |= (1 << PG5);

	/* setting call backs */

	button_setJoystickButtonCallback(&motorControl);
	button_setRotaryButtonCallback(&led_redToggle);

	/* adding needed tasks to scheduler */

	scheduler_add(&motorDisplay);

	/* super loop to run program forever */

	while (1) {

		/* running my scheduler */

		scheduler_run();
	}
}
