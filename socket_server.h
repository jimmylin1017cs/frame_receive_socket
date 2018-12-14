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
#include <string>
#include <vector>


// receive frame data and frame stamp
//
// @frame : put frame data which receive from client
// @port : server port number
// @timeout : for server to use select
// @quality : jpeg quality, not use in here
//
// @reture : frame stamp
//
int receive_frame_with_stamp(std::vector<unsigned char> &frame, int port, int timeout, int quality);


// receive frame data
//
// @port : server port number
// @timeout : for server to use select
// @quality : jpeg quality, not use in here
//
// @reture : frame data from client
//
std::vector<unsigned char> receive_frame(int port, int timeout, int quality);