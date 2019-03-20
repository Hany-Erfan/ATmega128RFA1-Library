#ifndef SES_PWM_H
#define SES_PWM_H

/*INCLUDES *******************************************************************/

#include <inttypes.h>
#include <avr/io.h>
#include "ses_common.h"

/* FUNCTION PROTOTYPES ******************************************* */

/* initialise pwm for motor */

void pwm_init(void);

/* set the duty cycle for motor to start or stop it */

void pwm_setDutyCycle(uint8_t dutyCycle);

#endif /* SES_PWM_H */
