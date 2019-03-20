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
#include "ses_common.h"
#include "ses_fsm.h"
#include"ses_rotary.h"

/* DEFINES & MACROS **********************************************************/

typedef struct fsm_s Fsm; //< typedef for alarm clock state machine
typedef struct event_s Event; //< event type for alarm clock fsm
typedef uint8_t fsmReturnStatus; //< typedef to be used with above enum
typedef fsmReturnStatus (*State)(Fsm *, const Event*);
#define TRANSITION(newState) (fsm->state = newState, RET_TRANSITION)

/*FUNCTIONS` SIGNATURES **********************************************************/

/*function to set first state of finite state machine in initialization */

fsmReturnStatus first_state(Fsm * fsm, const Event* event);

/* function to set system time hours* (2nd state in fsm)*/

fsmReturnStatus set_hour(Fsm * fsm, const Event* event);

/* function to set system time minutes (3rd state in fsm */

fsmReturnStatus set_minute(Fsm * fsm, const Event* event);

/* function to start and display system clock (4th state in fsm)*/

fsmReturnStatus display_clock(Fsm * fsm, const Event* event);

/*function to set alarm hour (5th state in fsm)*/

fsmReturnStatus set_alarmHour(Fsm * fsm, const Event* event);

/*function to set alarm minute (6th state in fsm)*/

fsmReturnStatus set_alarmMinute(Fsm * fsm, const Event* event);

/* function to update and display system clock time ,and control alarm operation */

void display_time(void*param);

/* function to toggle green led*/

void green_Toggle(void*param);

/* function to toggle red led with alarm*/

void red_toggleWithFrequency(void* param);

/* function to  increase hours with cw rotation of encoder*/

void set_rotaryHoursIncrement();

/* function to increase alarm hours with cw rotation of encoder*/

void set_rotaryAlarmHoursIncrement();

/* function to increase minutes with cw  rotation of encoder */

void set_rotaryMinutesIncrement();

/* function to increase alarm minutes with cw rotation of encoder*/

void set_rotaryAlarmMinutesIncrement();

/* function to  decrease hours with ccw rotation of encoder*/

void set_rotaryHoursDecrement();

/* function to decrease alarm hours with ccw rotation of encoder*/

void set_rotaryAlarmHoursDecrement();

/* function to decrease minutes with ccw  rotation of encoder */

void set_rotaryMinutesDecrement();

/* function to decrease alarm minutes with ccw rotation of encoder*/

void set_rotaryAlarmMinutesDecrement();

/* function to control rotary encoder added to scheduler as task */

void rotaryControl(void* param);

/* function to dispatch fsm with rotary button press event */

void rotaryPressedDispatch();

/* function to dispatch fsm with joystick button press event */

void joystickPressedDispatch();

/* PRIVATE VARIABLES **************************************************/

/* timing structure used to update system time */

timing t = { 0, 0, 0, 0 };

/* timing structure used to record alarm time set by user*/

timing ta = { 0, 0, 0, 0 };

/* timing structure used to display system time */

timing displayedTime = { 0, 0, 0, 0 };

/* finite state machine variable */

Fsm myFsm = { NULL, false, { 0, 0, 0, 0 } };

/* task to display system time every 1 second by scheduler*/

taskDescriptor displayTime = { display_time, NULL, 1000, 1000, 0, 0 };

/* task to toggle green led every 1 second by scheduler*/

taskDescriptor greenToggle = { green_Toggle, NULL, 1000, 1000, 0, 0 };

/* task to toggle led red with frequency of 4 hz by scheduler */

taskDescriptor redFrequency = { red_toggleWithFrequency, NULL, 250, 250, 0, 0 };

/* task for checking the state of the rotary encoder every 5 milliseconds and call callback functions accordingly */

taskDescriptor rotary = { rotary_checkState, NULL, 5, 5, 0, 0 };

/* flag to control alarm operation */

bool alarmFlag = false;

/* flag to set system time mins to 0 when mins exceed 59*/

