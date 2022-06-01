//
//  JBThreadAACDecodeFile.cpp
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#include "JBThreadAACDecodeFile.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"

/**
 * 步骤：
 * 1.  ...
 * 2.  avcodec_find_decoder_by_name || avcodec_find_decoder 获取解码器
 * 3.  av_packet_alloc 创建 AVPacket 对象，用来接收 输入文件的AAC 数据
 * 4.  av_parser_init 创建 AVCodecParserContext 解析器上下文。 用来处理 AVPacket 数据
 * 5.  avcodec_alloc_context3 创建编码器的上下文
 * 5.  avcodec_open2 配置好上下文的 参数后 打开编/解码器。 如果有当前编码器自带的参数如：vbr，获取命令行参数，在这里通过AVDictionary传入
 * 6.  打开输入 输出 文件
 * 7.  av_frame_alloc 创建 AVFrame 对象，用来存取输出缓冲区的数据
 * 8.  创建uint8_t * buffer 来读取 input 文件数据
 * 9.  循环调用 av_parser_parse2 来使用 解析器 将 buffer 解析成 AVPacket 对象
 * 10. 调用自定义方法 decode 来处理解码
 *      10.1  avcodec_send_packet 将 AVPacket 送入 解码器的上下文中
 *      10.2  循环调用 avcodec_receive_frame 来接收 解码器中的数据到 AVFrame中
 *      10.3  将AVFrame写入到输出文件
 * 11. 继续读取输入文件，进入9-11的循环，直到数据读取完成
 * 12. 读取和编码完成后，将输入AVPacket 设为 nullptr 后 再次 调用一次 10 步，确保缓冲区数据全部清空
 * 13. 释放所有对象
 *
 *
 * 流程: inputFile -> 原始data -> AVCodecParserContext -> AVPacket -> codec context -> AVFrame -> outputFile
 */

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

JBThreadAACDecodeFile::JBThreadAACDecodeFile(QObject* parent)
	: JBThreadBase{ parent } {}

static int decode(AVPacket* packet, AVFrame* frame, AVCodecContext* context, QFile& outFile) {
	int ret = avcodec_send_packet(context, packet);
	if (ret < 0)
	{
		JBCommonMethod::logError("avcodec_send_packet", ret);
		return ret;
	}
    
	while (true)
	{
		ret = avcodec_receive_frame(context, frame);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return 0;
		}
		if (ret < 0)
		{
            JBCommonMethod::logError("avcodec_receive_frame", ret);
            return ret;
        }
  
//        int lineSize = frame->linesize[0]; //姑且认为每个channel的size 相同
//        if (av_sample_fmt_is_planar((AVSampleFormat)frame->format) == 1) { //plannr
//            for (int i = 0; i < frame->nb_samples; i++) {
//                for (int ch = 0; i < context->channels ; ch++) {
//                    outFile.write((char*)frame->data[0], frame->linesize[0]);
//                }
//            }|
//        } else {
            outFile.write((char*)frame->data[0], frame->linesize[0]);
//        }
        
		av_frame_unref(frame);
	}
}

