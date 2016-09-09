# A Mix of Timers

This example shows varius ways to use the systimer module.

1. A one-shot timer called `boot_delay` is set for a 5 seconds timeout after initialization.
2. Also a one-shot timer `no_food_for_dog` is set for a 30 secods timeout, which we will
   get into in the last part.
3. After 5 seconds is over the `boot_delay` function initializes a timer task called
   the `one_sec_tick` with an exact 1 second timeout using the `SYS_TIME_SEC(1)` macro.
4. The `one_sec_tick` task continues to run by returning the new timeout, by using
   the `SYS_TIME_OFFSET_LATENCY(timeout, latency)` macro, thereby guaranteeing to keep
   track of exact seconds.
5. After 10 seconds, the `one_sec_tick` functions enables the watchdog with a 250ms
   timeout, and registers a new timer task `feed_the_dog` with a 200ms period to
   reset the watchdog continuosly. Then it deletes itself by returning 0.
6. After a total of 30 seconds has passed. The `no_food_for_dog` function is called,
   this deletes the `feed_the_dog` task, as a result causing a watchdog timeout reset.
