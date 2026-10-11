# tätä käytettiin testaamaan oliko koodin toimimattomuus uartin, laudan vai portin vika

import serial
import time

port = "/dev/cu.usbmodem0010501670471"

s = serial.Serial(port, 115200, timeout=0.5)
s.dtr = True

print("Port opened:", s.name)

time.sleep(1)
s.reset_input_buffer()

print("Sending 000120")
s.write(b"000120\n")
s.flush()

end = time.time() + 3

while time.time() < end:
    data = s.read(100)
    if data:
        print("RECEIVED:", repr(data))

s.close()
print("Done")