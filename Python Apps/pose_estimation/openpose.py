import json
import os
import time
import cv2
import numpy as np
from PIL import Image

import torch
import trt_pose.models
import torch2trt
from torch2trt import TRTModule
import torchvision.transforms as transforms
from .draw_objects import DrawObjects
from trt_pose.parse_objects import ParseObjects
from .human_detection import HumanDetection


class SkeletonSegment(object):
    
    def __init__(self,  x1,  y1,  x2,  y2,  joint_idx_1,  joint_idx_2,  colour,  name):
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        self.joint_idx_1 = joint_idx_1
        self.joint_idx_2 = joint_idx_2
        self.colour = colour
        self.name = name
        
    
class SkeletonJoint(object):
    
    def __init__(self,  x,  y,  name):
        self.x = x
        self.y = y
        self.name = name
        
        
class Skeleton(object):
    
    def __init__(self):
        self._segments = []
        self._joints = []
        
    def add_joint(self,  joint):
        self._joints.append(joint)
        
    def add_segment(self, segment):
        self._segments.append(segment)
        
    def get_joint(self,  name):
        for joint in self._joints:
            if joint is not None and joint.name == name:
                return joint
                
    def draw(self, image):
        height = image.shape[0]
        width = image.shape[1]

        for joint in self._joints:
            if joint is None:
                continue
                
            color = (0, 255, 0)
            x = round(joint.x * width)
            y = round(joint.y * height)
            cv2.circle(image, (x, y), 3, color, 2)

        for segment in self._segments:
            if segment is None:
                continue
                
            joint1 = self._joints[segment.joint_idx_1]
            joint2 = self._joints[segment.joint_idx_2]
            
            x1 = round(joint1.x * width)
            y1 = round(joint1.y * height)
            x2 = round(joint2.x * width)
            y2 = round(joint2.y * height)
            cv2.line(image, (x1, y1), (x2, y2), segment.colour, 2)        
        
        
def build_topology(coco_category):
    """Gets topology tensor from a COCO category
    """
    skeleton = coco_category['skeleton']
    keypoints = coco_category['keypoints']
    K = len(skeleton)
    topology_obj = {}
    
    topology = torch.zeros((K, 4)).int()
    topology_names = []
    topology_colors = []
    color_r = 0
    color_g = 0
    color_b = 0
    color_res = 100
    for k in range(K):
        topology[k][0] = 2 * k
        topology[k][1] = 2 * k + 1
        topology[k][2] = skeleton[k][0] - 1
        topology[k][3] = skeleton[k][1] - 1
        
        color_b += color_res
        if color_b >= 255:
            color_b = 0
            color_g += color_res
            if color_g >= 255:
                color_g = 0
                color_b = 0
                color_r += color_res
        topology_colors.append((color_r,  color_g,  color_b))
        topology_names.append(f"{keypoints[topology[k][2]]}-{keypoints[topology[k][3]]}")

    topology_obj['topology'] = topology
    topology_obj['segment_names'] = topology_names
    topology_obj['segment_colours'] = topology_colors
    
    topology_obj['joint_names'] = keypoints
    
    return topology_obj

