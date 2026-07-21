#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
volatile int water,inrx3;



void usart_receive_init(void) {
    UCSR3B = (1 << RXCIE3) | (1 << RXEN3);    // Enable receiver and RX complete interrupt
    UCSR3C = (1 << UCSZ31) | (1 << UCSZ30);   // Set data frame: 8 data bits, 1 stop bit
    UBRR3L = 51;                              // Set baud rate to 9600 bps
}



ISR(USART3_RX_vect) // Recieve
{
    inrx3++;
    water = UDR3;  // Receive data
}

int main()
 { 
   usart_receive_init();
   DDRF = 0xFF;
   sei();
   while (1)
   {
      if(water==1)
      {
	 PORTF = 0xFF;
      }
      else if(water==0)
      {
	 PORTF = 0;
      }
   }
   return 0;
 }