# 其他文件：ffmpeg命令行使用记录：[链接](https://github.com/goldWave/QTFFmpegDemo/blob/main/ffmpeg_command_lines.md)

# 简单介绍
本项目主要是用`Qt` + `c++` + `ffmpeg` 写的**音视频采集**、**播放**、**编码**、**解码**、**解封装**的代码demo。
每个按钮对应的文件均为相对独立的demo cpp文件。
由于代码数量太大，这里就不给出具体代码，只写出具体的流程逻辑，需要代码的可以到[github](https://github.com/goldWave/QTFFmpegDemo)下载

## 项目配置
1. 由于使用Qt开发， 所以需要安装
 -  [Qt creator](https://www.qt.io/download)
 - [FFmpeg](https://www.ffmpeg.org/download.html).  `win`: ffmpeg 下载 `shared` 文件. mac 直接brew 安装
  -   [SDL](https://www.libsdl.org/)。  fmpeg默认应该已经装了SDL
2.  `FFmpegDemo.pro` 中替换`ffmpeg`和`SDL`为自己的路径
3. win 需要将dll拷贝到项目exe所在的目录 | 或者将  /bin目录加到环境变量里面
4. `JBConst.cpp` 文件替换为自己的资源路径

## 具体运行的界面展示如下：
![在这里插入图片描述](https://img-blog.csdnimg.cn/bc30657cf65b427783195ee74d674bcc.png)
## 具体文件
![在这里插入图片描述](https://img-blog.csdnimg.cn/171e8537a4b04fd08e20ee1600432ada.png)
红框中代表的为每个独立的业务逻辑。

# 具体步骤：
## 2.1 录制 pcm
触发按钮：`开始 录制 pcm`
业务文件：[JBThreadAudioRecordPCM.cpp](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadAudioRecordPCM.cpp)
主要步骤：

```C
/**
* 步骤：
* 1. av_find_input_format 获取录音设备的具体参数
* 2. avformat_open_input 打开设备， 创建AVFormatContext设备
* 3. 打开写入的文件
* 4. av_packet_alloc 开辟接收数据的AVPacket空间
* 5. av_read_frame 循环读取数据AVFormatContext的信息到AVPacket中
* 6. 将AVPacket包含的数据写入文件
* 7. 退出及释放
*/
```


## 2.2 播放 pcm
触发按钮：`开始 播放 pcm`
业务文件：[JBThreadAudioPlayPCM.cpp](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadAudioPlayPCM.cpp)
主要步骤：

```C
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
```

## 2.3 录制 wav
触发按钮：`开始 录制 wav`
业务文件：[JBThreadAudioRecordWav.cpp](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadAudioRecordWav.cpp)
主要步骤：

```C
/**
* 步骤：
* 1. av_find_input_format 获取录音设备的具体参数
* 2. avformat_open_input 打开设备， 创建AVFormatContext设备
* 3. 打开写入的文件
* 4. av_packet_alloc 开辟接收数据的AVPacket空间
* 5. av_read_frame 循环读取数据AVFormatContext的信息到AVPacket中
* 6. 将AVPacket包含的数据写入文件
* 7. 退出及释放
*/
```

## 2.4 播放 wav
触发按钮：`开始 播放 wav`
业务文件：[JBThreadPlayWav.cpp](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadPlayWav.cpp)
主要步骤：

```C
/**
* 步骤：
* 1. SDL_Init 初始化SDL系统
* 2. SDL_LoadWAV 读取wav文件的头部信息，及总buffer 及 长度
* 3. SDL_OpenAudio打开音频设备，并设置设置所需参数
* 4. 在SDL_OpenAudio设置的回调中，进行数据的填充
* 5. buffer填充完毕后，SDL_Delay 等待播放完毕
* 6. 退出及释放
*/
```

## 2.5 重采样音频文件
触发按钮：`开始 重采样(Resample)`
业务文件：[JBThreadResample](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadResample.cpp)
主要步骤：

```C
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
```

## 2.6 AAC 文件的编码
触发按钮：`开始 aac 文件编码(encode)`
业务文件：[JBThreadAACEncodeFile](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadAACEncodeFile.cpp)
主要步骤：

```C
/**
 * 步骤：
 * 1.  打开输入 输出 文件
 * 2.  avcodec_find_encoder_by_name || avcodec_find_encoder 获取编码器
 * 3.  codec->sample_fmts 遍历编码器支持的format，检测输入format是否支持，不支持需要重采样
 * 4.  avcodec_alloc_context3 创建编码器的上下文
 * 5.  avcodec_open2 配置好上下文的 参数后 打开编码器。 如果有当前编码器自带的参数如：vbr，获取命令行参数，在这里通过AVDictionary传入
 * 6.  av_frame_alloc 创建 AVFrame 对象，用来存取输入缓冲区的数据
 * 7.  配置AVFrame的数据
 * 8.  通过6/7步配置的数据，用av_frame_get_buffer来真正创建 输入缓冲区
 * 9.  av_packet_alloc 创建 AVPacket 对象，用来接收 编码后的数据，即输出缓冲区
 * 10. 读取输入文件的数据 到 AVFrame的 data 和 lineSize 中
 * 11. 准备完成，开始进入编码步骤
 * 12. avcodec_send_frame 将 上下文 和  AVFrame 发送到编码器中. 也就是 AVFrame数据 编码到了 上下文context 中。
 * 13. avcodec_receive_packet 循环读取 context 的数据 到 AVPacket中， 需要特殊处理EAGAIN 和 EOF错误
 * 14. 将 AVPacket 数据写入到 输出文件中
 * 15. 释放av_packet_unref AVPacket资源
 * 16. 重复13-15步知道编码的数据读完
 * 17. 重复10步，继续读取下一次数据，然后重复进行编码
 * 18. 读取和编码完成后，将输入frame 设为 nullptr 后 再次 调用一次 12-15 步，确保缓冲区数据全部清空
 * 19. 释放所有对象
 *
 *
 * 流程: inputFile -> AVFrame -> codec context -> AVPacket -> outputFile
 * 主要参数配置： audio 三要素： format channel_layout sampleRate
 */
```

## 2.7 AAC文件解码成PCM
触发按钮：`开始 aac 文件解码(decode)
业务文件：[JBThreadAACDecodeFile](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadAACDecodeFile.cpp)
主要步骤：
```C
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
```

## 2.8 录制视频YUV文件
触发按钮：`开始 录制 yuv`
业务文件：[JBThreadVideoRecordYUV](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadVideoRecordYUV.cpp)
主要步骤：

```C
/**
* 步骤：
* 1. av_find_input_format 获取摄像头设备的具体参数
* 2. avformat_open_input 打开设备， 创建AVFormatContext设备
* 3. 打开写入的文件
* 4. av_packet_alloc 开辟接收数据的AVPacket空间
* 5. av_read_frame 循环读取数据AVFormatContext的信息到AVPacket中
* 6. 将AVPacket包含的数据写入文件
* 7. 退出及释放
*/
```

## 2.9 使用 SDL 播放 yuv
触发按钮：`开始 使用 SDL 播放 yuv`
业务文件：[JBThreadVideoPlayYUV](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadVideoPlayYUV.cpp)
主要步骤：

```C
/**
* 步骤：
* 1. SDL_CreateWindow 创建window 窗口
* 2. SDL_CreateRenderer 创建上下文
* 3. SDL_CreateTexture 根据上下文创建纹理
* 4. 打开对应的文件
* 5. 启动timer
* 6.   在循环中，从文件中读取一帧图片内存
* 7.   SDL_UpdateTexture ...  SDL_RenderPresent 一系列操作显示到window上
 */
```

## 2.10 使用Qt原生播放 yuv
触发按钮：`开始 使用Qt原生播放 yuv`
业务文件：[JBVideoQtPlayYuv](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBVideoQtPlayYuv.cpp)
主要步骤：

```C
/**
* 步骤：
* 1. 弹出 qt 的 dialog
* 2. 打开对应的文件
* 3. av_image_get_buffer_size 计算当前格式的视频，一帧的大小
* 4. 启动timer
* 5.   - 在循环中，从文件中读取一帧图片内存
* 6.   - 如果文件格式不是 rgb24， 则调用自己写的转换函数，对文件格式进行转换convertYuvFormat
* 7.   - 获取符合格式的一帧图片数据
* 8.   - 调用 update()， 会进入qt的重绘函数
* 9. 在paintEvent中drawImage 画出上步骤的图片
 */
```

## 2.11  h264 编码(encode)
触发按钮：`开始 h264 编码(encode)`
业务文件：[JBThreadH264EncodeFile](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadH264EncodeFile.cpp)
主要步骤：

```C
/**
 * 步骤：
 * 1.  打开输入 输出 文件
 * 2.  avcodec_find_encoder_by_name || avcodec_find_encoder 获取编码器 - libx264
 * 3.  codec->check_fmt 遍历编码器支持的format，检测输入format是否支持，不支持需要重采样
 * 4.  avcodec_alloc_context3 创建编码器的上下文
 * 5.  avcodec_open2 配置好上下文的 参数后 打开编码器。 如果有当前编码器自带的参数如：vbr，获取命令行参数，在这里通过AVDictionary传入
 * 6.  av_frame_alloc 创建 AVFrame 对象，用来存取输入缓冲区的数据
 * 7.  配置AVFrame的数据
 * 8.  通过6/7步配置的数据，用av_image_alloc来真正创建 输入缓冲区
 * 9.  av_packet_alloc 创建 AVPacket 对象，用来接收 编码后的数据，即输出缓冲区
 * 10. av_image_get_buffer_size 获取单帧图片的yuv大小
 * 11. 准备完成，开始进入编码步骤
 * 12. avcodec_send_frame 将 上下文 和  AVFrame 发送到编码器中. 也就是 AVFrame数据 编码到了 上下文context 中。
 * 13. avcodec_receive_packet 循环读取 context 的数据 到 AVPacket中， 需要特殊处理EAGAIN 和 EOF错误
 * 14. 将 AVPacket 数据写入到 输出文件中
 * 15. 释放av_packet_unref AVPacket资源
 * 16. 重复13-15步知道编码的数据读完
 * 17. 重复10步，继续读取下一次数据，然后重复进行编码
 * 18. 读取和编码完成后，将输入frame 设为 nullptr 后 再次 调用一次 12-15 步，确保缓冲区数据全部清空
 * 19. 释放所有对象
 *
 *
 * 流程: inputFile -> AVFrame -> codec context -> AVPacket -> outputFile
 */
```

## 2.12 h264 解码(decode)
触发按钮：`开始 h264 解码(decode)`
业务文件：[JBThreadH264DecodeFile](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadH264DecodeFile.cpp)
主要步骤：

```C
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
 * 9.   循环调用 av_parser_parse2 来使用 解析器 将 buffer 解析成 AVPacket 对象
 * 10.    调用自定义方法 decode 来处理解码
 * 11.    avcodec_send_packet 将 AVPacket 送入 解码器的上下文中
 * 12     循环调用 avcodec_receive_frame 来接收 解码器中的数据到 AVFrame中
 * 13     将AVFrame 按照planner 写入到输出文件
 * 12. 读取和编码完成后，将输入AVPacket 设为 nullptr 后 再次 调用一次 10 步，确保缓冲区数据全部清空
 * 13. 释放所有对象
 *
 *
 * 流程: inputFile -> 原始data -> AVCodecParserContext -> AVPacket -> codec context -> AVFrame -> outputFile
 */
```


## 2.13 解封装mp4 为 yuv 和 pcm
触发按钮：`开始 解封装(demuxer)`
业务文件：[JBThreadDemuxer](https://github.com/goldWave/QTFFmpegDemo/blob/main/code/JBThreadDemuxer.cpp)
主要步骤：

```C
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
```

# ffmpeg 配置步骤
## 集成步骤：
1. `win`: ffmpeg 下载 shared 文件 | mac 直接brew 安装
2. pro 中添加 INCLUDEPATH +=
3. pro 中添加 LIBS +=
4. extern "C" 引用头文件
5. win 需要将dll拷贝到项目exe所在的目录 | 或者将  /bin目录加到环境变量里面
6. 使用需要的方法

# xcode 工程配置
- xcode 工程： qmake -spec macx-xcode
- xcode 修改： Other Code Signing Flags --deep   OTHER_CODE_SIGN_FLAGS
- qmake -spec macx-xcode &&  python /Users/jimbo/Documents/python/pyDemo/pyTest.py
- alias pmake

- 如果签名有问题。需要 将 /usr/local/opt/ 内的所有动态库重新签名
`codesign -f -s "Apple Development: ren_jinbo@163.com (xxxxx)" /usr/local/opt/*/lib/*.dylib`
