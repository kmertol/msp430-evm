MSP430 Event Machine :  A low-power framework
=============================================

Bored of your super-loop, don't need an rtos at all?, need something lightweight?, low-power?.

Then enter the `event_machine();`

## What is the event machine?

A non-preemptive event-driven power management framework.

The framework consists of two modules:

* **event** module is the core where the event machine resides. This module can
  be used standalone.
* **systimer** is a multifunction timer that runs inside the event machine by
  registering its SYS_TICK event. This module can be excluded if you don't
  need the functionality.

As simple as they may seem, with the combination of two: you can quickly build
up a working system.

## Considerations

Before going any further, keep these in mind before deciding to use this framework:

* This is a non-preemptive system.
* Events are not queued, and not guaranteed to be prioritized or processed in FIFO manner.
* All hard deadlines should be handled by the ISRs. Since event handlers and timer callbacks
  are called outside of a ISR: they will not be blocking your ISRs or be hardly affected by
  them aside from the delay that they may cause.
* You should leave all the low power mode management to the event machine (this should be easy).
* No dynamic memory allocation is used, sorry :).

## Setup

Copy the whole evm folder to your project. To use the event machine you only need to include
two headers: *event.h* and *systimer.h*.
You can add the *evm/include* to your include path or just use the full paths _evm/include/*.h_
to the headers.

*The source code is written for CCS and TI's own MSP430 compiler. If you are using another compiler, you
may need to change a couple of things: probably the ISR declarations and maybe some intrinsics.*

## Configuration

**Event:**

Take a look at the template in *event.h* and create a *user_events.h* file: this is where you define your events.

**Systimer:**

* The implementation uses TimerA1, you can change it to a timer that is available in your system by modifying *systimer.c*.
  The only functions you need to change are `systimer_init`, `timer_start`, `timer_stop` and the `TIMER1_A0_VECTOR`.
* Make sure `EVENT_SYS_TICK` is also defined in the *user_events.h*.
* Set `SYSTIMER_MAX_COUNT` to a value that is high enough to accomodate all the timers
  that will be running simultaneously at one time. Since there is no dynamic allocation
  of timers, this should be defined at compile time.
* Set `SYSTIMER_TICK_MS` to a reasonable value depending on your systems needs, the default
  is one miliseconds.

## Overview of modules

### Event

The events are represented by bit flags, so they will behave like binary semaphores not queues.

There are 4 basic functions to use, which the `_isr` variants are to be used inside an ISR:

* `event_register`: Register a handler function for the corresponding event
* `event_set` or `event_set_isr`: Signal an event
* `event_lpm_set` or `event_lpm_set_isr`: Change the low power mode, default is LPM0
* `event_machine`: This is a call of no return; you can think it of as an analogy of a rtos
  where you call the task scheduler at the end of main, but instead here you will have events
  as opposed to tasks to be processed. The infinite loop works like this: the event machine processes
  events and when no more events are remaning enters into low power mode, and waits for an event
  to be signaled.

An example initialization and usage:

```c
#include "evm/include/event.h"
#include "evm/include/systimer.h"

void handler_foo(void) {}

void main(void)
{
    // if you are using the systimer module
    systimer_init();
    // register the event handler
    event_register(EVENT_FOO, handler_foo);
    // set the low power mode to LPM3
    event_lpm_set(EVENT_LPM3);
    // give control of execution to the event machine
    event_machine();
}

__interrupt void some_isr(void)
{
    // maybe do some other processing also
    event_set_isr(EVENT_FOO);
}
```

### Systimer

Let's get it straight, in most situations for a usable system you will also need to have some kind
of timed event or a scheduled task. If you decide to include this module, it will take care of you basic
timer needs to becoming a sort of task scheduler.

**Warning:**

> The timeout value that is passed as an argument to systimer functions is a 16 bit unsigned integer
  denoting miliseconds, which limits you to a maximum timeout value of 65535 miliseconds. Taking into
  account the internal arithmetics, I will half it. So as a rule of thumb don't use timeouts more
  than 30 seconds.

You have 2 timer callback types and corresponding functions for creating those timer instances:

* `tcb_noid_t`: These are one-shot timers. The callback functions take no arguments and return void.
* `tcb_id_t`: These are periodic timers. The functions used to create this type of timers have names
  ending with `_task`, you also have to supply an id as an argument for these. The periodicity is
  achieved by having the callback return its next timeout. The callback functions have the same id
  you supplied when creating the timer as a parameter, and a parameter called latency(in miliseconds):
  this can be used as an offset to normalize the next timeout.

Here we have 4 basic function types again with their variants: `_task` for periodic timers, `_isr` for the
only functions that you are allowed to call from an ISR:

* `systimer_new`: Create a new timer instance
* `systimer_renew`: Update the timer instance with a new timeout
* `systimer_delete`: Delete the timer instance
* `systimer_init`: Initialize the timer hardware and register EVENT_SYS_TICK

A quick example:

```c
// defining period to be 100ms
#define PERIOD 100

void one_shot(void) {}
u16 periodic(int id, u16 latency) { return PERIOD; }

void setup_all_timers(void)
{
    // set one shot timer to a 50ms timeout
    systimer_new(50, one_shot);
    // supply an id number to pass to the callback function (task)
    systimer_new_task(PERIOD, periodic, 0);
}
```

## Resource Usage

Just to give you an idea: this is the resource usage on my system
*(default evm configuration, small code, small data)*:

```
------------------------------------------------------------
systimer.obj
------------------------------------------------------------
    Initialized Data :         40  (4 timers) + 6 byte per extra timer
                Code :        586

------------------------------------------------------------
event.obj
------------------------------------------------------------
    Initialized Data :         6   (1 event) + 2 byte per extra event
                Code :        140
```

You can say that the code footprint is roughly **740 bytes**.
