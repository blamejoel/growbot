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
#include "usart_ATmega1284.h"

// Growbot include files
/* #include "../../growbot/config.h" */
#include "../../growbot/drivetrain.c"
#include "../../growbot/distance.c"
#include "../../growbot/nrf24l01.c"

/* void SendDebug(unsigned char dbug) { */
/*   if (dbug <= 255) { */
/*     USART_Send(dbug,0); */
/*   } */
/*   else { */
/*     USART_Send(dbug,0); */
/*   } */
/* } */

// Shared vars
static unsigned char ready;   // modified by radio, read by all
static unsigned char danger;  // modified by collision, read by drive
static unsigned char seek;    // modified by target, read by drive

// Misc defines
#define TARGET_REQUEST  0x11
#define TARGET_CENTERED 0x03
#define TARGET_MISSING  0xFE
#define TARGET_LEFT     0x01
#define TARGET_RIGHT    0x02

// Task Periods
#define PERIOD_COLLISION 150
#define PERIOD_DRIVE 100
#define PERIOD_RADIO 100
#define PERIOD_TARGET 100

// Task States
enum CollisionState {COLL_INIT,COLL_WAIT,COLL_FWD,COLL_FALL} collision_state;
enum DriveState {DRIVE_INIT,DRIVE_WAIT,DRIVE_FWD,DRIVE_BWD,DRIVE_AVOID,DRIVE_SEEK} drive_state;
enum RadioState {RADIO_INIT,RADIO_WAIT} radio_state;
enum AcqTargetState {TARGET_INIT,TARGET_WAIT,TARGET_SEEK,TARGET_ACQ,TARGET_LOST} target_state;

// Target Location
unsigned char RequestTargetStatus(unsigned char data_in) {
  unsigned char data;
  if (USART_HasReceived(0)) {
    data = USART_Receive(0);
  }
  else {
    data = data_in;
  }
  return data;
}
void TargetInit() {
  // TODO: Change init state back after testing!!!
  /* target_state = TARGET_INIT; */
  target_state = TARGET_ACQ;
  initUSART(0);
}

void TargetTick() {
  static unsigned char data;
  static unsigned char led = 0;
  //Actions
  switch (target_state) {
    case TARGET_INIT:
      data = 0;
      seek = 0;
      break;
    case TARGET_WAIT:
      break;
    case TARGET_SEEK:
      USART_Send(TARGET_REQUEST,0); // request target position
      data = RequestTargetStatus(data);
      seek = LEFT;
      break;
    case TARGET_ACQ:
      data = RequestTargetStatus(data);
      seek = STOP;
      PORTD |= (1<<5);
      break;
    case TARGET_LOST:
      data = RequestTargetStatus(data);
      if (data == TARGET_LEFT) {
        seek = LEFT;
      }
      else if (data == TARGET_RIGHT) {
        seek = RIGHT;
      }
      break;
    default:
      break;
  }
  //Transitions
  switch (target_state) {
    case TARGET_INIT:
      target_state = TARGET_WAIT;
      break;
    case TARGET_WAIT:
      break;
    case TARGET_SEEK:
      if (data == TARGET_CENTERED) {
        target_state = TARGET_ACQ;
      }
      else if (data == TARGET_LEFT || data == TARGET_RIGHT || 
          data == TARGET_MISSING) {
        target_state = TARGET_LOST;
      }
      break;
    case TARGET_ACQ:
      if (data == TARGET_LEFT || data == TARGET_RIGHT) {
        target_state = TARGET_LOST;
        PORTD &= ~(1<<5);
      }
      break;
    case TARGET_LOST:
      if (data == TARGET_CENTERED) {
        target_state = TARGET_ACQ;
      }
      break;
    default:
      target_state = TARGET_INIT;
      break;
  }
  // TODO: remove this test led stuff
  led = !led;
  if (led) {
    PORTB |= (1<<0);
  }
  else {
    PORTB &= ~(1<<0);
  }
}

void TargetTask() {
  TargetInit();
  for(;;)
  {
    TargetTick();
    vTaskDelay(PERIOD_TARGET);
  }
}

// Radio Communication
void RadioInit() {
  radio_state = RADIO_INIT;
  uint8_t tx_address[5] = {0xD7,0xD7,0xD7,0xD7,0xD7};
  uint8_t rx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};
  InitRadio(NRF_CH,PL_SIZE);
  SetTransmitAddress(tx_address);
  SetRadioAddress(rx_address);
}

