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
#define status (TWSR1 & 0b11111000)

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

void uart_send_num(uint32_t liczba)
{
	char tekst[11];
	const char liczby[] = "0123456789";
	tekst[10] = '\0';
	uint8_t n = 10;
	if (liczba == 0){uart_send_tekst("0"); return;}
	while(liczba != 0){
		n--;
		tekst[n] = liczby[liczba%10];
		liczba = liczba/10;
	}
	char tekst_cut[11-n];
	for(uint8_t i=0; i<=(10-n);i++){
		tekst_cut[i] = tekst[n+i];
	}
	uart_send_tekst(tekst_cut);
}

static void i2c_wait(void)
{
	uint16_t n = 60000;
	while (!(TWCR1 & (1<<7))) {
		if (--n == 0) {            // timeout: zresetuj TWI i wyjdź
			TWCR1 = 0;
			TWCR1 = (1<<2);
			return;
		}
	}
}

void I2C_init()
{
	TWSR1 = 0;	//TWPS0 i TWPS1 = 0 => preskaler = 1
	TWBR1 = (F_CPU/F_I2C - 16UL)/2UL;

	TWCR1 = (1<<2);

	// e-manuel
	/* 

	TWDR1
	w nadawaniu nastepny bajt do nadania (można wpisac), w odbiorze ostatni odebrany

	TWSR1& 0b11111000
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



	TWCR1
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

	TWCR1 = (1<<2) | (1<<5) | (1<<7);

	i2c_wait();

}

void I2C_send(uint8_t data)
{

	TWDR1 = data;
	TWCR1 = (1<<2) | (1<<7); // start = 0
	
	i2c_wait();

}

void I2C_stop()
{
	TWCR1 = (1<<2) | (1<<4) | (1<<7);
	_delay_us(100);
}

uint8_t I2C_write(uint8_t addr, uint8_t reg, uint8_t data)
{
	I2C_start();

	if (status!=0x08){
		uart_send_tekst("Blad wyslania start\r\n");
		I2C_stop();
		return 1;
	}

	I2C_send(addr<<1);

	if (status!=0x18){
		uart_send_tekst("Blad wyslania addr\r\n");
		I2C_stop();
		return 1;
	}

	I2C_send(reg);

	if (status!=0x28){
		uart_send_tekst("Blad wyslania reg\r\n");
		I2C_stop();
		return 1;
	}

	I2C_send(data);

	if (status!=0x28){
		uart_send_tekst("Blad wyslania data\r\n");
		I2C_stop();
		return 1;
	}

	I2C_stop();
	return 0;
}

uint8_t I2C_read(uint8_t addr, uint8_t reg_start, uint8_t ilosc, uint8_t *odczyt)
{
	I2C_start();

	if (status!=0x08){
		uart_send_tekst("Blad wyslania start\r\n");
		I2C_stop();
		return 1;
	}

	I2C_send(addr<<1);

	if (status!=0x18){
		uart_send_tekst("Blad wyslania addr\r\n");
		I2C_stop();
		return 1;
	}

	I2C_send(reg_start);

	if (status!=0x28){
		uart_send_tekst("Blad wyslania reg\r\n");
		I2C_stop();
		return 1;
	}

	I2C_start(); // tryb read

	if (status!=0x10){
		uart_send_tekst("Blad wyslania start\r\n");
		I2C_stop();
		return 1;
	}

	I2C_send((addr<<1)|1);

	if (status!=0x40){
		uart_send_tekst("Blad wyslania addr\r\n");
		I2C_stop();
		return 1;
	}

	for (uint8_t i = 0; i < (ilosc-1); i++){

		TWCR1 = (1<<2) | (1<<6) | (1<<7);

		i2c_wait();

		if (status!=0x50){
			uart_send_tekst("Blad odbioru\r\n");
			I2C_stop();
			return 1;
		}
		odczyt[i] = TWDR1;
	}
	
	TWCR1 = (1<<2) | (0<<6) | (1<<7);

	i2c_wait();

	if (status!=0x58){
		uart_send_tekst("Blad odbioru\r\n");
		I2C_stop();
		return 1;
	}

	odczyt[ilosc-1]=TWDR1;

	I2C_stop();
	return 0;
}

void kalibracja()
{
	uart_send_tekst("Rozpoczynam kalibracje...\r\n");
	uint8_t read[24];
	uint8_t n = 10;
	while(I2C_read(0x76, 0x88, 24, &read[0]) && n)
	{
		uart_send_tekst("Kalibracja nieudana, ponawiam...\r\n");
		_delay_ms(200);
		n--;
	};

	if(!n){uart_send_tekst("Przekroczono limit prob\r\n");}

	dig_T1 = (uint16_t)read[0] | ((uint16_t)read[1] << 8);
	dig_T2 = (int16_t)((uint16_t)read[2] | ((uint16_t)read[3] << 8));
	dig_T3 = (int16_t)((uint16_t)read[4] | ((uint16_t)read[5] << 8));
	
	dig_P1 = (uint16_t)read[6] | ((uint16_t)read[7] << 8);
	dig_P2 = (int16_t)((uint16_t)read[8] | ((uint16_t)read[9] << 8));
	dig_P3 = (int16_t)((uint16_t)read[10] | ((uint16_t)read[11] << 8));
	dig_P4 = (int16_t)((uint16_t)read[12] | ((uint16_t)read[13] << 8));
	dig_P5 = (int16_t)((uint16_t)read[14] | ((uint16_t)read[15] << 8));
	dig_P6 = (int16_t)((uint16_t)read[16] | ((uint16_t)read[17] << 8));
	dig_P7 = (int16_t)((uint16_t)read[18] | ((uint16_t)read[19] << 8));
	dig_P8 = (int16_t)((uint16_t)read[20] | ((uint16_t)read[21] << 8));
	dig_P9 = (int16_t)((uint16_t)read[22] | ((uint16_t)read[23] << 8));

}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T)
{
	BMP280_S32_t var1, var2, T;

	var1 = ((((adc_T>>3) - ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) - ((BMP280_S32_t)dig_T1))) >> 12) *
	((BMP280_S32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}
// Returns pressure in Pa as unsigned 32 bit integer. Output value of “96386” equals 96386 Pa = 963.86 hPa
BMP280_U32_t bmp280_compensate_P_int32(BMP280_S32_t adc_P)
{
	BMP280_S32_t var1, var2;
	BMP280_U32_t p;
	var1 = (((BMP280_S32_t)t_fine)>>1) - (BMP280_S32_t)64000;
	var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((BMP280_S32_t)dig_P6);
	var2 = var2 + ((var1*((BMP280_S32_t)dig_P5))<<1);
	var2 = (var2>>2)+(((BMP280_S32_t)dig_P4)<<16);
	var1 = (((dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((BMP280_S32_t)dig_P2) * var1)>>1))>>18;
	var1 =((((32768+var1))*((BMP280_S32_t)dig_P1))>>15);
	if (var1 == 0){return 0;} // avoid exception caused by division by zero
	p = (((BMP280_U32_t)(((BMP280_S32_t)1048576)-adc_P)-(var2>>12)))*3125;
	if (p < 0x80000000){p = (p << 1) / ((BMP280_U32_t)var1);}
	else{p = (p / (BMP280_U32_t)var1) * 2;}
	var1 = (((BMP280_S32_t)dig_P9) * ((BMP280_S32_t)(((p>>3) * (p>>3))>>13)))>>12;
	var2 = (((BMP280_S32_t)(p>>2)) * ((BMP280_S32_t)dig_P8))>>13;
	p = (BMP280_U32_t)((BMP280_S32_t)p + ((var1 + var2 + dig_P7) >> 4));
	return p;
}

void sens_conf()
{
	/*
	7,6,5 oversampling temp || 4,3,2 oversampling press || 1,0 power mode
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

	uart_send_tekst("Wysylam konfiguracje...\r\n");
	uint8_t ctrl_meas = 0b10000111;
	uint8_t n=10;
	while (I2C_write(0x76, 0xF4, ctrl_meas) && n)
	{	
		uart_send_tekst("Blad wyslania konfiguracji, ponawianie...\r\n");
		_delay_ms(200);
		n--;
	}

	if (!n){uart_send_tekst("Przekroczono limit prob\r\n");}

}

int main(void)
{
	_delay_ms(20);

	I2C_init();
	uart_init();

	_delay_ms(20);

	uart_send_tekst("Identyfikator: ");
	uint8_t identyfikator[1];
	I2C_read(0x76, 0xD0, 1, &identyfikator[0]);

	uart_send_num(identyfikator[0]);
	if (identyfikator[0] != 0x58) {uart_send_tekst("\r\nBledny odczyt identyfikatora, mozliwe bledy\r\n");}
	uart_send_tekst("\r\n====Pomiary====================\r\n");

	kalibracja();
	sens_conf();


	while (1)
	{
		_delay_ms(1000);

		uint8_t read[6];

		uint8_t n = 10;
		while (I2C_read(0x76, 0xF7, 6, &read[0]) && n)
		{	
			uart_send_tekst("Blad odbierania pomiaru, ponawianie...\r\n");
			_delay_ms(200);
			n--;
		}

		if (!n){uart_send_tekst("Przekroczono limit prob\r\n");}

		
		uint32_t cisn_msb = read[0]; // MSB  7:0
		uint32_t cisn_lsb = read[1]; // LSB  7:0
		uint32_t cisn_xlsb = read[2]; // XLSB 3:0 (7,6,5,4)

		uint32_t temp_msb = read[3]; // MSB  7:0
		uint32_t temp_lsb = read[4]; // LSB  7:0
		uint32_t temp_xlsb = read[5]; // XLSB 3:0 (7,6,5,4)


		UT = (temp_msb << 12) | (temp_lsb << 4) | (temp_xlsb >> 4);
		
		UP = (cisn_msb << 12) |	(cisn_lsb << 4) | (cisn_xlsb >> 4);


		int32_t temperatura = bmp280_compensate_T_int32(UT);
		uint32_t cisnienie = bmp280_compensate_P_int32(UP);

		uart_send_tekst("Temperatura: ");
		uart_send_num((uint32_t) (temperatura/100));
		uart_send_tekst(".");
		uart_send_num((uint32_t) ((temperatura/10)%10));
		uart_send_num((uint32_t) (temperatura%10));
		uart_send_tekst(" C\r\n");
		
		uart_send_tekst("Cisnienie: ");
		uart_send_num((uint32_t) (cisnienie));
		uart_send_tekst(" Pa\r\n");
	}
}

