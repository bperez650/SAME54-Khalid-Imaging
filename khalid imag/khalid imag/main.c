/*
 * Version for Khalid's Imaging system
 * Version is very striped down and only has digital and analog outputs
 *
 * Using Termite Terminal
 * USing SAME54P20A mcu on XPlained Pro developement board
 *
 * Created: 10/15/2018 11:49:12 AM
 * Author : bryant
 */ 

#include "sam.h"

/* Digital IO for port control */
#define D00		PORT_PA08
#define D01		PORT_PA09
#define D02		PORT_PA10
#define D03		PORT_PA11
#define D04		PORT_PB10
#define D05		PORT_PB11
#define D06		PORT_PB12
#define D07		PORT_PB13
#define D08		PORT_PB14
#define D09		PORT_PB15
#define D10		PORT_PD08
#define D11		PORT_PD09
#define D12		PORT_PD10
#define D13		PORT_PD11
#define D14		PORT_PD12
#define D15		PORT_PC10


#define SS0 PORT_PC06	
#define SS1 PORT_PC07

/* Prototypes */
void clock_setup(void);
void port_setup(void);
void wait(volatile int d);
void terminal_UART_setup(void);	//USART
void SPI_setup(void);	//SPI
void write_terminal(char *a);
void port_control(void);
void DAC_select(void);
void DAC_value(void);
void write_SPI(char *a);
void board_temp_ADC_setup(void);
void convert(int a);

/**********************************Terminal menu do not change or mess with *****************************************/
volatile char menuArray[7][63] = {	//DO NOT FUCK WITH THIS 
									{"\n\n_________________________________________________________  \n\n"},
									{"                Artium Technologies, Inc Gen 3.0 Rev 0.0      \n"},
									{"_________________________________________________________    \n\n"},
									{"M=> Show Menu                                                 \n"},
									{"DxxXXX D=Analog Out x=Cannel Number 0 to 15 XXX=000 to 255    \n"},
									{"KxxX K=Port K xx-Bit 0 to 15 X= State H or L                  \n"},
									{"T=> Show Current Temperature Status\n\n#                        "},
									};//DO NOT FUCK WITH THIS 
/*********************************************************************************************************************/

/* Global variables */
volatile char *menu_ptr;
volatile int receive_array_count = 0;	//receive array counter
volatile char terminal_input_array[10] = {"0000000000"};
volatile char receive_key;
volatile char *terminal_input_array_ptr;
volatile int state = 0;
volatile char DAC_arrar[2];
volatile char *DAC_array_ptr;
volatile int slave_select;
volatile char convert_array[4];
volatile char *convert_array_ptr;
	
