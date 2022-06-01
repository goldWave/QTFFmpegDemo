# ffmpeg 命令行使用

## 查看设备列表：

- win: `ffmpeg -f dshow -list_devices true -i dummpy`
```
[dshow @ 000002266c527fc0] "Microsoft® LifeCam Studio(TM)" (video)
[dshow @ 000002266c527fc0]   Alternative name "@device_pnp_\\?\usb#vid_045e&pid...
[dshow @ 000002266c527fc0] "PRISM Live Studio" (video)
[dshow @ 000002266c527fc0]   Alternative name "@device_sw_{860BB310-5D01-11D0-BD3B-00A0C911CE86}\{A49F51EE-8841-4425-BEC0-85D0C470BBDE}"
[dshow @ 000002266c527fc0] "桌面麦克风 (Microsoft® LifeCam Studio(TM))" (audio)
[dshow @ 000002266c527fc0]   Alternative name "@device_cm_{33D9A762-90C8-11D0-BD43-00A0C911CE86}\wave_{85A88CC0-ADB6-4E97-BB5D-AD3AAB334541}"
[dshow @ 000002266c527fc0] "FrontMic (Realtek(R) Audio)" (audio)
[dshow @ 000002266c527fc0]   Alternative name "@device_cm_{33D9A762-90C8-11D0-BD43-00A0C911CE86}\wave_{7A5C2EBB-2C09-402A-B380-D250BFF4588E}"
```

- mac: `ffmpeg -f avfoundation -list_devices true -i dummpy`


```
[AVFoundation indev @ 0x7f82ce905040] AVFoundation video devices:
[AVFoundation indev @ 0x7f82ce905040] [0] FaceTime HD Camera
[AVFoundation indev @ 0x7f82ce905040] [1] Capture screen 0
[AVFoundation indev @ 0x7f82ce905040] [2] Capture screen 1
[AVFoundation indev @ 0x7f82ce905040] AVFoundation audio devices:
[AVFoundation indev @ 0x7f82ce905040] [0] Built-in Microphone
[AVFoundation indev @ 0x7f82ce905040] [1] PRISM Live Studio
```
##查看Dshow/Avfoundation库支持的具体支持参数：

- MAC : `ffmpeg -h demuxer=avfoundation`
 查看 avfoundation 的可支持的具体命令，比如avfoundation 就不支持dshow的 `list_options` 命令
 
```bash
Demuxer avfoundation [AVFoundation input device]:
AVFoundation indev AVOptions:
  -list_devices      <boolean>    .D......... list available devices (default false)
  -video_device_index <int>        .D......... select video device by index for devices with same name ...
  -audio_device_index <int>        .D......... select audio device by index for devices with same name ...
  -pixel_format      <pix_fmt>    .D......... set pixel format (default yuv420p)
  -framerate         <video_rate> .D......... set frame rate (default "ntsc")
  -video_size        <image_size> .D......... set video size
  -capture_cursor    <boolean>    .D......... capture the screen cursor (default false)
  ......等
```

- WIN : `ffmpeg -h demuxer=dshow`
 
```bash
Demuxer dshow [DirectShow capture]:
dshow indev AVOptions:
  -video_size        <image_size> .D......... set video size given a...
  -pixel_format      <pix_fmt>    .D......... set video pixel format (default none)
  -framerate         <string>     .D......... set video frame rate
  -sample_rate       <int>        .D......... set audio sample rate ...
  -sample_size       <int>        .D......... set audio sample size (from 0 to 16) (default 0)
  -channels          <int>        .D......... set number of audio channels, ...
  -audio_buffer_size <int>        .D......... set audio device buffer latency size in ...
  -list_devices      <boolean>    .D......... list available devices (default false)
  -list_options      <boolean>    .D......... list available options for specified device...
  ......等
```


##查看具体设备的支持分辨率或采样格式等列表：

- WIN Video: `ffmpeg -f dshow -list_options true -i video="Microsoft® LifeCam Studio(TM)"`

