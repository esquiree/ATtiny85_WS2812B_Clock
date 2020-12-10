/*
 * Clock_WS2812B_ATMEGA328_Rev1.cpp
 *
 * Created: 6/11/2020 20:06:08
 * Author : Reed
 */ 

#define ws2812_port B
#define ws2812_pin  1
#define BTN_INT PB3
#define BTN_SET_TIME 0
#define BTN_SET_COLOR 1
#define BTN_SET_BRIGHT 2
#define BTN_NEXT_DIGIT 3
#define BTN_INCREMENT 4
#define BTN_DECREMENT 5
#define LED_TIME 8
#define LED_COLOR 9
#define LED_BRIGHTNESS 10


#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "USI_I2C_Master.h"			//I2C Read/Write Functions
#include "USI_I2C_Master.c"	
#include "light_ws2812.h"
#include "light_ws2812.c"
#include "MCP23017.h"
#include "MCP23017.c"

#define RTC_ADDR 0xD0
#define MAXPIX 86 //Amount of LEDs
#define COLORPATTERNS_MAX 4
#define FADESPEED 0.5

//cHSV leds_next[MAXPIX]; //HSV Color space. Will be converted to RGB before being output.
cRGB leds_out[MAXPIX]; //Create memory space for bytes that get written to WS2812B.
cHSV inceasingHue, tempColor;
cRGB tempRGB, LED_Off;
uint8_t BRIGHTNESS_DEVISOR = 2;
uint8_t digit0, digit1, digit2, digit3, colon;
uint8_t colorMode = 0;


void update_color_fill(void);
void apply_pixel_mask(uint8_t digit0, uint8_t digit1, uint8_t digit2, uint8_t digit3, uint8_t colon);
void digit_mask(uint8_t digit, uint8_t offset);
void get_time(void);
void set_time(uint8_t digit0, uint8_t digit1, uint8_t digit2, uint8_t digit3);
void _24hr_to_12hr(void);
void changeColor(void);
void changeTime(void);
void changeBrightness(void);
unsigned char EEPROM_read(uint8_t uiAddress);
void EEPROM_write(uint8_t uiAddress, uint8_t ucData);

int main(void)
{
    /* Replace with your application code */
	i2c_init();
	
	inceasingHue.h = 124;
	inceasingHue.s = 255;
	inceasingHue.v = EEPROM_read(0x01);

	colorMode = EEPROM_read(0x03);
	
	digit0 = 5;
	digit1 = 6;
	digit2 = 7;
	digit3 = 8;
	
	//Button Config
	set_IO_Dir(BTN_SET_TIME, 1); //Need to write this register before doing pullup. very stupid
	set_IO_Dir(LED_TIME, 0);
	set_IO_Dir(LED_BRIGHTNESS, 0);
	set_IO_Dir(LED_COLOR, 0);

	set_IO_PullUp(BTN_SET_TIME, 1);
	set_IO_PullUp(BTN_SET_COLOR, 1);
	set_IO_PullUp(BTN_SET_BRIGHT, 1);
	set_IO_PullUp(BTN_NEXT_DIGIT, 1);
	set_IO_PullUp(BTN_INCREMENT, 1);
	set_IO_PullUp(BTN_DECREMENT, 1);
	set_IO_Ena_INT(BTN_SET_TIME, 1);
	set_IO_Ena_INT(BTN_SET_COLOR, 1);
	set_IO_Ena_INT(BTN_SET_BRIGHT, 1);
	set_IO_Ena_INT(BTN_NEXT_DIGIT, 1);
	set_IO_Ena_INT(BTN_INCREMENT, 1);
	set_IO_Ena_INT(BTN_DECREMENT, 1);
	set_IO_INT_Config();
	_delay_ms(1);
	get_IO_INT(); //Clear Interrupt	
	
	GIMSK |= (1<<PCIE);
	PCMSK |= (1<<BTN_INT);
	sei();
	
	
	
    while (1) 
    {		
		update_color_fill();
		_delay_ms(100);
		get_time();	
		_24hr_to_12hr();
		apply_pixel_mask(digit0, digit1, digit2, digit3, colon);
		ws2812_sendarray((uint8_t *)leds_out,MAXPIX*3);
    }
}

