/* INCLUDES ******************************************************************/

#include "ses_common.h"
#include "ses_button.h"
#include "avr/interrupt.h"
#include"ses_led.h"
#include <stdbool.h>
#include"ses_timer.h"

/* DEFINES & MACROS **********************************************************/

#define ENCODER_BUTTON_PORT       	PORTB
#define ENCODER_BUTTON_PIN         	6
#define JOYSTICK_BUTTON_PORT       	PORTB
#define JOYSTICK_BUTTON_PIN      	7
#define BUTTON_NUM_DEBOUNCE_CHECKS  5
void (*localCallbackrot)(void);
void (*localCallbackjoy)(void);
void (*buttonCheckstate)(void);

/* FUNCTION DEFINITION *******************************************************/

void button_init(bool debouncing) {

	/* intialising the buttons */

	SREG |= 0b10000000;
	DDR_REGISTER(ENCODER_BUTTON_PORT) &= ~(1 << ENCODER_BUTTON_PIN);
	DDR_REGISTER(JOYSTICK_BUTTON_PORT) &= ~(1 << JOYSTICK_BUTTON_PIN);
	ENCODER_BUTTON_PORT |= (1 << ENCODER_BUTTON_PIN);
	JOYSTICK_BUTTON_PORT |= (1 << JOYSTICK_BUTTON_PIN);

	/* if we want to use the debouncing option */

	if (debouncing) {

		timer1_start();
		buttonCheckstate = button_checkState;
		timer1_setCallback(buttonCheckstate);

		/* if we do not want to use the debouncing option */

	} else {
		PCICR |= (1 << PCIE0);
		PCMSK0 |= ((1 << ENCODER_BUTTON_PIN) | (1 << JOYSTICK_BUTTON_PIN));

	}
}

void button_checkState() {

	static uint8_t state[BUTTON_NUM_DEBOUNCE_CHECKS] = { };
	static uint8_t index = 0;
	static uint8_t debouncedState = 0;
	uint8_t lastDebouncedState = debouncedState;

	/* each bit in every state byte represents one button*/

	state[index] = 0;
	if (button_isJoystickPressed()) {
		state[index] |= 1;
	}
	if (button_isRotaryPressed()) {
		state[index] |= 2;
	}
	index++;
	if (index == BUTTON_NUM_DEBOUNCE_CHECKS) {
		index = 0;
	}

	/* init compare value and compare with ALL reads, only if
	 we read BUTTON_NUM_DEBOUNCE_CHECKS consistent "1" in the state
	 array, the button at this position is considered pressed*/

	uint8_t j = 0xFF;
	for (uint8_t i = 0; i < BUTTON_NUM_DEBOUNCE_CHECKS; i++) {
		j = j & state[i];
	}
	debouncedState = j;

	if ((debouncedState & 0b00000001) && (lastDebouncedState != debouncedState))
		localCallbackjoy();
	if ((debouncedState & 0b00000010) && (lastDebouncedState != debouncedState))
		localCallbackrot();

}

void button_setJoystickButtonCallback(void (*callback)(void)) {

	if (callback != NULL)
		localCallbackjoy = callback;
	else
		return;
}

void button_setRotaryButtonCallback(void (*callback)(void)) {

	if (callback != NULL)
		localCallbackrot = callback;
	else
		return;
}

ISR(PCINT0_vect) {

//	if((PCMSK0&(1<<JOYSTICK_BUTTON_PIN))&&button_isJoystickPressed()){
	//	localCallbackjoy();
	//}
//	else if((PCMSK0&(1<<ENCODER_BUTTON_PIN))&&button_isRotaryPressed()){
	//	localCallbackrot();
	//}

}

bool button_isJoystickPressed(void) {

	if (PIN_REGISTER(JOYSTICK_BUTTON_PORT) & (1 << JOYSTICK_BUTTON_PIN))
		return false;
	else
		return true;
}

bool button_isRotaryPressed(void) {

	if (PIN_REGISTER(ENCODER_BUTTON_PORT) & (1 << ENCODER_BUTTON_PIN))
		return false;
	else
		return true;
}

