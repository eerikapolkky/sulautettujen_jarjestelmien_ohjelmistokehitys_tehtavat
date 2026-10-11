import serial
import time

ser = None


def open_uart(port, baudrate=115200):
    global ser

    ser = serial.Serial(
        port=port,
        baudrate=int(baudrate),
        timeout=2
    )

    ser.dtr = True

    time.sleep(1)
    ser.reset_input_buffer()

    
def close_uart():
    global ser

    if ser:
        ser.close()
        ser = None

def send_command(command):
    global ser

    ser.reset_input_buffer()
    ser.write((command + "\n").encode("utf-8"))
    ser.flush()

    response = ser.readline().decode(
        "utf-8",
        errors="replace"
    ).strip()
        
    return response


def send_sequence(sequence):
    global ser

    ser.reset_input_buffer()

    command = "S" + sequence + "\n"

    ser.write(command.encode("utf-8"))
    ser.flush()

    while True:
        response = ser.readline().decode(
            "utf-8",
            errors="replace"
        ).strip()

        if response.startswith("SEQUENCE:"):
            return response

        if response.startswith("SEQUENCE_ERROR:"):
            return response

def get_debug_time():
    global ser

    ser.reset_input_buffer()

    ser.write(b"T")
    ser.flush()

    end_time = time.time() + 10

    while time.time() < end_time:
        response = ser.readline().decode(
            "utf-8",
            errors="replace"
        ).strip()

        if response.startswith("DEBUG:"):
            return response

    return "DEBUG_TIMEOUT"