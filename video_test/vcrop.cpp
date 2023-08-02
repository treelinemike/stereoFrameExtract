// until we have a makefile or cmake, compile with:
// g++ -Wall -I/usr/include/opencv4 vcrop.cpp -lavutil -lavformat -lavcodec -lswscale -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -o vcrop  

// followed several ffmpeg examples to create this

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavfilter/avfilter.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
}
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <opencv2/opencv.hpp>

#define VIDEO_FILE "test_twoframe.mov"
//#define VIDEO_FILE "20230521_0deg_cal_L01.mov"

static AVFormatContext * ifmt_ctx;


int main(void){
    std::cout << "Analyzing " << VIDEO_FILE << "..." << std::endl;

    // open video file and read headers
    if( avformat_open_input(&ifmt_ctx, VIDEO_FILE, NULL, NULL) != 0 ){
        std::cout << "Could not open " << VIDEO_FILE << std::endl;
        return -1;
    }

    // get stream info from file
    if( avformat_find_stream_info( ifmt_ctx, NULL ) < 0){
        std::cout << "avformat_find_stream_info() failed!" << std::endl;
        return -1;
    }

    // print video format info to screen
    std::cout << "Number of streams found: " << ifmt_ctx->nb_streams << std::endl << std::endl;
    av_dump_format(ifmt_ctx, 0, VIDEO_FILE, false);
    std::cout << std::endl;

    // look for a single v210 video stream in this container
    unsigned int video_stream_idx = 0;
    bool found_video_stream = false;
    for(unsigned int stream_idx = 0; stream_idx < ifmt_ctx->nb_streams; ++stream_idx){
        AVStream *stream = ifmt_ctx->streams[stream_idx];
        AVCodecParameters *codec_params = stream->codecpar;

        printf("Located stream %02d: codec type = %03d, codec id = %03d\n",
                stream_idx,
                codec_params->codec_type,
                codec_params->codec_id);

        // determine whether this is v210 encoded video
        if( codec_params->codec_type == AVMEDIA_TYPE_VIDEO && codec_params->codec_id == AV_CODEC_ID_V210){
            //std::cout << "FOUND v210 ENCODED VIDEO" << std::endl;
            if(found_video_stream){
                std::cout << "ERROR: MORE THAN ONE v210 VIDEO STREAM FOUND!" << std::endl;
                return -1;
            } else {
                video_stream_idx = stream_idx;
                found_video_stream = true;
            }
        }
    }

    // error out if we didn't find v210 video
    if(!found_video_stream){
        std::cout << "ERROR: DID NOT FIND A VIDEO STREAM" << std::endl;
        return -1;
    }

    // report success
    AVStream *stream = ifmt_ctx->streams[video_stream_idx];
    AVCodecParameters *codec_params = stream->codecpar;
    printf("Working with stream %02d: codec type = %03d, codec id = %03d\n",
            video_stream_idx,
            codec_params->codec_type,
            codec_params->codec_id);
    std::cout << "stream timebase: " << (stream->time_base).num << "/" << (stream->time_base).den << std::endl;

    // open the decoder
    const AVCodec *dec = NULL;
    dec = avcodec_find_decoder(stream->codecpar->codec_id);
    if(!dec){
        std::cout << "ERROR: COULD NOT FIND DECODER" << std::endl;
        return -1;
    }

    // allocate decoder context
    AVCodecContext *dec_ctx;
    dec_ctx = avcodec_alloc_context3(dec);
    if(!dec_ctx){
        std::cout << "ERROR: COULD NOT OPEN DECODER CONTEXT" << std::endl;
        return -1;
    }

    // copy codec parameters from stream to decoder
    if( avcodec_parameters_to_context(dec_ctx, stream->codecpar) < 0){
        std::cout << "ERROR: COULD NOT COPY CODEC PARAMS FROM STREAM TO DECODER" << std::endl;
        return -1;
    }

    // initialize decoder
    if( avcodec_open2(dec_ctx, dec, NULL) < 0){
        std::cout << "ERROR: COULD NOT INITIALIZE DECODER" << std::endl;
        return -1;
    }

    // report frame width and height
    std::cout << "Width: " << dec_ctx->width << ", Height: " << dec_ctx->height << std::endl;
    int expected_width = dec_ctx->width;
    int expected_height = dec_ctx->height;
    AVPixelFormat expected_fmt = dec_ctx->pix_fmt;
    //std::cout << "decoder timebase: " << (dec_ctx->time_base).num << "/" << (dec_ctx->time_base).den << std::endl;


    // allocate image frame
    static uint8_t *video_dst_data[4] = {NULL};
    static int video_dst_linesize[4];
    if( av_image_alloc(video_dst_data, video_dst_linesize, dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, 1) < 0){
        std::cout << "ERROR: COULD NOT ALLOCATE IMAGE" << std::endl;
        return -1;
    }

    // allocate pointers for a packet and a frame
    AVPacket *pkt = NULL;
    pkt = av_packet_alloc();
    if(!pkt){
        std::cout << "ERROR: COULD NOT ALLOCATE DECODER PACKET POINTER" << std::endl;
        return -1;
    }

    static AVFrame *frame = NULL;
    frame = av_frame_alloc();
    if(!frame){
        std::cout << "ERROR: COULD NOT ALLOCATE FRAME POINTER" << std::endl;
        return -1;
    }

    // prepare an ENCODER to reencode video
    const AVCodec *enc = NULL;
    enc = avcodec_find_encoder(stream->codecpar->codec_id);
    if(!enc){
        std::cout << "ERROR: COULD NOT FIND ENCODER" << std::endl;
        return -1;
    }

    // allocate encoder context
    AVCodecContext *enc_ctx;
    enc_ctx = avcodec_alloc_context3(enc);
    if(!enc_ctx){
        std::cout << "ERROR: COULD NOT OPEN ENCODER CONTEXT" << std::endl;
        return -1;
    }

    // copy codec parameters from stream to encoder
    if( avcodec_parameters_to_context(enc_ctx, stream->codecpar) < 0){
        std::cout << "ERROR: COULD NOT COPY CODEC PARAMS FROM STREAM TO ENCODER" << std::endl;
        return -1;
    }

    // set encoder timebase
    enc_ctx->time_base = stream->time_base;

    // initialize encoder
    if( avcodec_open2(enc_ctx, enc, NULL) < 0){
        std::cout << "ERROR: COULD NOT INITIALIZE ENCODER" << std::endl;
        return -1;
    }




    // initialize some OpenCV matrices 
    cv::Mat cvFrameYUV(expected_height, expected_width, CV_8UC2);
    cv::Mat cvFrameBGR;

    // now we start reading
    unsigned int my_frame_counter = 0;
    int avreadframe_ret = 123;
    while( (avreadframe_ret = av_read_frame(ifmt_ctx,pkt)) >= 0 ){
        if((unsigned int)(pkt->stream_index) == video_stream_idx){
            // example "demux_decode.c" has a function: decode_packet(dec_ctx, pkt);

            // send packet to the decoder
            if( avcodec_send_packet(dec_ctx,pkt) < 0){
                std::cout << "ERROR: COULD NOT SEND PACKET TO DECODER" << std::endl;
                return -1;
            }

            // get all available frames from the decoder
            int retval = 0;
            while( retval >= 0 ){
                if((retval = avcodec_receive_frame(dec_ctx,frame)) == 0){


                    // we received a valid frame, so increment counter
                    ++my_frame_counter;
                }

                // unreference the frame pointer
                av_frame_unref(frame);
            }        
        }

        // unreference the packet pointer
        av_packet_unref(pkt);
    }

    std::cout << "Processed " << my_frame_counter << " frames!" << std::endl;

    // done
    return 0;
}





