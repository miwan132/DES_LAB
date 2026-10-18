#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

const uint8_t numbers[10] = {
    0b11000000,
    0b11111001,
    0b10100100,
    0b10110000,
    0b10011001,
    0b10010010,
    0b10000010,
    0b11111000,
    0b10000000,
    0b10010000
};

void ustaw_8_diod(uint8_t bajt)
{
	PORTC = (PORTC & ~0b00111111) | (bajt & 0b00111111);
	uint8_t bity_dla_portb = (bajt >> 4) & 0b00001100;
	PORTB = (PORTB & ~0b00001100) | bity_dla_portb;
}

int main(void)
{
	DDRD = 0b01111111;
	DDRB = 0b00001111;
	DDRC = 0b00111111;
	
	uint8_t down = 0;
	uint8_t diody = 0;
	
	ustaw_8_diod(diody);

	while (1)
	{
		for (uint8_t j = 0; j < 100; j++)
		{
			uint8_t dec = j / 10;
			uint8_t uni   = j % 10;

			// Wyœwietlaj dan¹ liczbê przez 1 sekundê (100 * 10 ms = 1000 ms)
			for (uint8_t i = 0; i < 100; i++)
			{
				PORTB &= ~((1 << PORTB0) | (1 << PORTB1));
				PORTD = numbers[dec];
				PORTB |= (1 << PORTB0);
				_delay_ms(0.1);

				PORTB &= ~((1 << PORTB0) | (1 << PORTB1));
				PORTD = numbers[uni];
				PORTB |= (1 << PORTB1);
				_delay_ms(0.1);
			}
		}
		if(diody == 0b11111111){down = 1;}
		else if(diody == 0b00000000){down = 0;}
		
		switch (down)
		{
			case 0:
			diody = (diody << 1) | 1;
			break;
			
			case 1:
			diody = (diody >> 1);
			break;
		}
		ustaw_8_diod(diody);
	}
}