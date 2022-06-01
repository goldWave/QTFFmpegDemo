#include "JBCommonMethod.h"
#include <QDebug>


void JBCommonMethod::logAvformat(AVFormatContext* ctx) {
	// 获取输入流
	AVStream* stream = ctx->streams[0];
	// 获取音频参数
	AVCodecParameters* params = stream->codecpar;
	// 声道数
	qDebug() << "channels: " << params->channels;
	// 采样率
	qDebug() << "sample_rate:" << params->sample_rate;
	// 采样格式
	qDebug() << "format:" << params->format;
	// 每一个样本的一个声道占用多少个字节
	qDebug() << "bytes_per_sample: " << av_get_bytes_per_sample((AVSampleFormat)params->format);

	qDebug() << "codec_id:" << params->codec_id; //65557 = AV_CODEC_ID_PCM_F32LE

	qDebug() << "av_get_bits_per_sample: " << av_get_bits_per_sample(params->codec_id);
	qDebug() << "codec_type:" << params->codec_type;
}

void JBCommonMethod::logError(const QString& preStr, int ret)
{
	if (ret == 99999)
	{
		qDebug() << preStr << "failed";
		return;
	}

	char errbuf[1024] = { 0 };
	av_strerror(ret, errbuf, sizeof(errbuf));
	qDebug() << preStr << "failed" << errbuf << ret;
}

void JBCommonMethod::logErrorSDL(const QString& preStr)
{

	qDebug() << preStr << "failed" << SDL_GetError();
}


static int get_format_from_sample_fmt(const char** fmt,
	enum AVSampleFormat sample_fmt)
{
	
