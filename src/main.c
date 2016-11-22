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
#include "../../growbot/config.h"
#include "../../growbot/drivetrain.c"
#include "../../growbot/distance.c"

// Lib includes
#include "../../growbot/lib/nrf24L01_plus/gateway/radioPinFunctions.c"
#include "../../growbot/lib/nrf24L01_plus/nrf24.c"

#define PERIOD_DEMO 10

enum DemoState {DEMO_INIT,DEMO_WAIT} demo_state;

void DemoInit() {
  demo_state = DEMO_INIT;
}

void DemoTick() {
  /* static unsigned char demo_move; */
  static uint8_t data[1] = {0x05};
  /* rx demo */
  uint8_t tx_address[5] = {0xD7,0xD7,0xD7,0xD7,0xD7};
  uint8_t rx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};
  /* tx demo */
  /* uint8_t tx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7}; */
  /* uint8_t rx_address[5] = {0xD7,0xD7,0xD7,0xD7,0xD7}; */

  //Actions
  switch (demo_state) {
    case DEMO_INIT:
      /* demo_move = 0; */
      /* init hardware pins */
      nrf24_init();
      
      /* Channel #2 , payload length: 1 */
      nrf24_config(2,1);
   
      /* Set the device addresses */
      nrf24_tx_address(tx_address);
      nrf24_rx_address(rx_address);
      break;
    case DEMO_WAIT:
      /* demo_move = (demo_move == 1) ? 4 : 1; */
      /* MoveDirection(demo_move); */

      // nrf24 demo rx
      if(nrf24_dataReady())
      {
          nrf24_getData(data);        
          // light up LED
          if (data[0] == 0x05) {
            PORTB |= (1<<0);
          }
          else if (data[0] == 0x03) {
            PORTB &= ~(1<<0);
          }
      }
      
      /* // nrf24 demo tx */
      /* data[0] = (data[0] == 0x03) ? 0x05 : 0x03; */
      /* nrf24_send(data);   // Automatically goes to TX mode */
      /* while(nrf24_isSending()); // Wait for transmission to end */

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
  /* xTaskCreate(StepperDemoTask, (signed portCHAR *)"StepperDemoTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL ); */
  /* xTaskCreate(DistanceDemoTask, (signed portCHAR *)"DistanceDemoTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL ); */
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
