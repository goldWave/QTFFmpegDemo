//
//  JBThreadDemuxer.cpp
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#include "JBThreadDemuxer.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"

/**
 * 解封装MP4 步骤：mp4 -> yuv , mp4 -> pcm
 * 1.  avformat_open_input 按照读取输入文件mp4的头信息，并创建格式化上下文
 * 2.  avformat_find_stream_info 查找和 av_dump_format 打印具体的输入文件的参数
 * 3.  自定义函数初始化 initAudioInfo & initVideoInfo 音视频配置参数
 * 3.1   initAudioInfo:
 * 3.2       initDecoder 初始化音频解码器
 * 3.2.1        av_find_best_stream 查找最优的音频流索引 （比如0音频，1视频）
 * 3.2.2        auto stream = m_formatCtx->streams[*streamIndex]; 根据流索引，找到对应的流数据
 * 3.2.3        avcodec_find_decoder 根据流的编码器ID，自动找到适合的默认解码器
 * 3.2.4        avcodec_alloc_context3 根据解码器，初始化音视频对应的上下文
 * 3.2.5        avcodec_parameters_to_context 自动将文件流中的参数信息 拷贝 到 上下文中， 不用手动一一赋值
 * 3.2.6        avcodec_open2 打开解码器
 * 3.3       打开需要输出的音频pcm文件
 * 3.4       保存需要的音频数据到自定义的struct中，供后面需要
 * 4.1   initVideoInfo:
 * 4.2       initDecoder 初始化音频解码器 具体步骤同3.2的子步骤一样
 * 4.3       打开需要输出的视频yuv文件
 * 4.4       保存需要的视频数据到自定义的struct中，供后面需要， fps需要av_guess_frame_rate来进行获取
 * 4.5       av_image_alloc 创建一帧图片的图片缓冲区冲区，用来存放一帧图，用作以后需要
 * 5.  av_frame_alloc创建AVFrame的指针对象, 最后用来接收pcm或yuv
 * 6.  av_packet_alloc创建AVPacket的指针对象， 用来接收原始的h264 或 aac数据
 * 7.  while循环读取 m_formatCtx 内容到 AVPacket 缓存中
 * 8.       通过获取codec_type来判断是音频数据还是视频数据。
 * 9.       调用自定义decode方法进行解码
 * 10.      avcodec_send_packet 将 AVPacket 送入 解码器的上下文中
 * 11.      循环调用 avcodec_receive_frame 来接收 解码器中的数据到 AVFrame中
 * 12.            分别写入音频或者让视频数据
 * 12.1           如果是视频: av_image_copy 拷贝yuv的个planner到m_imageBuf的连续内存区间，然后写入文件
 * 12.2           如果是音频: av_sample_fmt_is_planar判断是否是planner，然后分别写入
 * 13  读取和编码完成后，将输入AVPacket 设为 nullptr 后，再次 分别调用一次音频和视频的 9 步，确保缓冲区数据全部清空
 * 14. 释放所有对象
 *
 *
 * 流程: inputFile -> 原始编码data -> av_read_frame -> AVPacket -> codec context -> AVFrame -> outputFile
 */


JBThreadDemuxer::JBThreadDemuxer(QObject* parent)
	: JBThreadBase{ parent } {}



void JBThreadDemuxer::run() {

	JBYUVData yuvData;
	yuvData.file = k_out_temp_dir + "demuxerd_video.yuv";

	JBAudioData pcmData;
	pcmData.file = k_out_temp_dir + "demuxerd_audio.pcm";

	startDemuxer(k_resource_mp4_file, yuvData, pcmData);
}

