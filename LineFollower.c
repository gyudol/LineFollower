
#include <avr/io.h>			
#include <util/delay.h>		   
#include <avr/interrupt.h>		

#define MOTOR_PORT       PORTD	 
#define MOTOR_PORT_DDR   DDRD	  
#define SENSOR_PORT      PINC	  //To read the data from sensors

volatile unsigned char stepleft = 0, stepright = 0;
volatile int ck = 100;          // if you decrease this variable, it will move faster.
volatile int temp = 0; 
volatile char DIR = 'F'; 
				 	                      //'F' is for going straight.
                                //'R'is for turning right, L is for turning left.
                                // r is for turning 90 degrees right, l is for turning 90 degrees left
                  
volatile unsigned int timer = 0; 
volatile unsigned char sensor = 0x00; 
volatile char SIGNAL = 'F';
volatile int SIGNAL_time = 0;
volatile int StopSignal = 0;  


/* Stepping Motor derive---------------------------*/
unsigned char  LEFTmotorOneClock(unsigned char step, char dir)
{
   step = step & 0x0f;
   if (dir) {
      switch (step) {
      case 0x09: step = 0x01; break;
      case 0x01: step = 0x03; break;
      case 0x03: step = 0x02; break;
      case 0x02: step = 0x06; break;
      case 0x06: step = 0x04; break;
      case 0x04: step = 0x0c; break;
      case 0x0c: step = 0x08; break;
      case 0x08: step = 0x09; break;
      default: step = 0x0c; break;
      }
   }
   else {
      switch (step) {
      case 0x09: step = 0x08; break;
      case 0x01: step = 0x09; break;
      case 0x03: step = 0x01; break;
      case 0x02: step = 0x03; break;
      case 0x06: step = 0x02; break;
      case 0x04: step = 0x06; break;
      case 0x0c: step = 0x04; break;
      case 0x08: step = 0x0c; break;
      default: step = 0x0c; break;
      }
   }
   return step;

}


/* Stepping Motor derive---------------------------*/
unsigned char  RIGHTmotorOneClock(unsigned char step, char dir)
{
   step = step & 0xf0;
   if (dir) {
      switch (step) {
      case 0x90: step = 0x10; break;
      case 0x10: step = 0x30; break;
      case 0x30: step = 0x20; break;
      case 0x20: step = 0x60; break;
      case 0x60: step = 0x40; break;
      case 0x40: step = 0xc0; break;
      case 0xc0: step = 0x80; break;
      case 0x80: step = 0x90; break;
      default: step = 0xc0; break;
      }
   }
   else {
      switch (step) {
      case 0x90: step = 0x80; break;
      case 0x10: step = 0x90; break;
      case 0x30: step = 0x10; break;
      case 0x20: step = 0x30; break;
      case 0x60: step = 0x20; break;
      case 0x40: step = 0x60; break;
      case 0xc0: step = 0x40; break;
      case 0x80: step = 0xc0; break;
      default: step = 0xc0; break;
      }
   }
   return step;
}

void port_init(void)
{
   DDRA = 0xFF; 
   PORTA = 0x00;

   DDRC = 0x00;  
   PORTC = 0x00; 

   DDRD = 0xFF;  
   PORTD = 0x00; 

   DDRE = 0x0E;  
   PORTE = 0x30;  

   DDRF = 0xF0;  
   PORTF = 0x00; 

   DDRG = 0x00; 
}

// Ready to use interrupt of Atmega 128
void init_devices(void)
{
   cli();        //disable all interrupts
   XDIV = 0x00;  //xtal divider
   XMCRA = 0x00; //external memory
   port_init();   

   MCUCR = 0x00;
   EICRA = 0x00;
   EICRB = 0x0A; 
   EIMSK = 0x30; // we will use INT 4 and INT 5
   EIFR = 0x00;  // initiate 


   sei();  //re-enable interrupts
           //all peripherals are now initialized

}

// time overflow interrupt
void init_timer(void)
{
   TCCR0 = _BV(CS00);  // no prescailing  
   TCNT0 = 0x00; 
   TIMSK = 0x00;       // inactivate
}

