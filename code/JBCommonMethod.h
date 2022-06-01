#pragma once
#include <QObject>
#include "JBConst.h"
#include <qdatetime.h>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}


#ifdef Q_OS_WIN
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif



#define CHECK(judge, str) \
	if ((judge)) { \
		JBCommonMethod::logErrorSDL(str); \
		goto end;\
	}

#define RET(judge, str) \
    if ((judge)) { \
        JBCommonMethod::logErrorSDL(str); \
        return;\
    }

/*
WavHeader 分为三部分
1: riffID 和 后面的size
2: format 部分， 表示格式
3: data 表示真正的数据大小
*/
struct WavHeader
{
    //
    uint8_t riffID[4]{'R', 'I', 'F', 'F'};
    //后面的总字节数
    uint32_t riffDataSize;

    //wave 文件标志
    uint8_t format[4]{'W', 'A', 'V', 'E'};

    //波形文件标志
    uint8_t formatID[4]{'f', 'm', 't', ' '};
    //音频属性： 后面的属性数量字节数 固定
    uint32_t fmtDataSize{16};
    // 音频编码，1表示PCM，3表示Floating Point
    uint16_t fmtCompressionCode;
    uint16_t fmtNumChannels;
    //采样率
    uint32_t fmtSampleRate;
    //传输速率 字节率
    uint32_t fmtBytesPerSecond;
    //数据块的对齐，即Data数据块的长度 (header.fmtNumChannels * header.fmtBitsPerSample) >> 3;
    uint16_t fmtBlockAlign;
    //采样精度， PCM位宽
    uint16_t fmtBitsPerSample;


    uint8_t dataID[4]{'d', 'a', 't', 'a'};
    //真正资源的总字节数
    uint32_t dataSize;
};


struct JBAudioData {
    QString file{};
	uint32_t sampleRate = 0;
	int64_t channelLayout = 0;
	AVSampleFormat format = AV_SAMPLE_FMT_NONE;
};

struct JBAudioBuffer {
	int64_t len = 0;
	int64_t pullLen = 0;
	uint8_t* data{ nullptr };

    void* customData{nullptr};
};

struct JBYUVData {
	QString file{};
	int width = 0;
	int height = 0;
	AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;
	int fps = 0;
    char *pixels{ nullptr };
};

struct JBCommonMethod
{
    //打印 设备参数
    static void logAvformat(AVFormatContext *ctx);

    static void logError(const QString& preStr, int ret = 99999);
    static void logErrorSDL(const QString& preStr);

    static  QString getYuvComandline(AVCodecContext* c, const QString& file);

    static QString getPCMComandline(AVCodecContext* c, const QString& pcmFile);
    static QString getPCMComandline(AVFormatContext* ctx, const QString& pcmFile);
    static QString getPCMComandline(enum AVSampleFormat sfmt, int n_channels, int sample_rate, const QString& pcmFile);

    //初始化wavheader， 除了不定的size
    static void setWavHeaderWithCtx(WavHeader& header, AVFormatContext* ctx);
    
    static int64_t getCurrentTimeStamp();

    static int64_t caculateTotalTime(qint64 allSize, uint32_t bytes_per_sample, int32_t frameRate);
    
    static void convertYuvFormat(const JBYUVData &inData, JBYUVData &outData);
    
};
 
