#ifndef HEATERS_H_INCLUDED
#define HEATERS_H_INCLUDED

/* Global variable declarations */
extern int RTD_array[8];
extern int *RTD_array_ptr;
extern int setpoint_array[8];
extern volatile char *terminal_input_array_ptr;


/* Prototypes for the functions */
void heater_port_setup(void);
void ADC_0_Setup(void);
void ADC_1_Setup(void);
void rtd_TC_Setup(void);
void display_RTDs(void);
void heater_setpoint(void);

#endif