void changeTime(){
	cli();
	uint8_t active_digit = 0;
	uint16_t test = get_IO_INT();
	while (((test&(1<<BTN_SET_TIME)) == (1<<BTN_SET_TIME)))
	{
		test = get_IO_INT();
		
		tempColor.h = 0;
		tempColor.s = 255;
		tempColor.v = inceasingHue.v;
		for (uint8_t i = 0; i < MAXPIX; i++){		
			leds_out[i] = HSV2RGB(tempColor);
		}
		tempColor.s = 0;
		if(active_digit == 0){
			for (uint8_t i = 0; i < 21; i++){
				leds_out[i] = HSV2RGB(tempColor);
			}
		}
		if(active_digit == 1){
			for (uint8_t i = 21; i < 44; i++){
				leds_out[i] = HSV2RGB(tempColor);
			}
		}
		if(active_digit == 2){
			for (uint8_t i = 44; i < 65; i++){
				leds_out[i] = HSV2RGB(tempColor);
			}
		}
		if(active_digit == 3){
			for (uint8_t i = 65; i < MAXPIX; i++){
				leds_out[i] = HSV2RGB(tempColor);
			}
		}
		
		//_24hr_to_12hr();
		apply_pixel_mask(digit0, digit1, digit2, digit3, colon);
		ws2812_sendarray((uint8_t *)leds_out,MAXPIX*3);
		_delay_ms(100);
		//btn reads
		if (!((test&(1<<BTN_NEXT_DIGIT)) == (1<<BTN_NEXT_DIGIT)))
		{
			//while (!(test & (1<<BTN_SET_BRIGHT)){}; //do noting until release
			active_digit++;
			if (active_digit>3)
			{
				active_digit = 0;
			}
		}
		if (!((test&(1<<BTN_INCREMENT)) == (1<<BTN_INCREMENT)))
		{
			/*while (((PINB & ((1<<BTN1) | (1<<BTN2))) == (1<<BTN2))){}; //do noting until release*/
			if (active_digit == 0)
			{
				digit0++;
				if (digit0>2)
				{
					digit0 = 0;
				}
			}
			if (active_digit == 1)
			{
				digit1++;
				if (digit1>9)
				{
					digit1 = 0;
				}
			}
			if (active_digit == 2)
			{
				digit2++;
				if (digit2>6)
				{
					digit2 = 0;
				}
			}
			if (active_digit == 3)
			{
				digit3++;
				if (digit3>9)
				{
					digit3 = 0;
				}
			}
		}
		
		if (!((test&(1<<BTN_DECREMENT)) == (1<<BTN_DECREMENT)))
		{
			if (active_digit == 0)
			{
				digit0--;
				if (digit0==255)
				{
					digit0 = 2;
				}
			}
			if (active_digit == 1)
			{
				digit1--;
				if (digit1==255)
				{
					digit1 = 9;
				}
			}
			if (active_digit == 2)
			{
				digit2--;
				if (digit2==255)
				{
					digit2 = 6;
				}
			}
			if (active_digit == 3)
			{
				digit3--;
				if (digit3==255)
				{
					digit3 = 9;
				}
			}
		}
	}
	set_time(digit0, digit1, digit2, digit3);
	sei();
}

