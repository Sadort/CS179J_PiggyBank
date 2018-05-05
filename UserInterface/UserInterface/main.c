/*
 * UserInterface.c
 *
 * Created: 2018/4/25 0:59:18
 * Author : 黄俊杰
 */ 

#include <delay.h>
#include <avr/io.h>
#include <scheduler.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "lcd.h"
#include "keypad.h" //'A' Dispense, 'B' Enter, 'C' Back
#include <usart_ATmega1284.h>

#define D0 eS_PORTA0
#define D1 eS_PORTA1
#define D2 eS_PORTA2
#define D3 eS_PORTA3
#define D4 eS_PORTA4
#define D5 eS_PORTA5
#define D6 eS_PORTA6
#define D7 eS_PORTA7
#define RS eS_PORTB6
#define EN eS_PORTB7

#define WAITTIME 60

//global variables
unsigned char Coin_1 = 0;
unsigned char Coin_5 = 0;
unsigned char Coin_10 = 0;
unsigned char Coin_25 = 0;
unsigned char Coin_1_D = 0; //Coins need to be dispense
unsigned char Coin_5_D = 0;
unsigned char Coin_10_D = 0;
unsigned char Coin_25_D = 0;
unsigned char cnt = 0;
unsigned char typing = 0; // 0 doesn't need inputs, 1 needs to collect inputs
unsigned char dispensing = 0; // 0 not dispensing, 1 dispensing
unsigned char tmp_key = 0; // collecting inputs
unsigned char num = 0; // store the length of inputs
char input[9];

//stored strings
char Show_Coins_R1[11] = "1c:   10c:";
char Show_Coins_R2[11] = "5c:   25c:";
char Show_Dispense[17] = "Dispense Change:";
char Show_Dispensing[14] = "   Dispensing";
char No_Coins[17] = "No Enough Coins!";

unsigned char DispenseDecision() // 1 Dispensing, 2 Not Enough
{
	int INPUT = atoi(input) * 100;
	if ((Coin_1*1+Coin_5*5+Coin_10*10+Coin_25*25) < INPUT)
	{
		return 2;
	} 
	else
	{
		while ((INPUT >= 25 && Coin_25 > Coin_25_D) || ((Coin_1*1+Coin_5*5+Coin_10*10) < INPUT))
		{
			Coin_25_D++;
			INPUT = INPUT - 25;
		}
		while ((INPUT >= 10 && Coin_10 > Coin_10_D) || ((Coin_1*1+Coin_5*5) < INPUT))
		{
			Coin_10_D++;
			INPUT = INPUT - 10;
		}
		while ((INPUT >= 5 && Coin_5 > Coin_5_D) || ((Coin_1*1) < INPUT))
		{
			Coin_5_D++;
			INPUT = INPUT - 5;
		}
		while (INPUT >= 1 && Coin_1 > Coin_1_D)
		{
			Coin_1_D++;
			INPUT = INPUT - 1;
		}
		Coin_25 = Coin_25 - Coin_25_D;
		Coin_10 = Coin_10 - Coin_10_D;
		Coin_5 = Coin_5 - Coin_5_D;
		Coin_1 = Coin_1 - Coin_1_D;
		return 1;
	}
	
}

enum Keypad_States { Keypad_start, Keypad_init, GetInput_1, GetInput_2 };
int TickFct_Keypad(int state) {
	switch(state) { //Transitions
		case  Keypad_start:
			state = Keypad_init;
		break;
		
		case Keypad_init:
			state = GetInput_1;
		break;
		
		case GetInput_1:
			if (typing == 1)
			{
				tmp_key = GetKeypadKey();
				if (tmp_key <= '9' && tmp_key >= '0')
				{
					state = GetInput_2;
				}
			} 
			else
			{
				state = GetInput_1;
			}
		break;
		
		case GetInput_2:
			if (tmp_key == GetKeypadKey())
			{
				state = GetInput_2;
			}
			else
			{
				if (strlen(input)<=3)
				{
					num = strlen(input);
					input[num] = tmp_key;
					input[num+1] = '\0';
				}
				tmp_key = 0;
				state = GetInput_1;
			}
		break;
		
		default:
			state = Keypad_start;
		break;
	}
	switch(state) { //Actions
		case Keypad_init:
			memset(input, 0, strlen(input));
		break;
		
		case GetInput_1:
			
		break;
		
		case GetInput_2:
		
		break;
		
		default:
			state = Keypad_start;
		break;
	}
	return state;
}

