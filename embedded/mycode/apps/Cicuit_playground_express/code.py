from adafruit_circuitplayground import cp
import time
import board
import busio
import digitalio

flashing = True
brightness = (10/100) * 255

uart = busio.UART(board.TX, board.RX, baudrate=115200, timeout=0)

while True:
    
    data = uart.read(2)
    if data is not None:
        brightness = data[0] 
        flashing = data[1]
        
    if flashing:
        cp.pixels.fill((0, 0, 0))
        time.sleep(1)
    cp.pixels.fill((brightness, brightness, 0))
    time.sleep(1)
