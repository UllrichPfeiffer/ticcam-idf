#!/usr/bin/python3.11

# Note, to activate DHCP service on Opensuse leap 15.3 do the following:
# sudo firewall-cmd --add-service=dhcp --zone=trusted
# sudo firewall-cmd --zone=trusted --list-ports
# sudo firewall-cmd --zone=trusted --add-port=5000/tcp

import serial
import time
import os
import sys
import time
import numpy as np
import scipy.io
#import matplotlib.pyplot as plt
#import matplotlib.animation as animation
import json
import cv2
#import ffmpeg
#import click
#import configparser
import inspect
import argparse
import serial.tools.list_ports
import socket
#import cv2
from PIL import Image
import io
#import keyboard

#fig = plt.figure()

def handle_exception(exc_type, exc_value, exc_traceback):
    if issubclass(exc_type, KeyboardInterrupt):
        # Default handling for keyboard interrupts, allows you to exit with Ctrl+C
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
    else:
        # Handle non-interrupt exceptions as desired
        print(f"An error occurred: {exc_value}")

# Set the global exception hook
#sys.excepthook = handle_exception

def find_serial_port(device):
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if device in p.description:
            return p.device
    return None

# Global vars
ser = None # the serial port
client = None # the wifi client

config_list = []
response_stack = []
#baudrate=115200
#baudrate=2000000 
baudrate=2000000 
json_config_filename = "CAMctrl.json"
valid_baudrates = [9600, 19200, 38400, 57600, 115200, 250000, 500000, 1000000, 2000000]

def is_valid_baudrate(baudrate):
    if int(baudrate) in valid_baudrates:
        return int(baudrate)
    else:
        raise argparse.ArgumentTypeError(f"{baudrate} is not within the valid range: {valid_baudrates}.")

verbosity_level = 0;
 
# Create the parser
#parser = argparse.ArgumentParser(allow_abbrev=True, description=' Remote control for Camera.',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser = argparse.ArgumentParser(allow_abbrev=True, add_help=False, description=' Remote control for Camera.')
# parser.add_argument('-u', '--update', action='store_true', help='Update config file with known commands from device')
#parser.add_argument('-Update', action='store_true', dest='a', help='Update config file with known commands from device')
parser.add_argument('-h', '--help', action='store_true', help='Show this help message for CAMctrl and exit.')
parser.add_argument("-sp", "--serport", type=str, default="/dev/ttyUSB0", metavar='N', help="Overwrite serial port (default: auto)")
parser.add_argument("-b", "--baudrate", type=is_valid_baudrate, nargs=1, metavar='N', help='Set baudrate=N (default: 2000000)')
parser.add_argument("-ip", "--ipaddr", metavar='N', dest="ip_address", help="Set ip address=N of remote camera")
parser.add_argument("-np", "--netport", type=int, default=5000, metavar='N', help="Set port number=N for socket (default: 8080)")
parser.add_argument("-r", "--rotate", type=int, default=None, metavar='N', help="Rotate object by N degee")
parser.add_argument("-int16", action='store_true', help="CAMctrl expects int16 instead of int32")
parser.add_argument("-v", "--verbose", action='count', default=0, help="Increase verbosity level (e.g., -v, -vv, or -vvv).")
exclude_attributes = {"netport", "serport","ip_address", "int16", "verbose", "help", "rotate"}
args, unknown = parser.parse_known_args()
group = parser.add_mutually_exclusive_group()

#    0: Errors only
#    1: Basic information
#    2: Detailed information
#    3: Debug level information

def log(message, level):
    if args.verbose >= level:
        print(message)

def ParseCmd():   
    for item in config_list:
        fields = item.split(":")
        if len(fields) != 5:
            log("-> Error in field length", level=3)  
            sys.exit("Exit") 
        num = fields[0]
        cmd_name = fields[1]
        cmd_char = fields[2]
        cmd_help = fields[3]
        num_par = int(fields[4])
        if num_par == 0: num_par=argparse.REMAINDER
        if num_par == 2: metavar_txt =("N1", "N2")
        else: metavar_txt = 'N'
        if cmd_name in globals(): 
            group.add_argument(f"-{cmd_name}", help=cmd_help, metavar='N', nargs=num_par)
        else: group.add_argument(f"-{cmd_name}", help=cmd_help, metavar=metavar_txt, nargs=num_par, dest=cmd_char, action="append") # default=cmd_char
        #log (f"-> Registerin {cmd_name}", level=3)  

