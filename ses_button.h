#ifndef SES_BUTTON_H_
#define SES_BUTTON_H_

/* INCLUDES ******************************************************************/

#include "ses_common.h"
#include "avr/interrupt.h"

/* FUNCTION PROTOTYPES *******************************************************/

/**
 * Initializes rotary encoder and joystick button
 */
void button_init(bool debouncing);
/** 
 * Get the state of the joystick button.
 */
bool button_isJoystickPressed(void);
/** 
 * Get the state of the rotary button.
 */
bool button_isRotaryPressed(void);
/**
 * set call back of the rotary button.
 */
void button_setRotaryButtonCallback(void (*pButtonCallback)(void));
/**
 * set call back of the joy stick button.
 */
void button_setJoystickButtonCallback(void (*pButtonCallback)(void));
/**
 * check state method for button debouncing.
 */
void button_checkState(void);

#endif /* SES_BUTTON_H_ */
