import math

from utilities.vector import Vector


class MotionController(object):
    
    def __init__(self):
        self._roll = 0
        self._pitch = 0
        self._game_state = 0
        self._left_wing_roll_target = 0
        self._right_wing_roll_target = 0
        self._body_height = 0
        self._shoulder_vert_pos_history = []
        self._backface_ctr = 0
        
    def parse_objects(self,  objects):
        if len(objects) > 0:
            skeleton = objects[0]
            left_elbow = skeleton.get_joint('left_elbow')
            right_elbow = skeleton.get_joint('right_elbow')
            left_shoulder = skeleton.get_joint('left_shoulder')
            right_shoulder = skeleton.get_joint('right_shoulder')
            
            nose = skeleton.get_joint('nose')
            neck = skeleton.get_joint('neck')

            # Back face for 1 second to reset the game
            if self._is_reset_detected(left_shoulder,  right_shoulder):
                self._game_state = 0

            if left_elbow is not None and right_elbow is not None and left_shoulder is not None and right_shoulder is not None:
#                left_elbow.x = 10.0
#                left_elbow.y = 0.0
#                left_shoulder.x = 20.0
#                left_shoulder.y = 10.0
#                
#                right_elbow.x = 40.0
#                right_elbow.y = 40.0
#                right_shoulder.x = 30.0
#                right_shoulder.y = 30.0
#                self._game_state = 0

                # Calculate wing roll target from the delta between elbow and shoulder
                left_shoulder_to_elbow_vec = Vector(left_elbow.x - left_shoulder.x,  left_elbow.y - left_shoulder.y).normalise()            
                horizVec = Vector(1.0,  0.0)
                self._left_wing_roll_target = min(90,  max(-15,  left_shoulder_to_elbow_vec.angle_from_vector(horizVec) * 180.0 / math.pi + 30))

                right_shoulder_to_elbow_vec = Vector(right_elbow.x - right_shoulder.x,  right_elbow.y - right_shoulder.y).normalise()            
                horizVec = Vector(-1.0,  0.0)
                self._right_wing_roll_target = min(90,  max(-15,  horizVec.angle_from_vector(right_shoulder_to_elbow_vec) * 180.0 / math.pi + 30))
                
                # Calculate roll from the delta between left and right elbow
                left_elbow_to_right_elbow_vec = Vector(left_elbow.x - right_elbow.x,  left_elbow.y - right_elbow.y).normalise()
                horizVec = Vector(1.0,  0.0)
                self._roll = left_elbow_to_right_elbow_vec.angle_from_vector(horizVec) * 180.0 / math.pi

                # Use the distance between left and right shoulder as a measurement threshold
                shoulder_dist = abs(right_shoulder.x - left_shoulder.x)

                self._body_height = self._calc_body_height(nose,  neck,  shoulder_dist)
                
                # Detect jump by measuring the vertical movement of both shoulder
                # If detected, set game state to flying
                if self._is_jump_detected(left_shoulder, right_shoulder,  neck,  nose,  shoulder_dist):
                    self._game_state = 1
                                    
                print(f"Jump {self._game_state} Roll {self._right_wing_roll_target} Left {self._left_wing_roll_target} Right {self._right_wing_roll_target}")
            
        return self._roll,  self._pitch,  self._game_state,  self._left_wing_roll_target,  self._right_wing_roll_target,  self._body_height

    def _calc_body_height(self,  nose,  neck,  shoulder_dist):
        if nose is None or neck is None:
            return self._body_height
            
        nose_to_neck_vert = abs(nose.y - neck.y)
        # map 2x nose to neck dist to body_height of 0-5
        max_nose_to_neck_dist = shoulder_dist
        print(f"Nose To Neck: {nose_to_neck_vert} shoulder {max_nose_to_neck_dist}")
        body_height = min(8, ((nose_to_neck_vert / max_nose_to_neck_dist) - 0.5) * 16.0)
        
        return body_height
        
    def _is_reset_detected(self,  left_shoulder,  right_shoulder):
        if left_shoulder is None or right_shoulder is None:
            return False
            
        if self._game_state == 0:
            self._backface_ctr = 0
            return False
            
        # Face backward for longer than 1 second to reset
        if self._backface_ctr > 10:
            return True
            
        if left_shoulder.x < right_shoulder.x:
            self._backface_ctr += 1
        else:
            self._backface_ctr = 0
            
        return False
        
    def _is_jump_detected(self,  left_shoulder,  right_shoulder,  neck,  nose,  shoulder_dist):
        if nose is None or neck is None:
            return False
            
        nose_to_neck_vert = abs(nose.y - neck.y)
        # Make sure the left and right shoulder are not too close to yield false positive jump (eg: while turning back)
        left_to_right_shoulder_horiz_dist = left_shoulder.x - right_shoulder.x
        if self._game_state == 1 and left_to_right_shoulder_horiz_dist > nose_to_neck_vert / 2.0:
            return False
            
        cur_shoulder_y = (left_shoulder.y + right_shoulder.y) * 2
        self._shoulder_vert_pos_history.append(cur_shoulder_y)
        
        # Use the distance between left and right shoulder as a threshold to detect a jump
        jump_threshold = shoulder_dist * 2.0
        
        if len(self._shoulder_vert_pos_history) > 10:
            # Only keep the last 1 sec of history
            self._shoulder_vert_pos_history.pop(0)
    
        # Find the lowest point (peak of the jump)
        peak = min(self._shoulder_vert_pos_history)
        peak_idx = self._shoulder_vert_pos_history.index(peak)
        
        # Now find the largest point at both end of the jump
        try:
            start_bottom = max(self._shoulder_vert_pos_history[: peak_idx])
            end_bottom = max(self._shoulder_vert_pos_history[peak_idx:])
        except Exception as _:
            return False
        
        # It is a jump if the delta between start_bottom and end_bottom to peak is > threshold
        start_delta = abs(peak - start_bottom)
        end_delta = abs(peak - end_bottom)
        
        if start_delta >= jump_threshold and end_delta >= jump_threshold:
            self._shoulder_vert_pos_history = []
            return True
                    
        return False