void changeBrightness(){
	cli();
	uint16_t test = get_IO_INT();
	while (((test&(1<<BTN_SET_BRIGHT)) == (1<<BTN_SET_BRIGHT))) 
	{
			test = get_IO_INT();
			update_color_fill();
			_delay_ms(100);
			get_time();
			_24hr_to_12hr();
			apply_pixel_mask(digit0, digit1, digit2, digit3, colon);
			tempColor.h = 0;
			tempColor.s = 255;
			tempColor.v = inceasingHue.v;
			leds_out[42] = HSV2RGB(tempColor);
			leds_out[43] = HSV2RGB(tempColor);
			ws2812_sendarray((uint8_t *)leds_out,MAXPIX*3);
			
			if (!((test&(1<<BTN_INCREMENT)) == (1<<BTN_INCREMENT)))
			{
				inceasingHue.v +=5;
			}
			if (!((test&(1<<BTN_DECREMENT)) == (1<<BTN_DECREMENT)))
			{
				inceasingHue.v -=5;
			}	
	}
	EEPROM_write(0x01, inceasingHue.v);
	sei();
}

void changeColor(){
	cli();
	uint16_t test = get_IO_INT();
	while (((test&(1<<BTN_SET_COLOR)) == (1<<BTN_SET_COLOR)))
	{
		test = get_IO_INT();
		update_color_fill();
		_delay_ms(100);
		get_time();
		_24hr_to_12hr();
		apply_pixel_mask(digit0, digit1, digit2, digit3, colon);
		ws2812_sendarray((uint8_t *)leds_out,MAXPIX*3);
		
		if (!((test&(1<<BTN_INCREMENT)) == (1<<BTN_INCREMENT)))
		{
			colorMode++;
			
			if (colorMode>COLORPATTERNS_MAX)
			{
				colorMode = 0;
			}
		}
		if (!((test&(1<<BTN_DECREMENT)) == (1<<BTN_DECREMENT)))
		{
			colorMode--;
			
			if (colorMode>COLORPATTERNS_MAX+1)
			{
				colorMode = COLORPATTERNS_MAX;
			}
		}
	}
	EEPROM_write(0x03, colorMode);
	sei();
}


ISR(PCINT0_vect)
{
	//set_IO_Output_State(LED_BRIGHTNESS,1);
	uint16_t test = get_IO_INT();
 	_delay_ms(10);
 	if (!((test&(1<<BTN_SET_TIME)) == (1<<BTN_SET_TIME)))
 	{
 		set_IO_Output_State(LED_TIME, 1);
		 _delay_ms(100);
		changeTime();
 	}
 	if (!((test&(1<<BTN_SET_COLOR)) == (1<<BTN_SET_COLOR)))
 	{
 		set_IO_Output_State(LED_COLOR, 1);
		 _delay_ms(100);
		changeColor();
 	}
 	if (!((test&(1<<BTN_SET_BRIGHT)) == (1<<BTN_SET_BRIGHT)) )
 	{
 		set_IO_Output_State(LED_BRIGHTNESS, 1);
		 _delay_ms(100);
		changeBrightness();	
 	}
	set_IO_Output_State(LED_TIME, 0);
	set_IO_Output_State(LED_COLOR, 0);
	set_IO_Output_State(LED_BRIGHTNESS, 0);
}

void EEPROM_write(uint8_t uiAddress, uint8_t ucData) //Stolen from https://stackoverflow.com/questions/4412111/avr-eeprom-read-write
{
	while(EECR & (1<<EEPE));    /* Wait for completion of previous write */
	EEARH = 0x00;
	EEARL = uiAddress;
	EEDR = ucData;
	cli();
	EECR |= (1<<EEMPE);     /* Write logical one to EEMPE */
	EECR |= (1<<EEPE);      /* Start eeprom write by setting EEPE */
	sei();
}

unsigned char EEPROM_read(uint8_t uiAddress) //Stolen from https://stackoverflow.com/questions/4412111/avr-eeprom-read-write
{
	while(EECR & (1<<EEPE)); /* Wait for completion of previous write */

	EEARH = (uiAddress>>8);  /* Set up address register */
	EEARL = uiAddress;

	EECR |= (1<<EERE);       /* Start eeprom read by writing EERE */

	return EEDR;             /* Return data from Data Register */
}

