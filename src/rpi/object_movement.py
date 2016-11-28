from picamera.array import PiRGBArray
from picamera import PiCamera
from collections import deque
import numpy as np
import argparse
import imutils
import cv2
import time
import serial

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-b", "--buffer", type=int, default=32,
        help="max buffer size")
args = vars(ap.parse_args())

# start serial port
usart = serial.Serial(
        port='/dev/serial0', 
        baudrate=9600, 
        parity=serial.PARITY_NONE, 
        stopbits=serial.STOPBITS_ONE, 
        bytesize=serial.EIGHTBITS,
        timeout=1
        )

# verify serial port is open
if not usart.is_open:
    usart.open()

# usart.write(b'\x77')
rx_data = b'\x00'

TARGET_CENTERED = b'\x03'
TARGET_MISSING  = b'\xFE'
TARGET_LEFT     = b'\x01'
TARGET_RIGHT    = b'\x02'
TARGET_REQUEST  = b'\x11'
target_status = 'not found'
last_status = ''
initial_noise = 0
NOISE_LEVEL = 5

# flush tx and rx buffers
usart.flushInput()
usart.flushOutput()

# define the lower and upper boundaries of the "green" target in the HSV
# color space
colorLower = (29, 86, 6)        # green
colorUpper = (64, 255, 255)     # green

# initialize the list of tracked points and the frame counter,
pts = deque(maxlen=args["buffer"])
counter = 0

# initialize the camera and grab a reference to the raw camera capture
camera = PiCamera()
camera.resolution = (640, 480)
camera.framerate = 32
rawCapture = PiRGBArray(camera, size=(640, 480))

# allow the camera to warmup
time.sleep(0.1)

# keep looping
while True:
    # capture frames from the camera
    for camFrame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True): 

        # check for new serial data
        if usart.in_waiting != 0:
            rx_data = usart.read()
            print('Received: {}'.format(rx_data))
        else:
            rx_data = b'\x00'

        # grab the raw NumPy array representing the image, then initialize the timestamp
        # and occupied/unoccupied text
        frame = camFrame.array

        # resize the frame, blur it, and convert it to the HSV
        # color space
        frame = imutils.resize(frame, width=600)
        blurred = cv2.GaussianBlur(frame, (11, 11), 0)
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

        # construct a mask for the color of our earlier choice, then perform
        # a series of dilations and erosions to remove any small
        # blobs left in the mask
        mask = cv2.inRange(hsv, colorLower, colorUpper)
        mask = cv2.erode(mask, None, iterations=2)
        mask = cv2.dilate(mask, None, iterations=2)

        # find contours in the mask and initialize the current
        # (x, y) center of the ball
        cnts = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL, 
                cv2.CHAIN_APPROX_SIMPLE)[-2]
        center = None

        # only proceed if at least one contour was found
        if len(cnts) > 0:
            # find the largest contour in the mask, then use
            # it to compute the minimum enclosing circle and
            # centroid
            c = max(cnts, key=cv2.contourArea)
            ((x, y), radius) = cv2.minEnclosingCircle(c)
            M = cv2.moments(c)
            center = (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))

            # only proceed if the radius meets a minimum size
            if radius > 10:
                # draw the circle and centroid on the frame,
                # then update the list of tracked points
                cv2.circle(frame, (int(x), int(y)), int(radius),
                        (0, 255, 255), 2)
                cv2.circle(frame, center, 5, (0, 0, 255), -1)
                pts.appendleft(center)

        try:
            # if center[0] > 240 and center[0] < 360:
            #     print('Target Centered!')
            # elif center[0] >= 360:
            #     print('Rotate Left')
            # elif center[0] <= 240:
            #     print('Rotate Right')
            # show the x,y positions on the frame
            cv2.putText(frame, "x: {}, y: {}".format(center[0], center[1]),
                    (10, frame.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX,
                    0.35, (0, 0, 255), 1)
        except TypeError:
            pass
        # if center is None:
            # print('Seeking target...')

        # show the frame to our screen and increment the frame counter
        # cv2.imshow("Growbot Vision", frame)
        key = cv2.waitKey(1) & 0xFF
        counter += 1

        # clear the stream in preparation for the next frame
        rawCapture.truncate(0)

        # update target position
        if center is not None:
            # Target Centered!
            if center[0] > 240 and center[0] < 360:
                target_status = TARGET_CENTERED
                if initial_noise < NOISE_LEVEL:
                    initial_noise += 1
            # Rotate Left
            elif center[0] >= 360:
                target_status = TARGET_RIGHT
                if initial_noise < NOISE_LEVEL:
                    initial_noise = 0
            # Rotate Right
            elif center[0] <= 240:
                target_status = TARGET_LEFT
                if initial_noise < NOISE_LEVEL:
                    initial_noise = 0
            # Seeking target...
            else:
                target_status = TARGET_MISSING
                initial_noise = 0
        # Seeking target...
        else:
            target_status = TARGET_MISSING
            initial_noise = 0

        # check if x position has been requested
        if rx_data == TARGET_REQUEST:
            usart.write(target_status)
            print('Target status requested!')
            print('Status is {}'.format(target_status))

        # send update if target position has changed
        elif last_status != target_status and initial_noise >= NOISE_LEVEL:
            usart.write(target_status)
            last_status = target_status
            print('Target status has changed!')
            print('Status is now {}'.format(target_status))

        # if the 'q' key is pressed, stop the loop
        if key == ord("q"):
            break
    break

# close serial port and any open windows
usart.close()
cv2.destroyAllWindows()
