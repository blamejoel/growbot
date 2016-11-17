/*	jgome043_growbot.c - 11/16/2016
 *	Name & E-mail: Joel Gomez - jgome043@ucr.edu
 *	CS Login: jgome043
 *	Partner(s) Name & E-mail:
 *	Lab Section: 022
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
#include "../../growbot/config.h"
#include "../../growbot/drivetrain.c"
#include "../../growbot/distance.c"

#define PERIOD_DEMO 1000

enum DemoState {DEMO_INIT,DEMO_WAIT} demo_state;

void DemoInit() {
  demo_state = DEMO_INIT;
}

void DemoTick() {
  static unsigned char demo_move;
  static unsigned char pause;
  //Actions
  switch (demo_state) {
    case DEMO_INIT:
      demo_move = 0;
      pause = 1;
      break;
    case DEMO_WAIT:
      demo_move = (demo_move >= 3) ? 0 : demo_move + 1;
      if (ctrl % 2 == 0)
        PORTB |= (1<<0);
      else
        PORTB &= ~(1<<0);

      // pause before doing left/right
      /* if ((demo_move == 2) && pause) { */
      /*   demo_move = 0; */
      /*   pause = 0; */
      /* } */
      /* else if ((demo_move == 0 && !pause)) { */
      /*   demo_move = 3; */
      /* } */
      /* else { */
      /*   demo_move = (demo_move >= 4) ? 0 : demo_move + 1; */
      /* } */
      ctrl = demo_move;
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
      pause = (demo_move >= 4) ? 1 : pause;
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
  /* xTaskCreate(StepperTask, (signed portCHAR *)"StepperTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL ); */
  xTaskCreate(DistanceTask, (signed portCHAR *)"DistanceTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
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
