/*
 * BankActions.c
 *
 * Created: 2018/4/27 1:15:06
 * Author : 黄俊杰
 */ 

#include <avr/io.h>
#include <scheduler.h>
#include <adc.h>
#include <usart_ATmega1284.h>
#include <delay.h>

#define THRESHOLD 0xA0

//global variables
unsigned char ADC_1 = 0; 
unsigned char ADC_5 = 0;
unsigned char ADC_10 = 0;
unsigned char ADC_25 = 0;
unsigned char ADC_temp_1 = 0; //store returned values from ReadADC
unsigned char ADC_temp_5 = 0;
unsigned char ADC_temp_10 = 0;
unsigned char ADC_temp_25 = 0;

	
enum USART_States { USART_start, USART_init, USART_wait, USART_sending };
int TickFct_USART(int state)
{
	static unsigned char rec_tmp = 0;
	static unsigned char sed_tmp = 0;
	switch(state) //Transitions
	{
		case USART_start:
			state = USART_init;
		break;
		
		case USART_init:
			state = USART_wait;
		break;
		
		case USART_wait:
			if (ADC_1 > 0 || ADC_5 > 0 || ADC_10 > 0 || ADC_25 > 0)
			{
				state = USART_sending;
			} 
			else
			{
				state = USART_wait;
			}
		break;
		
		case USART_sending:
			state = USART_wait;
		break;
		
		default:
			state = USART_start;
		break;
	}
	switch(state) //Actions
	{
		case USART_init:
			initUSART(0);
			USART_Flush(0);
		break;
		
		case USART_wait:
			
		break;
		
		case USART_sending:
			if (USART_IsSendReady(0))
			{
				if (ADC_1 > 0)
				{
					sed_tmp = ADC_1 | 0x00;
					ADC_1 = 0;
					USART_Send(sed_tmp, 0);
				}
				else if (ADC_5 > 0)
				{
					sed_tmp = ADC_5 | 0x10;
					ADC_5 = 0;
					USART_Send(sed_tmp, 0);
				}
				else if (ADC_10 > 0)
				{
					sed_tmp = ADC_10 | 0x20;
					ADC_10 = 0;
					USART_Send(sed_tmp, 0);
				}
				else if (ADC_25 > 0)
				{
					sed_tmp = ADC_25 | 0x30;
					ADC_25 = 0;
					USART_Send(sed_tmp, 0);
				}
			}
		break;
		
		default:
			state = USART_start;
		break;
	}
	return state;
}

enum ADC_States { ADC_start, ADC_init, ADC_check, ADC_check_1, ADC_check_5, ADC_check_10, ADC_check_25 };
int TickFct_ADC (int state)
{
	switch (state) //Transitions
	{
		case ADC_start:
			state = ADC_init;
		break;
		
		case ADC_init:
			state = ADC_check;
		break;
		
		case ADC_check:
			if (ADC_temp_1 > THRESHOLD)
			{
				state = ADC_check_1;
			} 
			else if (ADC_temp_5 > THRESHOLD)
			{
				state = ADC_check_5;
			}
			else if (ADC_temp_10 > THRESHOLD)
			{
				state = ADC_check_10;
			}
			else if (ADC_temp_25 > THRESHOLD)
			{
				state = ADC_check_25;
			}
			else
			{
				state = ADC_check;
			}
		break;
		
		case ADC_check_1:
			if (ADC_temp_1 > THRESHOLD)
			{
				state = ADC_check_1;
			}
			else
			{
				ADC_1 = 1;
				state = ADC_check;
			}
		break;
		
		case ADC_check_5:
			if (ADC_temp_5 > THRESHOLD)
			{
				state = ADC_check_5;
			}
			else
			{
				ADC_5 = 1;
				state = ADC_check;
			}
		break;
		
		case ADC_check_10:
			if (ADC_temp_10 > THRESHOLD)
			{
				state = ADC_check_10;
			}
			else
			{
				ADC_10 = 1;
				state = ADC_check;
			}
		break;
		
		case ADC_check_25:
			if (ADC_temp_25 > THRESHOLD)
			{
				state = ADC_check_25;
			}
			else
			{
				ADC_25 = 1;
				state = ADC_check;
			}
		break;
		
		default:
			state = ADC_start;
		break;
	}
	switch (state) //Actions
	{
		case ADC_init:
			InitADC();
		break;
		
		case ADC_check:
			ADC_temp_1 = ReadADC(0);
			ADC_temp_5 = ReadADC(1);
			ADC_temp_10 = ReadADC(2);
			ADC_temp_25 = ReadADC(3);
		break;
		
		case ADC_check_1:
			ADC_temp_1 = ReadADC(0);
		break;
		
		case ADC_check_5:
			ADC_temp_5 = ReadADC(1);
		break;
		
		case ADC_check_10:
			ADC_temp_10 = ReadADC(2);
		break;
		
		case ADC_check_25:
			ADC_temp_25 = ReadADC(3);
		break;
		
		default:
			state = ADC_start;
		break;
	}
	return state;
}

int main(void)
{
     // initialize ports
     DDRA = 0x00; PORTA = 0xFF;
     
     tasksNum = 2; // declare number of tasks
     task tsks[2]; // initialize the task array
     tasks = tsks; // set the task array
     
     // define tasks
     unsigned char i=0; // task counter
     tasks[i].state = USART_start;
     tasks[i].period = 50;
     tasks[i].elapsedTime = tasks[i].period;
     tasks[i].TickFct = &TickFct_USART;
     ++i;
     tasks[i].state = ADC_start;
     tasks[i].period = 20;
     tasks[i].elapsedTime = tasks[i].period;
     tasks[i].TickFct = &TickFct_ADC;
     
     TimerSet(10); // value set should be GCD of all tasks
     TimerOn();

     while(1) {} // task scheduler will be called by the hardware interrupt
}

