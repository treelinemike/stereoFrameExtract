// g++ -Wall -I/usr/include/opencv4 -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -o cvtest

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.hpp>

int main(void){

    cv::Mat im_raw = cv::imread("greensq.tif");

    cv::Mat im_yuv;

    cv::cvtColor(im_raw, im_yuv, cv::COLOR_BGR2YUV_I420);

    cv::Mat im_bgr2;
    cv::cvtColor(im_yuv,im_bgr2,cv::COLOR_YUV2BGR_I420);


    std::cout << "Original " << im_raw.size() << " (" << im_raw.channels() << " ch):";
    for( unsigned int i = 0; i < 300; ++i){
        printf("%d ",*(im_raw.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
    
    
    std::cout << "YUV " << im_yuv.size() << " (" << im_yuv.channels() << " ch):";
    for( unsigned int i = 0; i < 300; ++i){
        printf("%d ",*(im_yuv.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;

    
    std::cout << "BGR RESTORE " << im_bgr2.size() << " (" << im_bgr2.channels() << " ch)" << std::endl;

    cv::imshow("window",im_bgr2);
    cv::waitKey();

    return 0;
}
