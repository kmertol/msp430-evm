/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

#ifndef SYSTIMER_H
#define SYSTIMER_H

#include "types.h"

/**************************   MODIFY   **************************************/
/* Maximum allowed amount of simultaneously running timers, increasing this
 * value will also increase the size of the statically allocated array */
#define SYS_TIMER_MAX_COUNT 4
/* Increase the tick for more low power, but less precise timekeeping. If you
 * actually look at the implementation of the timer ISR, there isn't much
 * overhead (excluding the wake-up from sleep delay). In most cases it can be
 * preferable to keep this low, only caveat is that you might miss ticks if
 * the interrupts remain disabled more than the tick time(e.g. flash erase).*/
#define SYS_TICK_MS 1
/* If defined will stop the timer when not in use. It is better to use this
 * mode if you are not using the same timer for other purposes */
#define SYS_TIMER_STOP_MODE
/****************************************************************************/

/* Since we are using 32768 ACLK as clock source, we can't get an exact 1ms
 * interrupt(about %2.4 error). If you prefer you can use this transformations
 * to get the exact(or close) values */
#define SYS_TICK_IN_SEC    1024
#define SYS_TIME_SEC(s)    (SYS_TICK_IN_SEC * (s))
#define SYS_TIME_MSEC(ms)  ((((u32)(ms)) * SYS_TICK_IN_SEC + 500)/1000)

/* A shortcut to add latency to a tasks new timeout return value */
#define SYS_TIME_OFFSET_LATENCY(timeout, latency) \
    ((timeout) > (latency) ? (timeout) - (latency) : 1)

void systimer_init(void);

/* Rather than controlling the return value of each systimer_new() call, it
 * is more convenient to handle all the conditions here when the creation of
 * a new timer fails(we are over SYS_TIMER_MAX_COUNT) */
void systimer_register_fail_callback(pfn_t callback);

/****************************************************************************/

typedef u16 (*tcb_id_t)(int id, u16 latency);
typedef void (*tcb_noid_t)(void);

bool _systimer_new(u16 timeout_ms, tcb_noid_t callback, int id);
bool _systimer_new_isr(u16 timeout_ms, tcb_noid_t callback, int id);
bool _systimer_renew(u16 timeout_ms, tcb_noid_t callback, int id);

/***************************** READ FIRST ***********************************/
/* - These functions will return False if they fail to create a new
 *   timer instance.
 * - What makes each timer instance unique is their function pointer and
 *   their id. So it is possible to register the same callback with
 *   different ids, and still be able to use systimer_renew on them.
 * - Using systimer_new with the same callback, will create a different timer
 *   instance each time, if this is not your intention, use systimer_renew.
 * - Ids supplied when creating a timer task should not be negative,
 *   these are reserved for internal use.
 * - The task callbacks should return the next timeout value, returning 0 will
 *   end(delete) the task. This is also the only way the tasks should change
 *   their timeout inside the callback, they shouldn't use renew on themselves.
 * - Generalizing the last sentence above, no timer callback should use renew
 *   on themselves. For tcb_noid_t, you should use new; for tcb_id_t, you should
 *   change your return value.
 * - Maximum value you should use as a timeout is 30 seconds.
 * - Only systimer_new_isr and systimer_new_task_isr can be called from an isr,
 *   systimer_renew and systimer_delete can not.
 */
/****************************************************************************/

/* Creates a new timer instance, use the isr version when calling this from an isr */
static inline bool systimer_new(u16 timeout_ms, tcb_noid_t callback)
{
    return _systimer_new(timeout_ms, callback, -1);
}

static inline bool systimer_new_task(u16 timeout_ms, tcb_id_t callback, int id)
{
    return _systimer_new(timeout_ms, (tcb_noid_t)callback, id);
}

/* The isr versions assume that the interrupts are disabled */
static inline bool systimer_new_isr(u16 timeout_ms, tcb_noid_t callback)
{
    return _systimer_new_isr(timeout_ms, callback, -1);
}

static inline bool systimer_new_task_isr(u16 timeout_ms, tcb_id_t callback, int id)
{
    return _systimer_new_isr(timeout_ms, (tcb_noid_t)callback, id);
}

/* Will change the timeout value of the timer that has the same function
 * pointer and if supplied the same id, if it can not find one, it will
 * create a new timer instance by calling systimer_new automatically.
 *
 * Warning: These are not thread safe, don't call from an isr
 */
static inline bool systimer_renew(u16 timeout_ms, tcb_noid_t callback)
{
    return _systimer_renew(timeout_ms, callback, -1);
}

static inline bool systimer_renew_task(u16 timeout_ms, tcb_id_t callback, int id)
{
    return _systimer_renew(timeout_ms, (tcb_noid_t)callback, id);
}

/* This is syntactic sugar for renew with timeout 0 */
static inline void systimer_delete(tcb_noid_t callback)
{
    _systimer_renew(0, callback, -1);
}

static inline void systimer_delete_task(tcb_id_t callback, int id)
{
    _systimer_renew(0, (tcb_noid_t)callback, id);
}

/****************************************************************************/
/* Just a convenient macro, that is used by the module */
#define _uninterrupted(codeline)               \
    do {                                       \
        uint _state = __get_interrupt_state(); \
        __disable_interrupt();                 \
        {                                      \
            codeline;                          \
        }                                      \
        __set_interrupt_state(_state);         \
    } while (0);


#endif /* SYSTIMER_H */
