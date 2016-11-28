#!/usr/bin/env python3
import serial
import sys

# start serial port
usart = serial.Serial(
        port='/dev/serial0', 
        baudrate=9600, 
        parity=serial.PARITY_NONE, 
        stopbits=serial.STOPBITS_ONE, 
        bytesize=serial.EIGHTBITS,
        timeout=1
        )

# verify serial port is open
if not usart.is_open:
    usart.open()

# flush tx and rx buffers
usart.flushInput()
usart.flushOutput()

TARGET_CENTERED = b'\x03'
TARGET_MISSING  = b'\xFE'
TARGET_LEFT     = b'\x01'
TARGET_RIGHT    = b'\x02'

arg = ''
try:
    if sys.argv[1] is '1':
        usart.write(TARGET_LEFT)
        arg = TARGET_LEFT
    elif sys.argv[1] is '2':
        usart.write(TARGET_RIGHT)
        arg = TARGET_RIGHT
    elif sys.argv[1] is '3':
        usart.write(TARGET_CENTERED)
        arg = TARGET_CENTERED
except IndexError:
    arg = 'nothing.'
print('Sending {}'.format(arg))
usart.close()
