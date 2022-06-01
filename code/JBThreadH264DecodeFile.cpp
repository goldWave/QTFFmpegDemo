//
//  JBThreadH264DecodeFile.cpp
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#include "JBThreadH264DecodeFile.h"
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
 * 5.  avcodec_open2 配置好上下文的 参数后 打开编/解码器.
 * 6.  打开输入 输出 文件
 * 7.  av_frame_alloc 创建 AVFrame 对象，用来存取输出缓冲区的数据
 * 8.  do 循环读取 inFile 内容到 inData buffer 缓存中
 * 9.		循环调用 av_parser_parse2 来使用 解析器 将 buffer 解析成 AVPacket 对象
 * 10.		调用自定义方法 decode 来处理解码
 * 11.		avcodec_send_packet 将 AVPacket 送入 解码器的上下文中
 * 12			循环调用 avcodec_receive_frame 来接收 解码器中的数据到 AVFrame中
 * 13			将AVFrame 按照planner 写入到输出文件
 * 12. 读取和编码完成后，将输入AVPacket 设为 nullptr 后 再次 调用一次 10 步，确保缓冲区数据全部清空
 * 13. 释放所有对象
 *
 *
 * 流程: inputFile -> 原始data -> AVCodecParserContext -> AVPacket -> codec context -> AVFrame -> outputFile
 */



#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

JBThreadH264DecodeFile::JBThreadH264DecodeFile(QObject* parent)
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

		qDebug() << frame->linesize[0] << frame->linesize[1] << frame->linesize[2];
		outFile.write((char*)frame->data[0], frame->linesize[0] * context->height);
		outFile.write((char*)frame->data[1], frame->linesize[1] * context->height >> 1);
		outFile.write((char*)frame->data[2], frame->linesize[2] * context->height >> 1);

		av_frame_unref(frame);
	}
}

void JBThreadH264DecodeFile::startDecoder(JBYUVData& outData, const QString& inName) {

	const AVCodec* codec = nullptr;
	AVPacket* packet = nullptr;
	AVCodecParserContext* parser = nullptr;
	AVCodecContext* context = nullptr;
	int ret = 0;
	QFile inFile;
	QFile outFile;
	AVFrame* frame;

	//demo 就会将空间加上AV_INPUT_BUFFER_PADDING_SIZE， 是为了防止溢出。可查看AV_INPUT_BUFFER_PADDING_SIZE的注释说明
	int _allBufferSize = AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE;
	uint8_t* inData = new uint8_t[_allBufferSize];
	memset(inData, 0, _allBufferSize);
	uint8_t* movePtr = inData; //主要是为了分段操控inData的数据
	int inSize = AUDIO_INBUF_SIZE;
	int readSize = 0;
	bool isReadEOF = false;

	//初始化解码器
//    codec = avcodec_find_decoder_by_name("h264");
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
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

	//初始化 yuv 的输出buffer
	frame = av_frame_alloc();
	if (!frame)
	{
		JBCommonMethod::logError("av_frame_alloc");
		goto end;
	}

	do {
		readSize = inFile.read((char*)inData, inSize);
		isReadEOF = readSize <= 0;
		movePtr = inData; //指向首元素
		qDebug() << "readSize: " << readSize;
		//缓冲区有需要解码的数据, 或者文件结尾(无条件解码一次)
		while (readSize > 0 || isReadEOF) {
			int parse_len = av_parser_parse2(parser, context,
				&packet->data, &packet->size,
				movePtr, readSize,
				AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

			if (parse_len < 0)
			{
				JBCommonMethod::logError("av_parser_parse2");
				goto end;
			}

			//移动已经解析的长度的数据
			movePtr += parse_len;
			readSize -= parse_len;
			qDebug() << "parse_len: " << parse_len << "readSize: " << readSize << "packet->size" << packet->size;


			//本地解析出来了数据，就送入解码器 
		   //packet->size 保证当前解析的数据有值，如果送入解析器的数据不能构成完成的packet数据的话，size就应该为0？
			if (packet->size > 0 && decode(packet, frame, context, outFile) < 0) {
				goto end;
			}
			if (isReadEOF) {
				break;
			}
		}


	} while (!isReadEOF);

	//刷新缓冲区
	decode(nullptr, frame, context, outFile);

	qDebug() << inFile.fileName() << "-->" << outFile.fileName();

	emit comandLineGenerate(JBCommonMethod::getYuvComandline(context, outFile.fileName()));


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

void JBThreadH264DecodeFile::run() {

	JBYUVData yuvData;
	yuvData.file = k_out_temp_dir + "de_movie_640x480_yuv420p.yuv";
	yuvData.width = 640;
	yuvData.height = 480;
	yuvData.pixelFormat = AV_PIX_FMT_YUV420P;
	yuvData.fps = 25;

	QString inName(k_out_temp_dir + "movie_640x480_yuv420p.h264");

	startDecoder(yuvData, inName);
}
