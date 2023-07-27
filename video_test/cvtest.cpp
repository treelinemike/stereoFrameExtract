// g++ -Wall -I/usr/include/opencv4 cvtest.cpp -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -o cvtest

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.hpp>

int main(void){

    // READ IN TIFF, CONVERT TO YUV, CONVERT BACK TO BGR, WRITE TO TIFF
    cv::Mat im_yuv;
    cv::Mat im_bgr_2;
    cv::Mat im_raw = cv::imread("greensq.tif");
    cv::cvtColor(im_raw, im_yuv, cv::COLOR_BGR2YUV_I420);
    cv::cvtColor(im_yuv,im_bgr_2,cv::COLOR_YUV2BGR_I420);
    cv::imwrite("restore.tif",im_bgr_2);

    std::cout << "Original " << im_raw.size() << " (" << im_raw.channels() << " ch): ";
    for( unsigned int i = 0; i < 24; ++i){
        printf("%d ",*(im_raw.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
    
    std::cout << "YUV " << im_yuv.size() << " (" << im_yuv.channels() << " ch): ";
    for( unsigned int i = 0; i < 12; ++i){
        printf("%d ",*(im_yuv.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
 
    std::cout << "im_bgr_2 " << im_bgr_2.size() << " (" << im_bgr_2.channels() << " ch) :";
    for( unsigned int i = 0; i < 24; ++i){
        printf("%d ",*(im_bgr_2.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;


    // create YUV420 input synthetically to test memory structure, then convert back to BGR and save 
    cv::Mat yuv420_test(3,4,CV_8UC1);
    cv::Mat im_bgr_3;
    
    uint8_t *yuv420start = yuv420_test.ptr<uint8_t>();
    for(unsigned int j = 0; j < 8; ++j){
        *yuv420start = 88;
        ++yuv420start;
    }
    for(unsigned int k = 0; k < 2; ++k){
        *yuv420start = 187;
        ++yuv420start;
    }
    for(unsigned int l = 0; l < 2; ++l){
        *yuv420start = 115;
        ++yuv420start;
    }
    
    cv::cvtColor(yuv420_test,im_bgr_3,cv::COLOR_YUV2BGR_I420);
    cv::imwrite("out.tif",im_bgr_3);

    std::cout << "yuv420_test " << yuv420_test.size() << " (" << yuv420_test.channels() << " ch): ";
    for( unsigned int i = 0; i < 12; ++i){
        printf("%d ",*(yuv420_test.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
    
    std::cout << "im_bgr_3 " << im_bgr_3.size() << " (" << im_bgr_3.channels() << " ch): ";
    for( unsigned int i = 0; i < 24; ++i){
        printf("%d ",*(im_bgr_3.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
   
    
    // try this with *8bit* synthetic YUV422 input
    cv::Mat yuv422_test(2,4,CV_8UC2);
    cv::Mat im_bgr_4;
    std::cout << "is yuv422_test continuous? " << yuv422_test.isContinuous() << std::endl;
    
    uint8_t *yuv422start = yuv422_test.ptr<uint8_t>();
    yuv422start += 1;
    for(unsigned int j = 0; j < 8; ++j){
        *yuv422start = 88;
        yuv422start += 2;
    }
       
    yuv422start = yuv422_test.ptr<uint8_t>();
    for(unsigned int k = 0; k < 4; ++k){
        *yuv422start = 187;
        yuv422start += 4;
    }
    yuv422start = yuv422_test.ptr<uint8_t>();
    yuv422start += 2;
    for(unsigned int l = 0; l < 4; ++l){
        *yuv422start = 115;
        yuv422start += 4;
    }
   
    cv::cvtColor(yuv422_test,im_bgr_4,cv::COLOR_YUV2BGR_UYVY);
    cv::imwrite("out.tif",im_bgr_4);

    std::cout << "yuv422_test " << yuv422_test.size() << " (" << yuv422_test.channels() << " ch): ";
    for( unsigned int i = 0; i < 16; ++i){
        printf("%d ",*(yuv422_test.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;
    
    std::cout << "im_bgr_4 " << im_bgr_4.size() << " (" << im_bgr_4.channels() << " ch): ";
    for( unsigned int i = 0; i < 24; ++i){
        printf("%d ",*(im_bgr_4.ptr<uint8_t>()+i));
    }
    std::cout << std::endl;

   
    // manual conversion of *8bit* YUV422 to BGR
    



    // repeat with *16bit* synthetic YUV422 input
    cv::Mat yuv422_test_2(2,4,CV_16UC2);
    cv::Mat im_bgr_5;
    std::cout << "is yuv422_test_2 continuous? " << yuv422_test_2.isContinuous() << std::endl;
    
    uint16_t *yuv422start_2 = yuv422_test_2.ptr<uint16_t>();
    yuv422start_2 += 1;
    for(unsigned int j = 0; j < 8; ++j){
        *yuv422start_2 = (88 << 0);
        yuv422start_2 += 2;
    }
       
    yuv422start_2 = yuv422_test_2.ptr<uint16_t>();
    for(unsigned int k = 0; k < 4; ++k){
        *yuv422start_2 = (187 << 0);
        yuv422start_2 += 4;
    }
    yuv422start_2 = yuv422_test_2.ptr<uint16_t>();
    yuv422start_2 += 2;
    for(unsigned int l = 0; l < 4; ++l){
        *yuv422start_2 = (115 << 0);
        yuv422start_2 += 4;
    }
   
    //THIS FAILS!
    //cv::cvtColor(yuv422_test_2,im_bgr_5,cv::COLOR_YUV2BGR_UYVY,2);
    //cv::imwrite("out_16bit.tif",im_bgr_5);



    std::cout << "yuv422_test_2 " << yuv422_test_2.size() << " (" << yuv422_test_2.channels() << " ch): ";
    for( unsigned int i = 0; i < 16; ++i){
        printf("%d ",*(yuv422_test_2.ptr<uint16_t>()+i));
    }
    std::cout << std::endl;
    
    std::cout << "im_bgr_5 " << im_bgr_5.size() << " (" << im_bgr_5.channels() << " ch): ";
    for( unsigned int i = 0; i < 24; ++i){
        printf("%d ",*(im_bgr_5.ptr<uint16_t>()+i));
    }
    std::cout << std::endl;


    
    // done
    return 0;
}