bool flagMin = false;

/* flag to set system time hours */

bool flagHour = false;

/* flag to increment system time mins when seconds exceed 59*/

bool flag_minIncrement = false;

/* alarm counter to switch alarm off after 5 seconds*/

int alarmCounter = 0;

/*FUNCTION DEFINITION *************************************************/

void green_Toggle(void* param) {
	led_greenToggle();
}
void red_toggleWithFrequency(void* param) {
	led_redToggle();
}

void display_time(void* param) {

	lcd_setCursor(0, 0);

	/* get updated system time*/

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		systemTime_t schedulerTime = scheduler_getTime();

		t.milli = (schedulerTime % 1000);

		t.second = ((schedulerTime / 1000) % 60);
	}

	t.minute = (((t.second) / 60) % 60);

	t.hour = (t.minute) / 60;

	/* setting up displayed time seconds */

	displayedTime.second = myFsm.timeSet.second + t.second;

	/* checking if alarm has been triggered for 5 seconds to switch it off */

	if (alarmCounter == 5) {
		scheduler_remove(&redFrequency);
		led_redOff();
		alarmFlag = false;
		alarmCounter = 0;
	}

	/* checking if alram is on the start counting 5 seconds */

	if (alarmFlag == true) {
		alarmCounter++;
	}

	/* incrementing mins if seconds rach 59*/

	if (flag_minIncrement == true) {
		myFsm.timeSet.minute++;
		flag_minIncrement = false;

	}

	/* setting mins to 0 if mins reach 59*/

	if (flagMin == true) {
		myFsm.timeSet.minute = 0;
		flagMin = false;
	}

	/* setting hours to 0 if hours reach 23 and mins reach 59 */

	if (flagHour == true) {
		myFsm.timeSet.hour = 0;
		flagHour = false;
	}

	/* setting flags for incrementing mins and hours as seconds reach 59 */

	if (displayedTime.second == 59) {
		if (myFsm.timeSet.minute == 59) {
			flagMin = true;

			if (myFsm.timeSet.hour == 23)
				flagHour = true;

			else
				myFsm.timeSet.hour++;
		} else {
			flag_minIncrement = true;

		}
	}

	/* setting up displayed time hours*/

	displayedTime.hour = myFsm.timeSet.hour;

	/* setting up displayed time mins*/

	displayedTime.minute = myFsm.timeSet.minute;

	/* comparing system time hours , mins and seconds with alarm set time to trigger alarm if there is a match*/

	if ((displayedTime.hour == ta.hour) && (displayedTime.minute == ta.minute)
			&& (displayedTime.second == ta.second) && (myFsm.isAlarmEnabled)
			&& (alarmFlag == false)) {
		alarmFlag = true;
		scheduler_add(&redFrequency);

	}

	/* displaying the system time in format HH:MM:SS*/

	/* checking hours for proper displaying of the time */

	if (displayedTime.hour < 10) {

		/* checking mins for proper displaying of the time */

		if (displayedTime.minute < 10) {

			/* checking seconds for proper displaying of the time */

			if (displayedTime.second < 10) {
				fprintf(lcdout, "0%d:0%d:0%d", displayedTime.hour,
						displayedTime.minute, displayedTime.second);
			} else {
				fprintf(lcdout, "0%d:0%d:%d", displayedTime.hour,
						displayedTime.minute, displayedTime.second);
			}
		} else {
			if (displayedTime.second < 10) {
				fprintf(lcdout, "0%d:%d:0%d", displayedTime.hour,
						displayedTime.minute, displayedTime.second);
			} else {
				fprintf(lcdout, "0%d:%d:%d", displayedTime.hour,
						displayedTime.minute, displayedTime.second);
			}
		}

	} else {
		if (displayedTime.minute < 10) {
			if (displayedTime.second < 10) {
				fprintf(lcdout, "%d:0%d:0%d", displayedTime.hour,
						displayedTime.minute, displayedTime.second);
			} else {
				fprintf(lcdout, "%d:0%d:%d", displayedTime.hour,
						displayedTime.minute, displayedTime.second);
			}
		} else {
			if (displayedTime.second < 10) {
				fprintf(lcdout, "%d:%d:0%d", displayedTime.hour,
						displayedTime.minute, displayedTime.second);
			} else {
				fprintf(lcdout, "%d:%d:%d", displayedTime.hour,
						displayedTime.minute, displayedTime.second);
			}
		}
	}

}

