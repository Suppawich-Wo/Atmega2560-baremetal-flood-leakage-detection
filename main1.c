/*
 * ===================================================================
 * Project: Bare-Metal Flood & Electrical Leakage Detection (Transmitter)
 * Target MCU: ATmega2560 @ 16MHz
 * 
 * HARDWARE PIN MAPPING:
 * - Ultrasonic Trig  : PB0 (Digital Output)
 * - Ultrasonic Echo  : PB1 / PCINT0 (Interrupt Input)
 * - ACS712 Sensor    : ADC0 / PF0 (Analog Input)
 * - Serial Tx        : TX3 / PJ1 (Connect to Receiver RX3)
 * - Buzzer           : PA0
 * - Status LEDs	  : PK0 - PK3
 * ===================================================================
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>

volatile float Vout,Ith=4,Iin,adcvalue;
volatile uint8_t water20,water60,leak,i,timer_flag;
volatile unsigned long final_count;
volatile float duration_ms,nominal_duration_ms=5.8,delta_time_ms,dept_cm,pureairdistcm,pcintup,pcintdn,inusart3;


void acs712init()
{
   ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0) | (1<<ADIE) | (1<<ADSC); // Set freq prescaler 128
   ADMUX = (1<<REFS0); 
}

void interrupt15ms_setup(void) {
    TCCR2A = (1 << WGM21); // CTC mode
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // Prescaler 1024
    TIMSK2 = (1 << OCIE2A); // Enable Output Compare Match A interrupt
    OCR2A = 235; // Set match for 15ms delay
}

void pcint_init()
{
     DDRB = 0x01;
     DDRB &= ~(1 << PB1);
     PCICR = (1<<PCIE0);
     PCMSK0 = (1 << PCINT1); 
}

void usart_transmit_init(void) {
    UCSR3B = (1 << TXEN3) | (1 << UDRIE3);               // Enable transmitter
    UCSR3C = (1 << UCSZ31) | (1 << UCSZ30);  // Set data frame: 8 data bits, 1 stop bit
    UBRR3L = 51;                         // Set baud rate to 9600 bps
}


ISR(ADC_vect) 
{
   adcvalue = ADC;
   Vout = (adcvalue/1024)*5;
   Iin = (Vout-2.5)*5;
   if(fabs(Iin-Ith)>0.2)
   {
      leak=1;
   }
   else if(fabs(Iin-Ith)<0.2)
   {
      leak=0;
   }
   ADCSRA |= (1 << ADSC); // Start another conversion
}

ISR(TIMER2_COMPA_vect) {
    PORTB = 0x01;
    _delay_us(10);
    PORTB = 0;  
}

ISR(PCINT0_vect) 
{
    if ((PINB&0x02)==0x02) // Start timer
    { 
      pcintup++;
      TCCR1B |= (1 << CS11);
      TCNT1 = 0;   
    }
    else if ((PINB&0x02)==0) // Stop timer
    { 
      pcintdn++;
      TCCR1B &= ~(1 << CS11);      // Stop Timer1
      final_count = TCNT1;         // Store final TCNT1 value
      timer_flag = 1;
      duration_ms = (0.5*final_count)/1000; // Time in ms
      delta_time_ms = duration_ms - nominal_duration_ms; 
    }
}

void timer1_init() {
    TCCR1A = 0; 
    TCCR1B = 0;    
    TCCR1B |= (1 << CS11); // prescaler 8
    TCNT1 = 0; 
}

ISR(USART3_UDRE_vect) // Send to another board
{
    inusart3++;
    if((water20==1)||(water60==1))
    {
      UDR3 = 0b01;  
    }
    else if((water20==0)&&(water60==0))
    {
      UDR3 = 0b00;  
    }
}

int main(void) 
{
   acs712init();
   interrupt15ms_setup();
   DDRA = 0x01;
   DDRK = 0x0F;
   pcint_init();
   usart_transmit_init() ;
   //leak=1;
   //water20=1;
   //water60=1;
   sei();
   while (1) 
   {
      if((water20==1) && (water60==0)) // Led
      {
	 PORTK = 0b00000101;
	 _delay_ms(50);
	 PORTK = 0b00001010;
	 _delay_ms(50);
	 PORTK = 0b00000100;
	 _delay_ms(50);
	 PORTK = 0b00001000;
	 _delay_ms(50);
	 PORTK = 0;
	 _delay_ms(50);
      }
      else if((water20==0) && (water60==1)) // Buzzer
      {
	 for(i=0;i<2;i++)
	 {
	    PORTA = 0x01;
	    _delay_ms(200);
	    PORTA = 0;
	    _delay_ms(200);
	 }
      }
      else if((water20==0) && (water60==0)) 
      {
	 PORTA = 0;
	 PORTK = 0;
      }
      if(leak==1) // LED + Buzzer
      {
	 for(i=0;i<10;i++)
	 {
	    PORTA = 0x01;
	    PORTK = 0x0F;
	    _delay_ms(50);
	    PORTA = 0;
	    PORTK = 0;
	    _delay_ms(50);
	 }
	 
      }
      if (timer_flag == 1) 
      {
	  timer_flag = 0;
	  pureairdistcm = (34.32*duration_ms)/2;
	  if(delta_time_ms<0) // Takes shorter time or have water
	  {
	     dept_cm = (duration_ms-5.82775)/(-0.0448);
	  }
	  if((dept_cm>20)&(dept_cm<60))
	  {
	     water20 = 1;
	     water60 = 0;
	  }
	  else if((dept_cm>60))
	  {
	     water20 = 0;
	     water60 = 1;
	  }
	  else if((dept_cm<20))
	  {
	     water20 = 0;
	     water60 = 0;
	  }
      }
   }
   return 0;
}
