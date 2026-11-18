#include "uart.h"

void uart_send(const char *slowo)
{
	while(*slowo != '\0')
	{
		while ( !(UCSR0A & (1 << UDRE0)) ); 	// czeka aż UDRE0 będzie 1 <=> bufor jest pusty
		UDR0 = *slowo;
		slowo++;
	}
}

void uart_init(void)
{
	UBRR0H = (UBRR_speed >> 8);					// prędkość
	UBRR0L = (UBRR_speed);						// prędkość
	UCSR0A = 0;
	UCSR0B |= (1 << TXEN0);						// włączenie nadajnika TX
	UCSR0C |= ((1 << UCSZ00) | (1 << UCSZ01));	// liczba bitów danych (8)
	UCSR0C &= ~(1 << USBS0);					// ilość bitów stopu (1)
	UCSR0C &= ~((1 << UPM00) | (1 << UPM01));	// bity parzystosci (brak)

}