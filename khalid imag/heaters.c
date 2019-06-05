/****************************************************************************
* This file contains everything for the RTDs
****************************************************************************/


#include "sam.h"
#include "temp.h"

/* Prototypes */
void display_RTDs(void);
void check_RTDs(void);

/*Global variables */	
int RTD_array[8] = {0,0,0,0,0,0,0,0,0};
int *RTD_array_ptr;

/* Setup ports for RTDs */
void rtd_port_setup(void){
	
	Port *por = PORT;
	PortGroup *porA = &(por->Group[0]);
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	PortGroup *porD = &(por->Group[3]);
	
	//12MHz crystal on board selected mapped to PB22/PB23
	
	porB->PMUX[2].bit.PMUXE = 1;	//PB04 ADC1 AIN[6] temp sensor
	porB->PINCFG[4].bit.PMUXEN = 1;
	
	porC->PMUX[0].bit.PMUXE = 1;	//PC00 ADC1 AIN[10] extra
	porC->PINCFG[0].bit.PMUXEN = 1;
	
	porC->PMUX[0].bit.PMUXO = 1;	//PC01 ADC1 AIN[11] extra
	porC->PINCFG[1].bit.PMUXEN = 1;

	porC->PMUX[1].bit.PMUXE = 1;	//PC02 ADC1 AIN[4] RTD0
	porC->PINCFG[2].bit.PMUXEN = 1;
	
	porC->PMUX[1].bit.PMUXO = 1;	//PC03 ADC1 AIN[5] RTD1
	porC->PINCFG[3].bit.PMUXEN = 1;
	
	porA->PMUX[1].bit.PMUXE = 1;	//PA02 ADC0 AIN[0] RTD2
	porA->PINCFG[2].bit.PMUXEN = 1;
	
	porA->PMUX[1].bit.PMUXO = 1;	//PA03 ADC0 AIN[1] RTD3
	porA->PINCFG[3].bit.PMUXEN = 1;
	
	porB->PMUX[2].bit.PMUXO = 1;	//PB05 ADC1 AIN[7] RTD4
	porB->PINCFG[5].bit.PMUXEN = 1;
	
	porD->PMUX[0].bit.PMUXE = 1;	//PD00 ADC1 AIN[14] RTD5
	porD->PINCFG[0].bit.PMUXEN = 1;
	
	porD->PMUX[0].bit.PMUXO = 1;	//PD01 ADC1 AIN[15] RTD6
	porD->PINCFG[1].bit.PMUXEN = 1;

	porB->PMUX[4].bit.PMUXE = 1;	//PB06 ADC1 AIN[8] RTD7
	porB->PINCFG[8].bit.PMUXEN = 1;
	
	
}

/* Setup ADC1 */
void ADC_1_Setup(void){
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
	while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
	ADC1->SWTRIG.bit.START = 1;	//start conversion
	while (ADC1->SYNCBUSY.bit.SWTRIG){}	//wait for sync to complete
	//ADC1->INTENSET.reg = ADC_INTENSET_RESRDY;	//setup interrupt when reg is ready to be read
	ADC1->CTRLA.reg |= 1<<1;	//enable ADC
	while (ADC1->SYNCBUSY.reg){}	//wait for enable to complete
	//NVIC->ISER[3] |= 1<<25;	//enable the NVIC handler
	//ADC0->OFFSETCORR.reg = 0b000000110100;	//shift down by 52, 2's comp
	//ADC0->GAINCORR.reg =   0b100010100000;	//when corren is enabled it enables gain comp too, fractional
}

