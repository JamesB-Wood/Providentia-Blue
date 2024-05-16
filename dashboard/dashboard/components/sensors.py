from collections import deque
from dataclasses import dataclass
from filterpy.kalman import KalmanFilter
from filterpy.common import Q_discrete_white_noise
from math import exp, sqrt

import bisect
import serial
import json
import time
import numpy as np

filter = None

INIT_GRAPH_DATA = [
    {
        "label": "tvoc",
        "data": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "borderColor": "blue",
        "yAxisID": "y4",
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
    def to_json(self) -> str:
        return json.dumps(self.as_dict())


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

def scale_vector_sigmoid(x, a, m):
    """
    Scale the sensor vector x around mean desired value m 
    adjusted with slope a
    """
    return ((-2/(1 + exp(a*(x-m)))) + 2)

def calculate_flower_state (
    data_queue: deque[SensorState], command_queue: deque[ControlState], is_flashing: bool
):
    global flower_state
    new_state = ControlState()
    new_state.flashing = is_flashing

    # brightness
    new_state.brightness = max(0, round((1 - (abs(data_queue[-1].average_ir_temp - 32.5))/14) * 100))

    # scale sensor vectors
    u_tvoc = scale_vector_sigmoid(data_queue[-1].tvoc, 0.005, 0)
    u_c02 = scale_vector_sigmoid(data_queue[-1].co2, 0.005, 400)
    u_humidity = scale_vector_sigmoid(data_queue[-1].humidity,0.075, 40)
    u_temp = scale_vector_sigmoid(data_queue[-1].temp, 0.1, 25)

    radius = sqrt((u_tvoc**2)+(u_c02**2)+(u_humidity**2)+(u_temp**2))
    new_state.openness = max(0, round((1 - (abs(0.5*(radius - 2))))* 100))

    # send command to queue if required
    if new_state != flower_state:
        command_queue.append(new_state.to_json())
        flower_state = new_state

    # return states
    return flower_state


ir_cache = [] 
def calibrate_temp(state: SensorState) -> SensorState:
    global filter
    global ir_cache
    #init kalman filter
    if filter == None:
        filter = KalmanFilter (dim_x=2, dim_z=1)
        filter.x = np.array([state.temp, 0.]) #initial position and velocity
        filter.F = np.array([[1.,1.], [0.,1.]]) #transition matrix
        filter.H = np.array([[1.,0.]]) #measurement function
        filter.P *= 5. #covalence matrix time uncertainty
        filter.R = 1 #measurement noise
        filter.Q = Q_discrete_white_noise(dim=2, dt=0.5, var=0.13)

    #pass in temp
    filter.predict()
    filter.update(state.temp)

    prob = max(0, round((1 - (abs(state.average_ir_temp - 32.5))/14) * 100)) #probability of human in frame

    # only if 80% sure of human don't use temp to predict
    if prob > 80:
        for item in ir_cache:
            #pass in ir grid
            filter.predict()
            filter.update(item[1])
        state.temp = round(filter.x[0],1)
        return state
    
    #use a normal distribution function multiplied by an inverted human detection algorithm
    prob_vals = [[min(1, abs(x - 31)/8) * exp(-0.5*(((x-31)/31)**2)) for x in row] for row in state.ir_vals]
    
    #use highest probability pixel/s for background temp
    possible_vals = []
    for i in range(8):
        for j in range(8):
            if prob_vals[i][j] > 0.85:
                bisect.insort(possible_vals, [prob_vals[i][j], state.ir_vals[i][j]], key=lambda x: -x[0])
                
    for item in possible_vals[:3]:
        #pass in ir grid
        filter.predict()
        filter.update(item[1])

    #cache results
    ir_cache = possible_vals[:3]
    state.temp = round(filter.x[0],1)
    return state

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
            # remove data if none for 5s
            continue

        json_data = json.loads(line.decode())
        data_time = time.time()

        grid_data = json_data["ir_grid"]
        grid_data = [item/100 for item in grid_data] 

        data = SensorState(
            [grid_data[i:i + 8] for i in range(0, len(grid_data), 8)],
            json_data["tvoc"],
            json_data["co2"],
            json_data["humidity"]/10,
            json_data["temp"]/10,
            round(sum(grid_data)/len(grid_data), 1),
        )

        data = calibrate_temp(data)
        data_queue.append(data)
        data_queue.popleft() #remove previous val