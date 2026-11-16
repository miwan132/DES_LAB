#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

//uart
#define uart_baudrate 9600UL
#define UBRR_speed (uint16_t)( ( F_CPU/(16UL * uart_baudrate) ) -1 )

//i2c
#define F_I2C 100000UL
#define status (TWSR0 & 0b11111000)

//kalib
typedef int32_t BMP280_S32_t;
typedef uint32_t BMP280_U32_t;
typedef int64_t BMP280_S64_t;

uint16_t dig_T1;
int16_t  dig_T2, dig_T3;

uint16_t dig_P1;
int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

int32_t UT, UP;

BMP280_S32_t t_fine; // t_fine carries fine temperature as global value

	

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

/*
	PINY

	PB 0-5
	PC 0-5
	PD 0-7

*/

void I2C_init()
{
	TWSR0 = 0;	//TWPS0 i TWPS1 = 0 => preskaler = 1
	TWBR0 = (F_CPU/F_I2C - 16UL)/2UL;

	TWCR0 = (1<<2);

	// e-manuel
	/* 

	TWDR0
	w nadawaniu nastepny bajt do nadania (można wpisac), w odbiorze ostatni odebrany

	TWSR0& 0b11111000
	informacje co robi teraz I2C, read only

	tx
	0x08 START
	0x10 repeated START
	0x18 ACK adresu
	0x20 NACK adresu
	0X28 DATA/ACK danej
	0x30 DATA/NACK danej
	
	rx

	0x58 mam data wyslane NACK



	TWCR0
	7. flaga konca zadania, ale tez start kazdego zadania (trzeba czyscic)
	6. robi dla 1 - ACK || 0-NACK
	5. robi start (trzeba czyscic)
	4. robi stop (sam sie czysci)
	3. wykrywa kolizje (czysci sie gdy edytuje sie jakos TWDR)
	2. wlacza I2C na pinach
	1. nic
	0. wlacza przerwania TWI
	czyszczenie W1C

	*/
}

void I2C_start()
{
	TWCR0 = (1<<2) | (1<<5) | (1<<7);

	while (!(TWCR0 & (1<<7)));

}

void I2C_send(uint8_t data)
{

	TWDR0 = data;
	TWCR0 = (1<<2) | (1<<7); // start = 0
	
	while (!(TWCR0 & (1<<7)));

}

void I2C_stop()
{
	TWCR0 = (1<<2) | (1<<4) | (1<<7);
}

void I2C_write(uint8_t addr, uint8_t reg, uint8_t data)
{
	I2C_start();

	if (status!=0x08){
		uart_send_tekst("Blad wyslania start");
		I2C_stop();
		return;
	}

	I2C_send(addr<<1);

	if (status!=0x18){
		uart_send_tekst("Blad wyslania addr");
		I2C_stop();
		return;
	}

	I2C_send(reg);

	if (status!=0x28){
		uart_send_tekst("Blad wyslania reg");
		I2C_stop();
		return;
	}

	I2C_send(data);

	if (status!=0x28){
		uart_send_tekst("Blad wyslania data");
		I2C_stop();
		return;
	}

	I2C_stop();
}

uint8_t I2C_read(uint8_t addr, uint8_t reg)
{

	I2C_start();

	if (status!=0x08){
		uart_send_tekst("Blad wyslania start");
		I2C_stop();
		return 0xff;
	}

	I2C_send(addr<<1);

	if (status!=0x18){
		uart_send_tekst("Blad wyslania addr");
		I2C_stop();
		return 0xff;
	}

	I2C_send(reg);

	if (status!=0x28){
		uart_send_tekst("Blad wyslania reg");
		I2C_stop();
		return 0xff;
	}

	I2C_start();

	if (status!=0x10){
		uart_send_tekst("Blad wyslania start");
		I2C_stop();
		return 0xff;
	}

	I2C_send((addr<<1)|1);

	if (status!=0x40){
		uart_send_tekst("Blad wyslania addr");
		I2C_stop();
		return 0xff;
	}

	

	TWCR0 = (1<<2) | (0<<6) | (1<<7);

	while (!(TWCR0 & (1<<7)));

	if (status!=0x58){
		uart_send_tekst("Blad odbioru");
		I2C_stop();
		return 0xff;
	}

	uint8_t odebrana = TWDR0;

	I2C_stop();

	return odebrana;
}

void uart_init()
{

	UBRR0H = (UBRR_speed >> 8);					// prędkość
	UBRR0L = (UBRR_speed);						// prędkość
	UCSR0B |= (1 << TXEN0);						// włączenie nadajnika TX
	UCSR0C |= ((1 << UCSZ00) | (1 << UCSZ01));	// liczba bitów danych (8)
	UCSR0C &= ~(1 << USBS0);					// ilość bitów stopu (1)
	UCSR0C &= ~((1 << UPM00) | (1 << UPM01));	// bity parzystosci (brak)

}

void uart_send_tekst(const char *slowo)
{
	while(*slowo != '\0')
	{
		while ( !(UCSR0A & (1 << UDRE0)) ); 	// czeka aż UDRE0 będzie 1 <=> bufor jest pusty
		UDR0 = *slowo;
		slowo++;
	}
}

void uart_send_num(uint8_t liczba)
{
const char liczby[] = "0123456789";
char tekst[4];

tekst[0] = liczby[liczba/100];
tekst[1] = liczby[(liczba/10)%10];
tekst[2] = liczby[liczba%10];
tekst[3] = '\0';

uart_send_tekst(tekst);
}

