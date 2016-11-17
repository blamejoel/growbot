/*	ir.c - 11/16/2016
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

#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))

#include "pwm.c"

enum IRState {IR_INIT,IR_WAIT} ir_state;

void InfraInit() {
  ir_state = IR_INIT;
}

void InfraTick() {
  static unsigned char out;
  //Actions
  switch (ir_state) {
    case IR_INIT:
      PWM_on();
      set_PWM(38200);
      out = 0;
      break;
    case IR_WAIT:
      out = (~PINB & 0x01) ? 0x40 : 0;
      if (~PINB & 0x01) {
        SET_BIT(PORTD,6);
      }
      else {
        CLR_BIT(PORTD,6);
      }
      break;
    default:
      break;
  }
  //Transitions
  switch (ir_state) {
    case IR_INIT:
      ir_state = IR_WAIT;
      break;
    case IR_WAIT:
      break;
    default:
      ir_state = IR_INIT;
      break;
  }
}

void InfraTask() {
  InfraInit();
  for(;;)
  {
    InfraTick();
    vTaskDelay(PERIOD_IR);
  }
}

