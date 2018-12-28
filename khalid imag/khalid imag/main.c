/*
 * GccApplication1.c
 *
 * Created: 10/15/2018 11:49:12 AM
 * Author : bryant
 */ 

#include "sam.h"

#define D00 PORT_PB26
#define D01 PORT_PB27
#define D02 PORT_PB28
#define D03 PORT_PB29

#define SS0 PORT_PC07	
//#define SS1 PORT_PB00


void clockSetup(void);
void portSetup(void);
void wait(volatile int d);
void sercom_0_Setup(void);	//USART
void sercom_6_Setup(void);	//SPI
void writeUart(char *a);
void portControl(void);
void dacSelect(void);
void dacValue(void);
void writeSPI(char *a);
void tempADC_Setup(void);
void tempTC_Setup(void);
void convert(int a);

volatile char menuArray[7][63] = {	//DO NOT FUCK WITH THIS 
									{"\n\n_________________________________________________________  \n\n"},
									{"                Artium Technologies, Inc Gen 3.0 Rev 0.0      \n"},
									{"_________________________________________________________    \n\n"},
									{"M=> Show Menu                                                 \n"},
									{"DxxXXX D=Analog Out x=Cannel Number 0 to 15 XXX=000 to 255    \n"},
									{"KxxX K=Port K xx-Bit 0 to 15 X= State H or L                  \n"},
									{"T=> Show Current Temperature Status\n\n#                        "},
									};//DO NOT FUCK WITH THIS 
volatile char *menuPtr;
volatile int receiveCount = 0;	//receive array counter
volatile char receiveArray[10] = {"0000000000"};
volatile char receiveKey;
volatile char *arrayPtr;
volatile int state = 0;
volatile char arrDAC[2];
volatile char *arrDACptr;
volatile int slaveSel;
//volatile int ADC_result;
volatile char convertArray[4];
volatile char *convertArrayPtr;
	
int main(void){

    SystemInit();	//Initializme the SAM system
	clockSetup();
	portSetup();
	sercom_0_Setup();	//USART
	sercom_6_Setup();	//SPI
	tempADC_Setup();
	tempTC_Setup();
	
	volatile char startArr[] = "\nStart\n";
	volatile char *startPtr;
	startPtr = startArr;
	writeUart(startPtr);

	arrayPtr = receiveArray;
	menuPtr = menuArray;
	arrDACptr = arrDAC;
	convertArrayPtr = convertArray;
	
	while(1){	
	
		if(receiveKey == 13){	//look for carriage return
		
			if(((*arrayPtr == 'm') || (*arrayPtr == 'M')) && (receiveCount = 1)){	//verifying valid menu selection
				writeUart(menuPtr);
				receiveCount = 0;
				receiveKey = 0;
			}
			else if(((*arrayPtr == 't') || (*arrayPtr == 'T')) && (receiveCount = 1)){	//verifying valid Temperature selection
				Tc *tc = TC4;
				TcCount16 *tc4 = &tc->COUNT16;
				tc4->CTRLBSET.bit.CMD = 1;	//force retrigger
				while(tc4->SYNCBUSY.bit.CTRLB){}	//wait for sync to complete
				receiveCount = 0;
				receiveKey = 0;
			}
			else if(((*arrayPtr == 'k') || (*arrayPtr == 'K')) && (receiveCount = 4)){	//verifying valid input for digital
				//writeUart(arrayPtr);
				receiveCount = 0;
				receiveKey = 0;
				portControl();
			}
		
			else if(((*arrayPtr == 'd') || (*arrayPtr == 'D')) && (receiveCount = 5)){	//analog stuff
				//writeUart(arrayPtr);
				receiveCount = 0;
				receiveKey = 0;
				dacSelect();
			}
		
			else{	//invalid key press, reset array 
				receiveCount = 0;
				receiveKey = 0;
			}
		}
	}
}

