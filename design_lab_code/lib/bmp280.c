#include "bmp280.h"
#include "twi1.h"

#include <avr/io.h>
#include <util/delay.h>

#include "BMP2-Sensor-API-master/bmp2.h"

static struct bmp2_dev sensor;
static struct bmp2_config sensor_config;
static uint8_t sensor_address;

/*
 * Funkcja odczytu dla interfejsu I2C biblioteki BMP2.
 * Wykonuje transakcję: START, adres czujnika z zapisem, adres rejestru,
 * powtórzony START, adres czujnika z odczytem, a następnie odbiera length
 * bajtów. Wysyła ACK po wszystkich bajtach poza ostatnim, po którym
 * wysyła NACK, i kończy transakcję warunkiem STOP.
 * reg_data jest buforem docelowym, intf_ptr wskazuje adres czujnika,
 * a reg_addr określa pierwszy rejestr do odczytu. Zwraca 0 przy sukcesie
 * albo -1 dla nieprawidłowych argumentów lub błędu transmisji.
 */
static int8_t bmp2_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	uint8_t address;

	if (reg_data == 0 || intf_ptr == 0 || length == 0)
	{
		return -1;
	}

	address = *(uint8_t *)intf_ptr;

	if (twi1_start() != 0 ||
		twi1_send((uint8_t)(address << 1), TWI1_STATUS_MT_SLA_ACK) != 0 ||
		twi1_send(reg_addr, TWI1_STATUS_MT_DATA_ACK) != 0 ||
		twi1_start() != 0 ||
		twi1_send((uint8_t)((address << 1) | 1), TWI1_STATUS_MR_SLA_ACK) != 0)
	{
		twi1_stop();
		return -1;
	}

	for (uint32_t i = 0; i < length; i++)
	{
		if (twi1_receive(&reg_data[i], i + 1 < length) != 0)
		{
			twi1_stop();
			return -1;
		}
	}

	twi1_stop();
	return 0;
}

/*
 * Funkcja zapisu dla interfejsu I2C biblioteki BMP2.
 * Każdy zapis wysyła START, adres czujnika z zapisem, adres rejestru
 * i jeden bajt danych, po czym kończy transakcję warunkiem STOP.
 * Pierwszy bajt z reg_data trafia do reg_addr; przy kolejnych bajtach
 * następny element bufora jest nowym adresem rejestru, a kolejny jego
 * wartością. Z tego powodu wielobajtowy bufor musi mieć nieparzystą
 * długość. reg_data jest źródłem danych, intf_ptr wskazuje adres
 * czujnika, a length określa liczbę bajtów. Zwraca 0 przy sukcesie
 * albo -1 dla argumentów nieprawidłowych lub błędu transmisji.
 */
static int8_t bmp2_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	uint8_t address;
	uint32_t i = 0;

	if (reg_data == 0 || intf_ptr == 0 || length == 0)
	{
		return -1;
	}

	address = *(uint8_t *)intf_ptr;

	if (length > 1 && (length % 2) == 0)
	{
		return -1;
	}

	while (i < length)
	{
		if (twi1_start() != 0 ||
			twi1_send((uint8_t)(address << 1), TWI1_STATUS_MT_SLA_ACK) != 0 ||
			twi1_send(reg_addr, TWI1_STATUS_MT_DATA_ACK) != 0 ||
			twi1_send(reg_data[i], TWI1_STATUS_MT_DATA_ACK) != 0)
		{
			twi1_stop();
			return -1;
		}

		twi1_stop();
		i++;

		if (i < length)
		{
			reg_addr = reg_data[i];
			i++;
		}
	}

	return 0;
}

/*
 * Implementuje opóźnienie wymagane przez bibliotekę BMP2.
 * Wykonuje period opóźnień po 1 mikrosekundzie; intf_ptr jest
 * niewykorzystywany, ponieważ opóźnienie realizuje sprzętowa funkcja
 * _delay_us() z biblioteki AVR.
 */
static void bmp2_delay_us(uint32_t period, void *intf_ptr)
{
	(void)intf_ptr;

	while (period-- > 0)
	{
		_delay_us(1);
	}
}

/*
 * Inicjalizuje port BMP280 i czujnik za pośrednictwem biblioteki BMP2.
 * Najpierw konfiguruje TWI1 i przekazuje bibliotece funkcje odczytu,
 * zapisu oraz opóźnienia. Próbuje wykryć czujnik pod podstawowym
 * adresem I2C, a w razie niepowodzenia pod adresem alternatywnym.
 * Po wykryciu ustawia rozdzielczość pomiaru, okres odświeżania,
 * filtrację i konfigurację interfejsu SPI, po czym przełącza czujnik
 * w tryb normalny. Zwraca kod błędu biblioteki, jeśli inicjalizacja,
 * konfiguracja lub ustawienie trybu zasilania się nie powiedzie;
 * w przeciwnym razie zwraca BMP2_OK.
 */
int8_t bmp280_port_init(void)
{
	int8_t result;

	twi1_init();

	sensor.intf = BMP2_I2C_INTF;
	sensor.intf_ptr = &sensor_address;
	sensor.read = bmp2_i2c_read;
	sensor.write = bmp2_i2c_write;
	sensor.delay_us = bmp2_delay_us;

	sensor_address = BMP2_I2C_ADDR_PRIM;
	result = bmp2_init(&sensor);

	if (result != BMP2_OK)
	{
		sensor_address = BMP2_I2C_ADDR_SEC;
		result = bmp2_init(&sensor);
	}

	if (result != BMP2_OK)
	{
		return result;
	}

	sensor_config.os_mode = BMP2_OS_MODE_STANDARD_RESOLUTION;
	sensor_config.odr = BMP2_ODR_1000_MS;
	sensor_config.filter = BMP2_FILTER_COEFF_4;
	sensor_config.spi3w_en = BMP2_SPI3_WIRE_DISABLE;

	result = bmp2_set_config(&sensor_config, &sensor);
	if (result != BMP2_OK)
	{
		return result;
	}

	return bmp2_set_power_mode(BMP2_POWERMODE_NORMAL, &sensor_config, &sensor);
}

/*
 * Odczytuje bieżące dane pomiarowe z czujnika.
 * temperature_centi_c wskazuje zmienną, do której zapisywana jest
 * temperatura w setnych częściach stopnia Celsjusza, a pressure_pa
 * wskazuje zmienną na ciśnienie w paskalach. Jeśli którykolwiek wskaźnik
 * jest pusty, zwraca BMP2_E_NULL_PTR. W przeciwnym razie pobiera dane
 * przez bibliotekę BMP2 i zapisuje wyniki tylko wtedy, gdy odczyt
 * zakończył się powodzeniem; zwraca kod wyniku tej operacji.
 */
int8_t bmp280_port_read(int32_t *temperature_centi_c, uint32_t *pressure_pa)
{
	struct bmp2_data data;
	int8_t result;

	if (temperature_centi_c == 0 || pressure_pa == 0)
	{
		return BMP2_E_NULL_PTR;
	}

	result = bmp2_get_sensor_data(&data, &sensor);
	if (result == BMP2_OK)
	{
		*temperature_centi_c = data.temperature;
		*pressure_pa = data.pressure;
	}

	return result;
}
