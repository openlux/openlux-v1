/*
 * openlux.c
 *
 * Created: 26/06/2021 15:34:56
 * Author : jpe
 */ 

#define F_CPU 20000000L

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "main.h"

/*int STATE0 = 0b01100011;
int STATE1 = 0b01111001;
int STATE2 = 0b01110000;
int STATE3 = 0b10110101;
int STATE4 = 0b10110111;
int STATE5 = 0b10110010;*/

#define CLK_V1      0b00000001
#define CLK_V2      0b00000010
#define CLK_V2H     0b00000100
#define CLK_FD      0b00001000
#define CLK_SHUTTER 0b00010000

#define ADC_SCLK  0b00000010
#define ADC_SLOAD 0b00100000
#define ADC_DATA  0b00100000

#define IDLE 0b01110010
#define LDLE 0b01110010

#define REG_CFG   0
#define REG_MUX   1
#define REG_PGA_R 2
#define REG_PGA_G 3
#define REG_PGA_B 4
#define REG_OFF_R 5
#define REG_OFF_G 6
#define REG_OFF_B 7

#define PIXEL PORTD = STATE0; \
              PORTD = STATE1; \
			  PORTD = STATE2; \
			  PORTD = STATE3; \
			  PORTD = STATE3; \
			  PORTD = STATE4; \
			  PORTD = STATE5; \
			  PORTD = STATE6; \
			  PORTD = STATE7; \
			  PORTD = STATE7;
			  
#define PIXEL4 PIXEL \
               PIXEL \
			   PIXEL \
			   PIXEL
			   
#define PIXEL16 PIXEL4 \
                PIXEL4 \
				PIXEL4 \
				PIXEL4
				
#define PIXEL64 PIXEL16 \
                PIXEL16 \
				PIXEL16 \
				PIXEL16
				
#define PIXEL256 PIXEL64 \
                 PIXEL64 \
				 PIXEL64 \
				 PIXEL64
				 
/*#define PIXEL2048 PIXEL256 \
                  PIXEL256 \
				  PIXEL256 \
				  PIXEL256 \
				  PIXEL256 \
				  PIXEL256 \
				  PIXEL256 \
				  PIXEL256*/

int8_t toread = 0;

int16_t seconds = 0;
int16_t millis = 0;

