//
//  JBThreadPlayWav.cpp
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#include "JBThreadPlayWav.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"


/**
* 步骤：
* 1. SDL_Init 初始化SDL系统
* 2. SDL_LoadWAV 读取wav文件的头部信息，及总buffer 及 长度
* 3. SDL_OpenAudio打开音频设备，并设置设置所需参数
* 4. 在SDL_OpenAudio设置的回调中，进行数据的填充
* 5. buffer填充完毕后，SDL_Delay 等待播放完毕
* 6. 退出及释放
*/

JBThreadPlayWav::JBThreadPlayWav(QObject* parent)
	: JBThreadBase{ parent }
{
}

void SDL_AudioCallback_pull(void* userdata, Uint8* stream, int len) {

	SDL_memset(stream, 0, len);

	JBAudioBuffer* bufer = static_cast<JBAudioBuffer*>(userdata);

	if (bufer->len <= 0)
	{
		//数据还没有准备好
		return;
	}

	bufer->pullLen = (len > bufer->len) ? bufer->len : len;

	//qDebug() << "read: " << bufer->pullLen;

	SDL_MixAudio(stream, bufer->data, bufer->pullLen, 128);

	bufer->data += bufer->pullLen;
	bufer->len -= bufer->pullLen;

	JBThreadPlayWav* playWav = static_cast<JBThreadPlayWav*>(bufer->customData);
	if (!playWav)
	{
		return;
	}
	int totalLenReaded = playWav->m_totalSize - bufer->len;
	int bytesPerSample = (SDL_AUDIO_BITSIZE(playWav->m_spec->format)) * playWav->m_spec->channels >> 3;
	auto usedTime = JBCommonMethod::caculateTotalTime(totalLenReaded, bytesPerSample, playWav->m_spec->freq);
	emit playWav->timeChanged(usedTime);
}

void JBThreadPlayWav::run() {
	//	 初始化Audio子系统 
	if (SDL_Init(SDL_INIT_AUDIO)) {
		qDebug() << "SDL_Init error" << SDL_GetError();
		return;
	}
	qDebug() << "SDL_Init succeed";


	uint8_t* buffer = nullptr;


	SDL_AudioSpec spec;

	if (!SDL_LoadWAV(k_resource_wav_file.toUtf8().constData(), &spec, &buffer, &m_totalSize)) {
		qDebug() << "SDL_LoadWAV failed" << SDL_GetError();
		SDL_Quit();
		return;
	}
	m_spec = &spec;

	int a = 0;

	m_spec->callback = SDL_AudioCallback_pull;
	JBAudioBuffer audioBuffer;
	audioBuffer.data = buffer;
	audioBuffer.len = m_totalSize;
	audioBuffer.customData = this;

	m_spec->userdata = static_cast<void*>(&audioBuffer);

	if (SDL_OpenAudio(m_spec, nullptr) != 0)
	{
		qDebug() << SDL_GetError();
		qDebug() << "SDL_OpenAudio failed" << SDL_GetError();
		SDL_FreeWAV(buffer);
		SDL_Quit();
		return;
	}

	SDL_PauseAudio(0);
	int bytesPerSample = (SDL_AUDIO_BITSIZE(m_spec->format)) * m_spec->channels >> 3;
	while (!isInterruptionRequested())
	{
		if (audioBuffer.len > 0)
		{
			continue;
		}

		//全部送入到缓冲区了, 等待SDL 把最后的数据播放完成
		if (audioBuffer.len <= 0) {
			auto leftTime = JBCommonMethod::caculateTotalTime(audioBuffer.pullLen, bytesPerSample, m_spec->freq);
			qDebug() << "delay time(ms): " << leftTime;
			SDL_Delay(leftTime);
			break;
		}

	}


	SDL_FreeWAV(buffer);
	SDL_CloseAudio();
	SDL_Quit();
}
