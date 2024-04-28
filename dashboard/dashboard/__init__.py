from flask import Flask, render_template
from jinja2 import Environment, PackageLoader, select_autoescape

from threading import Thread

from collections import deque

import time 
import sass
import os 

# from app.components import *
from dashboard.components import start_tracking_thread, start_sensor_thread, INIT_GRAPH_DATA

#global reference to all queues
queues = {}

#Global define for timeout = 1 minutes
TIMEOUT = 60 * 1

def create_threads():
    #Make Queues
    eye_track_queue = deque()
    sensor_data_queue = deque()

    #Generate Threads
    tracking_thread = Thread(target=start_tracking_thread, args=(eye_track_queue,))
    sensor_thread = Thread(target=start_sensor_thread, args=(sensor_data_queue,))

    #Run Threads
    tracking_thread.start()
    # sensor_thread.start()
    
    return {"eye_track_queue": eye_track_queue}


def create_app():
    app = Flask(__name__)   

    #create and start threads
    global queues
    queues = create_threads()

    return app

app = create_app()

@app.route('/')
def index():
    # Compile css
    path = os.path.dirname(os.path.realpath(__file__))
    sass.compile(dirname=(f'{path}/static/scss', f'{path}/static/css'), output_style='compressed')

    context = {
        "graph_data": INIT_GRAPH_DATA
    }

    return render_template('base.html.j2', **context)


@app.route('/flower-state')
def get_flower_state():

    flower_state = {
        "brightness": 50,
        "openness": 75,
        "flashing": False,
    }

    return render_template('./components/flower_state.html.j2', flower_state=flower_state)


last_state = ""
state_start_time = 0
@app.route('/eye-track')
def get_eye_tracking():
    """
    Returns current state of the eye tracking as a compiled HTML element
    """
    global queues
    eye_track_queue = queues["eye_track_queue"]

    global last_state
    global state_start_time

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
    except IndexError:
        #ignore empty queues
        pass

    #calculate time left til timout
    time_left = 0
    if eye_state == "Looking at Screen":
        time_left = TIMEOUT - (time.time() - state_start_time)

    eye_data = {
        "status": eye_state,
        "time": (time.time() - state_start_time),
        "time_left": time_left
    }

    return render_template('./components/eye_tracking.html.j2', eye_data=eye_data)

import random
@app.route('/graph-data')
def get_graph():
    """
    Returns new information for the graph to use as JSON
    """

    graph_data = {
        "TVOC": random.random() * 1000,
        "CO2": random.random() * 1000,
        "Humidity": random.random() * 100,
        "Temp": random.random() * 40,
        "Light": random.random() * 300,
    }

    return graph_data

@app.route('/sensors')
def get_sensors():
    """
    Returns current state of the sensors as a compile HTML element
    """

    sensor_data = {
        "TVOC": 2,
        "CO2": 2,
        "Humidity": 0.5,
        "Temp": 25,
        "Light": 200,
    }

    return render_template('./components/sensors.html.j2', sensor_data=sensor_data)