int main() {
	int temp = 0;

	cli();
	
	SPCR = 0b01000000;
	SPSR |= 1;
	
	DDRD |= 0b11111111;
	PORTD = LDLE;//0b01100011;//STATE0;

	DDRB |= 0b00010010;
	//PORTB |= 0b0;
	PORTB &= ~ADC_SCLK;

	DDRC |= 0b00111111;
	PORTC &= ~(CLK_V2 | CLK_V2H | CLK_SHUTTER | CLK_FD);
	PORTC |= CLK_V1;// | CLK_FD;

	//_delay_ms(1);
	
	_delay_ms(10);
	
	// 4v range, vref en, single ch, cds mode, 4v clamp bias, enabled, 0, 16 bits
	//adc_write(REG_CFG, 0b011011000);
	adc_write(REG_CFG, 0b011011100);
	_delay_us(10);
	
	// en red ch
	adc_write(REG_MUX, 0b011000000);
	_delay_us(10);

	//adc_write(REG_PGA_R, 59);
	adc_write(REG_PGA_R, 59);
	_delay_us(10);

	adc_write(REG_OFF_R, 10);
	_delay_us(10);
	
	uint8_t ptmp = PORTC;//0;
		for (uint16_t row = 0; row < 2720 * 4; row++) {
			ptmp |= CLK_V2;
			ptmp &= ~CLK_V1;
			PORTC = ptmp;
			
			_delay_us(15);
			
			ptmp |= CLK_V1;
			ptmp &= ~CLK_V2;
			PORTC = ptmp;
			
			//_delay_us(235);
			_delay_us(15);
		}
	
	_delay_ms(1);
	
	//uint8_t ptmp = PORTC;//0;

    while (1) 
    {
		cli();
		
		//while (SPSR & 0b10000000 == 0);
		while (1 == 1) {
			while(!(SPSR & (1<<SPIF))) {
				ptmp |= CLK_V2;
				ptmp &= ~CLK_V1;
				PORTC = ptmp;
					
				_delay_us(15);
					
				ptmp |= CLK_V1;
				ptmp &= ~CLK_V2;
				PORTC = ptmp;
					
				//_delay_us(985);
				_delay_us(2000);
				//_delay_us(5000); // TODO
			}
		
			temp = SPDR;
			
			if (toread > 0) {
				if (toread == 4) {
					seconds = temp;
					seconds <<= 8;
					toread--;
				} else if (toread == 3) {
					seconds |= temp;
					toread--;
				} else if (toread == 2) {
					millis = temp;
					millis <<= 8;
					toread--;
				} else if (toread == 1) {
					millis |= temp;
					toread--;
					break;
				}
			} else if (temp == 0xaa) {
				toread = 4;
			}
			
			temp = SPSR;
		}
		
		PORTC &= ~CLK_V1;
		_delay_us(15);
		
		PORTC |= CLK_SHUTTER;
		_delay_us(5);
		PORTC &= ~CLK_SHUTTER;
		
		for (uint16_t k = 0; k < seconds; k++) {
			_delay_ms(1000);
		}
		
		if (millis > 999) millis = 999;
		
		for (uint16_t k = 0; k < millis; k++) {
			_delay_ms(1);
		}
		/*for (uint16_t k = 0; k < 10; k++) {
			_delay_ms(1000);
		}*/
		
		readout();
		_delay_ms(1);
		/*PORTC |= 0b00100000;
		_delay_ms(1);
		PORTC &= ~0b00100000;
		_delay_ms(1);*/
    }
}

void adc_write(uint8_t addr, uint16_t val) {
	DDRC |= 0b00100000;
	PORTB &= ~ADC_SCLK;
	PORTD &= ~ADC_SLOAD;
	
	PORTC &= ~ADC_DATA; // r/w bit 0 for writes
	PORTB |= ADC_SCLK;
	PORTB &= ~ADC_SCLK;
	
	for (uint8_t i = 0; i < 3; i++) {
		PORTC = (PORTC & (~ADC_DATA)) | (((addr >> (2 - i)) & 1) << 5);
		PORTB |= ADC_SCLK;
		PORTB &= ~ADC_SCLK;
	}
	/*PORTC = (PORTC & (~ADC_DATA)) | (((addr >> 2) & 1) << 5)
	PORTB |= ADC_SCLK;
	PORTB &= ~ADC_SCLK;
	PORTC = (PORTC & (~ADC_DATA)) | (((addr >> 1) & 1) << 5)
	PORTB |= ADC_SCLK;
	PORTB &= ~ADC_SCLK;
	PORTC = (PORTC & (~ADC_DATA)) | (((addr >> 0) & 1) << 5)
	PORTB |= ADC_SCLK;
	PORTB &= ~ADC_SCLK;*/
	
	PORTC &= ~ADC_DATA;
	for (uint8_t i = 0; i < 3; i++) {
		PORTB |= ADC_SCLK;
		PORTB &= ~ADC_SCLK;
	}
	
	for (uint8_t i = 0; i < 9; i++) {
		PORTC = (PORTC & (~ADC_DATA)) | (((val >> (8 - i)) & 1) << 5);
		PORTB |= ADC_SCLK;
		PORTB &= ~ADC_SCLK;
	}
	
	PORTD |= ADC_SLOAD;
}

