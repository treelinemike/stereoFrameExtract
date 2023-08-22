// extract frames from a v210- or FFV1-encoded MOV video file to 16-bit RGB TIFF  

// followed several ffmpeg examples to create this

// allow windows builds which would otherwise
// compain about using fopen() instead of fopen_s()
// and sprintf() instead of sprintf_s(), etc...
#define _CRT_SECURE_NO_DEPRECATE

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}
#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
#include <cxxopts.hpp>
#include <chrono>

// CROPPING PARAMETERS FOR REDUICNG YUV422 (v210) VIDEO TO DA VINCI XI 720p FRAME SIZE
// NOTE: WILL LEAVE 1px BLACK BAR ON LEFT AND RIGHT, OTHERWISE WE'D BE SPLITING/SHIFTING CHROMA COMPONENTS
// ONCE FRAME IS CONVERTED TO RGB WE CAN CROP AGAIN
#define DAVINCI_FULL_CROP_WIDTH 894
#define DAVINCI_FULL_CROP_HEIGHT 714
#define DAVINCI_FULL_CROP_X 193       // zero indexed, first COLUMN to include in cropped ouput, USE FOR RGB ONLY
#define DAVINCI_FULL_CROP_Y 3         // zero indexed, first ROW to include in cropped output
#define DAVINCI_FINISH_CROP_WIDTH 894
#define DAVINCI_FINISH_CROP_HEIGHT 714
#define DAVINCI_FINISH_CROP_X 1         // zero indexed, first COLUMN to include in cropped ouput, USE FOR RGB ONLY
#define DAVINCI_FINISH_CROP_Y 0         // zero indexed, first ROW to include in cropped output


static AVFormatContext * ifmt_ctx;

// prepare a filter graph for cropping frames out of black border (da Vinci Xi)
// ref: https://stackoverflow.com/questions/38099422/how-to-crop-avframe-using-ffmpegs-functions
int crop_frame(const AVFrame* inframe, AVFrame* outframe, AVFilterContext* bufsrc_ctx, AVFilterContext* bufsnk_ctx) {

    // copy input frame to output frame
    if (av_frame_ref(outframe, inframe) < 0) {
        std::cout << "ERROR COPYING INPUT FRAME TO OUTPUT FRAME" << std::endl;
        return -1;
    }

    if (av_buffersrc_add_frame(bufsrc_ctx, outframe) < 0) {
        std::cout << "ERROR ADDING FRAME TO SOURCE BUFFER CONTEXT" << std::endl;
        return -1;
    }

    if (av_buffersink_get_frame(bufsnk_ctx, outframe) < 0) {
        std::cout << "ERROR ADDING FRAME TO SINK BUFFER CONTEXT" << std::endl;
        return -1;
    }

    // return 0 on success
    // cropped frame is in *outframe
    return 0;
}

