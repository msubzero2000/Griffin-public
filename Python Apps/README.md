# How to install

You should get a Jetson AGX Xavier and flash it with JetPack 4.2 or later.

* Note by 卜居 : If your JetPack version is diffrent, please find the compatable whls from NVIDIA's website or Jetson's developer forums. My environments:

| JetPack | 4.4 DP |
| - | - |
| Python | 3.6.9 |
| CUDA Toolkit | 10.2.89 |
| cuDNN Lib | 8.0.0 |
| TensorRT | 7.1.0 |
| PyTorch | torch-1.6.0-cp36-cp36m-linux_aarch64.whl |
| TensorFlow | tensorflow-2.1.0+nv20.4-cp36-cp36m-linux_aarch64.whl |

## Install dependencies

```bash
sudo apt update
sudo apt install -y python3-pip libjpeg-dev libcanberra-gtk-module libcanberra-gtk3-module python3-matplotlib
pip3 install tqdm cython pycocotools
```

## Install PyTorch & TensorFlow

PyTorch whl files can be downloaded from [this link](https://nvidia.box.com/shared/static/9eptse6jyly1ggt9axbja2yrmj6pbarc.whl). The downloaded file should be renamed to `torch-1.6.0-cp36-cp36m-linux_aarch64.whl` to prevent pip installation issues.

TensorFlow whl files can be downloaded from [this link](https://developer.download.nvidia.com/compute/redist/jp/v44/tensorflow/tensorflow-2.1.0+nv20.4-cp36-cp36m-linux_aarch64.whl)

```bash
pip3 install torch-1.6.0-cp36-cp36m-linux_aarch64.whl
pip3 install tensorflow-2.1.0+nv20.4-cp36-cp36m-linux_aarch64.whl
pip3 install torchvision
```

## Install jetcam, torch2trt and trt_pose

jetcam

```bash
git clone https://github.com/NVIDIA-AI-IOT/jetcam
cd jetcam/
python3 setup.py install
```

torch2trt

```bash
git clone https://github.com/NVIDIA-AI-IOT/torch2trt
cd torch2trt/
python3 setup.py install --plugins
```

trt_pose

```bash
git clone https://github.com/NVIDIA-AI-IOT/trt_pose
cd trt_pose/
python3 setup.py install
```

## Prepare SSD MobileNet saved model

You can find pretrained object detection models from [Google TensorFlow](https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/tf2_detection_zoo.md).

```bash
cd models/
mkdir ssd
wget http://download.tensorflow.org/models/object_detection/tf2/20200711/ssd_mobilenet_v2_fpnlite_320x320_coco17_tpu-8.tar.gz
tar zxvf ssd_mobilenet_v2_fpnlite_320x320_coco17_tpu-8.tar.gz
mv ssd_mobilenet_v2_fpnlite_320x320_coco17_tpu-8/saved_model/* .
```


## Change Camera

If you have CSI Cameras on your Jetson AGX Xavier, no special steps here.

As my case, I only have a USB Camera, so some code changes needed.



## Run!

After everythings ready, you just type this in your terminal:

```
python3 main.py
```

It will load torch/trt/tf models into memory and initialize the cameras. Wait for a while. Pose estimation results will be sended to port 2300 via sockets. Now you can control this game with just a camera, cool!


