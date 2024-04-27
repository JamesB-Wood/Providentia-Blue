from flask import Flask, render_template
from jinja2 import Environment, PackageLoader, select_autoescape

from threading import Thread

from queue import Queue

import sass
import os 

# from app.components import *
from dashboard.eye_tracking import start_tracking_thread


def create_app():
    app = Flask(__name__)

    #Start extra threads
    eye_track_queue = Queue()

    tracking_thread = Thread(target=start_tracking_thread, args=(eye_track_queue,))

    tracking_thread.start()

    return app

app = create_app()

curr_dir = os.getcwd()
sass.compile(dirname=(f'{curr_dir}/dashboard/static/scss', f'{curr_dir}/dashboard/static/css'), output_style='compressed')

@app.route('/')
def index():

    context = {
    }

    return render_template('base.html.j2', **context)