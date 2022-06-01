//
//  JBThreadDemuxer.h
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#pragma once

#include "JBThreadBase.h"
#include "JBCommonMethod.h"
#include <qfile.h>

class JBThreadDemuxer : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadDemuxer(QObject *parent = nullptr);


private:
    void run() final;

    void startDemuxer(const QString& inFile,const JBYUVData& yuvOut,const JBAudioData& pcmout);
    int initAudioInfo();
    int initVideoInfo();
    int initDecoder(AVCodecContext **decodeCtx, int *streamIndex, AVMediaType type);
    int decode(AVCodecContext* decodeCtx, AVPacket* packet, enum AVMediaType codec_type);
    
    QFile m_vOutFile;
    QFile m_aOutFile;
    
    JBYUVData m_yuvOut;
    JBAudioData m_pcmout;
    
    AVFormatContext *m_formatCtx= nullptr;
    
    AVCodecContext *m_aDecodeCtx = nullptr;
    AVCodecContext *m_vDecodeCtx = nullptr;
    
    //流索引
    int m_aStreamIdx = 0;
    int m_vStreamIdx = 0;

    //图片缓冲区
    uint8_t* m_imageBuf[4]{nullptr};
    int  m_imgLineSizes[4]{ 0 };
    int m_imageSize = 0; //一帧图片的大小

    //音频 样本大小
    int m_sampleSize = 0;
    //样本帧大小
    int m_sampleFrameSize = 0;

    AVFrame* m_frame{ nullptr };
    AVPacket* m_packet{ nullptr };
};


