# Performing Inference Using C++ API

## Description
This example demonstrates how to perform inference using the MIGraphX C++ API.
The model used is a Resnet50v2 model that has been pre-trained on ImageNet data, and inference is performed on frames from a video.
See the [C++ MNIST example](../cpp_mnist) for more detailed information about using the API.

## Content
- [Performing Inference Using C++ API](#performing-inference-using-c-api)
  - [Description](#description)
  - [Content](#content)
  - [Basic Setup](#basic-setup)
  - [Running this Example](#running-this-example)

## Basic Setup

Follow the instructions in the [Python Resnet50 Jupiter notebook](../python_resnet50/) to download the Resnet50 ONNX model and sample video.
You'll also need OpenCV.
One way to install it (if using Ubuntu 18.04+):

```shell
$ sudo apt-get update
$ sudo apt-get install libopencv-core-dev libopencv-imgcodecs-dev libopencv-imgproc-dev libopencv-videoio-dev
```

## Running this Example
To create the executable:
```
$ mkdir build
$ cd build
$ CXX=/opt/rocm/llvm/bin/clang++ cmake ..
$ make
```
There will now be an executable named `renset50_inference` in the `build` directory. This can be run with or without options. Executing without any options will produce the following output:
```
Usage: ./renset50_inference [options]
options:
         -c, --cpu      Compile for CPU
         -g, --gpu      Compile for GPU
         -f, --fpga     Compile for FPGA
         -p, --print    Print Graph at Each Stage