int write_avframe_to_file(AVFrame *frame, unsigned int frame_num){

    const AVCodec *tiff_codec = NULL;
    AVCodecContext *tiff_context = NULL;
    AVPacket *packet = NULL;
    FILE *outfile;
    char outfile_name[256];

    // find encoder
    if( (tiff_codec = avcodec_find_encoder(AV_CODEC_ID_TIFF)) == NULL ){
        std::cout << "ERROR FINDING TIFF ENCODER" << std::endl;
        return -1;
    }


    if( (tiff_context = avcodec_alloc_context3(tiff_codec)) == NULL ){
        std::cout << "ERROR ALLOCATING TIFF ENCODER" << std::endl;
        return -1;
    }

    tiff_context->pix_fmt = (AVPixelFormat) frame->format;
    tiff_context->height = frame->height;
    tiff_context->width = frame->width;
    tiff_context->time_base = AVRational{1,10};

    // set compression type (given in smallest to largest file order)
    av_opt_set(tiff_context->priv_data,"compression_algo","deflate",0);
    //av_opt_set(tiff_context->priv_data,"compression_algo","packbits",0);
    //av_opt_set(tiff_context->priv_data,"compression_algo","lzw",0);
    //av_opt_set(tiff_context->priv_data,"compression_algo","raw",0);

    if( avcodec_open2(tiff_context, tiff_codec, NULL) < 0 ){
        std::cout << "ERROR: CANNOT OPEN TIFF CODEC" << std::endl;
        return -1;
    }
    //av_opt_show2(tiff_context,NULL,AV_OPT_FLAG_ENCODING_PARAM,0);

    // prepare packet
    if( (packet = av_packet_alloc()) == NULL){
        std::cout << "ERROR: CANNOT ALLOCATE PACKET" << std::endl;
        return -1;
    }
    packet->data = NULL;
    packet->size = 0;

    // send frame to TIFF encoder
    if(avcodec_send_frame(tiff_context, frame) < 0){
        std::cout << "ERROR SENDING FRAME TO TIFF ENCODER" << std::endl;
        return -1;
    } 

    // retrieve encoded TIFF packet
    if(avcodec_receive_packet(tiff_context,packet) < 0){
        std::cout << "ERROR RETRIEVING PACKET FROM TIFF ENCODER" << std::endl;
        return -1;
    }

    // write packet to file
    sprintf(outfile_name,"frame%08d.tif",frame_num);
    outfile = fopen(outfile_name,"wb");
    fwrite(packet->data,1,packet->size,outfile);
    fclose(outfile);

    // clean up
    av_packet_unref(packet);
    avcodec_close(tiff_context);
    return 0;
}

