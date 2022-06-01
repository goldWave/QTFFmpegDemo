#include "JBThreadResample.h"
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
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}
/*
 * 步骤：
 * 1. 打开输入文件
 * 2. 打开输出文件
 * 3. swr_alloc_set_opts 创建上下文
 * 4. swr_init 初始化上下文
 * 5. av_samples_alloc_array_and_samples 填充输入 输出缓冲区
 * 6. swr_convert 循环转换
 * 7. swr_convert 处理参与buffer
 * 8. 释放资源
 */
JBThreadResample::JBThreadResample(QObject *parent)
: JBThreadBase{parent}
{
}

void JBThreadResample::run() {
    JBAudioData inData;
    inData.file = k_resource_pcm_file;
    inData.sampleRate = 44100;
    inData.channelLayout = AV_CH_LAYOUT_STEREO;
    inData.format = AV_SAMPLE_FMT_S16;
    
    JBAudioData outData;
    outData.sampleRate = 48000;
    outData.channelLayout = AV_CH_LAYOUT_STEREO;
    outData.format = AV_SAMPLE_FMT_FLT;
    
    //eg:48000_2_f32_resample.pcm
    outData.file = QString("%1%2_%3_%4_resample.pcm").arg(k_out_temp_dir).arg(outData.sampleRate).arg(av_get_channel_layout_nb_channels(outData.channelLayout)).arg(av_get_bytes_per_sample(outData.format)<<3);
    
    startWithData(inData, outData);
}

void JBThreadResample::startWithData(JBAudioData &inputData, JBAudioData &outputData) {
    QFile inFile(inputData.file);
    if (!inFile.open(QFile::ReadOnly))
    {
        qDebug() << "open failed" << inputData.file;
        return;
    }
    
    qDebug() << inputData.file << "---->" << outputData.file;
    QFile outFile(outputData.file);
    if (!outFile.open(QFile::WriteOnly|QFile::Truncate))
    {
        inFile.close();
        qDebug() << "open failed" << outputData.file;
        return;
    }
    

    //创建 context 上下文
    struct SwrContext* context = swr_alloc_set_opts(nullptr,
                                                    outputData.channelLayout, outputData.format, outputData.sampleRate,
                                                    inputData.channelLayout, inputData.format, inputData.sampleRate,
                                                    0, nullptr);
    
    if (!context) {
        qDebug() << "swr_alloc_set_opts failed";
        inFile.close();
        outFile.close();
        return;
    }
    
    //初始化重采样 上下文
    int ret = swr_init(context);
    if (ret < 0) {
        
        JBCommonMethod::logError("swr_init: ", ret);
        inFile.close();
        outFile.close();
        swr_free(&context);
    }
    
    //样本数量 需要为2的次方
    int in_nSamples = 1024;
    uint8_t** inBuffer = nullptr;
    //sample 大小
    int inLinesize = 0;
    //channel 数量
    int in_nChannels = av_get_channel_layout_nb_channels(inputData.channelLayout);
    
    int out_nSamples = ceil((1.0 * inputData.sampleRate) / outputData.sampleRate * in_nSamples);
    uint8_t** outBuffer = nullptr;
    int outLinesize = 0;
    int out_nChannels = av_get_channel_layout_nb_channels(outputData.channelLayout);
    
    // linesize  在 线性PCM 中代表， 多个声道的总大小
    // 在 planner 中，代表一个单声道数据的大小
    //创建输入缓冲区
    ret = av_samples_alloc_array_and_samples(
                                             &inBuffer,
                                             &inLinesize,
                                             in_nChannels,
                                             in_nSamples,
                                             inputData.format,
                                             1
                                             );
    if (ret < 0)
    {
        JBCommonMethod::logError("av_samples_alloc_array_and_samples in: ", ret);
        inFile.close();
        outFile.close();
        swr_free(&context);
        return;
    }
    
    //创建输出缓冲区
    ret = av_samples_alloc_array_and_samples(
                                             &outBuffer,
                                             &outLinesize,
                                             out_nChannels,
                                             out_nSamples,
                                             outputData.format,
                                             1
                                             );
    if (ret < 0)
    {
        JBCommonMethod::logError("av_samples_alloc_array_and_samples out: ", ret);
        inFile.close();
        outFile.close();
        swr_free(&context);
        return;
    }
    
    int inBytesPerSample  = av_get_bytes_per_sample(inputData.format) * in_nChannels;
    int ouBytesPerSample = av_get_bytes_per_sample(outputData.format) * out_nChannels;
    
    qint64 len = 0;
    while ((len = inFile.read((char*)inBuffer[0], inLinesize)) && len > 0)
    {
        //根据实际读取的数量，调整输入sample数量
        in_nSamples = static_cast<int>(len/inBytesPerSample);
        ret = swr_convert(context, outBuffer, out_nSamples, (const uint8_t **)inBuffer, in_nSamples);
        if (ret < 0) {
            JBCommonMethod::logError("swr_convert: ", ret);
            inFile.close();
            outFile.close();
            swr_free(&context);
            return;
        }
        
        //写入实际转换的buffer 数量
        outFile.write((char*)outBuffer[0], ret * ouBytesPerSample);
    }
    
    while ((ret = swr_convert(context, outBuffer, out_nSamples, nullptr, 0)) && ret > 0 ) {
//        qDebug() << "sample count: " << ret;
        //写入实际转换的buffer 数量
        outFile.write((char*)outBuffer[0], ret * ouBytesPerSample);
    }
    
    if (ret < 0) {
        JBCommonMethod::logError("swr_convert: ", ret);
        inFile.close();
        outFile.close();
        swr_free(&context);
        return;
    }
    
    outFile.flush();
    
    qDebug() << "inSize:" << inFile.size() << "---->" << "outSize: "<<outFile.size();
    
    emit comandLineGenerate(JBCommonMethod::getPCMComandline(outputData.format, out_nChannels, outputData.sampleRate, outFile.fileName()));

    //释放的东西 有点多
    inFile.close();
    outFile.close();
    
    // 释放输入缓冲区
    if (inBuffer) {
        av_freep(&inBuffer[0]);
    }
    av_freep(&inBuffer);
    
    // 释放输出缓冲区
    if (outBuffer) {
        av_freep(&outBuffer[0]);
    }
    av_freep(&outBuffer);
    
    swr_free(&context);
}