def main(my_sys):
    global config_list
    if os.path.exists(json_config_filename):
        log(f"-> Using config file '{json_config_filename} (use option -Update to reread from device)'", level=4)  
        with open(json_config_filename, "r") as f:
            config_list = json.load(f)    
    else: # write them to file
        OpenDevice()
        ConfigureCmds()
        with open(json_config_filename, "w") as f: 
            json.dump(response_stack, f)
    ParseCmd()
    if len(sys.argv) == 1 or args.help:
        parser.print_help()
#        input("Press any key to exit...")
        sys.exit("Exit")
    OpenDevice()
    DoItCmd()
    # Wait for user input before closing the console window
    if hasattr(sys, 'frozen'):
        input("Press any key to exit...")

def DoItCmd():
    global ser
    global baudrate
    global args
#    args, unknown = parser.parse_known_args()
    # log(f"args={args}", level=3)  
    if args.baudrate is not None: 
        baudrate = args.Baudrate[0]
        ser.baudrate = baudrate
        log(f"-> Connect with serial Baudrate={baudrate}", level=3)  
        # remove argument from argparse, run only onetime
        args_dict = vars(args)
        args_dict.pop('Baudrate', None)
        args = argparse.Namespace(**args_dict)
#    if args.update:
    if args.rotate is not None: 
        rotate = args.rotate
        log(f"-> Rotate object by {rotate} degree", level=3)  
        mount = ElliptecRotationStage('/dev/ttyUSB0', offset=-8529)
        for _ in range(rotate):
            mount.move_by(1)
            print(f"Rotated by {rotate} degrees")
            time.sleep(1)  # Allow time for motion

#        mount.move_by(rotate)
#        mount.home()

        # remove argument from argparse, run only onetime
        args_dict = vars(args)
        args_dict.pop('rotate', None)
        args = argparse.Namespace(**args_dict)
#    else:     
    for cmd in vars(args):
        if cmd not in exclude_attributes:
            if getattr(args, cmd) is not None:
                if callable(globals().get(cmd)): 
                    func = globals().get(cmd)
                    if getattr(args, cmd):
                        func(getattr(args, cmd)[0])
                    else: func()
                    #log(f"-> Called {cmd}: {getattr(args, cmd)}", level=3)  
                else:
                     #log(f"{args}: {getattr(args, cmd)}", level=3)  
    #                if(isinstance(getattr(args, cmd)[0], int)):
    #                    number_str = getattr(args, cmd)[0]
    #                else:
                    number_str = ' '.join(str(item) for sublist in getattr(args, cmd) for item in sublist)
                    CmdTxNRx(f"{cmd} {number_str}", 1, 0.5)
def Update():
    ConfigureCmds()
    with open(json_config_filename, "w") as f: 
        json.dump(response_stack, f) 
    
def ConfigureCmds():
    global config_list
    global response_stack
    CmdTxRx("a 1", 0.5)
    config_list = response_stack # load the configuration from response stack
    #print(config_list)

def readline():
    if ser is not None:
        return(ser.readline().decode('latin-1'))
    elif client is not None:
        return(client.recv(1024).decode('latin-1').strip())

