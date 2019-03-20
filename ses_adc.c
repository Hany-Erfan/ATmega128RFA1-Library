/*INCLUDES ************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "ses_adc.h"
#include "ses_lcd.h"
#include <stdint.h>
#include"ses_uart.h"

/* DEFINES & MACROS **********************************************************/

#define ADCperipheralsPORT         PORTF
#define TEMPERATURE_SENSOR_PIN     2
#define ADC_VREF_SRC               0b11000000
#define maxtemp                    40
#define mintemp                    20
#define maxtempraw                 0.399
#define mintempraw                 0.75
#define LIGHT_SENSOR_PIN       	   4
#define JOYSTICK_PIN      	       5

/* FUNCTION DEFINITION *******************************************************/

void adc_init(void) {

	DDR_REGISTER(ADCperipheralsPORT) &= ~(1 << TEMPERATURE_SENSOR_PIN);
	DDR_REGISTER(ADCperipheralsPORT) &= ~(1 << LIGHT_SENSOR_PIN);
	DDR_REGISTER(ADCperipheralsPORT) &= ~(1 << JOYSTICK_PIN);
	ADCperipheralsPORT &= ~(1 << TEMPERATURE_SENSOR_PIN);
	ADCperipheralsPORT &= ~(1 << LIGHT_SENSOR_PIN);
	ADCperipheralsPORT &= ~(1 << JOYSTICK_PIN);
	PRR0 &= ~(1 << PRADC);
	ADMUX |= ADC_VREF_SRC;
	ADMUX &= ~(1 << ADLAR);
	ADCSRA |= ADC_PRESCALE;
	ADCSRA &= ~(1 << ADATE);
	ADCSRA |= (1 << ADEN);
}

uint16_t adc_read(uint8_t adc_channel) {

	adc_init();
	if (adc_channel < 0 || adc_channel > 7) {
		return ADC_INVALID_CHANNEL;
	} else {
		ADMUX &= ~0b00011111;
		ADMUX |= adc_channel;
		ADCSRA |= (1 << ADSC);

		while (1) {
			if ((ADCSRA & (1 << ADSC)) == 0)
				break;
		}
	}

	return ADC;

}

uint16_t adc_getJoystickDirection(void) {

	uint16_t value = adc_read(ADC_JOYSTICK_CH);
	if (value >= 100 && value <= 300) {
		return RIGHT;
	} else if (value >= 300 && value <= 500) {
		return UP;
	} else if (value >= 500 && value <= 700) {

		return LEFT;
	} else if (value >= 700 && value <= 900) {
		return DOWN;
	} else if (value >= 900 && value <= 1100) {
		return NO_DIRECTION;
	} else {
		return 0;
	}
}

int16_t adc_getTemperature(void) {

	int16_t ADC_TEMP_FACTOR = -1000;
	int16_t adc = adc_read(ADC_TEMP_CH);
	int16_t slope = (maxtemp - mintemp) / (maxtempraw - mintempraw);
	int16_t offset = maxtemp - (maxtempraw * slope);
	return (adc * slope + offset) / ADC_TEMP_FACTOR;

}

