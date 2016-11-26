/*	cmdset.h - 11/26/2016
 *	Name & E-mail: Joel Gomez - jgome043@ucr.edu
 *	CS Login: jgome043
 *	Partner(s) Name & E-mail:
 *	Lab Section: 022
 *  uC: ATmega1284P
 *	Description:
 *	Command set definitions for Growbot radio communications
 *	Currently supports up to 6 plants
 *
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

// Debug-related    01xx xxxx
#define DEBUG 0x40

// Routine-related  11xx xxxx
// Bits 5:0 determine which plants should be included in the routine
#define ROUTINE_START 0xC0

// Water-related    10ww 0ppp
// ww - water level for plant, ppp - plant (1-6)
// 001 - plant 1    100 plant 4
// 010 - plant 2    101 plant 5
// 011 - plant 3    110 plant 6
// 11 - High
// 10 - Med
// 01 - Low
#define WATER_SET 0x80