void _24hr_to_12hr(void){
	
	//Convert 24HR to 12HR
	if ((digit0 == 1) && (digit1 > 2)) //Example: 13:24 becomes 1:24
	{
		digit0 = 10; //10 is a blank character
		digit1 = digit1 - 2;
	}
	if (digit0 == 2) //Example: 21:24 becomes 09:24
	{
		
		if(digit1 >= 2) //Example: 23:24 becomes 11:24
		{
			digit0 = 1;
			}else{			//Example: 21:24 becomes 09:24
			digit0 = 0;
		}
		
		digit1 = digit1 + 8;
		if (digit1>9)
		{
			digit1 = digit1 - 10;
		}
	}
	if ((digit0 == 0) && (digit1 == 0)) //Example: 00:24 becomes 12:24
	{
		digit0 = 1;
		digit1 = 2;
	}
	
	if (digit0 == 0) //Example: 09:24 becomes 9:24
	{
		digit0 = 10; //10 is a blank character
	}
}
	
void set_time(uint8_t digit0, uint8_t digit1, uint8_t digit2, uint8_t digit3){
	i2c_start(RTC_ADDR);
	i2c_write(0x00);
	i2c_write(0x00);  //0 seconds
	i2c_stop();
	
	i2c_start(RTC_ADDR);
	i2c_write(0x01);
	i2c_write((digit2<<4) | (digit3)); //Minute
	i2c_stop();
	
	i2c_start(RTC_ADDR);
	i2c_write(0x02);
	i2c_write((digit0<<4) | (digit1)); // Hour
	i2c_stop();
	
}
void get_time(void){
	i2c_start(RTC_ADDR);
	i2c_write(0x00);
	i2c_stop();
	i2c_start(RTC_ADDR | 0x01);
	uint8_t byte = i2c_read(1);
	i2c_stop();
	
	colon = (byte & 0x01);
	
	i2c_start(RTC_ADDR);
	i2c_write(0x01);
	i2c_stop();
	i2c_start(RTC_ADDR | 0x01);
	byte = i2c_read(1);
	i2c_stop();
	
	digit3 = (byte & 0b00001111);
	digit2 = ((byte & 0b01110000)>>4);
	
	i2c_start(RTC_ADDR);
	i2c_write(0x02);
	i2c_stop();
	i2c_start(RTC_ADDR | 0x01);
	byte = i2c_read(1);
	i2c_stop();
	
	digit1 = (byte & 0b00001111);
	digit0 = ((byte & 0b01110000)>>4);
}

void apply_pixel_mask(uint8_t digit0, uint8_t digit1, uint8_t digit2, uint8_t digit3, uint8_t colon){
	digit_mask(digit0, 0);
	digit_mask(digit1, 21);
	digit_mask(digit2, 44);
	digit_mask(digit3, 65);
	if(colon != 0x00){
		leds_out[42] = LED_Off;
		leds_out[43] = LED_Off;
	}
}	

