/*	distance.c - 11/16/2016
 *	Name & E-mail: Joel Gomez - jgome043@ucr.edu
 *	CS Login: jgome043
 *	Partner(s) Name & E-mail:
 *	Lab Section: 022
 *	Description:
 *
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#define F_CPU 8000000UL // Assume uC operates at 8MHz
#include <util/delay.h>
/* #include "usart_ATmega1284.h"   // for troubleshooting */

#define DIST_REG  DDRD  // sensor DDR
#define DIST_PORT PORTD // sensor PORT
#define ECHO_PIN  2     // input to listen for echo ***DEPENDENT ON INT0***
#define TRIG_PIN  3     // output to start sensor polling
#define DIST_TRIGGER 5  // output pin to trigger when object detected

#define UNIT_IN 148   // uS/148 = inches
#define UNIT_CM 58    // uS/58 = centimeters
#define THRESH 15     // output triggered when distance less than this

enum DistanceState {DIS_INIT,DIS_WAIT} distance_state;

volatile int pulse = 0;
volatile char pulse_flag = 0;
volatile unsigned short distance;

ISR(INT0_vect) {
  if (pulse_flag == 1) {
    TCCR3B = 0;     // disable counter
    pulse = TCNT3;  // store counter memory
    TCNT3 = 0;      // reset counter memory
    pulse_flag = 0;
  }
  if (pulse_flag == 0) {
    TCCR3B |= (1<<CS31);  // enable counter
    pulse_flag = 1;
  }
}

void EnableDistance() {
  SREG |= (1<<7);       // enable global interrupts
  EIMSK |= (1<<INT0);   // enable external interrupt 0 (PD2)
  EICRA |= (1<<ISC00);  // set interrupt to trigger on logic change

  // set sensor trigger pin as output
  DIST_REG |= (1<<TRIG_PIN); DIST_PORT &= ~(1<<TRIG_PIN);
  // set sensor echo pin as input, enable pull-up
  DIST_REG &= ~(1<<ECHO_PIN); DIST_PORT |= (1<<ECHO_PIN);
  // set sensor output pin as output
  DIST_REG |= (1<<DIST_TRIGGER); DIST_PORT &= ~(1<<DIST_TRIGGER);
}

void DistanceInit() {
  distance_state = DIS_INIT;
  EnableDistance();
  /* initUSART(0); */
  distance = 255;
}

void DistanceTick() {
  static unsigned char dist_char;
  //Actions
  switch (distance_state) {
    case DIS_INIT:
      dist_char = 0;
      break;
    case DIS_WAIT:
      DIST_PORT |= (1<<TRIG_PIN);   // set trigger pin high
      _delay_us(15); 
      DIST_PORT &= ~(1<<TRIG_PIN);  // set trigger pin low
      distance = pulse/UNIT_CM;

      if (distance < THRESH) {
        DIST_PORT |= (1<<6);
      }
      else {
        DIST_PORT &= ~(1<<6);
      }
      /* if (USART_IsSendReady(0)) { */
      /*   dist_char = (distance > 255) ? 255 : distance; */
      /*   USART_Send(dist_char,0); */
      /* } */
      break;
    default:
      break;
  }
  //Transitions
  switch (distance_state) {
    case DIS_INIT:
      distance_state = DIS_WAIT;
      break;
    case DIS_WAIT:
      break;
    default:
      distance_state = DIS_INIT;
      break;
  }
}

void DistanceTask() {
  DistanceInit();
  for(;;)
  {
    DistanceTick();
    vTaskDelay(PERIOD_DISTANCE);
  }
}

