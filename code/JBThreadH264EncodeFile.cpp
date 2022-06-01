//
//  JBThreadH264EncodeFile.cpp
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#include "JBThreadH264EncodeFile.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"



/**
 * 步骤：
 * 1.  打开输入 输出 文件
 * 2.  avcodec_find_encoder_by_name || avcodec_find_encoder 获取编码器 - libx264
 * 3.  codec->check_fmt 遍历编码器支持的format，检测输入format是否支持，不支持需要重采样
 * 4.  avcodec_alloc_context3 创建编码器的上下文
 * 5.  avcodec_open2 配置好上下文的 参数后 打开编码器。 如果有当前编码器自带的参数如：vbr，获取命令行参数，在这里通过AVDictionary传入
 * 6.  av_frame_alloc 创建 AVFrame 对象，用来存取输入缓冲区的数据
 * 7.  配置AVFrame的数据
 * 8.  通过6/7步配置的数据，用av_image_alloc来真正创建 输入缓冲区
 * 9.  av_packet_alloc 创建 AVPacket 对象，用来接收 编码后的数据，即输出缓冲区
 * 10. av_image_get_buffer_size 获取单帧图片的yuv大小
 * 11. 准备完成，开始进入编码步骤
 * 12. avcodec_send_frame 将 上下文 和  AVFrame 发送到编码器中. 也就是 AVFrame数据 编码到了 上下文context 中。
 * 13. avcodec_receive_packet 循环读取 context 的数据 到 AVPacket中， 需要特殊处理EAGAIN 和 EOF错误
 * 14. 将 AVPacket 数据写入到 输出文件中
 * 15. 释放av_packet_unref AVPacket资源
 * 16. 重复13-15步知道编码的数据读完
 * 17. 重复10步，继续读取下一次数据，然后重复进行编码
 * 18. 读取和编码完成后，将输入frame 设为 nullptr 后 再次 调用一次 12-15 步，确保缓冲区数据全部清空
 * 19. 释放所有对象
 *
 *
 * 流程: inputFile -> AVFrame -> codec context -> AVPacket -> outputFile
 */

JBThreadH264EncodeFile::JBThreadH264EncodeFile(QObject* parent)
: JBThreadBase{ parent }
{
    
}


static int check_fmt(const AVCodec* codec, AVPixelFormat pix_fmt) {
    const enum AVPixelFormat* p = codec->pix_fmts;
    
    while (*p != AV_PIX_FMT_NONE) {
        qDebug() << "support fmt: " << av_get_pix_fmt_name(*p);
        if (*p == pix_fmt) return 1;
        p++;
    }
    return 0;
}


static int startEncoder(AVFrame* frame, AVPacket* packet, AVCodecContext* context, QFile *outFile) {
    
    /* send the frame to the encoder */
      if (frame)
          qDebug() << "Send frame " <<  frame->pts;
    
    int ret = avcodec_send_frame(context, frame);
    if (ret < 0)
    {
        JBCommonMethod::logError("avcodec_send_frame", ret);
        return ret;
    }
    
    while (true)
    {
        ret = avcodec_receive_packet(context, packet);

        if (ret < 0) {
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                return 0;
            }
            return ret;
        }
        outFile->write((char*)packet->data, packet->size);
        av_packet_unref(packet);
    }
}