void kalibracja()
{
	dig_T1 = (uint16_t)I2C_read(0x76, 0x88) | ((uint16_t)I2C_read(0x76, 0x89) << 8);
	dig_T2 = (int16_t)((uint16_t)I2C_read(0x76, 0x8A) | ((uint16_t)I2C_read(0x76, 0x8B) << 8));
	dig_T3 = (int16_t)((uint16_t)I2C_read(0x76, 0x8C) | ((uint16_t)I2C_read(0x76, 0x8D) << 8));
	
	dig_P1 = (uint16_t)I2C_read(0x76, 0x8E) | ((uint16_t)I2C_read(0x76, 0x8F) << 8);
	dig_P2 = (int16_t)((uint16_t)I2C_read(0x76, 0x90) | ((uint16_t)I2C_read(0x76, 0x91) << 8));
	dig_P3 = (int16_t)((uint16_t)I2C_read(0x76, 0x92) | ((uint16_t)I2C_read(0x76, 0x93) << 8));
	dig_P4 = (int16_t)((uint16_t)I2C_read(0x76, 0x94) | ((uint16_t)I2C_read(0x76, 0x95) << 8));
	dig_P5 = (int16_t)((uint16_t)I2C_read(0x76, 0x96) | ((uint16_t)I2C_read(0x76, 0x97) << 8));
	dig_P6 = (int16_t)((uint16_t)I2C_read(0x76, 0x98) | ((uint16_t)I2C_read(0x76, 0x99) << 8));
	dig_P7 = (int16_t)((uint16_t)I2C_read(0x76, 0x9A) | ((uint16_t)I2C_read(0x76, 0x9B) << 8));
	dig_P8 = (int16_t)((uint16_t)I2C_read(0x76, 0x9C) | ((uint16_t)I2C_read(0x76, 0x9D) << 8));
	dig_P9 = (int16_t)((uint16_t)I2C_read(0x76, 0x9E) | ((uint16_t)I2C_read(0x76, 0x9F) << 8));

}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P)
{
	BMP280_S64_t var1, var2, p;
	var1 = ((BMP280_S64_t)t_fine) - 128000;
	var2 = var1 * var1 * (BMP280_S64_t)dig_P6;
	var2 = var2 + ((var1*(BMP280_S64_t)dig_P5)<<17);
	var2 = var2 + (((BMP280_S64_t)dig_P4)<<35);
	var1 = ((var1 * var1 * (BMP280_S64_t)dig_P3)>>8) + ((var1 * (BMP280_S64_t)dig_P2)<<12);
	var1 = (((((BMP280_S64_t)1)<<47)+var1))*((BMP280_S64_t)dig_P1)>>33;
	if (var1 == 0){return 0;} // avoid exception caused by division by zero
	p = 1048576-adc_P;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = (((BMP280_S64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((BMP280_S64_t)dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)dig_P7)<<4);
	return (BMP280_U32_t)p;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T)
{
	BMP280_S32_t var1, var2, T;
	var1 = ((((adc_T>>3) - ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) - ((BMP280_S32_t)dig_T1)))
	>> 12) *
	((BMP280_S32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}

void sens_conf()
{
	uint8_t ctrl_meas = 0b10000111;
	// 7,6,5 oversampling temp || 4,3,2 oversampling press || 1,0 power mode
	/*
	
	
	
	000 - skip
	001 - x1  16b
	010 - x2  17b
	011 - x4  18b
	100 - x8  19b
	101 - x16 20b
	110 - x16 20b
	111 - x16 20b
	
	power mode
	00 - sleep
	01 - forced
	10 - forced
	11 - normal
	*/
	I2C_write(0x76, 0xF4, ctrl_meas);

}

int main(void)
{

	I2C_init();
	uart_init();

	kalibracja();
	sens_conf();


	uart_send_tekst("Identyfikator: ");
	uint8_t identyfikator = I2C_read(0x76, 0xD0);

	uart_send_num(identyfikator);

	uart_send_tekst("(Jezeli wynosi 0x58 (0d88) to git)\r\n");

	while (1)
	{
		_delay_ms(2000);

		//press
		uint32_t cisn_msb = I2C_read(0x76, 0xF7); // MSB  7:0
		uint32_t cisn_lsb = I2C_read(0x76, 0xF8); // LSB  7:0
		uint32_t cisn_xlsb = I2C_read(0x76, 0xF9); // XLSB 3:0 (7,6,5,4)

		//temp
		uint32_t temp_msb = I2C_read(0x76, 0xFA); // MSB  7:0
		uint32_t temp_lsb = I2C_read(0x76, 0xFB); // LSB  7:0
		uint32_t temp_xlsb = I2C_read(0x76, 0xFC); // XLSB 3:0 (7,6,5,4)


		UT = (temp_msb << 12) | (temp_lsb << 4) | (temp_xlsb >> 4);
		
		UP = (cisn_msb << 12) |	(cisn_lsb << 4) | (cisn_xlsb >> 4);


		int32_t temperatura = bmp280_compensate_T_int32(UT);
		uint32_t cisnienie = bmp280_compensate_P_int64(UP);

		uart_send_tekst("Temperatura: ");
		uart_send_num((uint8_t) temperatura/100);
		uart_send_tekst(".");
		uart_send_num((uint8_t) temperatura%100);
		uart_send_tekst(" C\r\n");
		
		uart_send_tekst("Cisnienie: ");
		uart_send_num((uint8_t) cisnienie/256);
		uart_send_tekst(" Pa\r\n");
	}
}