enum USART_States { USART_start, USART_init, USART_receiving, USART_sending };
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
			if (Coin_1_D > 0 || Coin_5_D > 0 || Coin_10_D > 0 || Coin_25_D > 0)
			{
				state = USART_sending;
			} 
			else
			{
				state = USART_receiving;
			}
		break;
		
		case USART_sending:
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
				if (rec_tmp == 0x01)
				{
					Coin_1++;
				}
				else if (rec_tmp == 0x11)
				{
					Coin_5++;
				}
				else if (rec_tmp == 0x21)
				{
					Coin_10++;
				}
				else if (rec_tmp == 0x31)
				{
					Coin_25++;
				}
			}
		break;
		
		case USART_sending:
			if (USART_IsSendReady(0))
			{
				if (Coin_1_D > 0)
				{
					sed_tmp = (0x3F & Coin_1_D) | 0x00;
					Coin_1_D = 0;
					USART_Send(sed_tmp, 0);
				}
				else if (Coin_5_D > 0)
				{
					sed_tmp = (0x3F & Coin_5_D) | 0x40;
					Coin_5_D = 0;
					USART_Send(sed_tmp, 0);
				}
				else if (Coin_10_D > 0)
				{
					sed_tmp = (0x3F & Coin_10_D) | 0x80;
					Coin_10_D = 0;
					USART_Send(sed_tmp, 0);
				}
				else if (Coin_25_D > 0)
				{
					sed_tmp = (0x3F & Coin_25_D) | 0xC0;
					Coin_25_D = 0;
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

enum LCD_States { LCD_start, LCD_init, ShowCoins, ShowDispense, ShowDispensing, NoCoins };
int TickFct_LCD(int state) {
	switch(state) { // Transitions
		case LCD_start: // Initial transition
			state = LCD_init;
		break;
		
		case LCD_init:
			Lcd8_Clear();
			state = ShowCoins;
		break;
		
		case ShowCoins:
			if (GetKeypadKey() == 'A') //Back
			{
				Lcd8_Clear();
				typing = 1;
				state = ShowDispense;
			} 
			else
			{
				state = ShowCoins;
			}
		break;
		
		case ShowDispense:
			if (GetKeypadKey() == 'B') //Enter
			{
				if (DispenseDecision() == 1) //Dispensing
				{
					memset(input, 0, strlen(input));
					cnt = 0;
					Lcd8_Clear();
					typing = 0;
					dispensing = 1;
					state = ShowDispensing;
				} 
				else //Not enough
				{
					memset(input, 0, strlen(input));
					cnt = 0;
					Lcd8_Clear();
					typing = 0;
					state = NoCoins;
				}
			} 
			else if (GetKeypadKey() == 'C') //Back
			{
				memset(input, 0, strlen(input));
				Lcd8_Clear();
				state = ShowCoins;
			}
			else
			{
				state = ShowDispense;
			}
		break;
		
		case ShowDispensing:
			if (cnt < WAITTIME)
			{
				cnt++;
				state = ShowDispensing;
			} 
			else
			{
				Lcd8_Clear();
				state = ShowCoins;
			}
		break;
		
		case NoCoins:
			if (cnt < WAITTIME)
			{
				cnt++;
				state = NoCoins;
			}
			else
			{
				Lcd8_Clear();
				typing = 1;
				state = ShowDispense;
			}
		break;
		
		default:
		state = LCD_start;
		break;
	}

	switch(state) { // State actions
		case LCD_init:
			Lcd8_Init();
		break;
		
		case ShowCoins:
			//ROW1
			Lcd8_Set_Cursor(1,0);
			Lcd8_Write_String(Show_Coins_R1);
			Lcd8_Set_Cursor(1,3);
			Lcd8_Write_Char((Coin_1/10)+'0');
			Lcd8_Set_Cursor(1,4);
			Lcd8_Write_Char((Coin_1%10)+'0');
			Lcd8_Set_Cursor(1,10);
			Lcd8_Write_Char((Coin_10/10)+'0');
			Lcd8_Set_Cursor(1,11);
			Lcd8_Write_Char((Coin_10%10)+'0');
			
			//ROW2
			Lcd8_Set_Cursor(2,0);
			Lcd8_Write_String(Show_Coins_R2);
			Lcd8_Set_Cursor(2,3);
			Lcd8_Write_Char((Coin_5/10)+'0');
			Lcd8_Set_Cursor(2,4);
			Lcd8_Write_Char((Coin_5%10)+'0');
			Lcd8_Set_Cursor(2,10);
			Lcd8_Write_Char((Coin_25/10)+'0');
			Lcd8_Set_Cursor(2,11);
			Lcd8_Write_Char((Coin_25%10)+'0');
		break;
		
		case ShowDispense:
			Lcd8_Set_Cursor(1,0);
			Lcd8_Write_String(Show_Dispense);
			Lcd8_Set_Cursor(2,0);
			Lcd8_Write_Char('$');
			Lcd8_Set_Cursor(2,1);
			Lcd8_Write_String(input);
		break;
		
		case ShowDispensing:
			Lcd8_Set_Cursor(1,0);
			Lcd8_Write_String(Show_Dispensing);
		break;
		
		case NoCoins:
			Lcd8_Set_Cursor(1,0);
			Lcd8_Write_String(No_Coins);
		break;
		
		default:
		state = LCD_start;
		break;
	}
	
	return state;
}

int main(void)
{
    // initialize ports
    DDRA = 0xFF; PORTA = 0x00;
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0xF0; PORTC = 0x0F;
    
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
    tasks[i].state = Keypad_start;
    tasks[i].period = 50;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Keypad;
	++i;
	tasks[i].state = LCD_start;
	tasks[i].period = 50;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_LCD;
    
    
    TimerSet(50); // value set should be GCD of all tasks
    TimerOn();

    while(1) {} // task scheduler will be called by the hardware interrupt
}