void clockSetup(void){
	//12MHz crystal on board selected mapped to PB22/PB23
	OSCCTRL->XOSCCTRL[1].bit.ENALC = 1;	//enables auto loop ctrl to control amp of osc
	OSCCTRL->XOSCCTRL[1].bit.IMULT = 4;
	OSCCTRL->XOSCCTRL[1].bit.IPTAT = 3;
	OSCCTRL->XOSCCTRL[1].bit.ONDEMAND = 1;
	OSCCTRL->XOSCCTRL[1].bit.RUNSTDBY = 1;
	OSCCTRL->XOSCCTRL[1].bit.XTALEN = 1;	//select ext crystal osc mode
	OSCCTRL->XOSCCTRL[1].bit.ENABLE = 1;
	
	GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_XOSC1 | GCLK_GENCTRL_RUNSTDBY | !(GCLK_GENCTRL_DIVSEL) | GCLK_GENCTRL_OE | GCLK_GENCTRL_GENEN | 12<<16;	//divide by 12 1MHz
	while(GCLK->SYNCBUSY.bit.GENCTRL0){}	//wait for sync
	
	//channel 7, SERCOM0
	GCLK->PCHCTRL[7].bit.CHEN = 0;	//disable for safety first
	GCLK->PCHCTRL[7].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//SERCOM0
	GCLK->PCHCTRL[36].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//SERCOM6
	GCLK->PCHCTRL[30].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//TC4
	GCLK->PCHCTRL[41].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//ADC1
	
	MCLK->CPUDIV.reg = 1;	//divide by 1
	MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM0;	//unmask sercom0
	MCLK->APBDMASK.reg |= MCLK_APBDMASK_SERCOM6;	//unmask sercom6
	MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC4;	//unmask TC4
	MCLK->APBDMASK.reg |= MCLK_APBDMASK_ADC1;//unmask ADC1
}

void portSetup(void){
	Port *por = PORT;
	PortGroup *porA = &(por->Group[0]);
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	PortGroup *porD = &(por->Group[3]);
	
	///SERCOM0
	porA->PMUX[2].bit.PMUXE = 3;	//PA04 pad0
	porA->PINCFG[4].bit.PMUXEN = 1;	//PA05 pad1
	porA->PMUX[2].bit.PMUXO = 3;
	porA->PINCFG[5].bit.PMUXEN = 1;
	
	///SERCOM6
	porC->PMUX[2].bit.PMUXE = 2;	//PC04 pad0
	porC->PMUX[2].bit.PMUXO = 2;	//PC05 pad1
	porC->PMUX[3].bit.PMUXE = 2;	//PC06 pad2
	porC->PINCFG[4].bit.PMUXEN = 1;
	porC->PINCFG[5].bit.PMUXEN = 1;
	porC->PINCFG[6].bit.PMUXEN = 1;
	porC->DIRSET.reg |= PORT_PC07;	//SS0 for 1st DAC
	porC->DIRSET.reg |= PORT_PB00;	//SS1 for 2nd DAC
	porC->OUTSET.reg |= PORT_PC07;	//initialize SS0 high
	porC->OUTSET.reg |= PORT_PB00;	//initialize SS1 high
	
	//PORTs
	porB->DIRSET.reg = D00 | D01 | D02 | D03;	//PB26, PB27, PB28, PB29 //digital outputs
	
	//ADC
	porB->PMUX[2].bit.PMUXE = 1;	//PB04 ADC1 AIN[6]
	porB->PINCFG[4].bit.PMUXEN = 1;
	
}