fsmReturnStatus set_alarmMinute(Fsm * fsm, const Event* event) {

	/* when entering the set alarm minute state  we set call back functions for rotary encoder */

	if ((*event).signal == ENTRY) {
		rotary_setClockwiseCallback(set_rotaryAlarmMinutesIncrement);
		rotary_setCounterClockwiseCallback(set_rotaryAlarmMinutesDecrement);
	}

	/* incrementing alarm set minutes with each rotary press */

	if ((*event).signal == ROTARY_PRESSED) {

		/* checking hours for proper displaying of the time */

		if (ta.hour < 10) {

			/* setting alarm time mins to 0 if mins reach 59 other wise increment it by 1*/

			if (ta.minute == 59) {
				ta.minute = 0;
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:0%d", ta.hour, ta.minute);
			} else {
				ta.minute++;

				/* checking mins for proper displaying of the time */

				if (ta.minute < 10) {
					lcd_setCursor(0, 0);
					fprintf(lcdout, "0%d:0%d", ta.hour, ta.minute);
				} else if (ta.minute >= 10) {
					lcd_setCursor(0, 0);
					fprintf(lcdout, "0%d:%d", ta.hour, ta.minute);
				}
			}
		} else if (ta.hour >= 10) {
			if (ta.minute == 59) {
				ta.minute = 0;
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:0%d", ta.hour, ta.minute);
			} else {
				ta.minute++;
				if (ta.minute < 10) {
					lcd_setCursor(0, 0);
					fprintf(lcdout, "%d:0%d", ta.hour, ta.minute);
				} else if (ta.minute >= 10) {
					lcd_setCursor(0, 0);
					fprintf(lcdout, "%d:%d", ta.hour, ta.minute);
				}
			}

		}
		return RET_HANDLED;
	}

	/* changing to state diaplay_clock with the joystick button press */

	if ((*event).signal == JOYSTICK_PRESSED) {
		lcd_clear();
		lcd_setCursor(0, 1);
		fprintf(lcdout, "alarm clock is set");
		_delay_ms(200);
		return TRANSITION(display_clock);

	}

	/* clearing lcd as we exit this state*/

	if ((*event).signal == EXIT) {
		lcd_clear();
		return RET_HANDLED;

	}

	return RET_HANDLED;
}

fsmReturnStatus set_alarmHour(Fsm * fsm, const Event* event) {

	/* when entering the set alarm hour state  we set call back functions for rotary encoder */

	if ((*event).signal == ENTRY) {
		rotary_setClockwiseCallback(set_rotaryAlarmHoursIncrement);
		rotary_setCounterClockwiseCallback(set_rotaryAlarmHoursDecrement);

		lcd_setCursor(0, 0);
		fprintf(lcdout, "00;00");
		lcd_setCursor(0, 1);
		fprintf(lcdout, "set alarm hour by rotary");
		return RET_HANDLED;

	}

	/* incrementing alarm set hours with every rotary button press */

	if ((*event).signal == ROTARY_PRESSED) {

		/* setting system time hours to 0 if hours reach 23 other wise increment it by 1*/

		if (ta.hour == 23) {
			ta.hour = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:00", ta.hour);
		} else {
			ta.hour++;

			/* checking hours for proper displaying of the time */

			if (ta.hour < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:00", ta.hour);
			} else if (ta.hour >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:00", ta.hour);
			}

		}
		return RET_HANDLED;

		/* changing to state set_alarmMinutes with the joystick button press */
	}
	if ((*event).signal == JOYSTICK_PRESSED) {
		return TRANSITION(set_alarmMinute);
	}

	/* displaying set alarm minute instructions as we exit this state */

	if ((*event).signal == EXIT) {
		lcd_setCursor(0, 1);
		fprintf(lcdout, "set alarm minute by rotary");
		return RET_HANDLED;
	}

	return RET_HANDLED;

}

