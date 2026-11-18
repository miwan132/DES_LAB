#ifndef UART_H
#define UART_H

#include <avr/io.h>

#define uart_baudrate 9600UL
#define UBRR_speed (uint16_t)( ( F_CPU/(16UL * uart_baudrate) ) -1 )

void uart_send(const char *slowo);
void uart_init(void);

#endif