/*
 * main.h
 *
 * Created: 27/06/2021 15:54:27
 *  Author: jpe
 */ 


#ifndef MAIN_H_
#define MAIN_H_


int main();
void adc_write(uint8_t addr, uint16_t val);
uint16_t adc_read(uint8_t addr);
void readout();


#endif /* MAIN_H_ */