int main(void){

	/* Initializing functions*/
    SystemInit();
	clock_setup();
	port_setup();
	terminal_UART_setup();	
	SPI_setup();	
	board_temp_ADC_setup();
	
	/* Writes "start" to terminal upon reset */
	volatile char startArr[] = "Start\n";
	volatile char *startPtr;
	startPtr = startArr;
	write_terminal(startPtr);

	/* Assign pointers */
	terminal_input_array_ptr = terminal_input_array;
	menu_ptr = menuArray;
	DAC_array_ptr = DAC_arrar;
	convert_array_ptr = convert_array;
	
	/* Polling loop looking for Terminal request */
	while(1){	
	
		if(receive_key == 13){	//look for carriage return
			
			/* Menu Selection */
			if(((*terminal_input_array_ptr == 'm') || (*terminal_input_array_ptr == 'M')) && (receive_array_count = 1)){	
				write_terminal(menu_ptr);
				receive_array_count = 0;
				receive_key = 0;
			}
			
			/*  BoardTemperature */
			else if(((*terminal_input_array_ptr == 't') || (*terminal_input_array_ptr == 'T')) && (receive_array_count = 1)){	
				
				ADC1->SWTRIG.bit.START = 1;	//start conversion
				while (ADC1->SYNCBUSY.bit.SWTRIG){}				 
				while(ADC1->INTFLAG.bit.RESRDY == 0){}	//wait for conversion
				volatile int ADC_result = ADC1->RESULT.reg;	//read ADC conversion result
				volatile float result = (((float)ADC_result / 255) * 5);	//CHANGED float got changed from double
				result = (result - 1.375)/ .0225;
				int temp = (int)result;
				int *temp1 = &temp;
				convert(temp1);
				receive_array_count = 0;
				receive_key = 0;
			}
			
			/* Digital Out */
			else if(((*terminal_input_array_ptr == 'k') || (*terminal_input_array_ptr == 'K')) && (receive_array_count = 4)){	
				//writeUart(arrayPtr);
				receive_array_count = 0;
				receive_key = 0;
				port_control();
			}
		
			/* Analog Out */
			else if(((*terminal_input_array_ptr == 'd') || (*terminal_input_array_ptr == 'D')) && (receive_array_count = 5)){	
				//writeUart(arrayPtr);
				receive_array_count = 0;
				receive_key = 0;
				DAC_select();
			}
		
			/* Invalid Entry '?' */
			else{	
				volatile char Invalid_message_Arr[] = "?\n";
				volatile char *Invalid_message_Ptr;
				Invalid_message_Ptr = Invalid_message_Arr;
				write_terminal(Invalid_message_Ptr);
				receive_array_count = 0;
				receive_key = 0;
			}
		}
	}
}

/* CLock source is 12MHz divided to 1MHz */
void clock_setup(void){
	//12MHz crystal on board selected mapped to PB22/PB23
	OSCCTRL->XOSCCTRL[1].bit.ENALC = 1;	//enables auto loop ctrl to control amp of osc
	OSCCTRL->XOSCCTRL[1].bit.IMULT = 4;
	OSCCTRL->XOSCCTRL[1].bit.IPTAT = 3;
	OSCCTRL->XOSCCTRL[1].bit.ONDEMAND = 1;
	OSCCTRL->XOSCCTRL[1].bit.RUNSTDBY = 1;
	OSCCTRL->XOSCCTRL[1].bit.XTALEN = 1;	//select ext crystal osc mode
	OSCCTRL->XOSCCTRL[1].bit.ENABLE = 1;
	
	GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_XOSC1 | GCLK_GENCTRL_RUNSTDBY | !(GCLK_GENCTRL_DIVSEL) | GCLK_GENCTRL_OE | GCLK_GENCTRL_GENEN | 12<<16;	//divide by 12 1MHz
	while(GCLK->SYNCBUSY.bit.GENCTRL0){}	
	
	GCLK->PCHCTRL[7].bit.CHEN = 0;	//disable for safety first
	GCLK->PCHCTRL[7].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//SERCOM0
	GCLK->PCHCTRL[36].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//SERCOM6
	GCLK->PCHCTRL[41].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//ADC1
	
	MCLK->CPUDIV.reg = 1;	//divide by 1
	MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM0;	//unmask sercom0
	MCLK->APBDMASK.reg |= MCLK_APBDMASK_SERCOM6;	//unmask sercom6
	MCLK->APBDMASK.reg |= MCLK_APBDMASK_ADC1;//unmask ADC1
}