def read(size, timeout=1.0):
    """Reads exactly `size` bytes from serial or network client with a timeout."""
    start_time = time.monotonic()
    data = bytearray()  # Efficient way to accumulate received data

    while len(data) < size:
        if time.monotonic() - start_time > timeout:
            log(f"Timeout: Expected {size} bytes, but only received {len(data)} bytes.", level=0)
            sys.exit("Exit")  # Exit if we fail to get the expected data

        if ser is not None:
            try:
                chunk = ser.read(size - len(data))  # Read the remaining required bytes
                if not chunk:
                    time.sleep(0.01)  # Small delay to avoid CPU overload
                    continue  # Try again
                data.extend(chunk)  # Efficiently extend the received data
            except serial.SerialException as e:
                log(f"Error: {e}", level=0)
                sys.exit("Exit")  # Exit on serial error

        elif client is not None:
            try:
                chunk = client.recv(size - len(data))  # Read remaining bytes from network
                if not chunk:
                    time.sleep(0.01)
                    continue
                data.extend(chunk)
            except Exception as e:
                log(f"Network error: {e}", level=0)
                sys.exit("Exit")  # Exit on network error
    
    return bytes(data)  # Convert to immutable bytes before returning


import time

def write(data, timeout=1.0):
    """Writes data reliably over serial or network with timeout handling."""
    start_time = time.monotonic()
    
    if ser is not None:
        try:
            bytes_written = 0
            while bytes_written < len(data):
                if time.monotonic() - start_time > timeout:
                    log(f"Timeout: Failed to write all {len(data)} bytes.", level=0)
                    sys.exit("Exit")

                chunk_size = ser.write(data[bytes_written:])  # Write remaining data
                if chunk_size is None:
                    chunk_size = 0  # Handle cases where write returns None

                bytes_written += chunk_size
        except serial.SerialException as e:
            log(f"Serial write error: {e}", level=0)
            sys.exit("Exit")

    elif client is not None:
        try:
            client.sendall(data)  # `sendall()` ensures all data is sent
        except Exception as e:
            log(f"Network write error: {e}", level=0)
            sys.exit("Exit")

def OpenDevice():
    global ser
    global baudrate
    global client
    global args
    args, unknown = parser.parse_known_args()

    if args.ip_address: # Create a socket object
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # Set a timeout for socket operations (e.g., 10 seconds)
        log(f"-> Connect via wifi IP={args.ip_address} netport={args.netport}", level=3)  
        client.connect((args.ip_address, args.netport))
        timeout = 30  # Timeout in seconds
        client.settimeout(timeout)

    else: # configure the serial port
        ser = serial.Serial(baudrate=baudrate,
                                bytesize=serial.EIGHTBITS,
                                parity=serial.PARITY_NONE,
                                stopbits=serial.STOPBITS_ONE,
                                timeout=1)
        if ser is None: sys.exit("Exit")

        if args.serport:
            port = args.serport;
        else:
            port = find_serial_port("CP210")
        if port is not None:
            log(f"Connect to port: {port}", level=0)  
        else:
            log("Device not found", level=0)  

        if os.name == 'nt' or sys.platform == 'win32':
            try:
                ser.setDTR(0)
                ser.setRTS(0)
    #            ser.setPort('COM3')
                ser.setPort(port)
                ser.open()
    #            ser = serial.Serial('COM3', baudrate, timeout=0.1, rtscts=0, dsrdtr=False)
            except serial.SerialException as e:
                log(f"Error: {e}", level=0)  
                sys.exit("Exit")   
        elif os.name == 'posix':
            try:
    #            ser.setPort('/dev/ttyUSB0')
                ser.setPort(port)
                ser.open()
    #            ser = serial.Serial('/dev/ttyUSB0', baudrate, timeout=1) #baudrate=115200
        #                            parity=serial.PARITY_NONE,
        #                            stopbits=serial.STOPBITS_ONE,
        #                            bytesize=serial.EIGHTBITS,
            except serial.SerialException as e:
                log(f"Error: {e}", level=0)  
                sys.exit("Exit")   
        else:
            log("Unknown operating system", level=3)  
        #ser.write("pipe cleaner\n")
        ser.write(b'A\n') # Send a pipe cleaner
        readline()
        ser.flushInput() 
        ser.flushOutput()
        
