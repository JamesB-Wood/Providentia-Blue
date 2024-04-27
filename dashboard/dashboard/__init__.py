from flask import Flask, render_template
from jinja2 import Environment, PackageLoader, select_autoescape

from threading import Thread

from queue import Queue, LifoQueue
from collections import deque

import time 
import sass
import os 

# from app.components import *
from dashboard.components import start_tracking_thread

#global reference to all queues
queues = {}

def create_threads():
    #Make Queues
    eye_track_queue = deque()

    #Generate Threads
    tracking_thread = Thread(target=start_tracking_thread, args=(eye_track_queue,))

    #Run Threads
    tracking_thread.start()
    
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

    init_data = {
        "TVOC": 0,
        "CO2": 0,
        "Humidity": 0.5,
        "Temp": 25,
        "Light": 200,
    }

    eye_data = { 
        "status": "None",
    }

    context = {
        "sensor_data": init_data,
        "eye_data": eye_data,
    }

    return render_template('base.html.j2', **context)

last_state = ""
state_start_time = 0
@app.route('/eye-track')
def get_eye_tracking():
    """
    
    """
    global queues
    eye_track_queue = queues["eye_track_queue"]

    global last_state
    global state_start_time

    eye_state = ""
    try:
        states = {}
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

    eye_data = {
        "status": eye_state,
        "time": time.time() - state_start_time
    }

    return render_template('./components/eye_tracking.html.j2', eye_data=eye_data)

@app.route('/sensors')
def get_sensors():
    """
    
    """

    new_data = {
        "TVOC": 2,
        "CO2": 2,
        "Humidity": 0.5,
        "Temp": 25,
        "Light": 200,
    }

    return render_template('./components/sensors.html.j2', sensor_data=new_data)