/*  OLD CODE TRYING OUT ENCODER
// get our encoder ready
AVCodec *tiff_codec = NULL;
if( (tiff_codec = avcodec_find_encoder(AV_CODEC_ID_TIFF)) == NULL){
std::cout << "ERROR: COULD NOT FIND OUTPUT IMAGE ENCODER" << std::endl;
return -1;
}

AVCodecContext * img_ctx = NULL;
if( !(img_ctx = avcodec_alloc_context3(tiff_codec)) ){
std::cout << "ERROR: COULD NOT ALLOCATE CONTEXT FOR IMAGES" << std::endl;
return -1;
}
img_ctx->pix_fmt = AV_PIX_FMT_YUV420P;//AV_PIX_FMT_YUV422P;//(AVPixelFormat)expected_pix_fmt;
img_ctx->time_base = (AVRational){1,1};
img_ctx->width = expected_width;
img_ctx->height = expected_height;
// initialize encoder
std::cout << "test" << std::endl;
if( avcodec_open2(img_ctx, tiff_codec, NULL) < 0){
std::cout << "ERROR: COULD NOT INITIALIZE ENCODER" << std::endl;
return -1;
}

AVPacket *pkt_enc = NULL;
pkt_enc = av_packet_alloc();
if(!pkt_enc){
std::cout << "ERROR: COULD NOT ALLOCATE ENCODER PACKET POINTER" << std::endl;
return -1;
}

*/