void JBThreadDemuxer::startDemuxer(const QString& inFile, const JBYUVData& yuvOut, const JBAudioData& pcmout)
{
	m_yuvOut = yuvOut;
	m_pcmout = pcmout;
    QStringList commandLines;

	qDebug() << inFile << "\t->\t" << m_yuvOut.file;
	qDebug() << inFile << "\t->\t" << m_pcmout.file;

	int ret = 0;
	ret = avformat_open_input(&m_formatCtx, inFile.toUtf8().constData(), nullptr, nullptr);
	CHECK(ret < 0, "avformat_open_input");

	ret = avformat_find_stream_info(m_formatCtx, nullptr);
	CHECK(ret < 0, "avformat_find_stream_info");

	qDebug() << "---info\n";
	//    av_dump_format(m_formatCtx, 0, inFile.toUtf8().constData(), 0);

	ret = initAudioInfo();
	CHECK(ret < 0, "initAudioInfo");

	ret = initVideoInfo();
	CHECK(ret < 0, "initVideoInfo");

	m_frame = av_frame_alloc();
	CHECK(!m_frame, "av_frame_alloc");

	m_packet = av_packet_alloc();
	CHECK(!m_packet, "av_packet_alloc");
	m_packet->data = nullptr;
	m_packet->size = 0;

	while (av_read_frame(m_formatCtx, m_packet) == 0)
	{
		auto currentType = m_formatCtx->streams[m_packet->stream_index]->codecpar->codec_type;
		//或者通过m_packet->stream_index == m_aStreamIdx; 判断
			if (currentType == AVMEDIA_TYPE_AUDIO)
			{
				ret = decode(m_aDecodeCtx, m_packet, currentType);
			}
			else if (currentType == AVMEDIA_TYPE_VIDEO) {
				ret =decode(m_vDecodeCtx, m_packet, currentType);
			}
			av_packet_unref(m_packet);
			CHECK(ret < 0, "av_read_frame -> decode");
	}

	decode(m_aDecodeCtx, nullptr, AVMEDIA_TYPE_AUDIO);
	decode(m_vDecodeCtx, nullptr, AVMEDIA_TYPE_VIDEO);
    
    qDebug() << "解封装 成功";
    commandLines <<JBCommonMethod::getPCMComandline(m_aDecodeCtx, m_aOutFile.fileName());
    commandLines <<JBCommonMethod::getYuvComandline(m_vDecodeCtx, m_vOutFile.fileName());
    emit comandLinesGenerate(commandLines);
end:
	avformat_close_input(&m_formatCtx);
	av_frame_free(&m_frame);
	av_packet_free(&m_packet);
	m_aOutFile.close();
	m_vOutFile.close();
}


int JBThreadDemuxer::initAudioInfo() {

	int ret = initDecoder(&m_aDecodeCtx, &m_aStreamIdx, AVMEDIA_TYPE_AUDIO);
    if (ret < 0) {
        JBCommonMethod::logError("initAudioInfo initDecoder");
        return -1;
    }
	m_aOutFile.setFileName(m_pcmout.file);
	if (!m_aOutFile.open(QFile::WriteOnly)) {
		JBCommonMethod::logError("m_aOutFile open ");
		return -1;
	}


	//保存参数
	m_pcmout.sampleRate = m_aDecodeCtx->sample_rate;
	m_pcmout.format = m_aDecodeCtx->sample_fmt;
	m_pcmout.channelLayout = m_aDecodeCtx->channel_layout;

	//fltp == 32bit = 4byte
	m_sampleSize = av_get_bytes_per_sample(m_pcmout.format);
	m_sampleFrameSize = m_sampleSize * m_aDecodeCtx->channels;

	return 0;

}

int JBThreadDemuxer::initVideoInfo() {
	int ret = initDecoder(&m_vDecodeCtx, &m_vStreamIdx, AVMEDIA_TYPE_VIDEO);
    if (ret < 0) {
        JBCommonMethod::logError("initVideoInfo initDecoder");
        return -1;
    }
	m_vOutFile.setFileName(m_yuvOut.file);
	if (!m_vOutFile.open(QFile::WriteOnly)) {
		JBCommonMethod::logError("m_vOutFile open ");
		return -1;
	}

	m_yuvOut.height = m_vDecodeCtx->height;
	m_yuvOut.width = m_vDecodeCtx->width;
	m_yuvOut.pixelFormat = m_vDecodeCtx->pix_fmt;
	if (m_vDecodeCtx->framerate.num == 0 && m_vDecodeCtx->framerate.den == 1)
	{
		AVRational frameRate = av_guess_frame_rate(m_formatCtx, m_formatCtx->streams[m_vStreamIdx], nullptr);
		m_yuvOut.fps = frameRate.num / frameRate.den;
	}
	else {
		m_yuvOut.fps = m_vDecodeCtx->framerate.num / m_vDecodeCtx->framerate.den;
	}

	//创建 图片缓冲区，用来存放一帧图片
	ret = av_image_alloc(m_imageBuf, m_imgLineSizes, m_yuvOut.width, m_yuvOut.height, m_yuvOut.pixelFormat, 1);
	if (ret < 0) {
		JBCommonMethod::logError("av_image_alloc");
		return -1;
	}
	m_imageSize = ret; //width*height*1.5
	return 0;

}