void port_setup(void){
	Port *por = PORT;
	PortGroup *porA = &(por->Group[0]);
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	PortGroup *porD = &(por->Group[3]);
	
	/* SERCOM0 UART terminal */
	porA->PMUX[2].bit.PMUXE = 3;	//PA04 pad0 Tx
	porA->PINCFG[4].bit.PMUXEN = 1;	//PA05 pad1 Rx
	porA->PMUX[2].bit.PMUXO = 3;
	porA->PINCFG[5].bit.PMUXEN = 1;
	
	/* SERCOM6 SPI analog outputs*/
	porC->PMUX[2].bit.PMUXE = 2;	//PC04 pad0 DO
	porC->PMUX[2].bit.PMUXO = 2;	//PC05 pad1 SCLK
	porC->PINCFG[4].bit.PMUXEN = 1;
	porC->PINCFG[5].bit.PMUXEN = 1;
	
	porC->DIRSET.reg |= PORT_PC06;	//SS0 for 1st DAC
	porC->DIRSET.reg |= PORT_PC07;	//SS0 for 1st DAC
	porC->OUTSET.reg |= PORT_PC06;	//initialize SS0 high
	porC->OUTSET.reg |= PORT_PC07;	//initialize SS0 high

	//porC->PMUX[3].bit.PMUXE = 2;	//PC06 pad2
	//porC->PINCFG[6].bit.PMUXEN = 1;
	
	//porC->DIRSET.reg |= PORT_PB00;	//SS1 for 2nd DAC
	//porC->OUTSET.reg |= PORT_PB00;	//initialize SS1 high
	
	
	/* PORTs digital outputs*/
	porB->DIRSET.reg = D00 | D01 | D02 | D03;	//PB26, PB27, PB28, PB29 
	
	/* ADC */
	porB->PMUX[2].bit.PMUXE = 1;	//PB04 ADC1 AIN[6]
	porB->PINCFG[4].bit.PMUXEN = 1;
	
}

void terminal_UART_setup(void){
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	uart->CTRLA.reg = 0;	//enable protected regs
	while(uart->SYNCBUSY.reg){}
	uart->CTRLA.bit.DORD = 1;	//LSB transferred first
	uart->CTRLA.bit.CMODE = 0;	//asynchronous mode
	uart->CTRLA.bit.SAMPR = 0;	//16x oversampling using arithmetic
	uart->CTRLA.bit.RXPO = 1;	//RX is pad1 PA05
	uart->CTRLA.bit.TXPO = 2;	//TX is pad0 PA04
	uart->CTRLA.bit.MODE = 1;	//uart with internal clock
	uart->CTRLB.bit.RXEN = 1;	//enable RX
	uart->CTRLB.bit.TXEN = 1;	//enable TX
	uart->CTRLB.bit.PMODE = 0;	//even parity mode
	uart->CTRLB.bit.SBMODE = 0;	//1 stop bit
	uart->CTRLB.bit.CHSIZE = 0;	//8bit char size
	while(uart->SYNCBUSY.reg){}
	uart->BAUD.reg = 55470;	//for fbaud 9600 at 1Mhz fref
	uart->INTENSET.bit.RXC = 1;	//receive complete interr
	NVIC->ISER[1] |= 1<<16;	//enable sercom0 RXC int
	uart->CTRLA.reg |= 1<<1;	//enable
	while(uart->SYNCBUSY.reg){}
}

/* Handler for Terminal UART */
void SERCOM0_2_Handler(void){	
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	receive_key = uart->DATA.reg;
	if(receive_key != 13){
		terminal_input_array[receive_array_count++] = receive_key;
	}

}

/* Setup for communicating with the DACs */
void SPI_setup(void){	
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	spi->CTRLA.reg = 0<<1;	//disable first
	while(spi->SYNCBUSY.reg){}	
	spi->CTRLA.bit.DORD = 0;	//MSB first needed for AD5308
	//spi->CTRLA.bit.DORD = 1;	//LSB first needed for AD5308
	spi->CTRLA.bit.DOPO = 0;	//DO=pad0 PC04, SCK=pad1 PC05
	spi->CTRLA.bit.FORM = 0;	//SPI frame form
	spi->CTRLA.bit.MODE = 3;	//master mode
	spi->CTRLB.bit.MSSEN = 0;	//software controlled SS
	spi->CTRLB.bit.CHSIZE = 0;	//8 bit char size
	while(spi->SYNCBUSY.reg){}
	//spi->BAUD.reg = 55470;	//9600bps at 1MHz
	spi->BAUD.reg = 51;	//9600bps at 1MHz
	spi->INTENSET.bit.TXC = 1;	//transmit complete
	//NVIC->ISER[2] |= 1<<7;	//enable sercom6 TXC int
	spi->CTRLA.reg |= 1<<1;	//enable
	while(spi->SYNCBUSY.reg){}	
	
}

