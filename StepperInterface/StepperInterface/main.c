/*
 * BankActions.c
 *
 * Created: 2018/4/27 1:15:06
 * Author : 黄俊杰
 */ 

#include <avr/io.h>
#include <scheduler.h>
#include <usart_ATmega1284.h>
#include <delay.h>

#define FULL 2048

//global variables
unsigned char Stepper_1 = 0;
unsigned char Stepper_5 = 0;
unsigned char Stepper_10 = 0;
unsigned char Stepper_25 = 0;
	
const unsigned char phase_01[4] = { 0x01, 0x02, 0x04, 0x08 };
const unsigned char phase_10[4] = { 0x10, 0x20, 0x40, 0x80 };
	
enum USART_States { USART_start, USART_init, USART_receiving };
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
			state = USART_receiving;
		break;
		
		case USART_receiving:
			state = USART_receiving;
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
		
		case USART_receiving:
			if(USART_HasReceived(0))
			{
				rec_tmp = USART_Receive(0);
				if ((rec_tmp & 0xC0) == 0x00)
				{
					Stepper_1 = rec_tmp & 0x3F;
				}
				else if ((rec_tmp & 0xC0) == 0x40)
				{
					Stepper_5 = rec_tmp & 0x3F;
				}
				else if ((rec_tmp & 0xC0) == 0x80)
				{
					Stepper_10 = rec_tmp & 0x3F;
				}
				else if ((rec_tmp & 0xC0) == 0xC0)
				{
					Stepper_25 = rec_tmp & 0x3F;
				}
			}
		break;
		
		default:
			state = USART_start;
		break;
	}
	return state;
}
	
enum StepperA_States { StepperA_start, StepperA_init, StepperA_wait, MoveA_01, MoveA_10, MoveA_11 };
int TickFct_StepperA (int state)
{
	static unsigned char i;
	static unsigned int cnt;
	switch(state) //Transitions
	{
		case StepperA_start:
			state = StepperA_init;
		break;
		
		case StepperA_init:
			state = StepperA_wait;
		break;
		
		case StepperA_wait:
			if (Stepper_1 > 0 && Stepper_5 > 0)
			{
				Stepper_1--;
				Stepper_5--;
				cnt = FULL;
				state = MoveA_11;
			}
			else if (Stepper_1 > 0)
			{
				Stepper_1--;
				cnt = FULL;
				state = MoveA_01;
			}
			else if (Stepper_5 > 0)
			{
				cnt = FULL;
				Stepper_5--;
				state = MoveA_10;
			}
			else
			{
				state = StepperA_wait;
			}
		break;
		
		case MoveA_01:
			if (cnt > 0)
			{
				cnt--;
				state = MoveA_01;
			} 
			else
			{
				state = StepperA_wait;
			}
		break;
		
		case MoveA_10:
			if (cnt)
			{
				cnt--;
				state = MoveA_10;
			}
			else
			{
				state = StepperA_wait;
			}
		break;
		
		case MoveA_11:
			if (cnt)
			{
				cnt--;
				state = MoveA_11;
			}
			else
			{
				state = StepperA_wait;
			}
		break;
		
		default:
			state = StepperA_start;
		break;
	}
	switch(state) //Actions
	{
		case StepperA_init:
			i = 3;
			cnt = 0;
		break;
		
		case StepperA_wait:
		
		break;
		
		case MoveA_01:
			/*for (i = 7; i >= 0; i--)
			{
				PORTB = phase_01[i];
				_delay_ms(1);
			}*/
			PORTB = phase_01[i];
			i = (i == 0) ? 3 : i - 1;
		break;
		
		case MoveA_10:
			/*for (i = 7; i >= 0; i--)
			{
				PORTB = phase_10[i];
				_delay_ms(1);
			}*/
			PORTB = phase_10[i];
			i = (i == 0) ? 3 : i - 1;
			//_delay_ms(1);
		break;
		
		case MoveA_11:
			/*for (i = 7; i >= 0; i--)
			{
				PORTB = phase_01[i] | phase_10[i];
				_delay_ms(1);
			}*/
			PORTB = phase_01[i] | phase_10[i];
			i = (i == 0) ? 3 : i - 1;
			//_delay_ms(1);
		break;
		
		default:
			state = StepperA_start;
		break;
	}
	return state;
}

