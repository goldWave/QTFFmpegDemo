#include "JBThreadVideoRecordYUV.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"

/**
* 步骤：

*/


const static char *k_video_size = "640x480";
const static char *k_framerate = "30";
const static char *k_pixel_format = "yuyv422";

JBThreadVideoRecordYUV::JBThreadVideoRecordYUV(QObject *parent)
: JBThreadBase(parent)
{
}


void JBThreadVideoRecordYUV::run() {
    qDebug() << "\n\n\n\n";
    auto startTime = JBCommonMethod::getCurrentTimeStamp();

    QFile outFile;
    const AVInputFormat *inputFmt = nullptr;
    AVFormatContext *context = nullptr;
    AVDictionary *d = nullptr;
    int ret = 0;
    bool isSucceed;
    AVPacket *pkt = nullptr;
    QString cmdLine;
    QString fileName(k_out_temp_dir);
    fileName += "test.yuv";
    outFile.setFileName(fileName);
    
    inputFmt =  av_find_input_format(k_fmt_name.toUtf8().constData());
    if (!inputFmt)
    {
        JBCommonMethod::logError("av_find_input_format");
        goto end;
    }
    
    av_dict_set(&d, "video_size", k_video_size, 0);
    av_dict_set(&d, "framerate", k_framerate, 0);
    av_dict_set(&d, "pixel_format", k_pixel_format, 0);
    qDebug() << "start set" << k_device_name_video_first << k_device_name_video_first.toUtf8().constData();
    
    ret =  avformat_open_input(&context, k_device_name_video_first.toUtf8().constData(), inputFmt, &d);
    if (!context) {
        JBCommonMethod::logError("avformat_open_input", ret);
        goto end;
    }
    
    isSucceed = outFile.open(QIODevice::WriteOnly|QIODevice::Truncate);
    if (!isSucceed)
    {
        qDebug() << "file.open failed";
        goto end;
    }
    
    pkt = av_packet_alloc();
    
    while (!isInterruptionRequested()) {
        ret = av_read_frame(context, pkt);
        emit timeChanged(JBCommonMethod::getCurrentTimeStamp() - startTime);
        if (ret == AVERROR(EAGAIN)) {
            continue;;
        }
        if (ret < 0) {
            JBCommonMethod::logError("av_read_frame", ret);
            break;
        }
        
        qDebug() << "size "<< pkt->size;
        outFile.write((const char *)pkt->data, pkt->size);
        av_packet_unref(pkt);
    }
    
    cmdLine = QString("ffplay -video_size %1 -pixel_format %2 %3").arg(k_video_size).arg(k_pixel_format).arg(outFile.fileName());
    
    emit comandLineGenerate(cmdLine);
    
end:
    avformat_close_input(&context);
    av_packet_free(&pkt);
    outFile.close();
    av_dict_free(&d);

}

