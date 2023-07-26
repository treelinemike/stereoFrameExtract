// g++ -Wall -I/usr/include/opencv4 cvtest.cpp -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -o cvtest

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.hpp>

int main(void){

    // READ IN TIFF, CONVERT TO YUV, CONVERT BACK TO BGR, WRITE TO TIFF
    cv::Mat im_yuv;
    cv::Mat im_bgr2;
    cv::Mat im_raw = cv::imread("greensq.tif");
    cv::cvtColor(im_raw, im_yuv, cv::COLOR_BGR2YUV_I420);
    cv::cvtColor(im_yuv,im_bgr2,cv::COLOR_YUV2BGR_I420);
    cv::imwrite("restore.tif",im_bgr2);

    std::cout << "Original " << im_raw.size() << " (" << im_raw.channels() << " ch): ";
    for( unsigned int i = 0; i < 12; ++i){
        printf("%d ",*(im_raw.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
    
    std::cout << "YUV " << im_yuv.size() << " (" << im_yuv.channels() << " ch): ";
    for( unsigned int i = 0; i < 12; ++i){
        printf("%d ",*(im_yuv.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
 
    std::cout << "BGR RESTORE " << im_bgr2.size() << " (" << im_bgr2.channels() << " ch) :";
    for( unsigned int i = 0; i < 12; ++i){
        printf("%d ",*(im_bgr2.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;


    // create YUV input synthetically to test memory structure, then convert back to BGR and save 
    cv::Mat yuv422_test(3,4,CV_8UC1);
    cv::Mat new_bgr;
    
    uint8_t *yuvstart = yuv422_test.ptr<uint8_t>();
    for(unsigned int j = 0; j < 8; ++j){
        *yuvstart = 88;
        ++yuvstart;
    }
    for(unsigned int k = 0; k < 2; ++k){
        *yuvstart = 187;
        ++yuvstart;
    }
    for(unsigned int l = 0; l < 2; ++l){
        *yuvstart = 115;
        ++yuvstart;
    }
    
    cv::cvtColor(yuv422_test,new_bgr,cv::COLOR_YUV2BGR_I420);
    cv::imwrite("out.tif",new_bgr);

    std::cout << "yuv422_test " << yuv422_test.size() << " (" << yuv422_test.channels() << " ch): ";
    for( unsigned int i = 0; i < 12; ++i){
        printf("%d ",*(yuv422_test.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
    
    std::cout << "new_bgr " << new_bgr.size() << " (" << new_bgr.channels() << " ch): ";
    for( unsigned int i = 0; i < 12; ++i){
        printf("%d ",*(new_bgr.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
    
    
    // done
    return 0;
}
