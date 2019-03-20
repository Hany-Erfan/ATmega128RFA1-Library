/*
 * ses_rotary.h
 *
 *  Created on: Jul 4, 2018
 *      Author: Hany Erfan
 */
/* TYPES and DEFINES********************************************************************/

#ifndef SES_ROTARY_H_
#define SES_ROTARY_H_
typedef void (*pTypeRotaryCallback)();

/* FUNCTION PROTOTYPE ********************************************************************/

/* initilise rotary encoder */

void rotary_init();

/* set call back function when rotating rotary clock wise */

void rotary_setClockwiseCallback(void (*pTypeRotaryCallback)(void));

/* set call back function when rotating rotary counter clock wise */

void rotary_setCounterClockwiseCallback(void (*pTypeRotaryCallback)(void));

/* debouncing of rotary */

void rotary_checkState();

#endif /* SES_ROTARY_H_ */
