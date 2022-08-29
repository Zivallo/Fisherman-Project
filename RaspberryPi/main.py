import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import time
import serial
import random

# Fetch the service account key JSON file contents
cred = credentials.Certificate('/home/pi/test1-6b36f-firebase-adminsdk-22gyd-11e43f106e.json')
# Initialize the app with a service account, granting admin privileges
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://test1-6b36f-default-rtdb.firebaseio.com/'
})
port="/dev/ttyACM0"
ser=serial.Serial(port,9600)
ser.flushInput()

while True:
    
    now = time.gmtime(time.time())
    y = str(now.tm_year)
    mo = str(now.tm_mon)
    d = str(now.tm_mday)
    h = str(now.tm_hour+9)
    mi = str(now.tm_min)
    sec = str(now.tm_sec)
    ref = db.reference('data')
    ref2 = db.reference('time')
    ref2.update({
        'time': '\"'+h+'\:'+mi+'\:'+sec+'\"'
        })
    
    c = db.reference('controlmode').child('controlmode').get()
    d = db.reference('Control').child('Heater').get()
    e = db.reference('Control').child('LED').get()
    f = db.reference('Control').child('Pump1').get()
    g = db.reference('Control').child('Pump2').get()
    m = str(c)+str(d)+str(e)+str(f)+str(g)
    h=m.encode('utf-8')
    ser.write(h)
    print(h)
            
    if(ser.inWaiting()>0):
        ads=ser.readline()
        ac = ads.decode('utf-8')
        data=ac.split(",",4)
        ref.update({
            'cds': data[0],
            'temp': data[1],
            'waterlevel': data[2],})