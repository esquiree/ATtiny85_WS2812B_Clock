

#ifndef F_CPU
#define F_CPU 8000000UL	      // Sets up the default speed for delay.h
#endif
#include <util/delay.h>
#include <avr/io.h>
#include <stdint.h>
#define PIO_ADDR 0x40
#include "USI_I2C_Master.h"

void send_PIO_Byte(uint8_t addr, uint8_t data);
uint8_t read_PIO_Byte(uint8_t addr);
void set_IO_Dir(uint8_t pin, uint8_t dir);
void set_IO_Output_State(uint8_t pin, uint8_t ena);
void set_IO_PullUp(uint8_t pin, uint8_t ena);
void set_IO_Ena_INT(uint8_t pin, uint8_t ena);
uint16_t get_IO_INT(void);
void set_IO_INT_Clear(void);
void set_IO_INT_Config(void);