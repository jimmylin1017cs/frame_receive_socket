// ==============================
// By Jimmy
//
// 2018/12/14
//
// 1. sort out code
// ==============================

#include "socket_header.h"
#include "image.h"

#include <iostream>
#include "opencv2/opencv.hpp"

// send_mjpeg(im, 8090, 200, 95);
void send_mjpeg(cv::Mat im, int port, int timeout, int quality);
void send_mjpeg(image ipl, int port, int timeout, int quality);