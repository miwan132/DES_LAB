#define F_CPU 16000000UL
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "bmp280.h"
#include "twi1.h"
#include "uart.h"
#include "keypad.h"

static volatile uint8_t display_tens = 0; // zmienna do wyświetlania dziesiątek temperatury na wyświetlaczu 7-seg
static volatile uint8_t display_ones = 0; // zmienna do wyświetlania jedności temperatury na wyświetlaczu 7-seg
volatile uint8_t sensor_event = 0;  // flaga od sensora dla main
volatile uint8_t key_event = 0;  // flaga od klawiatury dla main
volatile uint8_t key_code = 0;   // kod naciśniętego klawisza
static uint8_t key_candidate = 0; // kandydat na kod naciśniętego klawisza
static uint8_t debounce_count = 0; // licznik przerwań dla debouncingu klawiatury
static uint8_t key_cooldown = 0; // flaga blokady klawiatury po naciśnięciu klawisza


static const uint8_t numbers[10] = {
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

static void display_timer_init(void)
{
    TCCR2A = (1 << WGM21);  /* CTC */
    TCCR2B = 0;
    TCNT2 = 0;
    OCR2A = 249;            /* 1 kHz przy F_CPU=16 MHz, preskalerze 64 */
    TIFR2 = (1 << OCF2A);
    TIMSK2 = (1 << OCIE2A);
    TCCR2B = (1 << CS22);   /* preskaler 64 */
}


static void set_tens(uint8_t liczby)
{
	PORTD = (PORTD & ~0b11111100) | ( (liczby << 1) & 0b11111100);
	PORTB = (PORTB & ~0b00000001) | ( (liczby >> 7) & 0b00000001 );
	PORTB &= ~((1 << PORTB1) | (1 << PORTB2));
	PORTB |= (1 << PORTB1);
}

static void set_ones(uint8_t liczby)
{
	PORTD = (PORTD & ~0b11111100) | ( (liczby << 1) & 0b11111100);
	PORTB = (PORTB & ~0b00000001) | ( (liczby >> 7) & 0b00000001 );
	PORTB &= ~((1 << PORTB1) | (1 << PORTB2));
	PORTB |= (1 << PORTB2);
}


ISR(TIMER2_COMPA_vect)
{
	static uint8_t digit = 0;
	static uint16_t ktimer = 0;
	static uint16_t stimer = 0;
	static uint16_t key_cooldown_counter = 0;

	// przerwanie wyświetlacza 1 kHz
    if (digit == 0)
    {
		set_tens(numbers[display_tens]);
		digit = 1;}
    else
    {
		set_ones(numbers[display_ones]);
		digit = 0;}

	// przerwanie sensora co 1 s
	if (++stimer >= 1000 && sensor_event == 0)
	{
		sensor_event = 1;
		stimer = 0;
	}

	// przerwanie klawiatury co 10 ms
	if (++ktimer >= 10 && key_cooldown == 0)
	{
		ktimer = 0;
		static uint8_t test;
		test = keypad_position();

		if (test != 0)
		{
			if(key_candidate == test && debounce_count < 6) debounce_count++;
			else if(key_candidate != test)
			{
				key_candidate = test;
				debounce_count = 0;
			}
			else
			{
				key_code = key_candidate;
				key_event = 1;
				debounce_count = 0;
			}
		}
	}

	if (key_cooldown == 1)
	{
		key_cooldown_counter++;
		if (key_cooldown_counter >= 100)
		{
			key_cooldown = 0;
			key_cooldown_counter = 0;
		}
	}
}

int main(void)
{
	DDRB = 0b00011111;
	DDRC = 0b00000011;
	DDRD = 0b11111100;
	DDRE = 0b00001100;

	PORTC = 0b00111111;

	uart_init();
	display_timer_init();
	sei();

	int32_t temperature_c = 0;
	uint32_t pressure_pa = 0;
	int8_t bmp280_status = bmp280_port_init();

	if (bmp280_status == BMP280_OK)
	{
		uart_send("BMP280 OK\r\n");
	}
	else
	{
		uart_send("BMP280 init error: ");

		char error_text[6];
		itoa(bmp280_status, error_text, 10);
		uart_send(error_text);
		uart_send("\r\n");
	}

	// KONIEC SETUPU, POCZĄTEK GŁÓWNEJ PĘTLI PROGRAMU
	while (1)
	{
		if(key_event)
		{
			//key_candidate = key_code;
			key_event = 0;
			key_cooldown = 1; // ustawienie flagi blokady klawiatury po naciśnięciu klawisza

			if(key_code != 0)
			{
				uart_send("Wcisnięty klawisz: ");
				char key_text[3];
				itoa(key_code, key_text, 10);
				uart_send(key_text);
				uart_send("\r\n");
			}
		}

		if (bmp280_status == BMP280_OK && sensor_event)
		{
			sensor_event = 0;
			int8_t bmp280_read_status = bmp280_port_read(&temperature_c, &pressure_pa);

			if (bmp280_read_status == BMP280_OK)
			{
				int16_t calkowita = (int16_t)(temperature_c / 100);
				uint16_t ulamek = (uint16_t)labs(temperature_c % 100);

				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					display_tens = (uint8_t)(calkowita / 10);
					display_ones = (uint8_t)(calkowita % 10);
				}

				char value[12];
				uart_send("BMP280 T=");
				ltoa((long)calkowita, value, 10);
				uart_send(value);
				uart_send(".");
				ultoa((unsigned long)ulamek, value, 10);
				uart_send(value);
				uart_send(" C, P=");
				ultoa((unsigned long)pressure_pa, value, 10);
				uart_send(value);
				uart_send(" Pa\r\n");
			}
			else
			{
				uart_send("BMP280 read error\r\n");
			}
		}
	}
}