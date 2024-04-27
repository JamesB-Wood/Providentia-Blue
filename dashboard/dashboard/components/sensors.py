from queue import Queue
from collections import deque
import serial
import json

def start_sensor_thread(data_queue: deque, uart_queue: Queue, json_queue: Queue, info_queue: Queue, port: str):
    """
    UART thread for reading and writing
    """
    ser = serial.Serial(port, 115200, timeout=0.5)
    while True:
        #check if need to send anything 
        if not uart_queue.empty():
            to_send = uart_queue.get()
            cmd = to_send[0]
            to_send = to_send.encode('utf-8')
            ser.write(to_send)

            #if view wait for response
            if cmd == 'V':
                line = ser.readline()
                line = line.decode().replace(",", "\n")
                info_queue.put(line)
                # print(line)

        line = ser.readline()
        if len(line) == 0:
            continue
        data = json.loads(line.decode())
        json_queue.put(data)