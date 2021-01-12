import tensorflow as tf
from PIL import Image
from numpy import asarray


class HumanDetection(object):
    _HUMAN_CLASS_ID = 1
    _MIN_ACCEPTABLE_SCORE = 0.5

    def __init__(self):
        self._model = tf.saved_model.load(
            "models/ssd", tags=None, options=None
        )
        self._infer = self._model.signatures["serving_default"]

    def detect(self, image):
        x = asarray(image)
        x = x.reshape([1, x.shape[0], x.shape[1], x.shape[2]])
        result = self._infer(tf.constant(x))
        num_detection = int(result['num_detections'][0].numpy())
        response_list = []
        for i in range(num_detection):
            class_id = int(result['detection_classes'].numpy()[0][i])
            score = result['detection_scores'].numpy()[0][i]

            if class_id == self._HUMAN_CLASS_ID and score > self._MIN_ACCEPTABLE_SCORE:
                y1 = result['detection_boxes'].numpy()[0][i][0]
                x1 = result['detection_boxes'].numpy()[0][i][1]
                y2 = result['detection_boxes'].numpy()[0][i][2]
                x2 = result['detection_boxes'].numpy()[0][i][3]

                response_list.append([x1, y1, x2, y2])

        return response_list