void JBThreadH264EncodeFile::h264Encoder(JBYUVData& yuvModel, const QString& outName) {
    
    QFile inFile(yuvModel.file);
    if (!inFile.open(QFile::ReadOnly)) {
        qDebug() << "open file failed: " << yuvModel.file;
        return;
    }
    
    QFile outFile(outName);
    if (!outFile.open(QFile::WriteOnly)) {
        qDebug() << "open file failed: " << outName;
        inFile.close();
        return;
    }
    qDebug() << inFile.fileName() << "-->" << outFile.fileName();
    
    AVCodecContext* context = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    
    int imgSize = 0;
    
    int ret = 0;

    //获取h264 编码器
    const AVCodec *codec  = avcodec_find_encoder_by_name("libx264");
//    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (codec == nullptr)
    {
        qDebug() << "avcodec_find_encoder_by_name failed";
        goto end;
    }
    
    //检测  当前的pcm数据 是否符合  fdk_aac 编码器的 要求。
    ret = check_fmt(codec, yuvModel.pixelFormat);
    if (ret == 0)
    {
        qDebug() << "check_sample_fmt not support" << av_get_pix_fmt_name(yuvModel.pixelFormat);
        goto end;
    }
    
    //创建编码的上下文对象
    context = avcodec_alloc_context3(codec);
    if (context == nullptr)
    {
        qDebug() << "avcodec_alloc_context3 failed";
        goto end;
    }
    context->time_base = {1,yuvModel.fps};
    context->pix_fmt = yuvModel.pixelFormat;
    context->width = yuvModel.width;
    context->height = yuvModel.height;
    
    qDebug() << "input data: " << av_get_pix_fmt_name(yuvModel.pixelFormat) << yuvModel.width << yuvModel.height << yuvModel.fps;
    
    ret = avcodec_open2(context, codec, nullptr);
    if (ret < 0)
    {
        JBCommonMethod::logError("avcodec_open2", ret);
        goto end;
    }
    
    
    //输入 配置
    frame =  av_frame_alloc();
    if (!frame)
    {
        JBCommonMethod::logError("av_frame_alloc", ret);
        goto end;
    }
    
    frame->format = context->pix_fmt;
    frame->width = context->width;
    frame->height = context->height;
//    frame->time_base = context->time_base;
    frame->pts = 0;
    
//    //方法1： 创建frams data 的输入缓冲区
//    ret = av_frame_get_buffer(frame, 0);
//    if (ret < 0)
//    {
//        JBCommonMethod::logError("av_frame_get_buffer", ret);
//        goto end;
//    }
//
    
    //方法2： 利用width、height、format创建缓冲区
    ret = av_image_alloc(frame->data, frame->linesize,
                         yuvModel.width, yuvModel.height, yuvModel.pixelFormat, 1);
    if (ret < 0) {
        JBCommonMethod::logError("av_image_alloc", ret);
        goto end;
    }
    
    //输出 配置
    packet = av_packet_alloc();
    if (!packet)
    {
        JBCommonMethod::logError("av_packet_alloc", ret);
        goto end;
    }
    
    imgSize =  av_image_get_buffer_size(context->pix_fmt, context->width, context->height, 1);
    if (imgSize < 0)
    {
        JBCommonMethod::logError("av_image_get_buffer_size", ret);
        goto end;
    }
    qDebug() << "read line size:" << imgSize;

    while ((ret = inFile.read((char*)frame->data[0], imgSize)) > 0)
    {
        if (startEncoder(frame, packet, context, &outFile) < 0) {
            goto  end;
        }
        // 设置帧的序号
         frame->pts++;
    }
    
    
    //最后刷新下 缓冲区的数据
    startEncoder(nullptr, packet, context, &outFile);
    
    emit comandLineGenerate(QString("ffplay %1").arg(outFile.fileName()));

    outFile.flush();
    qDebug() << "comandLineSize: " << 343447;
    qDebug() << "currentfilSize: " << outFile.size();
    qDebug() << "----------left: " << (343447 - outFile.size());
    
end:
    outFile.close();
    inFile.close();
    
    av_frame_free(&frame);
    
    av_packet_free(&packet);
    avcodec_free_context(&context);
    
}



void JBThreadH264EncodeFile::run() {
    
    JBYUVData yuvData;
    yuvData.file = k_out_temp_dir + "movie_640x480_yuv420p.yuv";
    yuvData.width = 640;
    yuvData.height = 480;
    yuvData.pixelFormat = AV_PIX_FMT_YUV420P;
    yuvData.fps = 25;
    
    h264Encoder(yuvData, k_out_temp_dir+"movie_640x480_yuv420p.h264");
    
}
