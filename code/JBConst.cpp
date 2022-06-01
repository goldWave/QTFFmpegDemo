#include "JBConst.h"
#include <qglobal.h>
#include <QObject>

#ifdef Q_OS_WIN
const QString k_resource_dir = "C:\\Users\\Administrator\\Documents\\source\\Other\\JBFFTest\\presources\\";
const QString k_out_temp_dir = "C:\\Users\\Administrator\\Desktop\\22\\";
const QString k_fmt_name = "dshow";
const QString k_device_name_audio_first = "audio=FrontMic (Realtek(R) Audio)";
const QString k_device_name_video_first = QString::fromUtf8("video=Microsoft\xC2\xAE LifeCam Studio(TM)"); //\xC2\xAE = ®
#else
const QString k_resource_dir = "/Users/jimbo/Documents/project/QTFFmpegDemo/presources/";
const QString k_out_temp_dir = "/Users/jimbo/Desktop/test/";
const QString k_fmt_name = "avfoundation";
const QString k_device_name_audio_first = ":2";
const QString k_device_name_video_first = "0:";
#endif


const QString k_resource_pcm_file = k_resource_dir + "44100_2_s16_changlu.pcm";
const QString k_resource_wav_file = k_resource_dir + "44100_2_s16_changlu.wav";
const QString k_resource_aac_file = k_resource_dir + "44100_2_s16_changlu.aac";
const QString k_resource_bmp_file = k_resource_dir + "test.bmp";
const QString k_resource_yuv_sigle_file = k_resource_dir + "200x300-bgr24-single.yuv";
const QString k_resource_mp4_file = k_resource_dir + "1920x1080_30fps_10s_aac_48000_stereo.mp4";


const QString k_out_wav_file = k_out_temp_dir + "44100_2_s16_changlu.wav";
const QString k_out_aac_file = k_out_temp_dir + "out.aac";
const QString k_out_pcm_file = k_out_temp_dir + "44100_2_s16_changlu.pcm";

const QString k_out_pcm2_file = k_out_temp_dir + "48000_2_f32.pcm";