```
[dshow @ 0000026dba738040] DirectShow video device options (from video devices)
[dshow @ 0000026dba738040]  Pin "捕获" (alternative pin name "捕获")
[dshow @ 0000026dba738040]   pixel_format=yuyv422  min s=640x480 fps=7.5 max s=640x480 fps=30
[dshow @ 0000026dba738040]   pixel_format=yuyv422  min s=960x544 fps=7.5 max s=960x544 fps=20
[dshow @ 0000026dba738040]   pixel_format=yuyv422  min s=800x448 fps=7.5 max s=800x448 fps=30
  ......等
```

- WIN Aduio: `ffmpeg -f dshow -list_options true -i audio="桌面麦克风 (Microsoft® LifeCam Studio(TM))"`

```
[dshow @ 0000019c5c2b8040] DirectShow audio only device options (from audio devices)
[dshow @ 0000019c5c2b8040]  Pin "Capture" (alternative pin name "Capture")
[dshow @ 0000019c5c2b8040]   ch= 2, bits=16, rate= 44100 
[dshow @ 0000019c5c2b8040]   ch= 1, bits=16, rate= 44100
[dshow @ 0000019c5c2b8040]   ch= 2, bits=16, rate= 32000
[dshow @ 0000019c5c2b8040]   ch= 1, bits=16, rate= 32000
[dshow @ 0000019c5c2b8040]   ch= 2, bits=16, rate= 22050
  ......等
```

## 录音：
前置说明： `-i v:a ` 冒号前面是video编号，后面是audio编号，如果只有摄像头采集的话冒号可以省略


默认参数版：

-mac: `ffmpeg -f avfoundation -i :0 out.wav`

-win: `ffmpeg -f dshow -i audio="麦克风 (Razer Seiren)" out.wav`

带参数版：
-win： `ffmpeg -f dshow -sample_rate 44100 -sample_size 16 -channels 2 -i audio="麦克风 (Razer Seiren)" out.wav`

### 播放pcm：
`ffplay -ar 44100 -ac 2 -f s16le out.pcm` pcm必须带必须的传入参数

## 视频:
基本配置选项： `-video_size` `-pixel_format` `framerate`

### 录屏:
如果 `-video_size  -pixel_format framerate` 都不写也能成功，只是会设置默认值。 查看设备支持的具体参数可以参考上面的`list_options`.

- WIN: ` ffmpeg -f dshow -video_size 1920x1080 -pixel_format yuyv422 -i video="Microsoft® LifeCam Studio(TM)" out1.yuv` 
- mac: `ffmpeg -f avfoundation -framerate 30 -i 0: out.yuv`, `0:` 可以省略成 `0`。
  mac不能查看设备支持的具体参数，可以在上条命令报错的时候，会列出支持的具体数据。 比如如果`framerate`不设置可以报错，


### 播放yuv
`ffplay -pixel_format yuyv422 -video_size 1920x1080 out1.yuv`

### 转换 format格式

- `ffmpeg -i 640x480.jpg -pix_fmt yuv420p out_yuv420p.yuv` 将 yuvj420p 转换成 yuv420p 成功。
- `ffmpeg -i 640x480.jpg -pixel_format yuv420p out_yuv420p.yuv` 格式转换失败。-pixel_format 用后，转换出的格式还是 yuvj420p。

### 视频编码

- `ffmpeg -pix_fmt yuv420p -video_size 640x480 -i  movie_640x480_yuv420p.yuv -c:v libx264 -r 30 ffmpge_640x480_yuv420p.h264` 将yuv 编码成 h264
 `-r 30` 代表输出 30fps

## 音视频解封装

- mp4 -> h264: `ffmpeg -i test.mp4 -c:v copy -an output.h264`, 丢弃音频流，只保存视频流
- mp4 -> aac: `ffmpeg -i test.mp4 -c:a copy -vn output.aac`, 丢弃视频流，只保存音频流 
- h264 -> yuv: `ffmpeg -c:v h264 -i output.h264 out.yuv`, h264 解码成yuv
- aac -> pcm: `ffmpeg -c:a aac -i output.aac out.pcm`, aac 解码成pcm


# 基础知识

## linesize
- linesize  在 线性PCM 中代表， 多个声道的总大小 = nChannels * 位深(AVSampleFormat) >> 3
- 在 planner 中，代表一个单声道数据的大小  = 位深 >> 3

## 样本帧
- 代表一个大样本（包含多个声道的样本）
- 每个声道代表一个小样本
