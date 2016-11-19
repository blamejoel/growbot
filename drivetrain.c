/*	drivetrain.c - 11/12/2016
 *	Name & E-mail: Joel Gomez - jgome043@ucr.edu
 *	CS Login: jgome043
 *	Partner(s) Name & E-mail:
 *	Lab Section: 022
 *  uC: ATmega1284P
 *	Description:
 *
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

// Stepper Phases
#define A1 0x01
#define B1 0x02
#define C1 0x04
#define D1 0x08
#define A2 0x10
#define B2 0x20
#define C2 0x40
#define D2 0x80
#define STEP_ANGLE 5.625
#define PHASES_PER_STEP 64

// Movement defines
#define STOP  0
#define FWD   1
#define BWD   2
#define LEFT  3
#define RIGHT 4

// Set movement - 1:fwd, 2:bwd, 3:left, 4:right;
unsigned char MoveDirection(unsigned char cmd) {
  static unsigned char ctrl;
  if (cmd == 0) {
    ctrl = cmd;
  }
  else if (cmd && (cmd == 0x01 || cmd == 0x02 || cmd == 0x03 || cmd == 0x04)) {
    ctrl = cmd;
  }
  return ctrl;
}

// Demo Task for FreeRTOS
#define PERIOD_STEPPER_DEMO  3

enum StepperState {STEPPER_INIT,STEPPER_WAIT,STEPPER_FWD,STEPPER_BWD,
  STEPPER_L,STEPPER_R} stepper_state;

void SetState() {
  unsigned char ctrl = MoveDirection(-1);
  if (ctrl == FWD) {
    stepper_state = STEPPER_FWD;
  }
  else if (ctrl == BWD) {
    stepper_state = STEPPER_BWD;
  }
  else if (ctrl == LEFT) {
    stepper_state = STEPPER_L;
  }
  else if (ctrl == RIGHT) {
    stepper_state = STEPPER_R;
  }
  else {
    stepper_state = STEPPER_WAIT;
  }
}

void StepperDemoInit() {
  stepper_state = STEPPER_INIT;
}

void StepperDemoTick() {
  static unsigned char i;
  static unsigned char j;
  static unsigned char totalPhases;
  // Half Step
  /* const unsigned char m1Phase[] = {A1, A1|B1, B1, B1|C1, C1, C1|D1, D1}; */
  /* const unsigned char m2Phase[] = {A2, A2|B2, B2, B2|C2, C2, C2|D2, D2}; */

  // Full Step
  const unsigned char m1Phase[] = {A1|B1,B1|C1,C1|D1,D1|A1};
  const unsigned char m2Phase[] = {A2|B2,B2|C2,C2|D2,D2|A2};

  //Actions
  switch (stepper_state) {
    case STEPPER_INIT:
      MoveDirection(STOP);
      i = 0;
      j = 0;
      totalPhases = sizeof(m1Phase)/sizeof(m1Phase[0]);
      break;
    case STEPPER_FWD:
      // clockwise
      PORTA = m1Phase[i] | m2Phase[j];
      // counter-clockwise
      i = (i == totalPhases-1) ? 0 : i + 1;
      // clockwise
      j = (j == 0) ? totalPhases-1 : j - 1;
      break;
    case STEPPER_BWD:
      PORTA = m1Phase[i] | m2Phase[j];
      // counter-clockwise
      j = (j == totalPhases-1) ? 0 : j + 1;
      // clockwise
      i = (i == 0) ? totalPhases-1 : i - 1;
      break;
    case STEPPER_L:
      // clockwise
      PORTA = m1Phase[i] | m2Phase[i];
      i = (i == 0) ? totalPhases-1 : i - 1;
      break;
    case STEPPER_R:
      // counter-clockwise
      PORTA = m1Phase[i] | m2Phase[i];
      i = (i == totalPhases-1) ? 0 : i + 1;
      break;
    default:
      break;
  }
  //Transitions
  switch (stepper_state) {
    case STEPPER_INIT:
      stepper_state = STEPPER_WAIT;
      break;
    case STEPPER_WAIT:
      SetState();
      break;
    case STEPPER_FWD:
      SetState();
      break;
    case STEPPER_BWD:
      SetState();
      break;
    case STEPPER_L:
      SetState();
      break;
    case STEPPER_R:
      SetState();
      break;
    default:
      stepper_state = STEPPER_INIT;
      break;
  }
}

void StepperDemoTask() {
  StepperDemoInit();
  for(;;)
  {
    StepperDemoTick();
    vTaskDelay(PERIOD_STEPPER_DEMO);
  }
}