/* Handler for SPI */
void SERCOM6_1_Handler(void){	//obsolete right now
	//pull SS up again and end transaction
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	spi->INTFLAG.bit.TXC = 1;
}

void board_temp_ADC_setup(void){
	ADC1->CTRLA.reg = 0<<1;	
	while (ADC1->SYNCBUSY.reg){}	
	ADC1->CTRLA.bit.PRESCALER = 0;	//2^n
	ADC1->CTRLA.bit.ONDEMAND = 1;
	ADC1->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1;	//internal reference = VDDann
	while(ADC1->SYNCBUSY.bit.REFCTRL){}
	ADC1->CTRLB.reg = ADC_CTRLB_RESSEL_8BIT | 0<<1 | 0<<0;	// freerun mode off, right adjust
	while (ADC1->SYNCBUSY.bit.CTRLB){}	
	ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN6;	//AIN6=PB04
	while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	
	ADC1->CTRLA.reg |= 1<<1;	//enable ADC
	while (ADC1->SYNCBUSY.reg){}	
}

void write_terminal(char *a){
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	if(receive_array_count == 1){
		while(*a != '#'){
			while(!(uart->INTFLAG.bit.DRE)){}
			uart->DATA.reg = *a++;
			while((uart->INTFLAG.bit.TXC)==0){}	// waiting for transmit to complete
		}
	}
	
	else{
		while(*a){
			while(!(uart->INTFLAG.bit.DRE)){}
			uart->DATA.reg = *a++;
			while((uart->INTFLAG.bit.TXC)==0){}	// waiting for transmit to complete
		}
		uart->DATA.reg = 10;
	}
}

/* Selects digital port and gives it value */
void port_control(void){
		Port *por = PORT;
		PortGroup *porB = &(por->Group[1]);
		
	if((*(terminal_input_array_ptr+1) >= 48) && (*(terminal_input_array_ptr+1) <= 57)){	//looking for number keys only
		if((*(terminal_input_array_ptr+2) >= 48) && (*(terminal_input_array_ptr+2) <= 57)){	//looking for number keys only
			volatile int zone = (*(terminal_input_array_ptr+1) - 48) * 10;
			zone += *(terminal_input_array_ptr+2) - 48;
			
			switch(zone){
				case 0:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D00;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D00;
				}
				break;
				
				case 1:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D01;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D01;
				}
				break;
				
				case 2:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D02;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D02;
				}
				break;
				
				case 3:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D03;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D03;
				}
				break;
				
				case 4:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D04;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D04;
				}
				break;
				
				case 5:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D05;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D05;
				}
				break;
				
				case 6:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D06;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D06;
				}
				break;
				
				case 7:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D07;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D07;
				}
				break;
				
				case 8:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D08;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D08;
				}
				break;
				
				case 9:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D09;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D09;
				}
				break;
				
				case 10:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D10;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D10;
				}
				break;
				
				case 11:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D11;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D11;
				}
				break;
				
				case 12:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D12;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D12;
				}
				break;
				
				case 13:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D13;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D13;
				}
				break;
				
				case 14:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D14;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D14;
				}
				break;
				
				case 15:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D15;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D15;
				}
				break;
				
				default:
				break;
			}
		}
	}
}

/* Selects which DAC out of 16 will be used */
void DAC_select(void){
	Port *por = PORT;
	PortGroup *porB = &(por->Group[1]);
		if((*(terminal_input_array_ptr+1) >= 48) && (*(terminal_input_array_ptr+1) <= 57)){	//looking for number keys only
		if((*(terminal_input_array_ptr+2) >= 48) && (*(terminal_input_array_ptr+2) <= 57)){	//looking for number keys only
			volatile int zone = (*(terminal_input_array_ptr+1) - 48) * 10;
			zone += *(terminal_input_array_ptr+2) - 48;
			if(zone <= 7){
				DAC_arrar[0] = 2 * zone;	// 2 * zone is to accomodate TLV only
				slave_select = 0;
			}
			else{
				DAC_arrar[0]= 2 * (zone - 8);	// 2 * zone is to accomodate TLV only
				slave_select = 1;
			}
			DAC_value();
			
		}
	}
}