int main(int argc, char ** argv){

    cxxopts::Options options("vtiff","video frame extraction to tiff");
    std::string framelist_file_name, input_file_name;    
    std::vector<uint64_t> framelist;
    std::string temp, line, word;
    std::fstream framelist_file;
    uint64_t pts_dts_scale;
    bool framecrop_flag = false;
    bool display_flag = false;

    // filter graph variables
    AVFilterContext* bufsrc_ctx = NULL;
    AVFilterContext* bufsnk_ctx = NULL;
    AVFilterGraph* fltgrph = avfilter_graph_alloc();
    AVFilterInOut* filt_in = avfilter_inout_alloc();
    AVFilterInOut* filt_out = avfilter_inout_alloc();
    AVFrame* frame_cropped = av_frame_alloc();
    char filtarg[255];
    int crop_w, crop_h, crop_x, crop_y;

    // add options
    try
    {
        options.add_options()
            ("c,crop", "boolean flag for cropping to da Vinci Xi frame",cxxopts::value<bool>()->default_value("false"))
            ("d,display", "display extracted frames using OpenCV",cxxopts::value<bool>()->default_value("false"))
            ("f,framelist" , "csv file containing frame numbers to extract", cxxopts::value<std::string>())
            ("i,input" , "name of input file", cxxopts::value<std::string>());
        auto cxxopts_result = options.parse(argc,argv);
        
        // show help if not enough options given
        if(     (cxxopts_result.count("framelist") != 1) ||
                (cxxopts_result.count("input") != 1) ) {
            std::cout << options.help() << std::endl;
            return -1;
        }

        framecrop_flag = cxxopts_result["crop"].as<bool>();
        display_flag = cxxopts_result["display"].as<bool>();
        framelist_file_name = cxxopts_result["framelist"].as<std::string>(); 
        input_file_name = cxxopts_result["input"].as<std::string>(); 

    }
    catch(const cxxopts::exceptions::exception &e)
    {
        std::cout << "Error parsing options: " << e.what() << std::endl;
        return -1;
    }

    // load framelist csv into a vector
    framelist_file.open(framelist_file_name, std::ios::in); 
    framelist.clear();
    while( std::getline(framelist_file, line) ){
        std::stringstream s(line);
        while(std::getline(s,word,',')){
            framelist.push_back((uint64_t)std::stol(word));
        }
    }
    std::cout << "Extracting frames: " << std::endl;
    for(auto & val : framelist)
        std::cout << " " << val << std::endl;

    std::cout << "Analyzing " << input_file_name << "..." << std::endl;

    // open video file and read headers
    if( avformat_open_input(&ifmt_ctx, input_file_name.c_str(), NULL, NULL) != 0 ){
        std::cout << "Could not open " << input_file_name << std::endl;
        return -1;
    }

    // get stream info from file
    if( avformat_find_stream_info( ifmt_ctx, NULL ) < 0){
        std::cout << "avformat_find_stream_info() failed!" << std::endl;
        return -1;
    }

    // print video format info to screen
    std::cout << "Number of streams found: " << ifmt_ctx->nb_streams << std::endl << std::endl;
    av_dump_format(ifmt_ctx, 0, input_file_name.c_str(), false);
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
        if( codec_params->codec_type == AVMEDIA_TYPE_VIDEO && (codec_params->codec_id == AV_CODEC_ID_V210 || codec_params->codec_id == AV_CODEC_ID_FFV1)){
            //std::cout << "FOUND v210 OR FFV1 ENCODED VIDEO" << std::endl;
            if(found_video_stream){
                std::cout << "ERROR: MORE THAN ONE READABLE VIDEO STREAM FOUND!" << std::endl;
                return -1;
            } else {    
                video_stream_idx = stream_idx;
                found_video_stream = true;
            }
        }
    }

    // error out if we didn't find v210 or FFV1 video
    if(!found_video_stream){
        std::cout << "ERROR: DID NOT FIND A PROPERLY ENCODED VIDEO STREAM" << std::endl;
        return -1;
    }

    // report success
    AVStream *stream = ifmt_ctx->streams[video_stream_idx];
    AVCodecParameters *codec_params = stream->codecpar;
    pts_dts_scale = (uint64_t)av_q2d(av_mul_q(av_inv_q(stream->time_base),av_inv_q(stream->avg_frame_rate)));

    printf("Working with stream %02d: codec type = %03d, codec id = %03d\n",
            video_stream_idx,
            codec_params->codec_type,
            codec_params->codec_id);


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

    AVFrame *converted_frame = NULL;
    converted_frame = av_frame_alloc();
    if(!frame){
        std::cout << "ERROR: COULD NOT ALLOCATE CONVERTED FRAME POINTER" << std::endl;
        return -1;
    } 
    converted_frame->width = expected_width;
    converted_frame->height = expected_height;
    converted_frame->format = AV_PIX_FMT_RGB48LE;
    if( av_frame_get_buffer(converted_frame,0) < 0){
        std::cout << "ERROR: COULD NOT GET BUFFER FOR CONVERTED FRAME" << std::endl;
        return -1;
    }

    // create software scaling context (for colorspace / pixel format conversion, not scaling...)
    SwsContext *sws_ctx = NULL;  
    if( (sws_ctx = sws_getContext(  expected_width,
                    expected_height,
                    expected_fmt,
                    converted_frame->width,
                    converted_frame->height,
                    (AVPixelFormat)converted_frame->format,
                    SWS_POINT,
                    nullptr,
                    nullptr,
                    nullptr)) == NULL){
        std::cout << "ERROR: COULD NOT GET/ALLOCATE SWS CONTEXT" << std::endl;
        return -1;
    }

    // PREPARE FILTER FOR CROPPING DOWN TO DAVINCI XI FRAME
    // prepare filter graph for cropping image to size
    // WE NEED THE DECODER ACTIVE TO DO THIS!
    // TODO: ADD A DIFFERENT FLAG (not just compress_flag)
    if (framecrop_flag && (dec_ctx->width == 1280) && (dec_ctx->height == 720)) {
        crop_w = DAVINCI_FULL_CROP_WIDTH;
        crop_h = DAVINCI_FULL_CROP_HEIGHT;
        crop_x = DAVINCI_FULL_CROP_X;
        crop_y = DAVINCI_FULL_CROP_Y;
    }
    else if (framecrop_flag && (dec_ctx->width == 896) && (dec_ctx->height == 714)) {
        crop_w = DAVINCI_FINISH_CROP_WIDTH;
        crop_h = DAVINCI_FINISH_CROP_HEIGHT;
        crop_x = DAVINCI_FINISH_CROP_X;
        crop_y = DAVINCI_FINISH_CROP_Y;
    } else if (framecrop_flag) {
        framecrop_flag = false;
        //std::cout << "\033[1;35mInput video dimensions not compatible with cropping, output will not be cropped\033[0m" << std::endl;
        std::cout << "Input video dimensions not compatible with cropping, output will not be cropped" << std::endl;
    }
    if (framecrop_flag) {
        snprintf(filtarg, sizeof(filtarg),
            "buffer=video_size=%dx%d:pix_fmt=%d:time_base=1/1:pixel_aspect=0/1[in];"
            "[in]crop=x=%d:y=%d:out_w=%d:out_h=%d:exact=true[out];"
            "[out]buffersink",
            dec_ctx->width, dec_ctx->height, converted_frame->format,
            crop_x, crop_y, crop_w, crop_h);
        std::cout << "filtarg: " << filtarg << std::endl;
        if (avfilter_graph_parse2(fltgrph, filtarg, &filt_in, &filt_out) < 0) {
            std::cout << "ERROR PARSING FILTER GRAPH" << std::endl;
            return -1;
        }
        if (filt_in != NULL) {
            std::cout << "ERROR: NON-NULL FILTER INPUTS RETURNED BY avfilter_graph_parse2()" << std::endl;
            return -1;
        }
        if (filt_out != NULL) {
            std::cout << "ERROR: NON-NULL FILTER OUPUTS RETURNED BY avfilter_graph_parse2()" << std::endl;
            return -1;
        }
        if (avfilter_graph_config(fltgrph, NULL) < 0) {
            std::cout << "ERROR CONFIGURING FILTER GRAPH" << std::endl;
            return -1;
        }
        if ((bufsrc_ctx = avfilter_graph_get_filter(fltgrph, "Parsed_buffer_0")) == 0) {
            std::cout << "ERROR GETTING SOURCE FILTER CONTEXT" << std::endl;
            return -1;
        }
        if ((bufsnk_ctx = avfilter_graph_get_filter(fltgrph, "Parsed_buffersink_2")) == 0) {
            std::cout << "ERROR GETTING SINK FILTER CONTEXT" << std::endl;
            return -1;
        }
    }

    // initialize some OpenCV matrices 
    cv::Mat cvFrameYUV(expected_height, expected_width, CV_8UC2);
    cv::Mat cvFrameBGR;

    // start timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // find the exact frame that was requested
    unsigned int my_frame_counter = 0;
    for(auto & val : framelist){

        std::cout << "Attempting to decode frame " << val << std::endl;
        if( av_seek_frame(ifmt_ctx,video_stream_idx,val*pts_dts_scale, AVSEEK_FLAG_BACKWARD) < 0){ // seeks to closest previous keyframe
            std::cout << "ERROR: SEEK FAILED" << std::endl;
            return -1;
        }
        bool found_frame = false;
        while(!found_frame){
            if(av_read_frame(ifmt_ctx,pkt) < 0){
                std::cout << "ERROR: COULD NOT READ FRAME" << std::endl;
                return -1;
            }
            if(((unsigned int)(pkt->stream_index) == video_stream_idx)){

                std::cout << " Decoding frame " << (uint64_t)(pkt->dts/pts_dts_scale) << std::endl;

                // determine whether we're at our desired frame or if we've passed it
                // if we're before it we need to decode everything from the previous keyframe
                if ((uint64_t)pkt->dts == val * pts_dts_scale) {    // we happened to seek right to where we wanted to be
                    found_frame = true;
                }
                else if ((uint64_t)pkt->dts > (val * pts_dts_scale)) { // we missed our frame!
                    std::cout << "ERROR: SEEK PASSED THE DESIRED FRAME!" << std::endl;
                    return -1;
                }

                // send packet to the decoder
                if( avcodec_send_packet(dec_ctx,pkt) < 0){
                    std::cout << "ERROR: COULD NOT SEND PACKET TO DECODER" << std::endl;
                    return -1;
                }

                // get all available frames from the decoder
                int retval = 0;
                while( retval >= 0 ){
                    if((retval = avcodec_receive_frame(dec_ctx,frame)) == 0){

                        // we have a valid AVFrame (frame) from the file video stream
                        // only process the frame if it is the one we want
                        if (found_frame) {

                            // this AVFrame has YUV422 pixel format, so convert it to
                            // RGB444 (16bit)
                            //std::cout << "frame: " << frame->width << "x" << frame->height << std::endl;
                            if (sws_scale_frame(sws_ctx, converted_frame, frame) < 0) {
                                std::cout << "ERROR: COULD NOT CONVERT FRAME" << std::endl;
                                return -1;
                            }

                            // crop frame if we want to do this
                            // needs to be here because we can't crop YUV422 with odd offsets
                            if (framecrop_flag) {
                                //std::cout << "converted_frame: " << converted_frame->width << "x" << converted_frame->height << std::endl;
                                crop_frame(converted_frame, frame_cropped, bufsrc_ctx, bufsnk_ctx);
                                //std::cout << "frame_cropped: " << frame_cropped->width << "x" << frame_cropped->height << std::endl;
                                av_frame_unref(converted_frame);
                                av_frame_ref(converted_frame, frame_cropped);
                            }

                            // write frame to file using only ffmpeg (lavf, lavc, etc...)
                            write_avframe_to_file(converted_frame, my_frame_counter);


                            // WE CAN ALSO USE OPENCV TO DISPLAY AND SAVE TIF FILE
                            // CONFIRMED 02-AUG-23 THAT TIFF PIXEL DATA IS IDENTICAL (AFTER LOADING INTO MATLAB)
                            if (display_flag) {

                                // generally we could just point the cv::Mat to converted_frame->data[0]
                                // but after cropping we may end up with extra data outside the frame
                                // so to be sure we'll just copy over line by line
                                // otherwise we could have copied the RGB frame using:
                                // cv::Mat cv_frame_rgb(expected_height, expected_width, CV_16UC3, (uint16_t*)converted_frame->data[0]);
                                cv::Mat cv_frame_rgb(converted_frame->height, converted_frame->width, CV_16UC3);
                                uint16_t* cvptr = cv_frame_rgb.ptr<uint16_t>();
                                uint16_t* avptr = NULL;
                                uint16_t linesize = converted_frame->linesize[0];
                                for (uint16_t rowidx = 0; rowidx < converted_frame->height; ++rowidx) {
                                    avptr = (uint16_t*)converted_frame->data[0] + rowidx * (linesize / 2);
                                    memcpy(cvptr, avptr, (uint64_t)6 * (uint64_t)converted_frame->width);
                                    cvptr += 3 * converted_frame->width;
                                }

                                // convert from RGB to BGR so we can display/manipulate/save with OpenCV
                                cv::Mat cv_frame_bgr;
                                cv::cvtColor(cv_frame_rgb, cv_frame_bgr, cv::COLOR_RGB2BGR);
                                
                                // write  to file
                                // char img_filename[255]; 
                                // sprintf(img_filename,"cv_frame_%08d.tif",my_frame_counter);
                                // cv::imwrite(img_filename,cv_frame_bgr);

                                // display the image on screen with OpenCV
                                char framenum_str[255];
                                sprintf(framenum_str, "%ld", val);
                                cv::putText(cv_frame_bgr, (std::string)framenum_str, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 2.0, CV_RGB(0, 65535, 0), 6);
                                cv::imshow("my window", cv_frame_bgr);
                                cv::waitKey(800);
                            }

                            std::cout << "Saved frame " << (uint64_t)(pkt->dts / pts_dts_scale) << std::endl;

                            // we received a valid frame, so increment counter
                            ++my_frame_counter;
                        }
                    }

                    // unreference the frame pointers
                    av_frame_unref(frame);
                    av_frame_unref(converted_frame);
                    av_frame_unref(frame_cropped);
                }        
            }

            // unreference the packet pointer
            av_packet_unref(pkt);
        }
    }

    // stop timer
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    std::cout << "Saved " << my_frame_counter << " frames in " << duration.count() << " seconds" << std::endl;

    // free filter graph
    avfilter_inout_free(&filt_in);
    avfilter_inout_free(&filt_out);
    avfilter_graph_free(&fltgrph);

    // free memory
    avformat_close_input(&ifmt_ctx);
    avformat_free_context(ifmt_ctx);

    // done
    return 0;
}