	struct sample_fmt_entry {
		enum AVSampleFormat sample_fmt; const char* fmt_be, * fmt_le;
	} sample_fmt_entries[] = {
		{ AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
		{ AV_SAMPLE_FMT_S16, "s16be", "s16le" },
		{ AV_SAMPLE_FMT_S32, "s32be", "s32le" },
		{ AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
		{ AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
	};
	*fmt = NULL;
    
    for (int i = 0; i < static_cast<int>(FF_ARRAY_ELEMS(sample_fmt_entries)); i++) {
		struct sample_fmt_entry* entry = &sample_fmt_entries[i];
		if (sample_fmt == entry->sample_fmt) {
			*fmt = AV_NE(entry->fmt_be, entry->fmt_le);
			return 0;
		}
	}

	qDebug() << "sample format %s is not supported as output format\n" << av_get_sample_fmt_name(sample_fmt);

	return -1;
}

QString JBCommonMethod::getYuvComandline(AVCodecContext* c, const QString& file) {

	QString str("ffplay -video_size %1x%2 -pixel_format %3 %4");
	
	str = str.arg(c->width).arg(c->height).arg(av_get_pix_fmt_name(c->pix_fmt)).arg(file);
	return str;
}


QString JBCommonMethod::getPCMComandline(AVCodecContext* c, const QString& pcmFile) {
	return getPCMComandline(c->sample_fmt, c->channels, c->sample_rate, pcmFile);
}

QString JBCommonMethod::getPCMComandline(AVFormatContext* ctx, const QString& pcmFile) {
	// 获取输入流
	AVStream* stream = ctx->streams[0];
	// 获取音频参数
	AVCodecParameters* params = stream->codecpar;
	return getPCMComandline((enum AVSampleFormat)params->format, params->channels, params->sample_rate, pcmFile);
}

QString JBCommonMethod::getPCMComandline(enum AVSampleFormat sfmt, int n_channels, int sample_rate, const QString& pcmFile) {
	int ret = 0;
	const char* fmt;

	if (av_sample_fmt_is_planar(sfmt)) {
		const char* packed = av_get_sample_fmt_name(sfmt);
		printf("Warning: the sample format the decoder produced is planar "
			"(%s). This example will output the first channel only.\n",
			packed ? packed : "?");
		sfmt = av_get_packed_sample_fmt(sfmt);
	}

	if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
		return "";

	QString str("ffplay -f %1 -ac %2 -ar %3 %5");


	str = str.arg(fmt).arg(n_channels).arg(sample_rate).arg(pcmFile);
	return str;
}

void JBCommonMethod::setWavHeaderWithCtx(WavHeader& header, AVFormatContext* ctx) {
	//从输出流中的 第一个流中获取 音频参数
	AVCodecParameters* params = ctx->streams[0]->codecpar;

	//这两数据会被改写
	header.riffDataSize = 0; //pcmSize - sizeof(WavHeader) - offsetof(WavHeader, format);
	header.dataSize = 0;

	header.fmtCompressionCode = 1;
	header.fmtNumChannels = params->channels;
	header.fmtSampleRate = params->sample_rate;

	//params->codec_id两个平台都有， params->format可能在mac有问题
	//位深
	header.fmtBitsPerSample = av_get_bits_per_sample(params->codec_id);

	//数据块的对齐，即Data数据块的长度
	header.fmtBlockAlign = (header.fmtNumChannels * header.fmtBitsPerSample) >> 3;

	//块*频率
	header.fmtBytesPerSecond = header.fmtBlockAlign * header.fmtSampleRate;
}


int64_t JBCommonMethod::getCurrentTimeStamp() {
	QDateTime dateTime = QDateTime::currentDateTime();
	// 转换成时间戳
	qint64 epochTime = dateTime.toMSecsSinceEpoch();
	return epochTime;
}


int64_t JBCommonMethod::caculateTotalTime(qint64 allSize, uint32_t bytes_per_sample, int32_t frameRate) {
	assert(bytes_per_sample != 0);
	assert(frameRate != 0);
	int64_t samples = allSize / bytes_per_sample;
	int64_t ms = samples * 1000 / frameRate;
	return ms;
}


void JBCommonMethod::convertYuvFormat(const JBYUVData &inData, JBYUVData &outData) {
    
    SwsContext *ctx = nullptr;
    // 输入输出缓冲区。 指向单个平面的数据
    uint8_t *inPixel[4];
    uint8_t *outPixel[4];
    
    //每个平面的大小
    int inLinsize[4];
    int outLinsize[4];
    
    //单帧图片的大小
    int inFrameSize;
    int outFrameSize;
    
    int ret = 0;
    
    //创建上下文
    ctx =  sws_getContext(inData.width, inData.height, inData.pixelFormat, outData.width, outData.height, outData.pixelFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
    CHECK(!ctx ,"sws_getContext" );
    
    //创建输入缓冲区
    ret =  av_image_alloc(inPixel, inLinsize, inData.width, inData.height, inData.pixelFormat, 1);
    CHECK(ret == 0 ,"av_image_alloc inPixel" );
    
    ret =  av_image_alloc(outPixel, outLinsize, outData.width, outData.height, outData.pixelFormat, 1);
    CHECK(ret == 0 ,"av_image_alloc outPixel" );
    
    //计算单帧图片大小
    inFrameSize = av_image_get_buffer_size(inData.pixelFormat, inData.width, inData.height, 1);
    outFrameSize = av_image_get_buffer_size(outData.pixelFormat, outData.width, outData.height, 1);
    
    //拷贝输入的数据
    memcpy(inPixel[0], inData.pixels, inFrameSize);
    
    //格式转换
    ret = sws_scale(ctx, inPixel, inLinsize, 0, inData.height, outPixel, outLinsize);
    CHECK(ret == 0 ,"sws_scale" );
    
    //将输出数据，拷贝到输出结构体
    outData.pixels = (char *)malloc(outFrameSize);
    memcpy(outData.pixels, outPixel[0], outFrameSize);
    
end:
    sws_freeContext(ctx);
    av_freep(&inPixel[0]);
    av_freep(&outPixel[0]);
}
