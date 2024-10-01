
#define F_CPU (20000000UL / 1UL)
#define CLK_PER F_CPU

#include <math.h>
#define TIMEBASE_VALUE ((uint8_t) ceil(CLK_PER * 0.000001))

#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>

#include <util/delay.h>
#include <avr/interrupt.h>

volatile uint16_t _tx_delay = 0;

static void ADC_0_init(void) {

	ADC0.MUXPOS = ADC_MUXPOS_AIN4_gc; /* ADC input pin 4 */

	ADC0.CTRLA = 1 << ADC_ENABLE_bp     /* ADC Enable: enabled */
		     | 0 << ADC_RUNSTDBY_bp; /* Run standby mode: disabled */

	ADC0.CTRLB = ADC_PRESC_DIV64_gc; /* CLK_PER divided by 64 */
	
	ADC0.CTRLC = TIMEBASE_VALUE << ADC_TIMEBASE_gp | ADC_REFSEL_VDD_gc; /* Internal reference */
}

static uint16_t ADC_0_get_conversion(uint8_t channel) {
	uint16_t res;

	ADC0.MUXPOS = channel;
	ADC0.COMMAND = ADC_MODE_SINGLE_8BIT_gc | ADC_START_IMMEDIATE_gc;
	while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
	res = ADC0.RESULT;
	ADC0.INTFLAGS |= ADC_RESRDY_bm;
	return res;
}

static void init_timer(void) {
	TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV64_gc;
	TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE0_bm;
	TCA0.SINGLE.CTRLC = TCA_SINGLE_CMP0OV_bm;
}

static void initSerial(void){
	_tx_delay = 0;
	uint16_t bit_delay = (F_CPU / 9600) / 4;
	if (bit_delay > 15 / 4)
		_tx_delay = bit_delay - (15 / 4);
	else
		_tx_delay = 1;
}

static void printC(char b) {
	const uint8_t mask = 1 << 4;
	const uint8_t imask = ~mask;
	cli();
	PORTB.OUT &= imask;
	_delay_loop_2(_tx_delay);
	for (uint8_t i = 8; i > 0; --i) {
		if (b & 1)
		PORTB.OUT |= mask;
		else
		PORTB.OUT &= imask;
		_delay_loop_2(_tx_delay);
		b >>= 1;
	}
	PORTB.OUT |= mask;
	_delay_loop_2(_tx_delay);
	sei();
}

static void printS(const char *str) {
	while(*str) {
		printC(*str);
		if(*str == '\n')
			printC('\r');
		str++;
	}
}

static void printH(uint8_t i) {
	const char hex[16]  = "0123456789abcdef";
	printC(hex[i >> 4]);
	printC(hex[i & 0xf]);
}


static void printI(uint16_t i) {
	uint16_t rev = 0;
	uint8_t count = 0;
	do {
		rev = (rev * 10) + i % 10;
		i /= 10;
		count++;
	} while (i);
	while(count--) {
		printC((rev % 10) + '0');
		rev /= 10;
	}
}

static void set_20M(void) {
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLA = CLKCTRL_CLKSEL_OSC20M_gc;
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLB = 0;
}

static void set_20M_DIV(uint8_t PDIV) {
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLA = CLKCTRL_CLKSEL_OSC20M_gc;
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLB = PDIV | CLKCTRL_PEN_bm;
}

int main(void) {
	PORTB.DIR = (1 << 4) | (1 << 0);
	PORTB.OUT = (1 << 4);
	set_20M();
	init_timer();
	initSerial();
	ADC_0_init();
	while (1) {
		uint16_t data = ADC_0_get_conversion(ADC_MUXPOS_AIN4_gc);
		TCA0.SINGLE.CMP0 = data << 8 | 0xff;
		printI(data);
		printS("\n");
    }
}
