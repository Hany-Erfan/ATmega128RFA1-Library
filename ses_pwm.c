/*INCLUDES ************************************************************/

#include "ses_timer.h"
#include "ses_scheduler.h"
#include "util/atomic.h"
#include"ses_led.h"
#include"util/atomic.h"
#include <stdbool.h>
#include <stdlib.h>
#include <avr/io.h>

/* FUNCTION DEFINITION *******************************************************/

void pwm_init(void) {
	timer0_start();

}

void pwm_setDutyCycle(uint8_t dutyCycle) {

	OCR0B = dutyCycle;

}
