from socket_sender.socket_sender import  SocketSender
from pose_estimation.openpose import OpenPose
from video_feed.video_offline_reader import VideoOfflineReader
from video_feed.video_csi_reader import VideoCSIReader
from motion.motion_controller import MotionController

import cv2
import numpy as np
from utilities.stopwatch import Stopwatch

socket_sender = SocketSender()
motion = MotionController()

pose_estimator = OpenPose('models')
video_reader = VideoCSIReader()
num_frames = 0
sw = Stopwatch()
show_preview = True
show_annot = False

while True:
    img = video_reader.read_frame()
    num_frames += 1
        
    if img is None:
        break

    objects,  annot_image = pose_estimator.detect(img,  return_annotated_image=show_preview)
    roll,  pitch,  game_state,  left_wing_roll_target,  right_wing_roll_target,  body_height = motion.parse_objects(objects)
    socket_sender.send(int(roll),  int(pitch),  game_state,  int(left_wing_roll_target),  int(right_wing_roll_target),  int(body_height))
    
    elapsed_ms = max(1, sw.get())
    if show_preview:
        cv2.imshow("Original",  img)
        if annot_image is not None:
            size = annot_image.shape
            height = size[0]
            width = size[1]
            if height >0 and width > 0:
                cv2.imshow("Annotated",  annot_image)
            
        cv2.waitKey(1)
