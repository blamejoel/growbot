/*	[growbot] main.c - 11/16/2016
 *	Name & E-mail: Joel Gomez - jgome043@ucr.edu
 *	CS Login: jgome043
 *	Partner(s) Name & E-mail:
 *	Lab Section: 022
 *  uC: ATmega1284P
 *	Exercise Description:
 *
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/portpins.h>
#include <avr/pgmspace.h>

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"

// CS122A include files
/* #include "usart_ATmega1284.h" */

// Growbot include files
/* #include "../../growbot/config.h" */
#include "../../growbot/drivetrain.c"
#include "../../growbot/distance.c"
/* #include "../../growbot/nrf24l01.c" */

#define PERIOD_DEMO 100

enum DemoState {DEMO_INIT,DEMO_WAIT} demo_state;

/* void SendDebug(unsigned char dbug) { */
/*   if (dbug <= 255) { */
/*     USART_Send(dbug,0); */
/*   } */
/*   else { */
/*     USART_Send(dbug,0); */
/*   } */
/* } */

void DemoInit() {
  demo_state = DEMO_INIT;
  EnableDistance();
  /* initUSART(0); */
}

void DemoTick() {
  static unsigned char demo_move;
  static unsigned short distance;
  //Actions
  switch (demo_state) {
    case DEMO_INIT:
      demo_move = 1;
      break;
    case DEMO_WAIT:
      distance = PingCM();
      /* SendDebug(distance); */
      if (distance < 30 && distance > 7) {
        PORTD |= (1<<6);
        demo_move = 0;
      }
      else {
        PORTD &= ~(1<<6);
        demo_move = 1;
      }
      /* demo_move = (demo_move == 1) ? 4 : 1; */
      MoveDirection(demo_move);

      break;
    default:
      break;
  }
  //Transitions
  switch (demo_state) {
    case DEMO_INIT:
      demo_state = DEMO_WAIT;
      break;
    case DEMO_WAIT:
      break;
    default:
      demo_state = DEMO_INIT;
      break;
  }
}

void DemoTask() {
  DemoInit();
  for(;;)
  {
    DemoTick();
    vTaskDelay(PERIOD_DEMO);
  }
}

void StartSecPulse(unsigned portBASE_TYPE Priority) {
  xTaskCreate(DemoTask, (signed portCHAR *)"DemoTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
  xTaskCreate(StepperDemoTask, (signed portCHAR *)"StepperDemoTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
  /* xTaskCreate(DistanceDemoTask, (signed portCHAR *)"DistanceDemoTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL ); */
  /* xTaskCreate(nRFDemoTask, (signed portCHAR *)"nRFDemoTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL ); */
}

int main(void) {
  // Inputs, enable pull-up
  /* DDRB = 0x00; PORTB=0xFF; */
  // Outputs
  DDRA = 0xFF; PORTA=0x00;
  // 1111 1011  0000 0100
  /* DDRD = 0xFB; PORTD=0x04; */
  DDRD = 0xFF; PORTD=0x00;
  DDRB = 0xFF; PORTB=0x00;
  //Start Tasks
  StartSecPulse(1);
  //RunSchedular
  vTaskStartScheduler();

  return 0;
}
