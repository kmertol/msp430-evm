# Debouncing made easy

This example shows how to define port pins as debounced input pins and
use the systimer module for debouncing.

Advantages of this implementation is:

* It is straightforward to add other input pins by using an id and a table,
  and the timer works seamlessly with it.
* Ability to define different debounce values for each individual pin.
* Low-power of course. No polling, no active timers even running when not
  debouncing. The example implementation basically sleeps all the time in LPM4.

The debouncing procedure works like this:

1. A falling or rising edge on the pin triggers an interrupt.
2. The ISR finds which pin the interrupt is triggered from, then disables
   the interrupt for that pin and creates a new timer with its unique
   debounce value. Disabling the individual interrupt prevents reentry
   during the debouncing period.
3. The debouncing period expires and its appropriate callback is called.
4. The callback function controls if the state of the pin is same
   as before when debouncing had started, if so it changes the input pins state
   by saving it into a variable. And if registered, calls the accompanying
   `on_change()` callback.

In the example we have two inputs pins and the debouncing values are set as:
* `input_0`: 100 miliseconds
* `input_1`: 2 seconds

**Considerations:**

* For each added input pin, you need one extra timer, keeping in mind that all pins
can be debouncing at the same time. So keep this in mind when setting the
`SYS_TIMER_MAX_COUNT` variable.

