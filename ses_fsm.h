#ifndef FSM_H_
#define FSM_H_

/*INCLUDES *******************************************************************/
#include"ses_scheduler.h"
#include "ses_common.h"
#include "ses_timer.h"

/*DEFINES , TYPES , DECLARATIONS ********************************************************************/
#define ENTRY 0
#define EXIT 3
#define  JOYSTICK_PRESSED 1
#define  ROTARY_PRESSED 2

typedef struct fsm_s Fsm; //< typedef for alarm clock state machine

typedef struct event_s Event; //< event type for alarm clock fsm

typedef uint8_t fsmReturnStatus; //< typedef to be used with above enum

typedef fsmReturnStatus (*State)(Fsm *, const Event*); /** typedef for state event handler functions */

typedef struct event_s {
	uint8_t signal; //< identifies the type of event
} Event;

/** return values */

enum {
	RET_HANDLED, //< event was handled
	RET_IGNORED, //< event was ignored; not used in this implementation
	RET_TRANSITION //< event was handled and a state transition occurred
};

/* FINITE STATE MACHINE structure*/

struct fsm_s {
	State state; //< current state, pointer to event handler
	bool isAlarmEnabled; //< flag for the alarm status
	timing timeSet; //< multi-purpose var for system time and alarm time
};

/* FUNCTIONS *******************************************************/

inline static void fsm_dispatch(Fsm* fsm, const Event* event) {
	static Event entryEvent = { .signal = ENTRY };
	static Event exitEvent = { .signal = EXIT };
	State s = fsm->state;
	fsmReturnStatus r = fsm->state(fsm, event);
	if (r == RET_TRANSITION) {
		s(fsm, &exitEvent); //< call exit action of last state
		fsm->state(fsm, &entryEvent); //< call entry action of new state
	}

}
/* sets and calls initial state of state machine */
inline static void fsm_init(Fsm* fsm, State init) {
//... other initialization
	Event entryEvent = { .signal = ENTRY };
	fsm->state = init;
	fsm->state(fsm, &entryEvent);
}

#endif /* FSM_H_ */