fsmReturnStatus display_clock(Fsm * fsm, const Event* event) {

	/* as we enter the dispaly_clock state we disable rotary encoder functions, and we start green led toggle and display time by shceduler */

	if ((*event).signal == ENTRY) {
		rotary_setClockwiseCallback(NULL);
		rotary_setCounterClockwiseCallback(NULL);
		scheduler_add(&greenToggle);
		scheduler_add(&displayTime);
		return RET_HANDLED;
	}

	/* we change to the state set_alarmHour with the joystick button press as well as stop the alarm if it was triggered */

	if ((*event).signal == JOYSTICK_PRESSED) {
		scheduler_remove(&redFrequency);
		//led_redOff();
		alarmFlag = false;
		return TRANSITION(set_alarmHour);
	}

	/*stopping alarm if it was triggered as well as enabling or disabling the alarm and switching yellow led on and off accordingly */

	if ((*event).signal == ROTARY_PRESSED) {
		scheduler_remove(&redFrequency);
		//led_redOff();
		alarmFlag = false;
		if (myFsm.isAlarmEnabled == false) {
			myFsm.isAlarmEnabled = true;
			led_yellowOn();
		} else {
			myFsm.isAlarmEnabled = false;
			led_yellowOff();

		}
		return RET_HANDLED;

	}

	/* stopping the display of the system time  as we exit this state as well as clearing the lcd */

	if ((*event).signal == EXIT) {

		scheduler_remove(&displayTime);
		lcd_clear();

	}
	return RET_HANDLED;

}

fsmReturnStatus set_minute(Fsm * fsm, const Event* event) {

	/* setting rotary encoder call back functions as we enter this state */

	if (((*event).signal == ENTRY)) {
		rotary_setClockwiseCallback(set_rotaryMinutesIncrement);
		rotary_setCounterClockwiseCallback(set_rotaryMinutesDecrement);
	}

	/* incrementing time set mins with every rotary button press */

	if ((*event).signal == ROTARY_PRESSED) {

		/* checking hours for proper displaying of the time */

		if (fsm->timeSet.hour < 10) {

			/* setting system time mins to 0 if mins reach 59 other wise increment it by 1*/

			if (fsm->timeSet.minute == 59) {
				fsm->timeSet.minute = 0;
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:0%d", fsm->timeSet.hour,
						fsm->timeSet.minute);
			} else {
				fsm->timeSet.minute++;

				/* checking mins for proper displaying of the time */

				if (fsm->timeSet.minute < 10) {
					lcd_setCursor(0, 0);
					fprintf(lcdout, "0%d:0%d", fsm->timeSet.hour,
							fsm->timeSet.minute);
				} else if (fsm->timeSet.minute >= 10) {
					lcd_setCursor(0, 0);
					fprintf(lcdout, "0%d:%d", fsm->timeSet.hour,
							fsm->timeSet.minute);
				}
			}
		} else if (fsm->timeSet.hour >= 10) {
			if (fsm->timeSet.minute == 59) {
				fsm->timeSet.minute = 0;
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:0%d", fsm->timeSet.hour,
						fsm->timeSet.minute);
			} else {
				fsm->timeSet.minute++;
				if (fsm->timeSet.minute < 10) {
					lcd_setCursor(0, 0);
					fprintf(lcdout, "%d:0%d", fsm->timeSet.hour,
							fsm->timeSet.minute);
				} else if (fsm->timeSet.minute >= 10) {
					lcd_setCursor(0, 0);
					fprintf(lcdout, "%d:%d", fsm->timeSet.hour,
							fsm->timeSet.minute);
				}
			}

		}
		return RET_HANDLED;
	}

	/* changing to the state dislay_clock with the joystick button press */

	if ((*event).signal == JOYSTICK_PRESSED) {
		lcd_clear();
		lcd_setCursor(0, 0);
		fprintf(lcdout, "time is set");
		_delay_ms(200);
		return TRANSITION(display_clock);

	}

	/* clearing the lcd as well as setting the start point for my system time clock as we exit this state */

	if ((*event).signal == EXIT) {
		lcd_clear();

		scheduler_setTime(0);
		return RET_HANDLED;

	}

	return RET_HANDLED;
}