def CmdTxRx(cmd, timeout):
    global ser
    global response_stack
    response_stack = []
    log(f"-> Sending '{cmd}' and expecting a response until timeout reached", level=3)  
    write(bytes(cmd + "\n", 'utf-8')) # get command list from connected device
    last_received = time.monotonic()
    while True:
        data = readline()  # Read a line of serial data and convert it to a string
        if data:
            last_received = time.monotonic()  # Update the last received time
            if data.startswith('#'):  # Ignore comment lines
                continue # ignore 
            response_stack.append(data)  # Append the data to the list
        elif time.monotonic() - last_received > timeout:  # If no data is received within the timeout period, break out of the loop
            break
    for item in response_stack:
        log(item, level=3)  

def CmdTx(cmd):
    global ser
    log(f"-> Sending '{cmd}' not expecting a response", level=3)  
    write(bytes(cmd + "\n", 'utf-8')) # get command list from connected device

def CmdTxNRx(cmd, num_lines, timeout):
    global ser
    global response_stack
    response_stack = []
    log(f"-> Sending '{cmd}' expecting exactly {num_lines} response", level=3)  
    write(bytes(cmd + "\n", 'utf-8')) # get command list from connected device
    last_received = time.monotonic()
    for num in range(num_lines):
        data = readline()  # Read a line of serial data and convert it to a string
        if data:
            last_received = time.monotonic()  # Update the last received time
            if data.startswith('#'):  # Ignore comment lines
                continue # ignore 
            response_stack.append(data)  # Append the data to the list
        elif time.monotonic() - last_received > timeout:  # If no data is received within the timeout period, break out of the loop
            break
    for item in response_stack:
        log(item, level=0)  

def GetStatus(lines):
    CmdTxNRx(f"h", int(lines), 1)

def ListDir(lines):
    CmdTxNRx(f"o", int(lines), 1)

# Function to find the start of the JPEG frame
def find_jpeg_start(data):
    for i in range(len(data) - 1):
        if data[i] == 0xFF and data[i + 1] == 0xD8:
            return i
    return -1

# Function to find the end of the JPEG frame (end + 2 is the actual end)
def find_jpeg_end(data):
    for i in range(len(data) - 1):
        if data[i] == 0xFF and data[i + 1] == 0xD9:
            return i + 2
    return -1

def display_image(frame_data):
    #image = Image.open(io.BytesIO(frame_data))
    #image.show()
    plt.imshow(Image.open(io.BytesIO(frame_data)))
#    plt.imshow(frame_data, cmap='gray')
    plt.show(block=False)
#    plt.pause(0.5)
#    plt.draw()

def Stream(num_seconds):
    global ser
    seconds= int(num_seconds)
#    data = bytes([])
    CmdTx(f"x {seconds}")
    # Buffer to hold bytes read from the serial port
    buffer = bytearray()
    num_frames = 0
    start_time = time.monotonic()
    while True:
        # Read data from serial port
        data = ser.read(ser.in_waiting or 1)
        if data:
            buffer += data

            # Try to find a complete JPEG frame in the buffer
            start = find_jpeg_start(buffer)
            end = find_jpeg_end(buffer)
            log(f"{start} to {end}", level=3) 

            if start != -1 and end != -1 and end > start:
                log("Frame received", level=3) 
                # Extract the frame and clear the buffer up to the end of the frame
                frame_data = buffer[start:end]
                buffer = buffer[end:]
                # Decode the JPEG frame
                try: 
                    frame = cv2.imdecode(np.frombuffer(frame_data, dtype=np.uint8), cv2.IMREAD_COLOR)
                    width = frame.shape[1]
                    hight = frame.shape[0]
                    log(f"width {width}", level=3) 
                    log(f"hight {hight}", level=3) 
                    fx = 2.0  # factor along the width
                    fy = 2.0  # factor along the height
                    
                    # Resize the image using scaling factors
                    if(width == hight == 33):
                        num_frames+=1
                        log(f"frame {num_frames}", level=3) 