void sercom_0_Setup(void){	//USART
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

void SERCOM0_2_Handler(void){	//for recieving
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	receiveKey = uart->DATA.reg;
	if(receiveKey != 13){
		receiveArray[receiveCount++] = receiveKey;
	}

}

void sercom_6_Setup(void){	//SPI
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	spi->CTRLA.reg = 0<<1;	//disable first
	while(spi->SYNCBUSY.reg){}	
	spi->CTRLA.bit.DORD = 0;	//MSB first needed for AD5308
	//spi->CTRLA.bit.DORD = 1;	//LSB first needed for AD5308
	spi->CTRLA.bit.DOPO = 0;	//DO=pad0 PC04, SCK=pad1 PC05, SS=pad2 PC06
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

void SERCOM6_1_Handler(void){	//obsolete right now
	//pull SS up again and end transaction
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	spi->INTFLAG.bit.TXC = 1;
}

void tempADC_Setup(void){
	ADC1->CTRLA.reg = 0<<1;	//disable so that we can reset
	while (ADC1->SYNCBUSY.reg){}	//wait for disable to complete
	ADC1->CTRLA.bit.PRESCALER = 0;	//2^n
	ADC1->CTRLA.bit.ONDEMAND = 1;
	
	ADC1->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1;	//internal reference = VDDann
	while(ADC1->SYNCBUSY.bit.REFCTRL){}
	//ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV512 | ADC_CTRLB_RESSEL_8BIT | ADC_CTRLB_FREERUN | 0<<0 | ADC_CTRLB_CORREN;
	ADC1->CTRLB.reg = ADC_CTRLB_RESSEL_8BIT | 0<<1 | 0<<0;	// freerun mode off, right adjust
	while (ADC1->SYNCBUSY.bit.CTRLB){}	//wait for sync to complete
	ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN6;	//AIN6=PB04
	//ADC1->INPUTCTRL.bit.GAIN = 0xF;	//gain = 1/2
	while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
	ADC1->SWTRIG.bit.START = 1;	//start conversion
	while (ADC1->SYNCBUSY.bit.SWTRIG){}	//wait for sync to complete
	ADC1->INTENSET.reg = ADC_INTENSET_RESRDY;	//setup interrupt when reg is ready to be read
	ADC1->CTRLA.reg |= 1<<1;	//enable ADC
	while (ADC1->SYNCBUSY.reg){}	//wait for enable to complete
	NVIC->ISER[3] |= 1<<25;	//enable the NVIC handler
	//ADC0->OFFSETCORR.reg = 0b000000110100;	//shift down by 52, 2's comp
	//ADC0->GAINCORR.reg =   0b100010100000;	//when corren is enabled it enables gain comp too, fractional
}

void ADC1_1_Handler(void){
	volatile int ADC_result = ADC1->RESULT.reg;	//read ADC conversion result
	volatile double result = (((double)ADC_result / 255) * 5);
	result = (result - 1.375)/ .0225;
	convert((int)result);
}

void tempTC_Setup(void){	//timer for reading the temp sensor
	Tc *tc = TC4;
	TcCount16 *tc4 = &tc->COUNT16;
	tc4->CTRLA.reg = 0;	//disable the TC4
	while(tc4->SYNCBUSY.reg){}	//wait for sync of disable
	tc4->CTRLA.bit.PRESCALER = 4;	//2^n;
	tc4->CTRLA.bit.MODE = 0;	//16 bit mode
	tc4->CTRLBSET.bit.ONESHOT = 1;	//turn on one shot mode
	while(tc4->SYNCBUSY.bit.CTRLB){}	//wait for sync to complete
	tc4->INTENSET.bit.OVF = 1;	//enable the overflow interrupt
	tc4->CTRLA.reg |= 1<<1;	//enable the TC4
	while(tc4->SYNCBUSY.reg){}	//wait for sync of enable
	NVIC->ISER[3] |= 1<<15;	//enable the NVIC handler for TC4
}

void TC4_Handler(void){
	Tc *tc = TC4;
	TcCount16 *tc4 = &tc->COUNT16;
	tc4->INTFLAG.bit.OVF = 1;	//clear the int flag
	ADC1->SWTRIG.reg = 1;//trigger ADC to start conversion
}

void writeUart(char *a){
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	if(receiveCount == 1){
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

void portControl(void){
	
		Port *por = PORT;
		PortGroup *porB = &(por->Group[1]);
		
	if((*(arrayPtr+1) >= 48) && (*(arrayPtr+1) <= 57)){	//looking for number keys only
		if((*(arrayPtr+2) >= 48) && (*(arrayPtr+2) <= 57)){	//looking for number keys only
			volatile int zone = (*(arrayPtr+1) - 48) * 10;
			zone += *(arrayPtr+2) - 48;
			
			switch(zone){
				case 0:
				if(*(arrayPtr+3) == 'L' || *(arrayPtr+3) == 'l'){
					porB->OUTCLR.reg = D00;
				}
				else if(*(arrayPtr+3) == 'H' || *(arrayPtr+3) == 'h'){
					porB->OUTSET.reg = D00;
				}
				break;
				
				case 1:
				if(*(arrayPtr+3) == 'L' || *(arrayPtr+3) == 'l'){
					porB->OUTCLR.reg = D01;
				}
				else if(*(arrayPtr+3) == 'H' || *(arrayPtr+3) == 'h'){
					porB->OUTSET.reg = D01;
				}
				break;
				
				case 2:
				if(*(arrayPtr+3) == 'L' || *(arrayPtr+3) == 'l'){
					porB->OUTCLR.reg = D02;
				}
				else if(*(arrayPtr+3) == 'H' || *(arrayPtr+3) == 'h'){
					porB->OUTSET.reg = D02;
				}
				break;
				
				case 3:
				if(*(arrayPtr+3) == 'L' || *(arrayPtr+3) == 'l'){
					porB->OUTCLR.reg = D03;
				}
				else if(*(arrayPtr+3) == 'H' || *(arrayPtr+3) == 'h'){
					porB->OUTSET.reg = D03;
				}
				break;
				
				case 4:
				break;
				case 5:
				break;
				case 6:
				break;
				case 7:
				break;
				case 8:
				break;
				case 9:
				break;
				case 10:
				break;
				case 11:
				break;
				case 12:
				break;
				case 13:
				break;
				case 14:
				break;
				case 15:
				break;
				
				default:
				break;
			}
		}
	}
}

void dacSelect(void){
	Port *por = PORT;
	PortGroup *porB = &(por->Group[1]);
		if((*(arrayPtr+1) >= 48) && (*(arrayPtr+1) <= 57)){	//looking for number keys only
		if((*(arrayPtr+2) >= 48) && (*(arrayPtr+2) <= 57)){	//looking for number keys only
			volatile int zone = (*(arrayPtr+1) - 48) * 10;
			zone += *(arrayPtr+2) - 48;
			if(zone <= 7){
				arrDAC[0] = 2 * zone;	// 2 * zone is to accomodate TLV only
				slaveSel = 0;
			}
			else{
				arrDAC[0]= 2 * (zone - 8);	// 2 * zone is to accomodate TLV only
				slaveSel = 1;
			}
			dacValue();
			
		}
	}
}

void dacValue(void){
	if((*(arrayPtr+1) >= 48) && (*(arrayPtr+1) <= 57)){	//looking for number keys only
		if((*(arrayPtr+2) >= 48) && (*(arrayPtr+2) <= 57)){	//looking for number keys only
			if((*(arrayPtr+3) >= 48) && (*(arrayPtr+3) <= 57)){	//looking for number keys only
				volatile int value = (*(arrayPtr+3) - 48) * 100;
				value += (*(arrayPtr+4) - 48) * 10;
				value += *(arrayPtr+5) - 48;
				arrDAC[1] = value;
				//arrDACptr = arrDAC;
				writeSPI(arrDACptr);
			}
		}
	}
	
}

void writeSPI(char *a){
	int SS;	//which dac 
	volatile static int j = 0;	//counter
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	Port *por = PORT;
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	
	while( j<2 ){
		
		spi->DATA.reg = arrDAC[j];
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
	if(slaveSel == 0){	
		porC->OUTCLR.reg = SS0;
		//wait(1);
		porC->OUTSET.reg = SS0;
	}
	else if(slaveSel == 1){
		porB->OUTCLR.reg = SS0;
		//wait(1);
		porB->OUTSET.reg = SS0;
	}
	j = 0;
}

void wait(volatile int d){
	int count = 0;
	while (count < d*1000){
		count++;
	}
}

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
				convertArray[j++] = '0';
				break;
				case 1:
				convertArray[j++] = '1';
				break;
				case 2:
				convertArray[j++] = '2';
				break;
				case 3:
				convertArray[j++] = '3';
				break;
				case 4:
				convertArray[j++] = '4';
				break;
				case 5:
				convertArray[j++] = '5';
				break;
				case 6:
				convertArray[j++] = '6';
				break;
				case 7:
				convertArray[j++] = '7';
				break;
				case 8:
				convertArray[j++] = '8';
				break;
				case 9:
				convertArray[j++] = '9';
				break;
				default:
				convertArray[j++] = 'A';
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
	convertArray[3] = 0;	//force pointer to end here
	writeUart(convertArrayPtr);
}