/* Setup ADC0 */
void ADC_0_Setup(void){
	ADC0->CTRLA.reg = 0<<1;	//disable so that we can reset
	while (ADC0->SYNCBUSY.reg){}	//wait for disable to complete
	ADC0->CTRLA.bit.PRESCALER = 0;	//2^n
	ADC0->CTRLA.bit.ONDEMAND = 1;
	
	ADC0->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1;	//internal reference = VDDann
	while(ADC0->SYNCBUSY.bit.REFCTRL){}
	//ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV512 | ADC_CTRLB_RESSEL_8BIT | ADC_CTRLB_FREERUN | 0<<0 | ADC_CTRLB_CORREN;
	ADC0->CTRLB.reg = ADC_CTRLB_RESSEL_8BIT | 0<<1 | 0<<0;	// freerun mode off, right adjust
	while (ADC0->SYNCBUSY.bit.CTRLB){}	//wait for sync to complete
	ADC0->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN6;	//AIN6=PB04
	while (ADC0->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
	ADC0->SWTRIG.bit.START = 1;	//start conversion
	while (ADC0->SYNCBUSY.bit.SWTRIG){}	//wait for sync to complete
	//ADC0->INTENSET.reg = ADC_INTENSET_RESRDY;	//setup interrupt when reg is ready to be read
	ADC0->CTRLA.reg |= 1<<1;	//enable ADC
	while (ADC0->SYNCBUSY.reg){}	//wait for enable to complete
	//NVIC->ISER[3] |= 1<<25;	//enable the NVIC handler
	//ADC0->OFFSETCORR.reg = 0b000000110100;	//shift down by 52, 2's comp
	//ADC0->GAINCORR.reg =   0b100010100000;	//when corren is enabled it enables gain comp too, fractional
}

/* Timer for RTD temp (1 sec) */
void rtd_TC_Setup(void){	
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

/* Handler for board temperature sensor 
void ADC1_1_Handler(void){
	volatile int ADC_result = ADC1->RESULT.reg;	//read ADC conversion result
	volatile float result = (((float)ADC_result / 255) * 5);	//CHANGED float got changed from double
	result = (result - 1.375)/ .0225;
	convert((int)result);
}
*/

/* Handler for RTD timer */
void TC4_Handler(void){
	Tc *tc = TC4;
	TcCount16 *tc4 = &tc->COUNT16;
	tc4->INTFLAG.bit.OVF = 1;	//clear the int flag
	check_RTDs();	
	tc4->CTRLBSET.bit.CMD = 1;	//force retrigger
	while(tc4->SYNCBUSY.bit.CTRLB){}	//wait for sync to complete
}

/* Check each RTD and store in array */
void check_RTDs(void){
	volatile int RTD = 0;
	
	while(RTD < 1){
		switch(RTD){
			
			case 0:	//rtd 0
			ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN4;	//AIN4=PC02
			while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
			ADC1->SWTRIG.reg = 1;	//trigger ADC to start conversion
			while(ADC1->INTFLAG.bit.RESRDY == 0){}
			RTD_array[RTD] = (((ADC1->RESULT.reg)*50)-50);
			break;
			
			//case 1:	//rtd1
			//ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN5;	//AIN5=PC03
			//while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
			//ADC1->SWTRIG.reg = 1;	//trigger ADC to start conversion
			//while(ADC1->INTFLAG.bit.RESRDY == 0){}
			//RTD_array[RTD] = (((ADC1->RESULT.reg)*50)-50);
			//break;
			//
			//case 2:	//rtd2
			//ADC0->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN0;	//AIN0=PA02
			//while (ADC0->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
			//ADC0->SWTRIG.reg = 1;	//trigger ADC to start conversion
			//while(ADC0->INTFLAG.bit.RESRDY == 0){}
			//RTD_array[RTD] = (((ADC0->RESULT.reg)*50)-50);
			//break;
			//
			//case 3:	//rtd3
			//ADC0->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN1;	//AIN1=PA03
			//while (ADC0->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
			//ADC0->SWTRIG.reg = 1;	//trigger ADC to start conversion
			//while(ADC0->INTFLAG.bit.RESRDY == 0){}
			//RTD_array[RTD] = (((ADC0->RESULT.reg)*50)-50);
			//break;
			//
			//case 4:	//rtd4
			//ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN7;	//AIN7=PB05
			//while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
			//ADC1->SWTRIG.reg = 1;	//trigger ADC to start conversion
			//while(ADC1->INTFLAG.bit.RESRDY == 0){}
			//RTD_array[RTD] = (((ADC1->RESULT.reg)*50)-50);
			//break;
			//
			//case 5:	//rtd5
			//ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN14;	//AIN14=PD00
			//while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
			//ADC1->SWTRIG.reg = 1;	//trigger ADC to start conversion
			//while(ADC1->INTFLAG.bit.RESRDY == 0){}
			//RTD_array[RTD] = (((ADC1->RESULT.reg)*50)-50);
			//break;
			//
			//case 6:	//rtd6
			//ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN15;	//AIN15=PD01
			//while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
			//ADC1->SWTRIG.reg = 1;	//trigger ADC to start conversion
			//while(ADC1->INTFLAG.bit.RESRDY == 0){}
			//RTD_array[RTD] = (((ADC1->RESULT.reg)*50)-50);
			//break;
			//
			//case 7:	//rtd7
			//ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN8;	//AIN8=PB06
			//while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
			//ADC1->SWTRIG.reg = 1;	//trigger ADC to start conversion
			//while(ADC1->INTFLAG.bit.RESRDY == 0){}
			//RTD_array[RTD] = (((ADC1->RESULT.reg)*50)-50);
			//break;
			
			default:
			RTD_array[RTD] = 0;
			break;
		}
		RTD++;
	}
	display_RTDs();
}

void display_RTDs(void){
	int i = 0;
	while(i < 1){
		convert(RTD_array_ptr++);
		i++;
	}
}


