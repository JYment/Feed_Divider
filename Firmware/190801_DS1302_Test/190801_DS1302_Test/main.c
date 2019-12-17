/*
 * 190801_DS1302_Test.c
 *
 * Created: 2019-08-01 오후 10:57:06
 * Author : JY
 */ 

#define F_CPU	16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define sbi(X, Y) (X |= ( 1 << Y))
#define cbi(X, Y) (X &= ~(1 << Y))

int main(void)
{
    sbi(DDRB, 5);
	
    while (1) 
    {
		sbi(PORTB, 5);
		_delay_ms(100);
		cbi(PORTB, 5);
		_delay_ms(100);		
    }
}