fsmReturnStatus set_hour(Fsm * fsm, const Event* event) {

	/* setting rotary encoder call back functions as we enter this state */

	if (((*event).signal == ENTRY)) {
		rotary_setClockwiseCallback(set_rotaryHoursIncrement);
		rotary_setCounterClockwiseCallback(set_rotaryHoursDecrement);
	}

	/* incrementing system time hours with every rotary button press*/

	if ((*event).signal == ROTARY_PRESSED || ((*event).signal == ENTRY)) {

		/* setting system time hours to 0 if hours reach 23 other wise increment it by 1*/

		if (fsm->timeSet.hour == 23) {
			fsm->timeSet.hour = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:00", fsm->timeSet.hour);
		} else {

			fsm->timeSet.hour++;

			/* checking hours for proper displaying of the time */

			if (fsm->timeSet.hour < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:00", fsm->timeSet.hour);
			} else if (fsm->timeSet.hour >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:00", fsm->timeSet.hour);
			}

		}
		return RET_HANDLED;
	}

	/* chaning to the state set_minute with the joystick button press */

	if ((*event).signal == JOYSTICK_PRESSED) {
		return TRANSITION(set_minute);
	}

	/* displaying instructions to set minutes as we exit this state */

	if ((*event).signal == EXIT) {
		lcd_setCursor(0, 1);
		fprintf(lcdout, "set minute by rotary");
		return RET_HANDLED;

	}

	return RET_HANDLED;

}

fsmReturnStatus first_state(Fsm * fsm, const Event* event) {

	/* chaning to the state set_hour as rotary button is pressed */

	if (((*event).signal == ROTARY_PRESSED)) {
		return TRANSITION(set_hour);
	}

	/* changing to the state set minute if joystick button is pressed */

	if (((*event).signal == JOYSTICK_PRESSED)) {
		return TRANSITION(set_minute);
	}

	/* displaying an uninitialized clock as we enter the first state as well as instructions for setting up the hours  */

	if ((*event).signal == ENTRY) {
		lcd_setCursor(0, 0);
		fprintf(lcdout, "00;00");
		lcd_setCursor(0, 1);
		fprintf(lcdout, "set hour by rotary ");

		return RET_HANDLED;
	}

	return RET_HANDLED;
}

void joystickPressedDispatch() {

	Event e = { .signal = JOYSTICK_PRESSED };

	/* dispatching the fsm with joystick button press event */

	fsm_dispatch(&myFsm, &e);
}

void rotaryPressedDispatch() {

	Event e = { .signal = ROTARY_PRESSED };

	/* dispatching the fsm with rotary button pres event */

	fsm_dispatch(&myFsm, &e);
}

void rotaryControl(void* param) {

	/* calling the rotary check state function */

	rotary_checkState();
}

void set_rotaryHoursIncrement() {

	/* setting system time hours to 0 if hours reach 23 other wise increment it by 1*/

	if (myFsm.timeSet.hour == 23) {
		myFsm.timeSet.hour = 0;
		lcd_setCursor(0, 0);
		fprintf(lcdout, "0%d:00", myFsm.timeSet.hour);
	} else {

		myFsm.timeSet.hour++;

		/* checking hours for proper displaying of the time */

		if (myFsm.timeSet.hour < 10) {
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:00", myFsm.timeSet.hour);
		} else if (myFsm.timeSet.hour >= 10) {
			lcd_setCursor(0, 0);
			fprintf(lcdout, "%d:00", myFsm.timeSet.hour);
		}
	}
}

