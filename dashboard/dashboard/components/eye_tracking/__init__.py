import cv2
from .gaze_tracking import GazeTracking
from queue import LifoQueue
from collections import deque


def start_tracking_thread(queue: deque):
    gaze = GazeTracking()
    webcam = cv2.VideoCapture(0)

    while True:
        # We get a new frame from the webcam
        _, frame = webcam.read()

        # We send this frame to GazeTracking to analyze it
        gaze.refresh(frame)

        frame = gaze.annotated_frame()
        text = "Looking away"

        if gaze.is_blinking() or gaze.is_right() or gaze.is_left() or gaze.is_center():
            text = "Looking at Screen"

        queue.append(text)

        cv2.putText(frame, text, (90, 60), cv2.FONT_HERSHEY_DUPLEX, 1.6, (147, 58, 31), 2)

        left_pupil = gaze.pupil_left_coords()
        right_pupil = gaze.pupil_right_coords()

        cv2.imshow("Demo", frame)

        if cv2.waitKey(1) == 27:
            break
    
    webcam.release()
    cv2.destroyAllWindows()
