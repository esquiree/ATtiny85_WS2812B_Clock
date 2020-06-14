/*
 * Attiny85_WS2812B.cpp
 *
 * Created: 3/14/2019 19:10:20
 * Author : Reed
 * This example is configured for a Attiny85 at 16MHz
 * Fuses burned to 0xF1, 0xD7, 0xFE. Phase Lock Loop Clock, 64ms startup / reset.
 * Fuses above need to be updated...
 */ 

#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"
#include "light_ws2812.c"


#define MAXPIX 86 //Amount of LEDs
#define HUELISTLENGTH 9 //length of uint8_t colorHueList[]

static uint8_t colorIndex = 0;
uint8_t colorHueList[] = {0, 21, 42, 85, 128, 171, 213, 234, 255}; //Specific Hue value used in colorList
cHSV colorList[HUELISTLENGTH];

cHSV leds_next[MAXPIX]; //HSV Color space. Will be converted to RGB before being output.
cRGB leds_out[MAXPIX]; //Create memory space for bytes that get written to WS2812B.
//cHSV last_leds[MAXPIX]; //This was created to fix a bug... Waste of 120 bytes...

uint8_t brightnessIndex = 0;
uint8_t brightnessList[] = {40, 30, 20, 10, 5, 2, 1};
uint8_t BRIGHTNESS_DEVISOR = 0; //inverse of brightness

uint8_t mode = 0;
uint8_t coolDown = 0;

uint8_t zoneMap[] = {6,7,8,9,9,9,9,9,9,9,9,8,7,6,6,7,7,7,6,5,4,3,2,1,1,0,0,0,0,1,1,2,3,4,5,6,7,7,7,6};
cHSV zoneColor[10];

uint8_t randSaturation, randHue, randBrightness, randPosition;
cHSV inceasingHue, tempColor, tempHSV;
cRGB tempRGB;

uint8_t k = 0; //Control selection
uint8_t bottom2top_ZonePattern[] = {0,1,2,3,4,5,6,7,8,9,9,9,8,7,6,5,4,3,2,1};
uint8_t m = 0; //used
uint8_t c = 0; //used in mode 3
uint8_t d = 0; //used in mode 3





void write_WS2812B_data(cHSV *HSVColorData){
	cHSV tempData;
	for(uint8_t i = 0; i < MAXPIX; i++)
	{
		tempData = HSVColorData[i]; 
		tempData.v /= BRIGHTNESS_DEVISOR;
		
		leds_out[i] = HSV2RGB(tempData);
	}
	ws2812_sendarray((uint8_t *)leds_out,MAXPIX*3);
}