void digit_mask(uint8_t digit, uint8_t offset){
	
	if (digit == 0)
	{
		leds_out[offset+18] = LED_Off;
		leds_out[offset+19] = LED_Off;
		leds_out[offset+20] = LED_Off;
	}
	if (digit == 1)
	{
		leds_out[offset+0] = LED_Off;
		leds_out[offset+1] = LED_Off;
		leds_out[offset+2] = LED_Off;
		
		leds_out[offset+3] = LED_Off;
		leds_out[offset+4] = LED_Off;
		leds_out[offset+5] = LED_Off;
		
		leds_out[offset+6] = LED_Off;
		leds_out[offset+7] = LED_Off;
		leds_out[offset+8] = LED_Off;
		
		leds_out[offset+15] = LED_Off;
		leds_out[offset+16] = LED_Off;
		leds_out[offset+17] = LED_Off;
		
		leds_out[offset+18] = LED_Off;
		leds_out[offset+19] = LED_Off;
		leds_out[offset+20] = LED_Off;
	}
	if (digit == 2)
	{
		//leds_out[offset+0] = LED_Off;
		//leds_out[offset+1] = LED_Off;
		//leds_out[offset+2] = LED_Off;
		
 		leds_out[offset+3] = LED_Off;
 		leds_out[offset+4] = LED_Off;
 		leds_out[offset+5] = LED_Off;
		
		//leds_out[offset+6] = LED_Off;
		//leds_out[offset+7] = LED_Off;
		//leds_out[offset+8] = LED_Off;
		//
		//leds_out[offset+9] = LED_Off;
		//leds_out[offset+10] = LED_Off;
		//leds_out[offset+11] = LED_Off;
		
 		leds_out[offset+12] = LED_Off;
 		leds_out[offset+13] = LED_Off;
 		leds_out[offset+14] = LED_Off;
		
		//leds_out[offset+15] = LED_Off;
		//leds_out[offset+16] = LED_Off;
		//leds_out[offset+17] = LED_Off;
		//
		//leds_out[offset+18] = LED_Off;
		//leds_out[offset+19] = LED_Off;
		//leds_out[offset+20] = LED_Off;
	}
	if (digit == 3)
	{
 		leds_out[offset+0] = LED_Off;
 		leds_out[offset+1] = LED_Off;
 		leds_out[offset+2] = LED_Off;
 		
 		leds_out[offset+3] = LED_Off;
 		leds_out[offset+4] = LED_Off;
 		leds_out[offset+5] = LED_Off;
		
		//leds_out[offset+6] = LED_Off;
		//leds_out[offset+7] = LED_Off;
		//leds_out[offset+8] = LED_Off;
		//
		//leds_out[offset+9] = LED_Off;
		//leds_out[offset+10] = LED_Off;
		//leds_out[offset+11] = LED_Off;
		//
		//leds_out[offset+12] = LED_Off;
		//leds_out[offset+13] = LED_Off;
		//leds_out[offset+14] = LED_Off;
		//
		//leds_out[offset+15] = LED_Off;
		//leds_out[offset+16] = LED_Off;
		//leds_out[offset+17] = LED_Off;
		//
		//leds_out[offset+18] = LED_Off;
		//leds_out[offset+19] = LED_Off;
		//leds_out[offset+20] = LED_Off;
	}
	if (digit == 4)
	{
 		leds_out[offset+0] = LED_Off;
 		leds_out[offset+1] = LED_Off;
 		leds_out[offset+2] = LED_Off;
 		
		//leds_out[offset+3] = LED_Off;
		//leds_out[offset+4] = LED_Off;
		//leds_out[offset+5] = LED_Off;
		
 		leds_out[offset+6] = LED_Off;
 		leds_out[offset+7] = LED_Off;
 		leds_out[offset+8] = LED_Off;
 		
		//leds_out[offset+9] = LED_Off;
		//leds_out[offset+10] = LED_Off;
		//leds_out[offset+11] = LED_Off;
		//
		//leds_out[offset+12] = LED_Off;
		//leds_out[offset+13] = LED_Off;
		//leds_out[offset+14] = LED_Off;
		
 		leds_out[offset+15] = LED_Off;
 		leds_out[offset+16] = LED_Off;
 		leds_out[offset+17] = LED_Off;
		
		//leds_out[offset+18] = LED_Off;
		//leds_out[offset+19] = LED_Off;
		//leds_out[offset+20] = LED_Off;
	}
	if (digit == 5)
	{
 		leds_out[offset+0] = LED_Off;
 		leds_out[offset+1] = LED_Off;
 		leds_out[offset+2] = LED_Off;
		
		//leds_out[offset+3] = LED_Off;
		//leds_out[offset+4] = LED_Off;
		//leds_out[offset+5] = LED_Off;
		//
		//leds_out[offset+6] = LED_Off;
		//leds_out[offset+7] = LED_Off;
		//leds_out[offset+8] = LED_Off;
		
 		leds_out[offset+9] = LED_Off;
 		leds_out[offset+10] = LED_Off;
 		leds_out[offset+11] = LED_Off;
		
		//leds_out[offset+12] = LED_Off;
		//leds_out[offset+13] = LED_Off;
		//leds_out[offset+14] = LED_Off;
		//
		//leds_out[offset+15] = LED_Off;
		//leds_out[offset+16] = LED_Off;
		//leds_out[offset+17] = LED_Off;
		//
		//leds_out[offset+18] = LED_Off;
		//leds_out[offset+19] = LED_Off;
		//leds_out[offset+20] = LED_Off;
	}
	if (digit == 6)
	{
		//leds_out[offset+0] = LED_Off;
		//leds_out[offset+1] = LED_Off;
		//leds_out[offset+2] = LED_Off;
		//
		//leds_out[offset+3] = LED_Off;
		//leds_out[offset+4] = LED_Off;
		//leds_out[offset+5] = LED_Off;
		//
		//leds_out[offset+6] = LED_Off;
		//leds_out[offset+7] = LED_Off;
		//leds_out[offset+8] = LED_Off;
		
 		leds_out[offset+9] = LED_Off;
 		leds_out[offset+10] = LED_Off;
 		leds_out[offset+11] = LED_Off;
		
		//leds_out[offset+12] = LED_Off;
		//leds_out[offset+13] = LED_Off;
		//leds_out[offset+14] = LED_Off;
		//
		//leds_out[offset+15] = LED_Off;
		//leds_out[offset+16] = LED_Off;
		//leds_out[offset+17] = LED_Off;
		//
		//leds_out[offset+18] = LED_Off;
		//leds_out[offset+19] = LED_Off;
		//leds_out[offset+20] = LED_Off;
	}
	if (digit == 7)
	{
 		leds_out[offset+0] = LED_Off;
 		leds_out[offset+1] = LED_Off;
 		leds_out[offset+2] = LED_Off;
 		
 		leds_out[offset+3] = LED_Off;
 		leds_out[offset+4] = LED_Off;
 		leds_out[offset+5] = LED_Off;
		
		//leds_out[offset+6] = LED_Off;
		//leds_out[offset+7] = LED_Off;
		//leds_out[offset+8] = LED_Off;
		//
		//leds_out[offset+9] = LED_Off;
		//leds_out[offset+10] = LED_Off;
		//leds_out[offset+11] = LED_Off;
		//
		//leds_out[offset+12] = LED_Off;
		//leds_out[offset+13] = LED_Off;
		//leds_out[offset+14] = LED_Off;
		
 		leds_out[offset+15] = LED_Off;
 		leds_out[offset+16] = LED_Off;
 		leds_out[offset+17] = LED_Off;
 		
 		leds_out[offset+18] = LED_Off;
 		leds_out[offset+19] = LED_Off;
 		leds_out[offset+20] = LED_Off;
	}
	if (digit == 8)
	{
		//leds_out[offset+0] = LED_Off;
		//leds_out[offset+1] = LED_Off;
		//leds_out[offset+2] = LED_Off;
		//
		//leds_out[offset+3] = LED_Off;
		//leds_out[offset+4] = LED_Off;
		//leds_out[offset+5] = LED_Off;
		//
		//leds_out[offset+6] = LED_Off;
		//leds_out[offset+7] = LED_Off;
		//leds_out[offset+8] = LED_Off;
		//
		//leds_out[offset+9] = LED_Off;
		//leds_out[offset+10] = LED_Off;
		//leds_out[offset+11] = LED_Off;
		//
		//leds_out[offset+12] = LED_Off;
		//leds_out[offset+13] = LED_Off;
		//leds_out[offset+14] = LED_Off;
		//
		//leds_out[offset+15] = LED_Off;
		//leds_out[offset+16] = LED_Off;
		//leds_out[offset+17] = LED_Off;
		//
		//leds_out[offset+18] = LED_Off;
		//leds_out[offset+19] = LED_Off;
		//leds_out[offset+20] = LED_Off;
	}
	if (digit == 9)
	{
 		leds_out[offset+0] = LED_Off;
 		leds_out[offset+1] = LED_Off;
 		leds_out[offset+2] = LED_Off;
		
		//leds_out[offset+3] = LED_Off;
		//leds_out[offset+4] = LED_Off;
		//leds_out[offset+5] = LED_Off;
		//
		//leds_out[offset+6] = LED_Off;
		//leds_out[offset+7] = LED_Off;
		//leds_out[offset+8] = LED_Off;
		//
		//leds_out[offset+9] = LED_Off;
		//leds_out[offset+10] = LED_Off;
		//leds_out[offset+11] = LED_Off;
		//
		//leds_out[offset+12] = LED_Off;
		//leds_out[offset+13] = LED_Off;
		//leds_out[offset+14] = LED_Off;
		
 		leds_out[offset+15] = LED_Off;
 		leds_out[offset+16] = LED_Off;
 		leds_out[offset+17] = LED_Off;
		
		//leds_out[offset+18] = LED_Off;
		//leds_out[offset+19] = LED_Off;
		//leds_out[offset+20] = LED_Off;
	}
	if (digit == 10)
	{
		leds_out[offset+0] = LED_Off;
		leds_out[offset+1] = LED_Off;
		leds_out[offset+2] = LED_Off;
		
		leds_out[offset+3] = LED_Off;
		leds_out[offset+4] = LED_Off;
		leds_out[offset+5] = LED_Off;
		
		leds_out[offset+6] = LED_Off;
		leds_out[offset+7] = LED_Off;
		leds_out[offset+8] = LED_Off;
		
		leds_out[offset+9] = LED_Off;
		leds_out[offset+10] = LED_Off;
		leds_out[offset+11] = LED_Off;
		
		leds_out[offset+12] = LED_Off;
		leds_out[offset+13] = LED_Off;
		leds_out[offset+14] = LED_Off;
		
		leds_out[offset+15] = LED_Off;
		leds_out[offset+16] = LED_Off;
		leds_out[offset+17] = LED_Off;
		
		leds_out[offset+18] = LED_Off;
		leds_out[offset+19] = LED_Off;
		leds_out[offset+20] = LED_Off;
	}
}


