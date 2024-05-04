from flask import Flask, render_template
from jinja2 import Environment, PackageLoader, select_autoescape

from threading import Thread

from collections import deque

import time
import sass
import os

# from app.components import *
from dashboard.components import (
    start_tracking_thread,
    start_sensor_thread,
    calculate_flower_state,
    INIT_GRAPH_DATA,
    SensorState,
)


QUEUES = {}  # global reference to all queues
TIMEOUT = 60 * 1  # Global define for timeout = 1 minutes
COMPORT = "COM5"


def create_threads():
    # Make Queues
    eye_track_queue = deque()
    sensor_data_queue = deque()
    command_queue = deque()

    # Generate Threads
    tracking_thread = Thread(target=start_tracking_thread, args=(eye_track_queue,))
    sensor_thread = Thread(
        target=start_sensor_thread, args=(sensor_data_queue, command_queue, COMPORT)
    )

    # Run Threads
    tracking_thread.start()
    sensor_thread.start()

    return {
        "eye_track_queue": eye_track_queue,
        "sensor_data_queue": sensor_data_queue,
        "command_queue": command_queue,
    }


import json


def create_app():
    app = Flask(__name__)

    # create and start threads
    global QUEUES
    QUEUES = create_threads()

    return app


app = create_app()


@app.route("/")
def index():
    # Compile css
    path = os.path.dirname(os.path.realpath(__file__))
    sass.compile(
        dirname=(f"{path}/static/scss", f"{path}/static/css"), output_style="compressed"
    )

    context = {"graph_data": INIT_GRAPH_DATA}

    return render_template("base.html.j2", **context)


@app.route("/flower-state")
def get_flower_state():
    global QUEUES
    flower_state = calculate_flower_state(
        QUEUES["sensor_data_queue"], QUEUES["command_queue"]
    )

    return render_template(
        "./components/flower_state.html.j2", flower_state=flower_state
    )


last_state = ""
state_start_time = 0
is_flashing = False


@app.route("/eye-track")
def get_eye_tracking():
    """
    Returns current state of the eye tracking as a compiled HTML element
    """
    global QUEUES
    eye_track_queue = QUEUES["eye_track_queue"]
    command_queue = QUEUES["command_queue"]

    global last_state
    global state_start_time
    global is_flashing

    eye_state = ""
    try:
        states = {}
        # Get average state
        for state in eye_track_queue:
            if state in states.keys():
                states[state] += 1
            else:
                states[state] = 1
        eye_track_queue.clear()
        if len(states) != 0:
            eye_state = max(states, key=states.get)
            if eye_state != last_state:
                state_start_time = time.time()
                last_state = eye_state
                # stop flashing if we stop looking at screen
                if eye_state != "Looking at Screen" and is_flashing:
                    command_queue.append("flash off")

    except IndexError:
        # ignore empty queues
        pass

    # calculate time left til timout
    time_left = 0
    if eye_state == "Looking at Screen":
        time_left = TIMEOUT - (time.time() - state_start_time)

        if time_left <= 0:
            command_queue.append("flash on")
            is_flashing = True

    eye_data = {
        "status": eye_state,
        "time": (time.time() - state_start_time),
        "time_left": time_left,
    }

    return render_template("./components/eye_tracking.html.j2", eye_data=eye_data)

@app.route("/graph-data")
def get_graph():
    """
    Returns new information for the graph to use as JSON
    """

    global QUEUES
    sensor_data_queue: deque[SensorState] = QUEUES["sensor_data_queue"]
    
    return sensor_data_queue[-1].as_dict()

@app.route("/grid")
def get_grid():
    """
    
    """

    global QUEUES
    sensor_data_queue: deque[SensorState] = QUEUES["sensor_data_queue"]

    return render_template("./components/grid.html.j2", grid_data=sensor_data_queue[-1].ir_vals)


@app.route("/sensors")
def get_sensors():
    """
    Returns current state of the sensors as a compile HTML element
    """

    global QUEUES
    sensor_data_queue: deque[SensorState] = QUEUES["sensor_data_queue"]

    return render_template("./components/sensors.html.j2", sensor_data=sensor_data_queue[-1])
