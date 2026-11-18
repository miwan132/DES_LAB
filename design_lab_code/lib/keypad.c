#include "keypad.h"


static const uint8_t keypad_map[4][4] = {
//          PC5 PC4 PC3 PC2
/*PC1*/    {1, 2, 3, 4},  // S1 S2 S3 S4
/*PC0*/    {5, 6, 7, 8},  // S5 S6 S7 S8
/*PE3*/    {9, 10, 11, 12},  // S9 S10 S11 S12
/*PE2*/	   {13, 14, 15, 16}   // S13 S14 S15 S16
};

static void keypad_select_row(uint8_t row)
{
	PORTC |= 0b00000011;
	PORTE |= 0b00001100;

	switch (row)
	{
		case 0: PORTC &= ~(1 << PORTC1); break;
		case 1: PORTC &= ~(1 << PORTC0); break;
		case 2: PORTE &= ~(1 << PORTE3); break;
		case 3: PORTE &= ~(1 << PORTE2); break;
		default: break;
	}
}

uint8_t keypad_position(void)
{
	for (uint8_t row = 0; row < 4; row++)
	{
		keypad_select_row((uint8_t)(row));

		for (uint8_t col = 0; col < 4; col++)
		{
			if (!(PINC & (1 << (PORTC5-col)))) return keypad_map[row][3-col];
		}
	}
	return 0;
}