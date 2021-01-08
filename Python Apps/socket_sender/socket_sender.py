import socket
import sys
import random
from ctypes import *


""" This class defines a C-like struct """
class Payload(Structure):
    _fields_ = [("target_roll", c_int32),
                ("target_pitch", c_int32), 
                ("game_state",  c_int32), 
                ("lwing_angle",  c_int32), 
                ("rwing_angle",  c_int32), 
                ("body_height",  c_int32)
                ]
                
class SocketSender(object):
    
    def __init__(self):
        server_addr = ('localhost', 2300)
        self._s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        if self._s is None:
            print("Error creating socket")

        try:
            self._s.connect(server_addr)
            print(f"Connected to {repr(server_addr)}")
        except:
            print(f"ERROR: Connection to {repr(server_addr)} refused")

    def send(self,  roll,  pitch,  game_state,  lwing_angle,  rwing_angle,  body_height):
        try:
            payload_out = Payload(roll,  pitch,  game_state,  lwing_angle,  rwing_angle,  body_height)
            print(f"Sending roll={payload_out.target_roll} pitch={payload_out.target_pitch}")
            nsent = self._s.send(payload_out)
            # Alternative: s.sendall(...): coontinues to send data until either
            # all data has been sent or an error occurs. No return value.
            print(f"Sent {nsent} bytes")
        finally:
            pass
