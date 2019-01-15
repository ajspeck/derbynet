#!/usr/bin/env python
from flask import Flask, render_template, Response
import picamera
from base_camera import BaseCamera
import requests
import time
import xml.etree.ElementTree as ET
import datetime
import queue
import collections
import os
import threading
import logging

qCmd = queue.Queue()
qResp = queue.Queue()
ReplayData = collections.namedtuple('ReplayData', ['CMD', 'DATA'])

def replay_response_thread(qCmd,qResp,ReplayData):
    s = requests.Session()
    replayURL=''
    while True:
#        print('Posting',flush=True)
        try:
            r = s.post('https://derby.speckfamily.org/derbynet/action.php', data = {'action':'replay-message',
                                                                                  'status':'1',
                                                                                   'finished-replay':'0',
                                                                                   'replay-url':replayURL
                                                                                  }, timeout=5.0)
            xml=ET.fromstring(r.content)
            for c in xml.getchildren():
                if c.tag == 'replay-message':
                    parts = c.text.split(' ')
                    print(parts)
                    if parts[0]=='START':
                        recName='{0}-{1}.h264'.format(parts[1],datetime.datetime.now().strftime("%y%m%d_%H%M%S"))
                        qCmd.put(ReplayData('START',recName))
                    elif parts[0]=='REPLAY':
                        skipBack=float(parts[1])
                        qCmd.put(ReplayData('REPLAY',skipBack))
        except:
            pass
        try:
            resp=qResp.get(timeout=0.1)
            if resp.CMD=='REPLAY-URL':
                replayURL=resp.DATA
                print(replayURL)
        except queue.Empty:
            pass
def camera_thread(qCmd,qResp,ReplayData,camera):
    stream = picamera.PiCameraCircularIO(camera, seconds=10)
    camera.start_recording(stream, format='h264')
    time.sleep(2) #wait for camera to warm up
    fName='test.h264' #Initial file
    try:
        while True:
#            print('camera',flush=True)
            camera.wait_recording(0)
            try:
                cmd=qCmd.get(timeout=1.0)
                if cmd.CMD == 'START':
                    fName=cmd.DATA
                elif cmd.CMD=='REPLAY':
                    # Keep recording for 2 seconds and only then write the
                    # stream to disk
                    camera.wait_recording(2)
                    stream.copy_to(os.path.join('/videos/',fName))
                    url='https://finish.speckfamily.org/videos/{0}'.format(fName)
                    qResp.put(ReplayData('REPLAY-URL',url))
            except queue.Empty:
                pass
    finally:
        camera.stop_recording()

camera = picamera.PiCamera(
            resolution=(640, 480),
            framerate=60.0,
        )

app = Flask(__name__)



@app.route('/')
def index():
    """Video streaming home page."""
    return render_template('index.html')


def gen(camera):
    """Video streaming generator function."""
    while True:
        frame = camera.get_frame()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')


@app.route('/video_feed')
def video_feed():
    """Video streaming route. Put this in the src attribute of an img tag."""
    return Response(gen(BaseCamera(camera)),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

print('starting-initial',flush=True)
print('starting',flush=True)
gunicorn_logger = logging.getLogger('gunicorn.error')
app.logger.handlers = gunicorn_logger.handlers
app.logger.setLevel(gunicorn_logger.level)

ct=threading.Thread(target=camera_thread,kwargs={'qCmd':qCmd,
                                                  'qResp':qResp,
                                                  'ReplayData':ReplayData,
                                                  'camera':camera})
rt=threading.Thread(target=replay_response_thread,kwargs={'qCmd':qCmd,
                                                  'qResp':qResp,
                                                  'ReplayData':ReplayData})
rt.start()
ct.start()

if __name__ == '__main__':
    app.run(host='0.0.0.0', threaded=True)