#                        frame_scaled = cv2.resize(frame, None, fx=fx, fy=fy, interpolation=cv2.INTER_CUBIC)
                        frame_scaled = cv2.resize(frame, (800,800) , interpolation=cv2.INTER_CUBIC)
                        if frame_scaled is not None:
                            cv2.imshow('MJPEG Stream', frame_scaled)

    #                        cv2.imshow('MJPEG Stream', frame_scaled)
                        cv2.waitKey(1)
                        #cv2.waitKey(0)
    #                    display_image(frame)
    #                    plt.plot(1,1)
                    # Break the loop when 'q' is pressed
                except Exception as e:
                    print(f"An error occurred while decoding the image: {e}")
        else:
            print("no data")
            break
    fps = num_frames/(time.monotonic()-start_time)
    log(f"-> FPS reached: {fps:.2f} for {num_frames} frames", level=0)  
    
    while True:
        # Display the image and wait for a key press for 1 ms
        if cv2.waitKey(1) & 0xFF == ord('q'):  # Optionally allow exiting with 'q'
            break
        # Check if the window is still open
        if cv2.getWindowProperty('MJPEG Stream', cv2.WND_PROP_VISIBLE) < 1:
            break

def GetMAT(num_frames):
    global args
    num_frames = int(num_frames)
    global ser
    data = bytearray()  # Use bytearray for efficient appending
    CmdTx(f"g {num_frames}")
    log(f"-> Frames requested: {num_frames}", level=1)
    num_rows = 33
    num_cols = 33
    num_elements = num_frames * num_rows * num_cols  # Number of total 33x33 matrices
        # Determine number of bytes per frame based on int16 or int32
    if args.int16:
        num_bytes_in_fb = 2
    else:
        num_bytes_in_fb = 4
    num_bytes_in_frame = num_bytes_in_fb * num_rows * num_cols
    log(f"-> Number of bytes expected per frame: {num_bytes_in_frame}", level=2)
    # Pre-allocate memory for the data (this avoids reallocations)
    frames_data = np.zeros((num_frames, num_rows, num_cols), dtype=np.int16 if args.int16 else np.int32)
    start = time.monotonic()
    for num in range(num_frames):
        # Read the image data
        image = read(num_bytes_in_frame)  # Use a helper function to manage timeout and retries
        num_bytes_in_frame_read = len(image)
        if num_bytes_in_frame_read < num_bytes_in_frame:
            log(f"Error: Expected {num_bytes_in_frame} bytes, but got {num_bytes_in_frame_read} bytes", level=0)
            sys.exit("Exit")  # Exit on data mismatch
        log(f"-> Number of bytes read: {num_bytes_in_frame_read}", level=2)
        data += image  # Efficiently accumulate data in a bytearray
        # Read the image data into the 2D matrix (use numpy to reshape)
        try:
            if args.int16:
                frames_data[num] = np.frombuffer(image, dtype=np.int16).reshape(num_rows, num_cols)
            else:
                frames_data[num] = np.frombuffer(image, dtype=np.int32).reshape(num_rows, num_cols)
        except ValueError as e:
            log(f"Unexpected error: {e}", level=0)
            sys.exit("Exit")  # Exit if reshaping fails

        # Acknowledge receipt of the frame
        write(b'A')  # Send acknowledgment to ESP32 after receiving one frame

    fps = num_frames / (time.monotonic() - start)
    log(f"-> FPS reached: {fps:.2f} ", level=0)
    log(f"-> MAT file created: video.mat", level=0)
    # After all frames are read, save the data to a .mat file
    scipy.io.savemat('video.mat', {'data': frames_data})

def GetScreen(filename):
    global ser
    basename, extension = os.path.splitext(filename)
    image = bytes([])
    CmdTx(f"r {filename}")
    log(f"-> Screen Shot requested: {filename}", level=3)  
    num_rows = 240  # Change this to the actual number of rows in your 2-D array
    num_cols = 320  # Change this to the actual number of columns in your 2-D array
    num_header = 54
    pad = (4-(3*num_cols)%4)%4
    log(f"-> pad={pad}", level=3)  
    num_bytes_in_row = num_cols*3+pad # RGB = 3x
    num_bytes_in_image = num_bytes_in_row*num_cols+num_header  # Change this to the actual number of elements in your 2-D array
    log(f"-> Number of bytes expected: {num_bytes_in_image}", level=3)  
    line = read(num_header)
    log(f"-> Header: {len(line)} bytes", level=3)  
    image += line
    start = time.monotonic()
    for num in range(num_rows):
        line = read(num_bytes_in_row)
