#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QDir>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
}


const static int info_level = AV_LOG_INFO;
//重新输出 ffmpeg 内部log，不然不能输出到控制台
void FFMPEG_Callback(void*, int level, const char* fmt, va_list vl)
{
        char buffer[1024];
        vsprintf(buffer, fmt, vl);
    if (info_level < level) {
        return;
    }
        qDebug() << "[ffmpeg]" <<"lv:" << level  <<"\tmsg:" << buffer;
}

int main(int argc, char *argv[])
{
    avdevice_register_all();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    av_log_set_callback(&FFMPEG_Callback);
    av_log_set_level(info_level);

    return a.exec();
}
