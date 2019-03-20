/* INCLUDES ******************************************************************/

#include "ses_timer.h"
#include"ses_led.h"
#include"ses_scheduler.h"

/* DEFINES & MACROS **********************************************************/

#define TIMER2_CYC_FOR_1MILLISEC   250
#define TIMER5_CYC_FOR_tenthofSEC   25000
#define TIMER1_CYC_FOR_5MILLISEC    	 1250
void (*localcallbacktimer1)(void);
void (*localcallbacktimer2)(void);
void (*localcallbacktimer5)(void);
void (*localcallbacktimer0)(void);

/*FUNCTION DEFINITIONS***********************************************************/

void timer2_setCallback(void (*cb)(void)) {

	if (cb != NULL)
		localcallbacktimer2 = cb;
}

void timer2_start() {

	/* start the timer */

	PRR0 &= ~(1 << PRTIM2);

	/* allowing the interrups */

	TIMSK2 |= 1 << OCIE2A;
	TCCR2A |= 0b00000010;

	/* configure the prescaler */

	TCCR2B |= 0b00000100;

	/* configure mode of operation */

	OCR2A |= TIMER2_CYC_FOR_1MILLISEC;
	TIFR2 |= 1 << OCF2A;

	/* enable global interrupts */

	sei();

}

void timer2_stop() {

	PRR0 |= (1 << PRTIM2);
}

void timer1_setCallback(void (*cb)()) {

	if (cb != NULL)
		localcallbacktimer1 = cb;
}

void timer1_start() {

	/* start the timer */

	PRR0 &= ~(1 << PRTIM1);

	/* configure the prescaler */

	TCCR1B |= (1 << CS10);
	TCCR1B |= (1 << CS11);
	TCCR1B &= ~(1 << CS12);

	/* allowing the interrups */

	TIMSK1 |= (1 << OCIE1A);
	TIFR1 |= (1 << OCF1A);
	unsigned char sreg = SREG;
	cli();

	/* configuring CTC mode */

	OCR1A = TIMER1_CYC_FOR_5MILLISEC;
	SREG = sreg;
	TCCR1A &= ~(1 << WGM10);
	TCCR1A &= ~(1 << WGM11);
	TCCR1B |= (1 << WGM12);
	TCCR1B &= ~(1 << WGM13);

	/* enable global interrupts */

	sei();

}

void timer1_stop() {

	PRR0 |= (1 << PRTIM1);
}

ISR(TIMER1_COMPA_vect) {

	localcallbacktimer1();

}
void timer5_setCallback(void (*cb)(void)) {

	localcallbacktimer5 = cb;
}

void timer5_start() {

	/* start the timer */

	PRR0 &= ~(1 << PRTIM5);

	/* configure the prescaler */

	TCCR5B |= (1 << CS50);
	TCCR5B |= (1 << CS51);
	TCCR5B &= ~(1 << CS52);

	/* allowing the interrups */

	TIMSK5 |= (1 << OCIE5A);
	TIFR5 |= (1 << OCF5A);
	unsigned char sreg = SREG;
	cli();

	/* configuring operation mode */

	OCR5A = TIMER5_CYC_FOR_tenthofSEC;
	SREG = sreg;
	TCCR5A &= ~(1 << WGM50);
	TCCR5A &= ~(1 << WGM51);
	TCCR5B |= (1 << WGM52);
	TCCR5B &= ~(1 << WGM53);

	/* enable global interrupts */

	sei();
}

void timer5_stop() {

	PRR0 |= (1 << PRTIM5);
}

void timer0_setCallback(void (*cb)(void)) {

	if (cb != NULL)
		localcallbacktimer0 = cb;
}

void timer0_start() {

	PRR0 &= ~(1 << PRTIM0);

	/* configuring operation mode */

	TCCR0A |= (1 << WGM00);
	TCCR0A |= (1 << WGM01);
	TCCR0A |= (1 << COM0B0);
	TCCR0A |= (1 << COM0B1);
	TCCR0B &= ~(1 << WGM02);

	/* configure the prescaler */

	TCCR0B &= ~(1 << CS02);
	TCCR0B &= ~(1 << CS01);
	TCCR0B |= (1 << CS00);

	/* setting flags for compatability with pwm*/

	TCCR0B &= ~(1 << FOC0B);
	TCCR0B &= ~(1 << FOC0A);

	/* enable global interrupts */

	sei();

}

void timer0_stop() {

	PRR0 |= (1 << PRTIM0);

}