void RadioTick() {
  static unsigned char data;
  const unsigned char START_ROUTINE = 0x01;
  const unsigned char STOP_ROUTINE = 0x00;
  //Actions
  switch (radio_state) {
    case RADIO_INIT:
      // TODO: Restore this data...
      data = START_ROUTINE;
      /* data = STOP_ROUTINE; */
      ready = 0;
      break;
    case RADIO_WAIT:
      if (RadioDataReady()) {
        data = GetRadioData();
      }
      if (data == START_ROUTINE) {
        ready = 1;
      }
      else if (data == STOP_ROUTINE) {
        ready = 0;
      }
      else if (data == 0x03) {
        PORTB |= (1<<0);
      }
      else if (data == 0x05) {
        PORTB &= ~(1<<0);
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

// Basic Movement
void DriveInit() {
  drive_state = DRIVE_INIT;
}

void DriveTick() {
  static unsigned char move;
  static unsigned char cnt;
  //Actions
  switch (drive_state) {
    case DRIVE_INIT:
      move = STOP;
      cnt = 0;
      break;
    case DRIVE_WAIT:
      move = STOP;
      break;
    case DRIVE_FWD:
      // TODO: remove this stop and uncomment FWD
      /* move = FWD; */
      move = STOP;
      break;
    case DRIVE_BWD:
      move = BWD;
      break;
    case DRIVE_AVOID:
      move = LEFT;
      break;
    case DRIVE_SEEK:
      move = seek;
      break;
    default:
      break;
  }
  //Transitions
  switch (drive_state) {
    case DRIVE_INIT:
      drive_state = DRIVE_WAIT;
      break;
    case DRIVE_WAIT:
      drive_state = (ready) ? DRIVE_FWD : drive_state;
      break;
    case DRIVE_FWD:
      if (!ready) {
        drive_state = DRIVE_WAIT;
      }
      else if (seek) {
        drive_state = DRIVE_SEEK;
      }
      else if (danger) {
        drive_state = DRIVE_BWD;
      }
      break;
    case DRIVE_BWD:
      if (!ready) {
        drive_state = DRIVE_WAIT;
      }
      else if (cnt > 2000/PERIOD_DRIVE) {
        cnt = 0;
        drive_state = DRIVE_AVOID;
      }
      else {
        cnt++;
      }
      break;
    case DRIVE_AVOID:
      if (!ready) {
        drive_state = DRIVE_WAIT;
      }
      else if (!danger && cnt > 4000/PERIOD_DRIVE) {
        cnt = 0;
        drive_state = DRIVE_FWD;
      }
      else {
        cnt++;
      }
      break;
    case DRIVE_SEEK:
      drive_state = (!seek) ? DRIVE_FWD : drive_state;
      break;
    default:
      drive_state = DRIVE_INIT;
      break;
  }
  MoveDirection(move);
}

void DriveTask() {
  DriveInit();
  for(;;)
  {
    DriveTick();
    vTaskDelay(PERIOD_DRIVE);
  }
}

// Collision Avoidance
void CollisionInit() {
  collision_state = COLL_INIT;
  EnableDistance();
}

void CollisionTick() {
  static unsigned short distance;
  static unsigned char cnt;
  //Actions
  switch (collision_state) {
    case COLL_INIT:
      distance = 0;
      cnt = 0;
      break;
    case COLL_WAIT:
      break;
    case COLL_FWD:
      distance = PingCM();
      /* SendDebug(distance); */
      if (distance < 40) {
        PORTD |= (1<<6);
        if (cnt > 400/PERIOD_COLLISION) {
          danger = 1;
          cnt = 0;
        }
        else {
          cnt++;
          danger = 0;
        }
      }
      else {
        danger = 0;
        PORTD &= ~(1<<6);
      }
      break;
    case COLL_FALL:
      break;
    default:
      break;
  }
  //Transitions
  switch (collision_state) {
    case COLL_INIT:
      collision_state = COLL_WAIT;
      break;
    case COLL_WAIT:
      collision_state = (ready) ? COLL_FWD : collision_state;
      break;
    case COLL_FWD:
      collision_state = (ready) ? COLL_FALL : COLL_WAIT;
      break;
    case COLL_FALL:
      collision_state = (ready) ? COLL_FWD : COLL_WAIT;
      break;
    default:
      collision_state = COLL_INIT;
      break;
  }
}

void CollisionTask() {
  CollisionInit();
  for(;;)
  {
    CollisionTick();
    vTaskDelay(PERIOD_COLLISION);
  }
}

void StartSecPulse(unsigned portBASE_TYPE Priority) {
  xTaskCreate(DriveTask, (signed portCHAR *)"DriveTask", 
      configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
  xTaskCreate(StepperDemoTask, (signed portCHAR *)"StepperDemoTask", 
      configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
  /* xTaskCreate(nRFDemoTask, (signed portCHAR *)"nRFDemoTask", */ 
  /*     configMINIMAL_STACK_SIZE, NULL, Priority, NULL ); */
  xTaskCreate(RadioTask, (signed portCHAR *)"RadioTask", 
      configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
  xTaskCreate(CollisionTask, (signed portCHAR *)"CollisionTask", 
      configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
  xTaskCreate(TargetTask, (signed portCHAR *)"TargetTask", 
      configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
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