void update_color_fill(void){
	if (colorMode == 0) // Full Rainbow
	{
		for (uint8_t i = 0; i < MAXPIX; i++){
			tempColor = inceasingHue;
			tempColor.h = tempColor.h + (i * 0.5);
			if (tempColor.h > 255)
			{
				tempColor.h = tempColor.h - 255;
			}
			leds_out[i] = HSV2RGB(tempColor);
		}
		inceasingHue.h = inceasingHue.h + FADESPEED;
		if (inceasingHue.h > 255)
		{
			inceasingHue.h = inceasingHue.h - 255;
		}
	}else if(colorMode == 1){ //Just White
		tempColor.s = 0;
		tempColor.v = inceasingHue.v;
		for (uint8_t i = 0; i < MAXPIX; i++){		
			leds_out[i] = HSV2RGB(tempColor);
		}
	}else if(colorMode == 2){ //Just Red
		tempColor.s = 255;
		tempColor.v = inceasingHue.v;
		tempColor.h = 0;
		for (uint8_t i = 0; i < MAXPIX; i++){
			leds_out[i] = HSV2RGB(tempColor);
		}
	}else if(colorMode == 3){ //Just Blue
		tempColor.s = 255;
		tempColor.v = inceasingHue.v;
		tempColor.h = 171;
		for (uint8_t i = 0; i < MAXPIX; i++){
			leds_out[i] = HSV2RGB(tempColor);
		}
	}else if(colorMode == 4){ //Just Green
		tempColor.s = 255;
		tempColor.v = inceasingHue.v;
		tempColor.h = 85;
		for (uint8_t i = 0; i < MAXPIX; i++){
			leds_out[i] = HSV2RGB(tempColor);
		}
	}
	
}