int JBThreadDemuxer::initDecoder(AVCodecContext** decodeCtx, int* streamIndex, AVMediaType type) {

	//查找包含信息的最优索引
	int ret = av_find_best_stream(m_formatCtx, type, -1, -1, nullptr, 0);
	if (ret < 0) {
		JBCommonMethod::logError("av_find_best_stream", ret);
		return ret;
	}

	*streamIndex = ret;
	//最优流索引，查找对应的stream 信息
	auto stream = m_formatCtx->streams[*streamIndex];
	if (!stream) {
		JBCommonMethod::logError("get stream");
		return -1;
	}

	//根据 id 初始化 音频或者 视频
	const AVCodec* deCoder = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!deCoder) {
		JBCommonMethod::logError("avcodec_find_decoder");
		return -1;
	}

	//根据解码器 初始化上下文
	*decodeCtx = avcodec_alloc_context3(deCoder);
	if (!decodeCtx) {
		JBCommonMethod::logError("avcodec_alloc_context3");
		return -1;
	}

	//将文件流中的参数信息 拷贝 到 上下文中， 不用手动一一赋值
	ret = avcodec_parameters_to_context(*decodeCtx, stream->codecpar);
	if (ret < 0) {
		JBCommonMethod::logError("avcodec_parameters_to_context", ret);
		return ret;
	}

	//打开解码器
	ret = avcodec_open2(*decodeCtx, deCoder, nullptr);
	if (ret < 0) {
		JBCommonMethod::logError("avcodec_open2", ret);
		return ret;
	}

	return 0;
}

int JBThreadDemuxer::decode(AVCodecContext* decodeCtx, AVPacket* packet,  enum AVMediaType codec_type)
{
	int ret = avcodec_send_packet(decodeCtx, packet);
	if (ret < 0)
	{
		JBCommonMethod::logError("avcodec_send_packet", ret);
		return ret;
	}

	while (true)
	{
		ret = avcodec_receive_frame(decodeCtx, m_frame);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return 0;
		}
		if (ret < 0)
		{
			JBCommonMethod::logError("avcodec_receive_frame", ret);
			return ret;
		}

		//或者通过m_packet->stream_index == m_aStreamIdx; 判断
		if (codec_type == AVMEDIA_TYPE_AUDIO)
		{
            //planar 的话需要特殊输入处理
            if (av_sample_fmt_is_planar(m_pcmout.format)) {
               // LLLLLRRRRR -> LRLRLRLRLR
                for (int i = 0 ; i < m_frame->nb_samples; i++) { //L 或者 R 的单独个数
                    for (int cI = 0; cI < decodeCtx->channels; cI++) { //有多少个channel
                        char *begin = (char *)(m_frame->data[cI] + i * m_sampleSize);
                        m_aOutFile.write(begin, m_sampleSize);
                    }
                }
                
            } else {
                m_aOutFile.write((char*)m_frame->data[0], m_frame->linesize[0]);
            }
            
		} else if (codec_type == AVMEDIA_TYPE_VIDEO) {
            //方法一：
//			m_vOutFile.write((char*)m_frame->data[0], m_frame->linesize[0] * decodeCtx->height);
//			m_vOutFile.write((char*)m_frame->data[1], m_frame->linesize[1] * decodeCtx->height >> 1);
//			m_vOutFile.write((char*)m_frame->data[2], m_frame->linesize[2] * decodeCtx->height >> 1);
            
            
            //方法二：
            //将yuv buffer 的各个planner， 拷贝到一个连续的内存 空间
            av_image_copy(m_imageBuf, m_imgLineSizes, (const uint8_t **)m_frame->data, m_frame->linesize, m_yuvOut.pixelFormat, m_yuvOut.width, m_yuvOut.height);
            m_vOutFile.write((char*)m_imageBuf[0], m_imageSize);
		}

		av_frame_unref(m_frame);
	}
	return 0;
}