/* Selects the value written to the DAC after it has been selected */
void DAC_value(void){
	if((*(terminal_input_array_ptr+1) >= 48) && (*(terminal_input_array_ptr+1) <= 57)){	//looking for number keys only
		if((*(terminal_input_array_ptr+2) >= 48) && (*(terminal_input_array_ptr+2) <= 57)){	//looking for number keys only
			if((*(terminal_input_array_ptr+3) >= 48) && (*(terminal_input_array_ptr+3) <= 57)){	//looking for number keys only
				volatile int value = (*(terminal_input_array_ptr+3) - 48) * 100;
				value += (*(terminal_input_array_ptr+4) - 48) * 10;
				value += *(terminal_input_array_ptr+5) - 48;
				DAC_arrar[1] = value;
				//arrDACptr = arrDAC;
				write_SPI(DAC_array_ptr);
			}
		}
	}
	
}

void write_SPI(char *a){
	int SS;	//which dac 
	volatile static int j = 0;	//counter
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	Port *por = PORT;
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	
	while( j<2 ){
		
		spi->DATA.reg = DAC_arrar[j];
		//spi->DATA.reg = 0xab;	//test
		while(spi->INTFLAG.bit.DRE == 0){}	//wait for DATA reg to be empty
		while(spi->INTFLAG.bit.TXC == 0){}	//wait for tx to finish
		j++;	//increment counter
		//if(j == 2){
			//spi->INTENCLR.bit.DRE = 1;	//clear the DRE flag
			//Port *por = PORT;
			//PortGroup *porA = &(por->Group[0]);
			//porA->OUTCLR.reg = SS;	//pull SS down
			//wait(1);
			//porA->OUTSET.reg = SS;	//pull SS up
		//}
	}
	wait(1);
	////// pulse SS (load) to clk data into dacs for TLV only
	if(slave_select == 0){	
		porC->OUTCLR.reg = SS0;
		//wait(1);
		porC->OUTSET.reg = SS0;
	}
	else if(slave_select == 1){
		porB->OUTCLR.reg = SS1;
		//wait(1);
		porB->OUTSET.reg = SS1;
	}
	j = 0;
}

void wait(volatile int d){
	int count = 0;
	while (count < d*1000){
		count++;
	}
}

/* Converts a decimal value into and array of char for serial comm */
void convert(int a){
	int i = 100;   //divisor
	int j = 0;  //array counter
	int m = 1;  //counter
	int n = 100;    //increment to divisor

	while(j <= 3){
		int b = a % i;
		if(b == a) {
			int p = (m-1);
			switch(p) {
				case 0:
				convert_array[j++] = '0';
				break;
				case 1:
				convert_array[j++] = '1';
				break;
				case 2:
				convert_array[j++] = '2';
				break;
				case 3:
				convert_array[j++] = '3';
				break;
				case 4:
				convert_array[j++] = '4';
				break;
				case 5:
				convert_array[j++] = '5';
				break;
				case 6:
				convert_array[j++] = '6';
				break;
				case 7:
				convert_array[j++] = '7';
				break;
				case 8:
				convert_array[j++] = '8';
				break;
				case 9:
				convert_array[j++] = '9';
				break;
				default:
				convert_array[j++] = 'A';
				break;
			}
			a = a - (n*(m-1));
			m = 1;

			if(j == 1){
				i = 10;
				n = 10;
			}
			if(j == 2){
				i = 1;
				n = 1;
			}
			
		}
		else{
			m++;
			i = i + n;
		}
	}
	convert_array[3] = 0;	//force pointer to end here
	write_terminal(convert_array_ptr);
}