#include "JBThreadAudioRecordPCM.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"

/**
* 步骤：
* 1. av_find_input_format 获取录音设备的具体参数
* 2. avformat_open_input 打开设备， 创建AVFormatContext设备
* 3. 打开写入的文件
* 4. av_packet_alloc 开辟接收数据的AVPacket空间
* 5. av_read_frame 循环读取数据AVFormatContext的信息到AVPacket中
* 6. 将AVPacket包含的数据写入文件
* 7. 退出及释放
*/

JBThreadAudioRecordPCM::JBThreadAudioRecordPCM(QObject *parent)
: JBThreadBase(parent)
{
}


void JBThreadAudioRecordPCM::run() {
    
    auto startTime = JBCommonMethod::getCurrentTimeStamp();
    
    //获取输入格式对象
    const AVInputFormat *fmt = av_find_input_format(k_fmt_name.toUtf8().constData());
    
    if (!fmt) {
        qDebug() << "av_find_input_format failed" << k_fmt_name;
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

	QString fileName(k_out_temp_dir);
	fileName += QDateTime::currentDateTime().toString("MM_dd_HH_mm_ss");
	fileName += ".pcm";
    //QString fileName(k_out_pcm2_file);
    
    QFile file(fileName);
    
    qDebug() << fileName;
    //会清空文件
    bool isSucceed = file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    if (!isSucceed)
    {
        qDebug() << "file.open failed";
        avformat_close_input(&ctx);
        return;
    }
    emit comandLineGenerate(JBCommonMethod::getPCMComandline(ctx, file.fileName()));
    
    AVPacket* pkt = av_packet_alloc();
    
    
    while (!isInterruptionRequested())
    {
        emit timeChanged(JBCommonMethod::getCurrentTimeStamp() - startTime);
        ret = av_read_frame(ctx, pkt);
        //        qDebug() << "ret:" <<ret <<"size: "<< pkt->size;
        if (ret == AVERROR(EAGAIN)) {
            // 资源临时不可用
            continue;
        }
        else if (ret < 0)
        {
            char errbuf[1024] = { 0 };
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "av_read_frame failed: " << errbuf << ret;
            break;
        }
        /**
         * pkt->size == 88200 bytes, 
         * 88200/4 = 22050个样本帧
         * sample_rate: 44100, 22050/44100=0.5秒。 也就是说这里存了0.5秒的数据
         */
        file.write((const char*)pkt->data, pkt->size);
        //不用了，释放
        av_packet_unref(pkt);
    }
    
    //关文件
    file.close();
    //释放资源
    av_packet_free(&pkt);
    //关闭设备
    avformat_close_input(&ctx);
    
    qDebug() << this << "exit 0";

}