int main(void)
{   

   init_devices();	            //initialize the device ports
   init_timer();	              //initialize the timer
   sensor = SENSOR_PORT & 0x0F;	// get the initial value from sensors
   MOTOR_PORT_DDR = 0xFF;		    

   temp = 0;
   timer = 0;	
   SIGNAL = 'F';	
   SIGNAL_time = 0;
   StopSignal = 0;
   
   PORTE=0x00;                //SEGMENT OFF.
   while (1)                  
   {
   }
}

//start with the switch "INT4"

ISR(INT5_vect) 
{
   cli();
   TIMSK = 0B00000001;     
   EIFR = 0x00;            
   sei();                  

   StopSignal = 0;         //if the line follower is stopped, reinitialize and restart
}

ISR(TIMER0_OVF_vect){
   temp++;                 //Count how many times the TIMER0 ISR is called.

   if (temp >= ck){
      sensor = SENSOR_PORT & 0x0F; 
      timer++;               

      temp = 0;                  

      switch (sensor) {

      case 0x00:          // 교차로  
       if ((timer - SIGNAL_time)<500)  
            DIR = SIGNAL;
        
       else DIR = 'F'; 
         break;

      case 0x01: //0001 : 교차로 앞  
         DIR = 'F';
         break;

      case 0x02:   //0010 
         DIR = 'L';
         SIGNAL = 'r'; 
         SIGNAL_time = timer;
         break;


      case 0x03:  //0011
         DIR = 'L';     
		     break;


      case 0x04: //0100
         DIR = 'R';
         SIGNAL = 'l';
         SIGNAL_time = timer;
         break;

      case 0x05: //0101
         DIR = 'R';
           SIGNAL = 'l';
		   SIGNAL_time = timer;
         break;

      case 0x06: //0110 
         DIR = 'F';
         StopSignal++;      
         if (StopSignal>93) //if the StopSignal is larger than 93, stop the device
                            // you have to adjust the value to adapt on your environment 
         {
            cli();			
            TIMSK = 0x00; 
            PORTD = 0x00; 
            sei();   //stop the line follower
         }
         break;

      case 0x07: //0111 
         DIR = 'F';
         SIGNAL = 'l';
         SIGNAL_time = timer;
         break;

      case 0x08: //1000 
         DIR = 'F';
         break;


      case 0x09:  //1001 
         DIR = 'F';
      
         break;

      case 0x0A: //1010 
         DIR = 'L';
         SIGNAL = 'r'; 
         SIGNAL_time = timer; 

         break;


      case 0x0B: //1011
         DIR = 'L';
    
         break;


      case 0x0C: //1100
         DIR = 'R';
      
         break;


      case 0x0D: //1101
         DIR = 'R';
      
         break;


      case 0x0E: //1110  
         DIR = 'F';
         SIGNAL = 'r';
         SIGNAL_time = timer;
      
         break;

      case 0x0F:  //1111
         DIR = 'F';
         break;

      default:
         DIR = 'F';
         break;
      }


      switch (DIR) { 

      case 'F': //  go forward 

         stepright = RIGHTmotorOneClock(stepright, 1);
         stepleft = LEFTmotorOneClock(stepleft, 0);
         MOTOR_PORT = stepleft | stepright; 
         PORTF = 0xF0;
         break;

      case 'L': // turn left

         stepleft = LEFTmotorOneClock(stepleft, 0); 
         MOTOR_PORT = stepleft | stepright; 
         break;

      case 'R': // turn right 

         stepright = RIGHTmotorOneClock(stepright, 1);
         MOTOR_PORT = stepleft | stepright; 
         break;

      case 'r': //turn 90 degrees right

         stepright = RIGHTmotorOneClock(stepright, 1);
         stepleft = LEFTmotorOneClock(stepleft, 1);
         
         MOTOR_PORT = stepleft | stepright; 
         break;

      case 'l': //turn 90 degrees left

         stepleft = LEFTmotorOneClock(stepleft, 0);
         stepright = RIGHTmotorOneClock(stepright, 0);
         
         MOTOR_PORT = stepleft | stepright; 
         break;
      }

   }
}
