#include "JBThreadAudioRecordWav.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

/*
 * 具体逻辑步骤
 * 1： 打开 文件
 * 2： 打开音频设备
 * 3： 根据音频设备的属性，将具体配置写入wavheader
 * 4： 将wavheader 写入文件
 * 5： 将采集到pcm 持续写入文件
 * 6： 统计采集到的总大小，调整文件的header头的写入的size大小
 * 7： 关闭文件
 */

JBThreadAudioRecordWav::JBThreadAudioRecordWav(QObject *parent)
: JBThreadBase{parent}
{
    connect(this, &JBThreadAudioRecordWav::finished, this, &JBThreadAudioRecordWav::deleteLater);
}


void getFileName(QString &fileName) {
    fileName.clear();
    fileName += k_out_temp_dir;
    fileName += QDateTime::currentDateTime().toString("MM_dd_HH_mm_ss");
    fileName += ".wav";
    qDebug() << fileName;
}

void writeTheDataSizeToFile(WavHeader& header, QFile& file, int64_t totalSize) {
    //最好强刷以下buffer
    file.flush();
    
    //写入riffDataSize
    file.seek(offsetof(WavHeader, riffDataSize));
    uint32_t riffDataSize = static_cast<uint32_t>(totalSize - sizeof(WavHeader) - offsetof(WavHeader, format));
    file.write((const char*)(&riffDataSize), sizeof(header.riffDataSize));
    qDebug() << "riffDataSize" << riffDataSize;
    
    //写入dataSize
    file.seek(offsetof(WavHeader, dataSize));
    file.write((char*)(&totalSize), sizeof(header.dataSize));
    qDebug() << "totalSize: " << totalSize;
}

void JBThreadAudioRecordWav::run() {
    auto startTime = JBCommonMethod::getCurrentTimeStamp();
    //获取输入格式对象
    const AVInputFormat *fmt = av_find_input_format(k_fmt_name.toUtf8().constData());
    if (!fmt) {
        qDebug() << "av_find_input_format failed" << k_fmt_name.toUtf8().constData();
        return;
    }
    
    // 打开设备
    AVFormatContext *ctx = nullptr;
    auto ret = avformat_open_input(&ctx, k_device_name_audio_first.toUtf8().constData(), fmt, nullptr);
    if (ret < 0) {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "avformat_open_input failed: " << errbuf << ret;
        return;
    }
    
    // 打印一下录音设备的参数信息
    JBCommonMethod::logAvformat(ctx);
    
    QString fileName;
    getFileName(fileName);
    QFile file(fileName);
    //会清空文件
    bool isSucceed = file.open(QIODevice::ReadWrite | QIODevice::Truncate);
    if (!isSucceed)
    {
        qDebug() << "file.open failed";
        avformat_close_input(&ctx);
        return;
    }

    emit comandLineGenerate(QString("ffplay %1").arg(file.fileName()));

    // 写入wav的头
    WavHeader header;
    JBCommonMethod::setWavHeaderWithCtx(header, ctx);
    
    file.write((const char*)&header, sizeof(WavHeader));
    
    AVPacket* pkt = av_packet_alloc();
    
    int64_t totalSize = 0;
    
    while (!isInterruptionRequested())
    {
        emit timeChanged(JBCommonMethod::getCurrentTimeStamp() - startTime);
        ret = av_read_frame(ctx, pkt);
        if (ret == AVERROR(EAGAIN)) {
            // 资源临时不可用
            continue;
        }
        else if (ret < 0)
        {
            JBCommonMethod::logError("av_read_frame", ret);
            break;
        }
        
        file.write((const char*)pkt->data, pkt->size);
        totalSize += pkt->size;
        //不用了，释放
        av_packet_unref(pkt);
    }
    
    //写入wav header 的补充信息
    writeTheDataSizeToFile(header, file, totalSize);
    
    //关文件
    file.close();
    //释放资源
    av_packet_free(&pkt);
    //关闭设备
    avformat_close_input(&ctx);
    qDebug() << this << "exit 0";
}
