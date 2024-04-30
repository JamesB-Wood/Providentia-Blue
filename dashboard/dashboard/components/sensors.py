from collections import deque
from dataclasses import dataclass
import serial
import json

INIT_GRAPH_DATA = [
    {
        "label": "TVOC",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "blue",
        "yAxisID": "y1",
    },
    {
        "label": "CO2",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "purple",
        "yAxisID": "y1",
    },
    {
        "label": "Humidity",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "green",
        "yAxisID": "y2",
    },
    {
        "label": "Temp",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "red",
        "yAxisID": "y3",
    },
    {
        "label": "Light",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "magenta",
        "yAxisID": "y4",
    },
]


# Class for storing data with defaults
@dataclass
class SensorState:
    tvoc: int = 0
    co2: int = 300
    humidity: float = 50.4
    temp: float = 24.5
    lux: int = 200


flower_state = {
    "brightness": 50,
    "openness": 0,
    "flashing": False,
}


def calculate_flower_state(data_queue: deque[SensorState], command_queue: deque):

    global flower_state

    # perform filtering on the data

    # send command to queue if required
    json.dumps(flower_state).encode()

    # return states
    return flower_state


def start_sensor_thread(
    data_queue: deque[SensorState], command_queue: deque, port: str
):
    """
    UART thread for reading and writing
    """
    # init data in data queue
    data_queue.append(SensorState())

    ser = serial.Serial(port, 115200, timeout=0.5)
    while True:
        # check if need to send anything
        while len(command_queue) != 0:
            to_send = command_queue.popleft()
            to_send = to_send.encode("utf-8")
            ser.write(to_send)

        line = ser.readline()

        if len(line) == 0:
            continue

        json_data = json.loads(line.decode())

        data = SensorState(
            json_data["TVOC"],
            json_data["CO2"],
            json_data["HUMID"],
            json_data["TEMP"],
            json_data["LUX"],
        )
        data_queue.append(data)
