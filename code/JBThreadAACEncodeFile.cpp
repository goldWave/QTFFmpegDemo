//
//  JBThreadAACEncodeFile.cpp
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#include "JBThreadAACEncodeFile.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"


/**
 * 步骤：
 * 1.  打开输入 输出 文件
 * 2.  avcodec_find_encoder_by_name || avcodec_find_encoder 获取编码器
 * 3.  codec->sample_fmts 遍历编码器支持的format，检测输入format是否支持，不支持需要重采样
 * 4.  avcodec_alloc_context3 创建编码器的上下文
 * 5.  avcodec_open2 配置好上下文的 参数后 打开编码器。 如果有当前编码器自带的参数如：vbr，获取命令行参数，在这里通过AVDictionary传入
 * 6.  av_frame_alloc 创建 AVFrame 对象，用来存取输入缓冲区的数据
 * 7.  配置AVFrame的数据
 * 8.  通过6/7步配置的数据，用av_frame_get_buffer来真正创建 输入缓冲区
 * 9.  av_packet_alloc 创建 AVPacket 对象，用来接收 编码后的数据，即输出缓冲区
 * 10. 读取输入文件的数据 到 AVFrame的 data 和 lineSize 中
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
 * 主要参数配置： audio 三要素： format channel_layout sampleRate
 */

JBThreadAACEncodeFile::JBThreadAACEncodeFile(QObject* parent)
: JBThreadBase{ parent }
{
    
}

static int check_sample_fmt(const AVCodec* codec, AVSampleFormat sample_fmt) {
    const enum AVSampleFormat* p = codec->sample_fmts;
    
    while (*p != AV_SAMPLE_FMT_NONE) {
        qDebug() << "support fmt: " << av_get_sample_fmt_name(*p);
        if (*p == sample_fmt) return 1;
        p++;
    }
    return 0;
}


static int startEncoder(AVFrame* frame, AVPacket* packet, AVCodecContext* context, QFile *outFile) {
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

void JBThreadAACEncodeFile::aacEncoder(JBAudioData& pcmModel, const QString& outName) {
    QFile inFile(pcmModel.file);
    if (!inFile.open(QFile::ReadOnly)) {
        qDebug() << "open file failed: " << pcmModel.file;
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
    int ret = 0;
    int maxC = 0;
    //获取fdk_aac 编码器
    const AVCodec *codec  = avcodec_find_encoder_by_name("libfdk_aac");
    //const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (codec == nullptr)
    {
        qDebug() << "avcodec_find_encoder_by_name failed";
        goto end;
    }
    
    //检测  当前的pcm数据 是否符合  fdk_aac 编码器的 要求。
    ret = check_sample_fmt(codec, pcmModel.format);
    if (ret == 0)
    {
        qDebug() << "check_sample_fmt not support" << av_get_sample_fmt_name(pcmModel.format);
        goto end;
    }
    
    //创建编码的上下文对象
    context = avcodec_alloc_context3(codec);
    if (context == nullptr)
    {
        qDebug() << "avcodec_alloc_context3 failed";
        goto end;
    }
    context->sample_rate = pcmModel.sampleRate;
    context->sample_fmt = pcmModel.format;
    context->channel_layout = pcmModel.channelLayout;
    //必须手动设置channels
    context->channels = av_get_channel_layout_nb_channels(context->channel_layout);
    //context->bit_rate = 32000;
    //context->profile = FF_PROFILE_AAC_HE_V2;
    
    qDebug() << context->sample_rate << av_get_sample_fmt_name(context->sample_fmt) << av_get_channel_layout_nb_channels(context->channel_layout);
    
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
    frame->channel_layout = context->channel_layout;
    frame->format = context->sample_fmt;
    frame->sample_rate = context->sample_rate;
    frame->channels = context->channels;
    frame->nb_samples = context->frame_size; //1024个样本，每个大样本两个channel， 每个channel样本2字节16位深
    
    //frame->nb_samples = context->sample
    
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0)
    {
        JBCommonMethod::logError("av_frame_get_buffer", ret);
        goto end;
    }
    
    //输出 配置
    packet = av_packet_alloc();
    if (!packet)
    {
        JBCommonMethod::logError("av_packet_alloc", ret);
        goto end;
    }
    qDebug() << "read line size:" << frame->linesize[0];
    
    while ((ret = inFile.read((char*)frame->data[0], frame->linesize[0])) > 0)
    {
        //主要是最后，ret 读取的数量比较小，所以需要重新调整nb_samples的大小，来进行编码
        auto minSize = qMin(ret, frame->linesize[0]);
        auto currentSamples =  minSize / (av_get_bytes_per_sample(context->sample_fmt) * context->channels);
        maxC +=minSize;
        //qDebug() << "read line size:" << ret << frame->linesize[0] << currentSamples << maxC;
        frame->nb_samples = currentSamples;
        
        if (startEncoder(frame, packet, context, &outFile) < 0) {
            goto  end;
        }
    }
    
    //最后刷新下 缓冲区的数据
    startEncoder(nullptr, packet, context, &outFile);
    
    emit comandLineGenerate(QString("ffplay %1").arg(outFile.fileName()));

    outFile.flush();
    qDebug() << "comandLineSize: " << 150443;
    qDebug() << "currentfilSize: " << outFile.size();
    qDebug() << "----------left: " << (150443 - outFile.size());
    
end:
    inFile.close();
    outFile.close();
    
//    avcodec_close(context); //TODO： 确认需不需要
    
    av_frame_free(&frame);
    
    av_packet_free(&packet);
    avcodec_free_context(&context);
    
}

void JBThreadAACEncodeFile::run() {
    
    JBAudioData pcmData;
    pcmData.file = k_resource_pcm_file;
    pcmData.sampleRate = 44100;
    pcmData.channelLayout = AV_CH_LAYOUT_STEREO;
    pcmData.format = AV_SAMPLE_FMT_S16;
    //pcmData.format = AV_SAMPLE_FMT_FLT;
    
    aacEncoder(pcmData, k_out_aac_file);
    
}