void setup_Hue_Table(void){
	for(uint8_t i = 0; i < HUELISTLENGTH; i++)
	{
		colorList[i].h = colorHueList[i];
		colorList[i].s = 255;
		colorList[i].v = 255;
	}
	colorList[8].s = 0;
	for (uint8_t i = 0; i < 10; i++)
	{
		zoneColor[i].s = 255;
		zoneColor[i].v = 255;
	}
	
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

//Button Press Interrupt
ISR(PCINT0_vect)
{
	
	if (coolDown == 0)
	{
		coolDown = 50;
		if ((PINB&(1<<PB2)) == 0)
		{
			_delay_ms(250);
			if (!((PINB&(1<<PB2)) == 0))
			{
				mode++;
				if (mode > 4){ //Max mode options
					mode = 0;
				}
				
				EEPROM_write(0x06,mode);
			}
			else{
				if ((PINB&(1<<PB2)) == 0)
				{
					colorIndex++;
					if (colorIndex > HUELISTLENGTH)
					{
						colorIndex = 0;
					}
					EEPROM_write(0x05,colorIndex);
				}
				while ((PINB&(1<<PB2)) == 0)
				{
					_delay_us(100);
					tempHSV.h = colorHueList[colorIndex];
					tempHSV.s = 255;
					if (colorIndex == HUELISTLENGTH-1)
					{
						tempHSV.s = 0;
					}
					if (colorIndex == HUELISTLENGTH)
					{
						tempHSV.h = inceasingHue.h;
					}
					tempHSV.v = 255.0/BRIGHTNESS_DEVISOR;
					
					tempRGB = HSV2RGB(tempHSV);
					
					inceasingHue.h++;
					for (uint8_t i = 0; i<MAXPIX; i++)
					{
						leds_out[i] = tempRGB;
					}
					ws2812_sendarray((uint8_t *)leds_out,MAXPIX*3);
				}
				_delay_ms(50);
			}
		}
	}	
}

void fade2Black(uint8_t fadeAmount){
	for (uint8_t i = 0; i < MAXPIX; i++)
	{
		if ((leds_next[i].v - fadeAmount)> 0)
		{
			leds_next[i].v -= fadeAmount;
		}
		else
		{
			leds_next[i].v = 0;
		}
	}
}


void mode_0(void){
	tempColor = colorList[colorIndex];
	if (colorIndex == HUELISTLENGTH)
	{
		tempColor = inceasingHue;
	}
	
	for (uint8_t i = 0; i <MAXPIX; i++)
	{
		leds_next[i] = tempColor;
	}
	
	_delay_ms(1);
	write_WS2812B_data(leds_next);
}
void mode_1(void){
	for (uint8_t i = 0; i < MAXPIX; i++){
		tempColor = inceasingHue;
		tempColor.h = tempColor.h + (6.375 * i);
		leds_next[i] = tempColor;
	}
	
	_delay_ms(1);
	write_WS2812B_data(leds_next);
}
void mode_2(void){
	uint8_t hueChange = 16;
	for (uint8_t i = 0; i < MAXPIX; i++){
		tempColor = inceasingHue;
		if (zoneMap[i] == 0)
		{
			tempColor.h = tempColor.h + (hueChange * 0);
		}
		else if (zoneMap[i] == 1)
		{
			tempColor.h = tempColor.h + (hueChange * 1);
		}
		else if (zoneMap[i] == 2)
		{
			tempColor.h = tempColor.h + (hueChange * 2);
		}
		else if (zoneMap[i] == 3)
		{
			tempColor.h = tempColor.h + (hueChange * 3);
		}
		else if (zoneMap[i] == 4)
		{
			tempColor.h = tempColor.h + (hueChange * 4);
		}
		else if (zoneMap[i] == 5)
		{
			tempColor.h = tempColor.h + (hueChange * 5);
		}
		else if (zoneMap[i] == 6)
		{
			tempColor.h = tempColor.h + (hueChange * 6);
		}
		else if (zoneMap[i] == 7)
		{
			tempColor.h = tempColor.h + (hueChange * 7);
		}
		else if (zoneMap[i] == 8)
		{
			tempColor.h = tempColor.h + (hueChange * 8);
		}
		else if (zoneMap[i] == 9)
		{
			tempColor.h = tempColor.h + (hueChange * 9);
		}
		leds_next[i] = tempColor;
	}
	
	
	_delay_ms(1);
	write_WS2812B_data(leds_next);
}
void mode_1and2 (void){
	if (colorIndex%2 == 0)
	{
		mode_1();
	}
	else
	{
		mode_2();
	}
}
void mode_3(void){
	if (inceasingHue.h % 15 == 0)
	{
		c++;
		if(c>19){
			c = 0;
		}
		d = bottom2top_ZonePattern[c];
	}
	
	tempColor = colorList[colorIndex];
	if (colorIndex == HUELISTLENGTH)
	{
		tempColor = inceasingHue;
	}
	
	
	zoneColor[1].v = 0;
	zoneColor[2].v = 0;
	zoneColor[3].v = 0;
	zoneColor[4].v = 0;
	zoneColor[5].v = 0;
	zoneColor[6].v = 0;
	zoneColor[7].v = 0;
	zoneColor[8].v = 0;
	zoneColor[9].v = 0;
	
	if (d == 0)
	{
		zoneColor[0] = tempColor;
	}
	else if (d == 1)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
	}
	else if (d == 2)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
		zoneColor[2] = tempColor;
	}
	else if (d == 3)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
		zoneColor[2] = tempColor;
		zoneColor[3] = tempColor;
	}
	else if (d == 4)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
		zoneColor[2] = tempColor;
		zoneColor[3] = tempColor;
		zoneColor[4] = tempColor;
	}
	else if (d == 5)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
		zoneColor[2] = tempColor;
		zoneColor[3] = tempColor;
		zoneColor[4] = tempColor;
		zoneColor[5] = tempColor;
	}
	else if (d == 6)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
		zoneColor[2] = tempColor;
		zoneColor[3] = tempColor;
		zoneColor[4] = tempColor;
		zoneColor[5] = tempColor;
		zoneColor[6] = tempColor;
	}
	else if (d == 7)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
		zoneColor[2] = tempColor;
		zoneColor[3] = tempColor;
		zoneColor[4] = tempColor;
		zoneColor[5] = tempColor;
		zoneColor[6] = tempColor;
		zoneColor[7] = tempColor;
	}
	else if (d == 8)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
		zoneColor[2] = tempColor;
		zoneColor[3] = tempColor;
		zoneColor[4] = tempColor;
		zoneColor[5] = tempColor;
		zoneColor[6] = tempColor;
		zoneColor[7] = tempColor;
		zoneColor[8] = tempColor;
	}
	else if (d == 9)
	{
		zoneColor[0] = tempColor;
		zoneColor[1] = tempColor;
		zoneColor[2] = tempColor;
		zoneColor[3] = tempColor;
		zoneColor[4] = tempColor;
		zoneColor[5] = tempColor;
		zoneColor[6] = tempColor;
		zoneColor[7] = tempColor;
		zoneColor[8] = tempColor;
		zoneColor[9] = tempColor;
	}
	
	
	for (uint8_t i = 0; i < MAXPIX; i++){
		if (zoneMap[i] == 0)
		{
			leds_next[i] = zoneColor[0];
		}
		else if (zoneMap[i] == 1)
		{
			leds_next[i] = zoneColor[1];
		}
		else if (zoneMap[i] == 2)
		{
			leds_next[i] = zoneColor[2];
		}
		else if (zoneMap[i] == 3)
		{
			leds_next[i] = zoneColor[3];
		}
		else if (zoneMap[i] == 4)
		{
			leds_next[i] = zoneColor[4];
		}
		else if (zoneMap[i] == 5)
		{
			leds_next[i] = zoneColor[5];
		}
		else if (zoneMap[i] == 6)
		{
			leds_next[i] = zoneColor[6];
		}
		else if (zoneMap[i] == 7)
		{
			leds_next[i] = zoneColor[7];
		}
		else if (zoneMap[i] == 8)
		{
			leds_next[i] = zoneColor[8];
		}
		else if (zoneMap[i] == 9)
		{
			leds_next[i] = zoneColor[9];
		}
	}
	//asm("nop");
	_delay_ms(1);
	write_WS2812B_data(leds_next);
}