void JBThreadAACDecodeFile::startDecoder(JBAudioData& outData, const QString& inName) {

	const AVCodec* codec = nullptr;
	AVPacket* packet = nullptr;
	AVCodecParserContext* parser = nullptr;
	AVCodecContext* context = nullptr;
	int ret = 0;
	QFile inFile;
	QFile outFile;
	AVFrame* frame;

	//demo 就会将空间加上AV_INPUT_BUFFER_PADDING_SIZE， 是为了防止溢出。可查看AV_INPUT_BUFFER_PADDING_SIZE的注释说明
	uint8_t* inData = new uint8_t[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	memset(inData, 0, AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
	uint8_t* movePtr = inData; //主要是为了分段操控inData的数据
	int inSize = AUDIO_INBUF_SIZE;
	int readSize = 0;
	bool isReadEOF = false;

	//初始化解码器
	codec = avcodec_find_decoder_by_name("libfdk_aac");
	if (!codec)
	{
		JBCommonMethod::logError("avcodec_find_encoder_by_name");
		goto end;
	}

	//开辟输入buffer
	packet = av_packet_alloc();
	if (!packet)
	{
		JBCommonMethod::logError("av_packet_alloc");
		goto end;
	}

	//初始化解析器
	parser = av_parser_init(codec->id);
	if (!parser)
	{
		JBCommonMethod::logError("av_parser_init");
		goto end;
	}

	//初始化编码器的上下文
	context = avcodec_alloc_context3(codec);
	if (!context)
	{
		JBCommonMethod::logError("avcodec_alloc_context3");
		goto end;
	}

	//打开编码器
	ret = avcodec_open2(context, codec, nullptr);
	if (ret < 0)
	{
		JBCommonMethod::logError("avcodec_open2", ret);
		goto end;
	}

	inFile.setFileName(inName);
	if (!inFile.open(QFile::ReadOnly)) {
		qDebug() << "open file failed: " << inName;
		return;
	}

	outFile.setFileName(outData.file);
	if (!outFile.open(QFile::WriteOnly)) {
		qDebug() << "open file failed: " << outData.file;
		outFile.close();
		return;
	}
	qDebug() << inFile.fileName() << "-->" << outFile.fileName();

	//初始化 pcm 的输出buffer
	frame = av_frame_alloc();
	if (!frame)
	{
		JBCommonMethod::logError("av_frame_alloc");
		goto end;
	}

	//将数据 写入bufData
	readSize = inFile.read((char*)movePtr, inSize);
	while (readSize > 0)
	{
		//解析器将原始aac数据，解析成 packet 可以读取的数据
		int parse_len = av_parser_parse2(parser, context,
			&packet->data, &packet->size,
			movePtr, readSize,
			AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		//qDebug() << "parse_len:" << parse_len << readSize;
		if (parse_len < 0)
		{
			JBCommonMethod::logError("av_parser_parse2");
			goto end;
		}

		//移动已经解析的长度的数据
		movePtr += parse_len;
		readSize -= parse_len;
		if (parse_len > 0)
		{
			//本地解析出来了数据，就送入解码器
			if (decode(packet, frame, context, outFile) < 0) {
				goto end;
			}
		}

		//读取后的 还没有进行 parser 的数据不多了，在阈值内了，就重新进行读取操作
		if (readSize < AUDIO_REFILL_THRESH && !isReadEOF)
		{
			//将剩下的数据移动到开始位置
			//然后都指向下标0的地址
			memmove(inData, movePtr, readSize);
			movePtr = inData + readSize; //移动到需要填充的首地址
			//本地需要读的数据大小为剩余的空间，填充的首地址为遗留的数据的末尾
			auto reSize = inFile.read((char*)movePtr, inSize - readSize);
			if (reSize <= 0)
			{
				isReadEOF = true;
			}
			//计算本地读取的长度和遗留的长度的和
			readSize += reSize;
			//将读取的指针移动下标0的位置
			movePtr = inData;
		}
	}

	//刷新缓冲区
	decode(nullptr, frame, context, outFile);

	qDebug() << inFile.fileName() << "-->" << outFile.fileName();

	emit comandLineGenerate(JBCommonMethod::getPCMComandline(context, outFile.fileName()));


end:
	if (inData)
	{
		delete[] inData;
	}
	inFile.close();
	outFile.close();
	av_packet_free(&packet);
	av_frame_free(&frame);
	avcodec_free_context(&context);
}

void JBThreadAACDecodeFile::run() {

	JBAudioData outData;
	outData.file = k_out_pcm_file;
	outData.sampleRate = 44100;
	outData.channelLayout = AV_CH_LAYOUT_STEREO;
	outData.format = AV_SAMPLE_FMT_S16;

	QString inName(k_resource_aac_file);

	startDecoder(outData, inName);
}


