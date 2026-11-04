#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define uart_baudrate 9600UL
#define UBRR_speed (uint16_t)( ( F_CPU/(16UL * uart_baudrate) ) -1 )

/*
	PINY

	PB 0-5
	PC 0-5
	PD 0-7

*/


const uint8_t numbers[10] = {
    0b00000011,
    0b10011111,
    0b00100100,
    0b00001101,
    0b10011001,
    0b01001001,
    0b01000001,
    0b00011111,
    0b00000001,
    0b00001001
};


void uart_init()
{

	UBRR0H = (UBRR_speed >> 8);					// prędkość
	UBRR0L = (UBRR_speed);						// prędkość
	UCSR0B |= (1 << TXEN0);						// włączenie nadajnika TX
	UCSR0C |= ((1 << UCSZ00) | (1 << UCSZ01));	// liczba bitów danych (8)
	UCSR0C &= ~(1 << USBS0);					// ilość bitów stopu (1)
	UCSR0C &= ~((1 << UPM00) | (1 << UPM01));	// bity parzystosci (brak)

}

void uart_send(const char *slowo)
{
	while(*slowo != '\0')
	{
		while ( !(UCSR0A & (1 << UDRE0)) ); 	// czeka aż UDRE0 będzie 1 <=> bufor jest pusty
		UDR0 = *slowo;
		slowo++;
	}
}

void ustaw_8_diod(uint8_t diody)
{
	PORTC = (PORTC & ~0b00111111) | ( (diody >> 2) & 0b00111111);
	PORTB = (PORTB & ~0b00011000) | ( (diody << 3) & 0b00011000 );
}

void ustaw_licznik(uint8_t liczby)
{
	PORTD = (PORTD & ~0b11111100) | ( (liczby << 1) & 0b11111100);
	PORTB = (PORTB & ~0b00000001) | ( (liczby >> 7) & 0b00000001 );
}

int main(void)
{
	DDRD = 0b11111100;
	DDRB = 0b00011111;
	DDRC = 0b00111111;

	uint8_t tryb = 0;
	uint8_t diody = 0;
	uint8_t zapalone_diody = 0;
	
	uart_init();
	ustaw_8_diod(diody);

	while (1)
	{
		for (uint8_t j = 0; j < 100; j++)
		{
			uint8_t dziesietne = j / 10;
			uint8_t jednosci = j % 10;

			for (uint8_t i = 0; i < 100; i++)
			{
				PORTB &= ~((1 << PORTB1) | (1 << PORTB2));
				ustaw_licznik(numbers[dziesietne]);
				PORTB |= (1 << PORTB1);
				_delay_ms(0.1);

				PORTB &= ~((1 << PORTB1) | (1 << PORTB2));
				ustaw_licznik(numbers[jednosci]);
				PORTB |= (1 << PORTB2);
				_delay_ms(0.1);
			}
		}

		if(diody == 0b11111111){tryb = 1;}
		else if(diody == 0b00000000){tryb = 0;}
		
		switch (tryb)
		{
			case 0:
			diody = (diody << 1) | 1;
			zapalone_diody++;
			break;
			
			case 1:
			diody = (diody >> 1);
			zapalone_diody--;
			break;
		}
		ustaw_8_diod(diody);

		

		char komunikat[] = "Zapalone diody: ";
		
		komunikat[16] = '0' + zapalone_diody;
		komunikat[17] = '\r';
		komunikat[18] = '\n';
		komunikat[19] = '\0';

		uart_send(komunikat);
	}

}