//Sinelon
void mode_4(){

	if (inceasingHue.h % 5 == 0)
	{
		m++;
		if (m> MAXPIX)
		{
			m = 0;
		}
	}
	
	tempColor = colorList[colorIndex];
	if (colorIndex == HUELISTLENGTH)
	{
		tempColor = inceasingHue;
	}
	
	fade2Black(2);
	leds_next[m] = tempColor;
	write_WS2812B_data(leds_next);
}

//Confetti Pattern
void mode_5(void){
	randPosition = ((uint8_t)rand())/51;
	randHue = ((uint8_t)rand())/6; //This makes the Hue random around a central color.
	uint8_t randSaturation = ((uint8_t)rand())/5 + 200; //This makes the whiteness/coloredness random	
	
	m++;
	if (m >= 28)
	{
		m = 0;
	}
	fade2Black(1); //Decrement all pixels
	if (m == 0)
	{
		leds_next[randPosition].h = randHue;
		leds_next[randPosition].s = randSaturation;
		leds_next[randPosition].v = 60;	
	}
	for (uint8_t i = 0; i < 5; i++)
	{
		leds_out[i] = HSV2RGB(leds_next[i]);
	}
}


/*
#################### Main ##################
*/
int main(void)
{
	
	GIMSK |= (1<<PCIE);
	PCMSK |= (1<<PB2);
	sei();
	PORTB ^= (1<<PB2); //Btn 
	
	brightnessIndex = EEPROM_read(0x04);
	if(brightnessIndex > 0x05){ //catch any errors in EEPROM
		brightnessIndex = 0x00;
	}
	
	mode = EEPROM_read(0x06); // Read mode selection from last power cycle
	if(mode > 0x08){ //catch any errors in EEPROM
		mode = 0x00;
	}
	colorIndex = EEPROM_read(0x05); // Read color index selection from last power cycle
	if(colorIndex > 0x0A){ //catch any errors in EEPROM
		colorIndex = 0x00;
	}
	
	if ((PINB&(1<<PB2)) == 0)
	{
		brightnessIndex++;
		if (brightnessIndex > 0x05)
		{
			brightnessIndex = 0;
		}
		EEPROM_write(0x04,brightnessIndex);
	}
	BRIGHTNESS_DEVISOR = brightnessList[brightnessIndex];
	
	DDRB|=_BV(ws2812_pin); //Set PB1 as Output.
	setup_Hue_Table();

	inceasingHue.h = 124;
	inceasingHue.s = 255;
	inceasingHue.v = 10;
	
	//uint8_t increasingTemp[25];
	//uint8_t direction[25];
	uint8_t i = 4;
	uint8_t k = 0;
	
// 	for (uint8_t v = 5; v<86; v++)
// 	{
// 		increasingTemp[v] = 128 + v*4;
// 		direction[v] = 1;
// 	}
	while(1){	
		
		mode_1();			
		
// 		for (uint8_t n = 5; n!=26; n++)
// 		{
// 			if (increasingTemp[n]<128)
// 			{
// 				direction[n] = 1;
// 			}
// 			if (increasingTemp[n]>224)
// 			{
// 				direction[n] = 0;
// 			}
// 			
// 			if (direction[n] == 1){
// 				increasingTemp[n] = increasingTemp[n] +1;
// 			}
// 			if (direction[n] == 0)
// 			{
// 				increasingTemp[n] = increasingTemp[n] -1;
// 			}
// 			
// 			inceasingHue.h = increasingTemp[n];
// 			leds_out[n]= HSV2RGB(inceasingHue);
// 		}
					
		ws2812_sendarray(((uint8_t *)leds_out),MAXPIX*3);
		_delay_ms(20);
	} //end of while(1)
	return 1;	//never reached
}//Exit main