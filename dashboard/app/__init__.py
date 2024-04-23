from flask import Flask, render_template
from jinja2 import Environment, PackageLoader, select_autoescape
# from app.components import *
import sass
import os 


def create_app():
    app = Flask(__name__)

    return app

app = create_app()

curr_dir = os.getcwd()
sass.compile(dirname=(f'{curr_dir}/app/static/scss', f'{curr_dir}/app/static/css'), output_style='compressed')

@app.route('/')
def index():

    context = {
    }

    return render_template('base.html.j2', **context)