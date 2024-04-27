from queue import Queue
import serial
import json

def run_uart(data_queue: Queue, uart_queue: Queue, json_queue: Queue, info_queue: Queue, port: str):
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
        
        #convert rssi to distance
        dist_arr = []
        for rssi in data:
            distance = 1000 #base value to ignore
            if rssi != None:
                #may want to change 0
                distance = 10**((-60-rssi)/(10*2))
                distance = 5.5 if distance > 5.5 else distance
                distance = 0 if distance < 0 else distance
            dist_arr.append(distance)
        data_queue.put(dist_arr)