enum StepperB_States { StepperB_start, StepperB_init, StepperB_wait, MoveB_01, MoveB_10, MoveB_11 };
int TickFct_StepperB (int state)
{
	static unsigned char i;
	static unsigned int cnt;
	switch(state) //Transitions
	{
		case StepperB_start:
			state = StepperB_init;
		break;
		
		case StepperB_init:
			state = StepperB_wait;
		break;
		
		case StepperB_wait:
			if (Stepper_10 > 0 && Stepper_25 > 0)
			{
				Stepper_10--;
				Stepper_25--;
				cnt = FULL;
				state = MoveB_11;
			}
			else if (Stepper_10 > 0)
			{
				Stepper_10--;
				cnt = FULL;
				state = MoveB_01;
			}
			else if (Stepper_25 > 0)
			{
				cnt = FULL;
				Stepper_25--;
				state = MoveB_10;
			}
			else
			{
				state = StepperB_wait;
			}
		break;
		
		case MoveB_01:
			if (cnt)
			{
				cnt--;
				state = MoveB_01;
			}
			else
			{
				state = StepperB_wait;
			}
		break;
		
		case MoveB_10:
			if (cnt)
			{
				cnt--;
				state = MoveB_10;
			}
			else
			{
				state = StepperB_wait;
			}
		break;
		
		case MoveB_11:
			if (cnt)
			{
				cnt--;
				state = MoveB_11;
			}
			else
			{
				state = StepperB_wait;
			}
		break;
		
		default:
			state = StepperB_start;
		break;
	}
	switch(state) //Actions
	{
		case StepperB_init:
			i = 3;
			cnt = 0;
		break;
		
		case StepperB_wait:
		
		break;
		
		case MoveB_01:
			/*for (i = 7; i >= 0; i--)
			{
				PORTC = phase_01[i];
				_delay_ms(1);
			}*/
			PORTA = phase_01[i];
			i = (i == 0) ? 3 : i - 1;
			//_delay_ms(1);
		break;
		
		case MoveB_10:
			/*for (i = 7; i >= 0; i--)
			{
				PORTC = phase_10[i];
				_delay_ms(1);
			}*/
			PORTA = phase_10[i];
			i = (i == 0) ? 3 : i - 1;
			//_delay_ms(1);
		break;
		
		case MoveB_11:
			/*for (i = 7; i >= 0; i--)
			{
				PORTC = phase_01[i] | phase_10[i];
				_delay_ms(1);
			}*/
			PORTA = phase_01[i] | phase_10[i];
			i = (i == 0) ? 3 : i - 1;
			//_delay_ms(1);
		break;
		
		default:
			state = StepperB_start;
		break;
	}
	return state;
}

int main(void)
{
     // initialize ports
     DDRA = 0xFF; PORTA = 0x00;
     DDRB = 0xFF; PORTB = 0x00;
     
     tasksNum = 3; // declare number of tasks
     task tsks[3]; // initialize the task array
     tasks = tsks; // set the task array
     
     // define tasks
     unsigned char i=0; // task counter
     tasks[i].state = USART_start;
     tasks[i].period = 50;
     tasks[i].elapsedTime = tasks[i].period;
     tasks[i].TickFct = &TickFct_USART;
     ++i;
     tasks[i].state = StepperA_start;
     tasks[i].period = 10;
     tasks[i].elapsedTime = tasks[i].period;
     tasks[i].TickFct = &TickFct_StepperA;
     ++i;
     tasks[i].state = StepperB_start;
     tasks[i].period = 10;
     tasks[i].elapsedTime = tasks[i].period;
     tasks[i].TickFct = &TickFct_StepperB;
     
     TimerSet(10); // value set should be GCD of all tasks
     TimerOn();

     while(1) {} // task scheduler will be called by the hardware interrupt
}