void set_rotaryHoursDecrement() {

	/* checking if time hours reach 0 so as not to decrement any further, else we decrement the hours by 1*/

	if (myFsm.timeSet.hour <= 0) {
		myFsm.timeSet.hour = 0;
		lcd_setCursor(0, 0);
		fprintf(lcdout, "0%d:00", myFsm.timeSet.hour);
	} else {

		myFsm.timeSet.hour--;

		/* checking hours for proper displaying of the time */

		if (myFsm.timeSet.hour < 10) {
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:00", myFsm.timeSet.hour);
		} else if (myFsm.timeSet.hour >= 10) {
			lcd_setCursor(0, 0);
			fprintf(lcdout, "%d:00", myFsm.timeSet.hour);
		}
	}
}

void set_rotaryMinutesIncrement() {

	/* checking hours for proper displaying of the time */

	if (myFsm.timeSet.hour < 10) {

		/* setting system time minutes to 0 if mins reach 59 other wise increment it by 1*/

		if (myFsm.timeSet.minute == 59) {
			myFsm.timeSet.minute = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:0%d", myFsm.timeSet.hour,
					myFsm.timeSet.minute);
		} else {
			myFsm.timeSet.minute++;

			/* checking mins for proper displaying of the time */

			if (myFsm.timeSet.minute < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:0%d", myFsm.timeSet.hour,
						myFsm.timeSet.minute);
			} else if (myFsm.timeSet.minute >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:%d", myFsm.timeSet.hour,
						myFsm.timeSet.minute);
			}
		}
	} else if (myFsm.timeSet.hour >= 10) {
		if (myFsm.timeSet.minute == 59) {
			myFsm.timeSet.minute = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "%d:0%d", myFsm.timeSet.hour, myFsm.timeSet.minute);
		} else {
			myFsm.timeSet.minute++;
			if (myFsm.timeSet.minute < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:0%d", myFsm.timeSet.hour,
						myFsm.timeSet.minute);
			} else if (myFsm.timeSet.minute >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:%d", myFsm.timeSet.hour,
						myFsm.timeSet.minute);
			}
		}
	}
}

void set_rotaryMinutesDecrement() {

	/* checking hours for proper displaying of the time */

	if (myFsm.timeSet.hour < 10) {

		/* checking if time mins reach 0 so as not to decrement any further, else we decrement the mins by 1*/

		if (myFsm.timeSet.minute <= 0) {
			myFsm.timeSet.minute = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:0%d", myFsm.timeSet.hour,
					myFsm.timeSet.minute);
		} else {
			myFsm.timeSet.minute--;

			/* checking mins for proper displaying of the time */

			if (myFsm.timeSet.minute < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:0%d", myFsm.timeSet.hour,
						myFsm.timeSet.minute);
			} else if (myFsm.timeSet.minute >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:%d", myFsm.timeSet.hour,
						myFsm.timeSet.minute);
			}
		}
	} else if (myFsm.timeSet.hour >= 10) {
		if (myFsm.timeSet.minute <= 0) {
			myFsm.timeSet.minute = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "%d:0%d", myFsm.timeSet.hour, myFsm.timeSet.minute);
		} else {
			myFsm.timeSet.minute--;
			if (myFsm.timeSet.minute < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:0%d", myFsm.timeSet.hour,
						myFsm.timeSet.minute);
			} else if (myFsm.timeSet.minute >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:%d", myFsm.timeSet.hour,
						myFsm.timeSet.minute);
			}
		}
	}

}