#        log(f"-> {num} {len(line)}", level=3)  
        image += line
        ser.write(b'A') # send acknollage
    fps = 1/(time.monotonic()-start)
    log(f"-> FPS reached: {fps:.2f} ", level=3)  
    log(f"-> image {len(image)}", level=3)  
    # Save the array as a MATLAB data file
    # scipy.io.savemat('my_screen.mat', {'image': image})
    # Open a file for writing
    with open(f"{basename}.bmp", 'wb') as f:
        # Write the buffer to the file
        f.write(image)


def from_twos_complement(n, bits=32):
    if n < (1 << (bits-1)): return n
    return n - (1 << bits)

def to_twos_complement(n, bits=32):
    return (1 << bits) + n if n < 0 else n


# Encoder counts per revolution (from manual)
COUNTS_PER_REVOLUTION = 262144
 

# List of status responses
RESPONSES = [
    'ok',
    'communication timeout',
    'mechanical timeout',
    'command error',
    'value out of range',
    'module isolated',
    'module out of isolation',
    'initialization error',
    'thermal error',
    'busy',
    'sensor error',
    'motor error',
    'out of range',
    'overcurrent',
]

class ElliptecRotationStage:
    def __init__(
            self,
            port='/dev/ELL14K', 
            address: int = 0, 
            offset: int = 0, 
        ):
        self._conn = serial.Serial(port, baudrate=9600, stopbits=1, parity='N', timeout=0.05)
        self.address = 0
        self._offset = offset

    def send(self, command, data=b''):
        """Send the given command type, with the given data payload."""
        packet = (
            str(self.address).encode('utf-8')
            + command.encode('utf-8')
            + data.hex().upper().encode('utf-8')
            + b'\n'
        )
        self._conn.write(packet)

    def query(self, command, data=b''):
        """
        Send the given command type, with the given data payload.
        Return the response type and decoded data payload from the Elliptec controller.
        """
        self.send(command, data=data)

        response = b''
        while True:
            response += self._conn.read(8192)
            if response.endswith(b'\r\n'): break
            time.sleep(0.2)

        header, data = response[:3], response[3:-2]
        assert chr(header[0]) == str(self.address)
        return header[1:].decode(), int(data.decode(), 16)


    ##### Debug/Internal Commands ######
    @property
    def status(self):
        header, response = self.query('gs')
        assert header == 'GS'
        return RESPONSES[response]

    @property
    def _position(self):
        header, response = self.query('gp')
        assert header == 'PO'
        return from_twos_complement(response)


    ##### Public Interface #####
    def home(self):
        """
        Return the stage to the home position.
        (May correspond to 0 degrees in software.)
        """
        self.query('ho')

    def tare(self):
        """Mark the current position as 0° in software."""
        self._offset = -self._position

    @property
    def angle_unwrapped(self):
        """Return the current angle (CCW), counting full turns."""
        return -360 * (self._position + self._offset) / COUNTS_PER_REVOLUTION

    @angle_unwrapped.setter
    def angle_unwrapped(self, degrees):
        self.move_by(degrees - self.angle_unwrapped)

    @property
    def angle(self):
        """Return the current angle (CCW)."""
        return self.angle_unwrapped % 360

    @angle.setter
    def angle(self, degrees):
        delta = degrees - self.angle
        if delta > 180: delta -= 360
        if delta < -180: delta += 360

        self.move_by(delta)

    def move_by(self, degrees):
        """Move by the given number of degrees, counterclockwise."""
        delta = -round(degrees * COUNTS_PER_REVOLUTION/360)
        data = to_twos_complement(delta).to_bytes(4, 'big')
        header, response = self.query('mr', data=data)
        assert header in ['GS', 'PO']
        if header == 'GS': raise ValueError(RESPONSES[response])

    def close(self):
        self._conn.close()

if __name__ == '__main__':
    main(sys)