class OpenPose():
    
    _OPTIMIZED_MODEL_NAME = 'resnet18_baseline_att_224x224_A_epoch_249_trt.pth'
    _IMAGE_WIDTH = 224
    _IMAGE_HEIGHT = 224
    
    mean = torch.Tensor([0.485, 0.456, 0.406]).cuda()
    std = torch.Tensor([0.229, 0.224, 0.225]).cuda()
    device = torch.device('cuda')

    def __init__(self,  model_folder):
        human_pose_path = os.path.join(model_folder,  'human_pose.json')
        with open(human_pose_path, 'r') as f:
            self._human_pose = json.load(f)

            self._topology = build_topology(self._human_pose)
            num_parts = len(self._human_pose['keypoints'])
            num_links = len(self._human_pose['skeleton'])

            model = trt_pose.models.resnet18_baseline_att(num_parts, 2 * num_links).cuda().eval()
            MODEL_WEIGHTS = os.path.join(model_folder, 'resnet18_baseline_att_224x224_A_epoch_249.pth')

            optimized_model_path = os.path.join(model_folder,  self._OPTIMIZED_MODEL_NAME)
            if not os.path.exists(optimized_model_path):
                print("Converting Torch OpenPose model to TFRT")
                model.load_state_dict(torch.load(MODEL_WEIGHTS))    
                
                data = torch.zeros((1, 3, self._IMAGE_HEIGHT, self._IMAGE_WIDTH)).cuda()
                model_trt = torch2trt.torch2trt(model, [data], fp16_mode=True, max_workspace_size=1<<25)
                torch.save(model_trt.state_dict(), optimized_model_path)
                
            print("Loading TFRT OpenPose model")

            self._model_trt = TRTModule()
            self._model_trt.load_state_dict(torch.load(optimized_model_path))
            
            self._parse_objects = ParseObjects(self._topology['topology'])
            self._draw_objects = DrawObjects(self._topology)

            self._human_detector = HumanDetection()
            
    def _preprocess(self,  image):
        global device
        device = torch.device('cuda')
        image = transforms.functional.to_tensor(image).to(device)
        image.sub_(self.mean[:, None, None]).div_(self.std[:, None, None])
        return image[None, ...]
        
    def benchmark(self):
        data = torch.zeros((1, 3, self._IMAGE_HEIGHT, self._IMAGE_WIDTH)).cuda()        
        t0 = time.time()
        torch.cuda.current_stream().synchronize()
        for i in range(50):
            y = self._model_trt(data)
            
        torch.cuda.current_stream().synchronize()
        t1 = time.time()

        print(f"OpenPose FPS={50.0 / (t1 - t0)}")

    def _in_detected_humans(self, x, y, detected_humans):
        for entry in detected_humans:
            x1 = entry[0]
            y1 = entry[1]
            x2 = entry[2]
            y2 = entry[3]
            if x1 <= x <= x2 and y1 <= y <= y2:
                return True

        return False

    def _construct_skeletons(self, object_counts, objects, normalized_peaks, detected_humans):
        topology = self._topology['topology']
        topology_segment_names = self._topology['segment_names']
        topology_joint_names = self._topology['joint_names']
        topology_segment_colours = self._topology['segment_colours']        

        skeletons = []
        K = topology.shape[0]
        count = int(object_counts[0])
        for i in range(count):
            obj = objects[0][i]
            skeleton = Skeleton()
            skeletons.append(skeleton)
            
            C = obj.shape[0]
            for j in range(C):
                k = int(obj[j])
                if k >= 0:
                    peak = normalized_peaks[0][j][k]
                    x = float(peak[1])
                    y = float(peak[0])
                    if self._in_detected_humans(x, y, detected_humans):
                        joint = SkeletonJoint(x,  y,  topology_joint_names[j])
                        skeleton.add_joint(joint)
                    else:
                        skeleton.add_joint(None)
                else:
                    skeleton.add_joint(None)
                    
            for k in range(K):
                c_a = topology[k][2]
                c_b = topology[k][3]
                if obj[c_a] >= 0 and obj[c_b] >= 0:
                    peak0 = normalized_peaks[0][c_a][obj[c_a]]
                    peak1 = normalized_peaks[0][c_b][obj[c_b]]
                    x0 = float(peak0[1])
                    y0 = float(peak0[0])
                    x1 = float(peak1[1])
                    y1 = float(peak1[0])
                    if self._in_detected_humans(x0, y0, detected_humans) and self._in_detected_humans(x1, y1, detected_humans):
                        color = topology_segment_colours[k]
                        segment = SkeletonSegment(x0,  y0,  x1,  y1,  c_a,  c_b,  color,  topology_segment_names[k])
                        skeleton.add_segment(segment)
                    else:
                        skeleton.add_segment(None)
                else:
                    skeleton.add_segment(None)
        
        return skeletons
        
    def detect(self,  image,  return_annotated_image=False):
        np_img = cv2.resize(image,  (self._IMAGE_WIDTH,  self._IMAGE_HEIGHT))
        
        # Block 15% from each side
        #block_width = int(self._IMAGE_WIDTH * 0.15)
        
        #cv2.rectangle(np_img,  (0,  0),  (block_width,  self._IMAGE_HEIGHT),  (0,  0,  0),  cv2.FILLED)
        #cv2.rectangle(np_img,  (self._IMAGE_WIDTH - block_width,  0),  (self._IMAGE_WIDTH,  self._IMAGE_HEIGHT),  (0,  0,  0),  cv2.FILLED)
        
        data = self._preprocess(np_img)
        cmap, paf = self._model_trt(data)
        cmap, paf = cmap.detach().cpu(), paf.detach().cpu()
        counts, objects, peaks = self._parse_objects(cmap, paf)#, cmap_threshold=0.15, link_threshold=0.15)
                
        detected_humans = self._human_detector.detect(image)
        skeletons = self._construct_skeletons(counts, objects, peaks, detected_humans)

        annot_image = None

        if return_annotated_image:
            for skeleton in skeletons:
                annot_image = cv2.resize(image,  (224,  224))
                skeleton.draw(annot_image)
                    
        return skeletons, annot_image

