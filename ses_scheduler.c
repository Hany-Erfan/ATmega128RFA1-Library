/*INCLUDES ************************************************************/

#include "ses_timer.h"
#include "ses_scheduler.h"
#include "util/atomic.h"
#include"ses_led.h"
#include"util/atomic.h"

/* PRIVATE VARIABLES **************************************************/

/* global variable used to store and update system clock*/

static systemTime_t time = 0;

/** list of scheduled tasks */

static taskDescriptor* taskList = NULL;

/*FUNCTION DEFINITION *************************************************/

static void scheduler_update(void) {

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		taskDescriptor* temp = taskList;

		/* iterate through task list and update the expiries of each task by subtracting 1
		 * if task expiry reaches zero update execute of task to 1 and update expiry if task should be repeated by period > 0 */

		while (temp != NULL) {

			((*temp).expire)--;

			if ((*temp).expire == 0 && (*temp).period == 0) {
				(*temp).execute = 1;
			}

			else if ((*temp).expire == 0 && (*temp).period > 0) {
				(*temp).execute = 1;
				(*temp).expire = (*temp).period;
			}
			temp = (*temp).next;

		}
	}
}

void scheduler_init() {

	timer2_start();
	timer2_setCallback(scheduler_update);
}

void scheduler_run() {

	taskDescriptor* temp = taskList;

	/* iterate through task list and execute tasks with execute of 1 and remove from scheduler if period =0 */

	while (temp != NULL) {
		if ((*temp).execute == 1 && (*temp).period == 0) {
			(*temp).task((*temp).param);
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				(*temp).execute = 0;
			}
			scheduler_remove(temp);
		} else if ((*temp).execute == 1 && (*temp).period > 0) {
			(*temp).task((*temp).param);
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				(*temp).execute = 0;
			}
		}
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			temp = (*temp).next;
		}
	}
}

bool scheduler_add(taskDescriptor * toAdd) {

	taskDescriptor* temp = taskList;

	/* if task to be added is not a valid task we return 0 */

	if (toAdd == NULL)
		return 0;

	/* if task list is already empty, we add our task as the first task in the list */

	if (taskList == NULL) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			taskList = toAdd;
		}
		return 1;

		/* if task list is not empty , we iterate through tasklist till we reach the end of the list and then add our task*/

	} else {
		while ((*temp).next != NULL) {
			if (temp == toAdd)
				return 0;
			else
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					temp = (*temp).next;
				}
		}
		if (temp == toAdd)
			return 0;
		else {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				(*temp).next = toAdd;
			}
			return 1;
		}
	}
}

void scheduler_remove(taskDescriptor * toRemove) {

	taskDescriptor* temp = taskList;
	taskDescriptor* pointer;

	/* if task list has only one task and the task should be removed */

	if ((*temp).next == NULL && (temp == toRemove))
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			taskList = NULL;
		}

	/* if task list has more than one task and the first task should be removed */

	else if (temp == toRemove) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			pointer = (*temp).next;
			(*temp).next = NULL;
			temp = pointer;
			taskList = temp;
		}

		/* if task list has more than one task and the task that should be removed is not the first task in the list */

	} else {
		while ((*temp).next != toRemove && (*temp).next != NULL) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				temp = (*temp).next;
			}
		}
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			pointer = (*((*temp).next)).next;
			(*((*temp).next)).next = NULL;
			(*temp).next = pointer;

		}
	}
}

systemTime_t scheduler_getTime() {
	return time;
}

void scheduler_setTime(systemTime_t time2) {

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		time = time2;
	}
}

/* ISR timer 2 to call callback function scheduler update
 * also update system clock every 1 millisecond
 */

ISR(TIMER2_COMPA_vect) {

	localcallbacktimer2();
	time++;

	/* set maximum value for time (23 hrs 59 minutes 59 seconds converted to milliseconds)*/

	if (time > 86399000) {
		time = 0;
	}

}

