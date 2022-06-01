#include "JBThreadPCM2WavFile.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBCommonMethod.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}


/**
* 步骤：
* 1. 打开读取和写入的文件
* 2. 将wav头数据写入到需要写入的文件中
* 3. 根据文件大小，批量将读取的文件数据，写入到文件中
* 4. 关闭两个文件
*/


JBThreadPCM2WavFile::JBThreadPCM2WavFile(QObject *parent)
    : JBThreadBase{parent}
{
  
}


void JBThreadPCM2WavFile::run() {
    
    QFile filePCM(k_resource_pcm_file);
    if (!filePCM.open(QFile::ReadOnly)) {
        qDebug() << "open file failed: " << k_resource_pcm_file;
        return;
    }
    
    QFile fileWav(k_out_wav_file);
    if (!fileWav.open(QFile::WriteOnly)) {
        qDebug() << "open file failed: " << k_out_wav_file;
        return;
    }
    emit comandLineGenerate(QString("ffplay %1").arg(fileWav.fileName()));
    auto pcmSize = filePCM.size();
    
    WavHeader header;
    header.riffDataSize = pcmSize - sizeof(WavHeader) - offsetof(WavHeader, format);
    header.fmtCompressionCode = 1; //    // 音频编码，1表示PCM，3表示Floating Point
    header.fmtNumChannels = 2;
    header.fmtSampleRate = 44100;
    header.fmtBitsPerSample = 16;
    header.fmtBlockAlign = (header.fmtNumChannels * header.fmtBitsPerSample) >> 3;
    header.fmtBytesPerSecond = header.fmtBlockAlign * header.fmtSampleRate;

    qDebug() << "--++pcmSize size:" << pcmSize;
    qDebug() << "--++header size:" << sizeof(WavHeader);
    qDebug() << "--++header offset size:" << offsetof(WavHeader, format);
    
    fileWav.write((const char *)&header, sizeof(WavHeader));

    fileWav.write(filePCM.readAll().constData(), pcmSize);
    filePCM.close();
    fileWav.close();
    
}
