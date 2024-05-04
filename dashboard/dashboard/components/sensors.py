from collections import deque
from dataclasses import dataclass
import serial
import json

INIT_GRAPH_DATA = [
    {
        "label": "tvoc",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "blue",
        "yAxisID": "y1",
    },
    {
        "label": "co2",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "purple",
        "yAxisID": "y1",
    },
    {
        "label": "humidity",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "green",
        "yAxisID": "y2",
    },
    {
        "label": "temp",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "red",
        "yAxisID": "y3",
    },
    {
        "label": "average_ir_temp",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "magenta",
        "yAxisID": "y3",
    },
]


# Class for storing data with defaults
@dataclass
class SensorState:
    ir_vals: list[float]
    tvoc: int = 0
    co2: int = 300
    humidity: float = 50.4
    temp: float = 24.5
    average_ir_temp: float = 25.5

    def as_dict(self) -> dict:
        return {
            "tvoc": self.tvoc,
            "co2": self.co2,
            "humidity": self.humidity,
            "temp": self.temp,
            "average_ir_temp": self.average_ir_temp,
        }


@dataclass
class ControlState:
    brightness: int = 50
    openness: int = 100
    flashing: bool = False

    def to_json(self) -> str:
        as_dict = {
            "brightness": self.brightness,
            "openness": self.openness,
            "flashing": self.flashing,
        }
        return json.dumps(as_dict)


flower_state = ControlState()


def calculate_flower_state(
    data_queue: deque[SensorState], command_queue: deque[ControlState]
):

    global flower_state
    new_state = ControlState()

    # perform filtering on the data

    # send command to queue if required
    if new_state != flower_state:
        command_queue.append(new_state)
        flower_state = new_state

    # return states
    return flower_state


def start_sensor_thread(
    data_queue: deque[SensorState], command_queue: deque[ControlState], port: str
):
    """
    UART thread for reading and writing
    """
    # init data in data queue
    data_queue.append(SensorState([
        [5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0,],
        [10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0,],
        [15.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0,],
        [20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0,],
        [25.0, 25.0, 25.0, 25.0, 25.0, 25.0, 25.0, 25.0,],
        [30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0,],
        [35.0, 35.0, 35.0, 35.0, 35.0, 35.0, 35.0, 35.0,],
        [40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0,],
    ]))

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

        grid_data = json_data["ir_grid"]
        grid_data = [item/10 for item in grid_data] 
        #split list into 2d 
        grid_data = [grid_data[i:i + 8] for i in range(0, len(grid_data), 8)]

        data = SensorState(
            grid_data,
            json_data["tvoc"],
            json_data["co2"],
            json_data["humidity"]/10,
            json_data["temp"]/10,
            sum(grid_data)/len(grid_data),
        )
        data_queue.append(data)
        data_queue.popleft() #remove previous val
