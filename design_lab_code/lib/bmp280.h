#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

#define BMP280_OK 0

int8_t bmp280_port_init(void);
int8_t bmp280_port_read(int32_t *temperature_centi_c, uint32_t *pressure_pa);

#endif