#include "twi1.h"

#include <avr/io.h>
#include <util/delay.h>

#define TWI1_TIMEOUT 60000UL
#define TWI1_FREQUENCY 100000UL

/*
 * Czeka na zakończenie bieżącej operacji kontrolera TWI1.
 * Odczytuje flagę TWINT1 i ogranicza czas oczekiwania licznikiem,
 * aby program nie zawiesił się bez końca w razie problemu z magistralą.
 * Zwraca 0 po zakończeniu operacji albo -1 po przekroczeniu limitu.
 */
static int8_t twi1_wait(void)
{
	uint32_t timeout = TWI1_TIMEOUT;

	while (!(TWCR1 & (1 << TWINT1)))
	{
		if (timeout-- == 0)
		{
			return -1;
		}
	}

	return 0;
}

/*
 * Zwraca kod stanu kontrolera TWI1 z rejestru TWSR1.
 * Maska usuwa bity preskalera, pozostawiając bity opisujące
 * wynik ostatniej operacji na magistrali.
 */
static uint8_t twi1_status(void)
{
	return TWSR1 & TWI1_STATUS_MASK;
}

/*
 * Generuje warunek START na magistrali I2C i czeka na jego wykonanie.
 * Akceptuje zarówno START rozpoczynający nową transakcję, jak i
 * powtórzony START, używany do zmiany kierunku transmisji w transakcji.
 * Zwraca 0, jeśli kontroler zgłosił jeden z oczekiwanych stanów, lub -1
 * w przypadku timeoutu albo nieoczekiwanego stanu.
 */
int8_t twi1_start(void)
{
	TWCR1 = (1 << TWINT1) | (1 << TWSTA1) | (1 << TWEN1);

	if (twi1_wait() != 0)
	{
		return -1;
	}

	uint8_t status = twi1_status();
	return (status == TWI1_STATUS_START || status == TWI1_STATUS_REPEATED_START) ? 0 : -1;
}

/*
 * Wysyła jeden bajt przez TWI1 i sprawdza, czy kontroler zgłosił
 * oczekiwany kod stanu (np. potwierdzenie adresu lub danych).
 * value jest wysyłanym bajtem, a expected_status stanem wymaganym
 * po zakończeniu transmisji. Zwraca 0 przy powodzeniu albo -1,
 * jeśli operacja przekroczyła limit czasu lub stan jest inny niż oczekiwany.
 */
int8_t twi1_send(uint8_t value, uint8_t expected_status)
{
	TWDR1 = value;
	TWCR1 = (1 << TWINT1) | (1 << TWEN1);

	if (twi1_wait() != 0)
	{
		return -1;
	}

	return twi1_status() == expected_status ? 0 : -1;
}

/*
 * Odbiera jeden bajt z magistrali TWI1.
 * value wskazuje miejsce, do którego zapisany zostanie odebrany bajt.
 * acknowledge wybiera odpowiedź mastera: ACK, gdy będą odbierane dalsze
 * bajty, albo NACK dla ostatniego bajtu. Funkcja weryfikuje odpowiedni
 * kod stanu i zwraca 0 po poprawnym odbiorze lub -1 w razie błędu.
 */
int8_t twi1_receive(uint8_t *value, uint8_t acknowledge)
{
	TWCR1 = (1 << TWINT1) | (1 << TWEN1) | (acknowledge ? (1 << TWEA1) : 0);

	if (twi1_wait() != 0)
	{
		return -1;
	}

	uint8_t status = twi1_status();
	if (status != (acknowledge ? TWI1_STATUS_MR_DATA_ACK : TWI1_STATUS_MR_DATA_NACK))
	{
		return -1;
	}

	*value = TWDR1;
	return 0;
}

/*
 * Kończy bieżącą transakcję, generując warunek STOP.
 * Czeka na skasowanie bitu TWSTO1, ale tylko do limitu TWI1_TIMEOUT,
 * dzięki czemu oczekiwanie nie jest nieskończone. Funkcja nie zwraca
 * statusu; ewentualny timeout nie jest przekazywany do wywołującego.
 */
void twi1_stop(void)
{
	TWCR1 = (1 << TWINT1) | (1 << TWSTO1) | (1 << TWEN1);

	uint32_t timeout = TWI1_TIMEOUT;
	while ((TWCR1 & (1 << TWSTO1)) && timeout-- > 0)
	{
	}
}

/*
 * Konfiguruje sprzętowy kontroler TWI1 do komunikacji I2C.
 * Ustawia linie PE0 i PE1 jako wejścia oraz wyłącza ich wewnętrzne
 * rezystory podciągające. Zeruje konfigurację preskalera, wylicza
 * wartość bit rate dla częstotliwości TWI1_FREQUENCY przy założeniu
 * zegara F_CPU, a następnie włącza kontroler.
 */
void twi1_init(void)
{
	DDRE &= ~((1 << DDE0) | (1 << DDE1));
	PORTE &= ~((1 << PE0) | (1 << PE1));

	TWSR1 = 0;
	TWBR1 = (uint8_t)((F_CPU / TWI1_FREQUENCY - 16UL) / 2UL);
	TWCR1 = (1 << TWEN1);
}
