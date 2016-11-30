/*	[growbot] main-ms.c - 11/16/2016
 *	Name & E-mail: Joel Gomez - jgome043@ucr.edu
 *	CS Login: jgome043
 *	Partner(s) Name & E-mail:
 *	Lab Section: 022
 *  uC: ATmega1284P
 *	Exercise Description: RF Master
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
#include "../../growbot/nrf24l01.c"

/* void SendDebug(unsigned char dbug) { */
/*   if (dbug <= 255) { */
/*     USART_Send(dbug,0); */
/*   } */
/*   else { */
/*     USART_Send(dbug,0); */
/*   } */
/* } */

// Task Periods
#define PERIOD_RADIO 100

// Task States
enum RadioState {RADIO_INIT,RADIO_WAIT,RADIO_REQ} radio_state;

// Radio Communication
void RadioInit() {
  radio_state = RADIO_INIT;
  uint8_t rx_address[5] = {0xD7,0xD7,0xD7,0xD7,0xD7};
  uint8_t tx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};
  InitRadio(NRF_CH,PL_SIZE);
  SetTransmitAddress(tx_address);
  SetRadioAddress(rx_address);
}

void RadioTick() {
  static unsigned char data[1];
  static unsigned char low_water;
  const unsigned char LOW_WATER = 0x80;
  const unsigned char START_ROUTINE = 0x01;
  const unsigned char STOP_ROUTINE = 0x02;
  unsigned char btn = (~PIND & 0x80);
  //Actions
  switch (radio_state) {
    case RADIO_INIT:
      data[0] = STOP_ROUTINE;
      low_water = 0;
      break;
    case RADIO_WAIT:
      if (RadioDataReady()) {
        unsigned char incoming = GetRadioData();
        low_water = (incoming == LOW_WATER) ? 1 : 0;
      }
      if (low_water) {
        PORTA |= (1<<1);
      }
      else {
        PORTA &= ~(1<<1);
      }
      break;
    default:
      break;
  }
  //Transitions
  switch (radio_state) {
    case RADIO_INIT:
      radio_state = RADIO_WAIT;
      break;
    case RADIO_WAIT:
      if (btn && data[0] == STOP_ROUTINE) {
        data[0] = START_ROUTINE;
        SendRadioData(data);
        PORTA |= (1<<0);
      }
      else if (btn && data[0] == START_ROUTINE) {
        data[0] = STOP_ROUTINE;
        SendRadioData(data);
        PORTA &= ~(1<<0);
      }
      radio_state = (btn) ? RADIO_REQ : radio_state;
      break;
    case RADIO_REQ:
      radio_state = (!btn) ? RADIO_WAIT : radio_state;
      break;
    default:
      radio_state = RADIO_INIT;
      break;
  }
}

void RadioTask() {
  RadioInit();
  for(;;)
  {
    RadioTick();
    vTaskDelay(PERIOD_RADIO);
  }
}

void StartSecPulse(unsigned portBASE_TYPE Priority) {
  xTaskCreate(RadioTask, (signed portCHAR *)"RadioTask", 
      configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void) {
  // Inputs, enable pull-up
  DDRD = 0x00; PORTD=0xFF;
  // Outputs
  DDRA = 0xFF; PORTA=0x00;
  DDRB = 0xFF; PORTB=0x00;
  //Start Tasks
  StartSecPulse(1);
  //RunSchedular
  vTaskStartScheduler();

  return 0;
}
