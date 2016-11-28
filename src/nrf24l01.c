/*	nrf24l01.c - 11/22/2016
 *	Name & E-mail: Joel Gomez - jgome043@ucr.edu
 *	CS Login: jgome043
 *	Partner(s) Name & E-mail:
 *	Lab Section: 022
 *  uC: ATmega1284P
 *	Description:
 *  nRF24L01+ functions and Demo Task, includes nrf24L01+ lib functions from 
 *  https://github.com/kehribar/nrf24L01_plus
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#include <util/delay.h>

#define NRF_DDR   DDRB
#define NRF_PORT  PORTB
#define NRF_PIN   PINB
#define NRF_SCK   DDB7    // SPI Clock
#define NRF_MISO  DDB6    // SPI Master Input Slave Output
#define NRF_MOSI  DDB5    // SPI Master Output Slave Input
#define NRF_CSN   DDB4    // Chip Select Not  (same function as SS')
#define NRF_CE    DDB3    // Chip Enable

// Lib includes
/* nrf24L01+ library credit: https://github.com/kehribar/nrf24L01_plus */
#include "lib/nrf24L01_plus/radioPinFunctions.c"
#include "lib/nrf24L01_plus/nrf24.c"

#define NRF_CH  2   // nRF Channel
#define PL_SIZE 1   // payload size

unsigned char RadioDataReady() {
  return nrf24_dataReady();
}

unsigned char GetRadioData() {
  uint8_t data[PL_SIZE];
  if (nrf24_dataReady()) {
    nrf24_getData(data);
  }
  unsigned char data_char = data[0];
  return data_char;
}

uint8_t SendRadioData(uint8_t *data) {
  uint8_t status;
  nrf24_send(data);   // switch to tx mode and send
  while(nrf24_isSending()); // wait for tx to end
  status = nrf24_lastMessageStatus();
  nrf24_powerUpRx();  // switch to rx mode
  _delay_ms(10);      // wait a bit
  return status;
}

void InitRadio(uint8_t channel, uint8_t payload) {
  nrf24_init();
  nrf24_config(channel, payload);
}

void SetRadioAddress(uint8_t *address) {
  nrf24_rx_address(address);
}

void SetTransmitAddress(uint8_t *address) {
  nrf24_tx_address(address);
}


// Demo Task for FreeRTOS
#define PERIOD_NRF_DEMO 1000  // tx period
/* #define PERIOD_NRF_DEMO 10    // rx period */

enum NRFState {NRF_INIT,NRF_WAIT} nrf_state;

void nRFDemoRX() {
  unsigned char data;
  // light up LED
  data = GetRadioData();
  if (data == 0x05) {
    PORTB |= (1<<0);
  }
  else if (data == 0x03) {
    PORTB &= ~(1<<0);
  }
}

void nRFDemoTX() {
  static uint8_t data[1] = {0x05};
  data[0] = (data[0] == 0x03) ? 0x05 : 0x03;
  SendRadioData(data);   // Automatically goes to TX mode
  if (data[0] == 0x05) {
    PORTB |= (1<<0);
  }
  else if (data[0] == 0x03) {
    PORTB &= ~(1<<0);
  }
}

void nRFDemoInit() {
  nrf_state = NRF_INIT;
  /* rx demo */
  uint8_t tx_address[5] = {0xD7,0xD7,0xD7,0xD7,0xD7};
  uint8_t rx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};

  /* tx demo */
  /* uint8_t tx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7}; */
  /* uint8_t rx_address[5] = {0xD7,0xD7,0xD7,0xD7,0xD7}; */

  InitRadio(NRF_CH,PL_SIZE);

  /* Set the device addresses */
  SetTransmitAddress(tx_address);
  SetRadioAddress(rx_address);
}

void nRFDemoTick() {

  //Actions
  switch (nrf_state) {
    case NRF_INIT:
      break;
    case NRF_WAIT:
      nRFDemoRX();
      /* nRFDemoTX(); */
      break;
    default:
      break;
  }
  //Transitions
  switch (nrf_state) {
    case NRF_INIT:
      nrf_state = NRF_WAIT;
      break;
    case NRF_WAIT:
      break;
    default:
      nrf_state = NRF_INIT;
      break;
  }
}

void nRFDemoTask() {
  nRFDemoInit();
  for(;;)
  {
    nRFDemoTick();
    vTaskDelay(PERIOD_NRF_DEMO);
  }
}

