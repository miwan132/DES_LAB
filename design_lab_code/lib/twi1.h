#ifndef TWI_H
#define TWI_H

#include <stdint.h>

#define TWI1_STATUS_MASK 0xF8
#define TWI1_STATUS_START 0x08
#define TWI1_STATUS_REPEATED_START 0x10
#define TWI1_STATUS_MT_SLA_ACK 0x18
#define TWI1_STATUS_MT_DATA_ACK 0x28
#define TWI1_STATUS_MR_SLA_ACK 0x40
#define TWI1_STATUS_MR_DATA_ACK 0x50
#define TWI1_STATUS_MR_DATA_NACK 0x58

void twi1_init(void);
int8_t twi1_send(uint8_t value, uint8_t expected_status);
int8_t twi1_receive(uint8_t *value, uint8_t acknowledge);
int8_t twi1_start(void);
void twi1_stop(void);

#endif