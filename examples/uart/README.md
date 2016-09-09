# Uart

This is a bit more complicated use of events and timers, but very useful to understand.

**Configuration**:

Here we will be defining two more events in the `user_events.h` file:

* `EVENT_UART_RX` : Set when a new character is received from the uart
* `EVENT_UART_TX_END` : Set when the tx buffer is empty

----------------

As events are defined as binary semaphores, there is no way to catch if an event
has triggered multiple times before its callback its called. An serial communication
willl be a good example (probably you have done this before lots of times).

We will still be using events, but we just need a queue big enough that is able to hold
all the received chars until the event callback.

The ISR UART RX procedure will be like this:

* A new char in the RXBUF triggers an interrupt.
* The RXBUF is read, and the received char is added to the `rx_queue`.
* An event is set for the `rx_queue` consumer.

The job of the consumer is actually simple, read from the queue into its own buffer
for later parsing. But we will go a bit further than that.

**The procedure**:

On the first receive of a char we will start a reception timeout and continue collecting
chars until we receive a newline '\n'.

1. If we receive a newline, the whole receive buffer will be echoed back.
2. If the reception timeout is expired before we received a newline
   the buffer will be cleared, and no echoes.
