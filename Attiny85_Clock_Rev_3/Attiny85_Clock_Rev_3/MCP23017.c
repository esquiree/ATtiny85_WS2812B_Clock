#include "MCP23017.h"



void send_PIO_Byte(uint8_t addr, uint8_t data){
	i2c_start(PIO_ADDR);
	i2c_write(addr);
	i2c_write(data);
	i2c_stop();
}

uint8_t read_PIO_Byte(uint8_t addr){
	i2c_start(PIO_ADDR);
	i2c_write(addr);
	i2c_stop();
	i2c_start(PIO_ADDR | 0x01);
	uint8_t byte = i2c_read(1);
	i2c_stop();
	return byte;
}

void set_IO_Dir(uint8_t pin, uint8_t dir){
	//1 = input
	//0 = output
	if (pin>7)
	{
		//bank B
		pin = pin - 8;
		uint8_t temp = read_PIO_Byte(0x01);
		if ((temp & (1<<pin)) == (1<<pin))
		{
			//pin is set as input
			if (dir == 0)
			{
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x01, temp);
			}
		}else{
			//pin is set as output
			if (dir == 1){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x01, temp);
			}
		}
	}else{
		//Bank A
		uint8_t temp = read_PIO_Byte(0x00);
		if ((temp & (1<<pin)) == (1<<pin))
		{
			//pin is set as input
			if (dir == 0){
				//Change bit in temp and send
				temp ^= (1<<pin);	
				send_PIO_Byte(0x00, temp);	
			}
		}else{
			//pin is set as output
			if (dir == 1){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x00, temp);
			}
		}
	}		
}

void set_IO_Output_State(uint8_t pin, uint8_t ena){
	//1 = Enabled
	//0 = Disabled
	if (pin>7)
	{
		//bank B
		pin = pin - 8;
		uint8_t temp = read_PIO_Byte(0x13);
		if ((temp & (1<<pin)) == (1<<pin))
		{
			//pin is set as Enabled
			if (ena == 0)
			{
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x13, temp);
			}
			}else{
			//pin is set as Disabled
			if (ena == 1){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x13, temp);
			}
		}
		}else{
		//Bank A
		uint8_t temp = read_PIO_Byte(0x12);
		if ((temp & (1<<pin)) == (1<<pin))
		{
			//pin is set as Enabled
			if (ena == 0){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x12, temp);
			}
			}else{
			//pin is set as Disabled
			if (ena == 1){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x12, temp);
			}
		}
	}
}

void set_IO_PullUp(uint8_t pin, uint8_t ena){
	//1 = Enabled
	//0 = Disabled
	if (pin>7)
	{
		//bank B
		pin = pin - 8;
		uint8_t temp = read_PIO_Byte(0x0D);
		if ((temp & (1<<pin)) == (1<<pin))
		{
			//pin is set as Enabled
			if (ena == 0)
			{
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x0D, temp);
			}
			}else{
			//pin is set as Disabled
			if (ena == 1){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x0D, temp);
			}
		}
		}else{
		//Bank A
		uint8_t temp = read_PIO_Byte(0x0C);
		if ((temp & (1<<pin)) == (1<<pin))
		{
			//pin is set as Enabled
			if (ena == 0){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x0C, temp);
			}
			}else{
			//pin is set as Disabled
			if (ena == 1){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x0C, temp);
			}
		}
	}
}

void set_IO_Ena_INT(uint8_t pin, uint8_t ena){
	//1 = Enabled
	//0 = Disabled
	if (pin>7)
	{
		//bank B
		pin = pin - 8;
		uint8_t temp = read_PIO_Byte(0x05);
		if ((temp & (1<<pin)) == (1<<pin))
		{
			//pin is set as Enabled
			if (ena == 0)
			{
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x05, temp);
			}
			}else{
			//pin is set as Disabled
			if (ena == 1){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x05, temp);
			}
		}
		}else{
		//Bank A
		uint8_t temp = read_PIO_Byte(0x04);
		if ((temp & (1<<pin)) == (1<<pin))
		{
			//pin is set as Enabled
			if (ena == 0){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x04, temp);
			}
			}else{
			//pin is set as Disabled
			if (ena == 1){
				//Change bit in temp and send
				temp ^= (1<<pin);
				send_PIO_Byte(0x04, temp);
			}
		}
	}
}
uint16_t get_IO_INT(void){
	uint8_t lower = read_PIO_Byte(0x12);
	uint8_t upper = read_PIO_Byte(0x13);
	uint16_t temp;							//Create a signed int.
	uint16_t* ptrTemp = &temp;				//Create a pointer to the signed int. Pointers are needed to save a data type uint8_t to a int.
	*ptrTemp = ((upper<<8) | lower);	//Shift the upper byte 8 places to the left, bitwise OR with lower byte. Save to deferenced pointer.
	return temp;						//signed int temp now holds ((upper<<8) | lower).
}

void set_IO_INT_Clear(void){

}

void set_IO_INT_Config(void){
	send_PIO_Byte(0x0A, 0x42);
	//send_PIO_Byte(0x0B, 0x42);
}