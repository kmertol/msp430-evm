#ifndef USER_EVENTS_H
#define USER_EVENTS_H

#define EVENT_COUNT 3
typedef enum user_events {
	EVENT_SYS_TICK = 0,
	EVENT_UART_RX,
	EVENT_UART_TX_END
} event_id_t;

#endif