void set_rotaryAlarmHoursIncrement() {

	/* setting alarm time hours to 0 if hours reach 23 other wise increment it by 1*/

	if (ta.hour == 23) {
		ta.hour = 0;
		lcd_setCursor(0, 0);
		fprintf(lcdout, "0%d:00", ta.hour);
	} else {

		ta.hour++;

		/* checking hours for proper displaying of the time */

		if (ta.hour < 10) {
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:00", ta.hour);
		} else if (ta.hour >= 10) {
			lcd_setCursor(0, 0);
			fprintf(lcdout, "%d:00", ta.hour);
		}
	}

}
void set_rotaryAlarmHoursDecrement() {

	/* checking if alarm time hours reach 0 so as not to decrement any further, else we decrement the hours by 1*/

	if (ta.hour <= 0) {
		ta.hour = 0;
		lcd_setCursor(0, 0);
		fprintf(lcdout, "0%d:00", ta.hour);
	} else {

		ta.hour--;

		/* checking hours for proper displaying of the time */

		if (ta.hour < 10) {
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:00", ta.hour);
		} else if (ta.hour >= 10) {
			lcd_setCursor(0, 0);
			fprintf(lcdout, "%d:00", ta.hour);
		}
	}

}

void set_rotaryAlarmMinutesIncrement() {

	/* checking hours for proper displaying of the time */

	if (ta.hour < 10) {

		/* setting alarm time minutes to 0 if mins reach 59 other wise increment it by 1*/

		if (ta.minute == 59) {
			ta.minute = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:0%d", ta.hour, ta.minute);
		} else {
			ta.minute++;

			/* checking mins for proper displaying of the time */

			if (ta.minute < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:0%d", ta.hour, ta.minute);
			} else if (ta.minute >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:%d", ta.hour, ta.minute);
			}
		}
	} else if (ta.hour >= 10) {
		if (ta.minute == 59) {
			ta.minute = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "%d:0%d", ta.hour, ta.minute);
		} else {
			ta.minute++;
			if (ta.minute < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:0%d", ta.hour, ta.minute);
			} else if (ta.minute >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:%d", ta.hour, ta.minute);
			}
		}
	}

}

void set_rotaryAlarmMinutesDecrement() {

	/* checking hours for proper displaying of the time */

	if (ta.hour < 10) {

		/* checking if time mins reach 0 so as not to decrement any further, else we decrement the mins by 1*/

		if (ta.minute <= 0) {
			ta.minute = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "0%d:0%d", ta.hour, ta.minute);
		} else {
			ta.minute--;

			/* checking mins for proper displaying of the time */

			if (ta.minute < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:0%d", ta.hour, ta.minute);
			} else if (ta.minute >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "0%d:%d", ta.hour, ta.minute);
			}
		}
	} else if (ta.hour >= 10) {
		if (ta.minute <= 0) {
			ta.minute = 0;
			lcd_setCursor(0, 0);
			fprintf(lcdout, "%d:0%d", ta.hour, ta.minute);
		} else {
			ta.minute--;
			if (ta.minute < 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:0%d", ta.hour, ta.minute);
			} else if (ta.minute >= 10) {
				lcd_setCursor(0, 0);
				fprintf(lcdout, "%d:%d", ta.hour, ta.minute);
			}
		}
	}

}

/** my main :) */

int main() {

	/* enabling interrupts */

	sei();

	/* initilising the scheduler */

	scheduler_init();

	/* initilising all three lids */

	led_redInit();
	led_yellowInit();
	led_greenInit();

	/* initializing the LCD */

	lcd_init();

	/*initialising the buttons with the button debouncing option */

	button_init(true);

	/* initilising my finite state machine with first_state being the first state */

	fsm_init(&myFsm, first_state);

	/* adding the rotary check state task to my scheduler */

	scheduler_add(&rotary);

	/* setting  joystick button call back function */

	button_setJoystickButtonCallback(&joystickPressedDispatch);

	/* setting rotary button call back function */

	button_setRotaryButtonCallback(&rotaryPressedDispatch);

	/* initialise rotary encoder pins*/

	void rotary_init();

	/** super loop that runs the scheduler forever*/

	while (1) {
		scheduler_run();

	}

}

