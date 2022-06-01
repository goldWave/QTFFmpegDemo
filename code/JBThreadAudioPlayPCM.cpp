#include "JBThreadAudioPlayPCM.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"

/**
* 步骤：
* 1. SDL_Init 初始化SDL系统
* 2. 配置SDL_AudioSpec 里面的pcm信息
* 3. SDL_OpenAudio打开音频设备，并设置设置所需参数
* 4. 打开文件，获取data 原始pcm数据
* 5. 在SDL_OpenAudio设置的回调中，进行数据的填充
* 6. buffer填充完毕后，SDL_Delay 等待播放完毕
* 7. 退出及释放
*/

JBThreadAudioPlayPCM::JBThreadAudioPlayPCM(QObject *parent)
    : JBThreadBase{parent}
{

}

void _SDL_AudioCallback(void *userdata,
                        //需要往 SDL 中填充的数据
                        Uint8 * stream,
                        //希望填充的大小
                        int len) {
    
    SDL_memset(stream, 0, len);
    
    JBAudioBuffer*buffer = static_cast<JBAudioBuffer*>(userdata);
    if (!buffer) {
        qDebug() << "_SDL_AudioCallback buffer is nil";
        return;
    }
    
    //数据还没有准备好
    if (buffer->len <= 0) {
        return;
    }
    
    //取最小值,防止越界
    buffer->pullLen = qMin(static_cast<qint64>(len), buffer->len);
    
    //audio volum 0 - 128
    //填数据
    SDL_MixAudio(stream, buffer->data, static_cast<Uint32>(buffer->pullLen), 128/* 最大音量*/);
    
    //计算剩余的 长度
    buffer->len -= buffer->pullLen;
    //移动到剩余的指针位置
    buffer->data += buffer->pullLen;
    
}

void JBThreadAudioPlayPCM::run() {

    //	 初始化Audio子系统 
	if (SDL_Init(SDL_INIT_AUDIO)) {
		qDebug() << "SDL_Init error" << SDL_GetError();
		return;
	}
    qDebug() << "SDL_Init succeed";
    
    
    SDL_AudioSpec spec;
//    //采样率
//    spec.freq = 44100;
//    //采样格式
//    spec.format = AUDIO_S16;
    
    //采样率
    spec.freq = 48000;
    //采样格式
    spec.format = AUDIO_F32;
    
    spec.channels = 2;
    //缓冲区的样本数量, 必须是2的次方
    spec.samples = 1024;
    //采样回调
    spec.callback = _SDL_AudioCallback;
    
    JBAudioBuffer buffer;
    spec.userdata = &buffer;
    
    int ret = SDL_OpenAudio(&spec, nullptr);
    
    if (ret != 0) {
        qDebug() << "SDL_OpenAudio error: " << SDL_GetError();
        SDL_Quit();
        return;
    }
    qDebug() << "SDL_OpenAudio succeed";
//    QFile file(k_resource_pcm_file);
    QFile file(k_out_pcm2_file);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "open file failed: " << k_resource_pcm_file;
        SDL_CloseAudio();
        SDL_Quit();
        return;
    }
    
    //开始播放
    SDL_PauseAudio(false);
    
    //每个样本的字节数 位深 * n_channels
    uint32_t bytes_per_sample = (SDL_AUDIO_BITSIZE(spec.format) * spec.channels) >> 3;
    //文件缓冲区的大小
    
    uint64_t bSize = spec.samples * bytes_per_sample;
    
    qDebug() << "total ms:" << JBCommonMethod::caculateTotalTime(file.size(), bytes_per_sample, spec.freq);
    
    //从文件中读取数据到 文件缓冲区
    uint8_t *data= new uint8_t[bSize];
    int totalLenReaded = 0;
    while (!isInterruptionRequested()) {
        //说明数据还没有被SDL读取消化完毕
        if (buffer.len > 0) {
            continue;;
        }
        
        buffer.len =  file.read((char *)data, bSize);
        
        //文件读完了
        if (buffer.len <= 0) {
            //需要等待 SDL 也将音频 播放完毕，不然最后一点可能没有播放
            SDL_Delay(static_cast<Uint32>(JBCommonMethod::caculateTotalTime(buffer.pullLen, bytes_per_sample, spec.freq)));
            break;
        }
        
        //读取到的文件数据，送入到 自定义的数据缓冲区. 准备在 SDL的回调中，填充到SDL的缓冲区数据中
        buffer.data = data;
        
        totalLenReaded += buffer.len;
        auto usedTime = JBCommonMethod::caculateTotalTime(totalLenReaded, bytes_per_sample, spec.freq);
        emit timeChanged(usedTime);
    }
    
    delete[] data;
    file.close();
    SDL_CloseAudio();
    SDL_Quit();
}
