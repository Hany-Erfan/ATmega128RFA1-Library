#ifndef SES_MOTORFREQUENCY_H
#define SES_MOTORFREQUENCY_H

/*INCLUDES *******************************************************************/

#include <inttypes.h>
#include <avr/io.h>
#include "ses_common.h"

/* FUNCTION PROTOTYPES ******************************************* */

/*initialise motor*/

void motorFrequency_init();

/* check if motor is turned off output frequency of 0 Hz
 *  else output  the most recent frequency calculated from the time of one revolution */

uint16_t motorFrequency_getRecent();

/* check if motor is turned off output median frequency of 0 Hz
 *  else output  the median calculated from the copy of the frequencies Array */

uint16_t motorFrequency_getMedian();

#endif /* SES_MOTORFREQUENCY_H */