uint16_t adc_read(uint8_t addr) {
	uint16_t val = 0;
	
	DDRC |= 0b00100000;
	PORTB &= ~ADC_SCLK;
	PORTD &= ~ADC_SLOAD;
	
	PORTC |= ADC_DATA; // r/w bit 1 for reads
	PORTB |= ADC_SCLK;
	PORTB &= ~ADC_SCLK;
	
	for (uint8_t i = 0; i < 3; i++) {
		PORTC = (PORTC & (~ADC_DATA)) | (((addr >> (2 - i)) & 1) << 5);
		PORTB |= ADC_SCLK;
		PORTB &= ~ADC_SCLK;
	}
	
	PORTC &= ~ADC_DATA;
	DDRC &= ~0b00100000;
	for (uint8_t i = 0; i < 3; i++) {
		PORTB |= ADC_SCLK;
		PORTB &= ~ADC_SCLK;
	}
	
	for (uint8_t i = 0; i < 9; i++) {
		//PORTC = (PORTC & (~ADC_DATA)) | (((val >> (8 - i)) & 1) << 5)
		val |= (PORTC >> 5 & 1) << (8 - i);
		PORTB |= ADC_SCLK;
		PORTB &= ~ADC_SCLK;
	}
	
	PORTD |= ADC_SLOAD;
	DDRC |= 0b00100000;
	
	return val;
}

void readout() {
	int STATE0 = 0b01100010;
	int STATE1 = 0b01110011;
	int STATE2 = 0b01111001;
	int STATE3 = 0b01110000;
	int STATE4 = 0b10110000;
	int STATE5 = 0b10110101;
	int STATE6 = 0b10110111;
	int STATE7 = 0b10110010;
	/*int STATE0 = 0b01100011;
	int STATE1 = 0b01111000;
	int STATE2 = 0b01110001;
	int STATE3 = 0b10110101;
	int STATE4 = 0b10110110;
	int STATE5 = 0b10110011;*/
	
	//PORTD = IDLE;//STATE0;
	
	uint8_t temp = PORTC;
	temp |= CLK_V1;
	PORTC = temp;
	_delay_us(15);
	
	for (uint16_t row = 0; row < 2720 * 4; row++) {
		temp |= CLK_V2;
		temp &= ~CLK_V1;
		PORTC = temp;
		
		_delay_us(15);
		
		temp |= CLK_V1;
		temp &= ~CLK_V2;
		PORTC = temp;
		
		//_delay_us(235);
		_delay_us(15);
	}
	
	PORTC |= CLK_FD;
	
	PORTC |= CLK_V2;
	
	_delay_us(140);
	
	//uint8_t temp = PORTC;
	temp = PORTC;
	temp |= CLK_V2H;
	temp &= ~CLK_V1;
	//PORTC |= CLK_V2H; // TODO V1
	PORTC = temp;
	
	_delay_us(15);
	
	temp &= ~CLK_V2H;
	temp |= CLK_V1;
	PORTC = temp;

	_delay_us(30);
	
	PORTC &= ~CLK_V2;
	
	PORTD = IDLE; // TODO
	
	adc_write(REG_CFG, 0b011011000);
	
	_delay_us(4);

	for (uint16_t row = 0; row < 2720; row++) {
		// TODO row
		for (uint8_t i = 0; i < 16; i++) {
			PIXEL256;
			PORTD = IDLE;
		}
		
		/*for (uint16_t i = 0; i < 256; i++) {
			PORTD = 0b01100011;
			PORTD = 0b01100010;
		}*/
		
		PORTD = IDLE;//STATE0;
		//PIXEL2048;
		//PIXEL2048;
		
		_delay_us(1);
		
		temp |= CLK_V2;
		temp &= ~CLK_V1;
		PORTC = temp;
		
		_delay_us(15);
		
		temp |= CLK_V1;
		temp &= ~CLK_V2;
		PORTC = temp;
		
		_delay_us(12);
	}
	
	PORTD = LDLE;
	
	adc_write(REG_CFG, 0b011011100);
	
	PORTC &= ~CLK_FD;
}
