QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    code/JBCommonMethod.cpp \
    code/JBConst.cpp \
    code/JBThreadAudioRecordPCM.cpp \
    code/main.cpp \
    code/mainwindow.cpp \
    code/JBThreadAudioPlayPCM.cpp \
    code/JBThreadPCM2WavFile.cpp\
    code/JBThreadAudioRecordWav.cpp \
    code/JBThreadResample.cpp \
    code/JBThreadBase.cpp \
    code/JBThreadPlayWav.cpp \
    code/JBThreadAACEncodeFile.cpp \
    code/JBThreadAACDecodeFile.cpp \
    code/JBThreadVideoRecordYUV.cpp \
    code/JBThreadVideoPlayYUV.cpp \
    code/JBVideoQtPlayYuv.cpp \
    code/JBThreadH264EncodeFile.cpp\
    code/JBThreadH264DecodeFile.cpp \
    code/JBThreadDemuxer.cpp

HEADERS += \
    code/JBCommonMethod.h \
    code/JBConst.h \
    code/JBThreadAudioRecordPCM.h \
    code/mainwindow.h \
    code/JBThreadAudioPlayPCM.h \
    code/JBThreadPCM2WavFile.h \
    code/JBThreadAudioRecordWav.h \
    code/JBThreadResample.h \
    code/JBThreadBase.h \
    code/JBThreadPlayWav.h \
    code/JBThreadAACEncodeFile.h \
    code/JBThreadAACDecodeFile.h \
    code/JBThreadVideoRecordYUV.h \
    code/JBThreadVideoPlayYUV.h \
    code/JBVideoQtPlayYuv.h \
    code/JBThreadH264EncodeFile.h\
    code/JBThreadH264DecodeFile.h \
    code/JBThreadDemuxer.h


FORMS += \
    code/mainwindow.ui
    #README.md

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


#ffmpeg
win32 {
    FFMPEG_DIR= C:/msys64/usr/local/ffmpeg
    SDL_DIR= C:/Users/Administrator/Documents/source/third-lib/SDL2-2.0.20
    SDL_LIB_DIR= C:/Users/Administrator/Documents/source/third-lib/SDL2-2.0.20/lib/x64
}

macx {
    FFMPEG_DIR= /usr/local/ffmpeg
    SDL_DIR=/usr/local/Cellar/sdl2/2.0.20
    SDL_LIB_DIR= /usr/local/Cellar/sdl2/2.0.20/lib

    #添加plist
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_TARGET_BUNDLE_PREFIX = com.ffmpeg.test
    QMAKE_BUNDLE = jimbo

}

INCLUDEPATH += $${FFMPEG_DIR}/include

LIBS += -L$${FFMPEG_DIR}/bin \
        -L$${FFMPEG_DIR}/lib \
        -lavcodec \
        -lavformat \
        -lavutil \
        -lavdevice \
        -lswresample \
        -lswscale

INCLUDEPATH += $${SDL_DIR}/include
LIBS += -L$${SDL_LIB_DIR} \
        -lSDL2

message($${INCLUDEPATH})
message($${SDL_LIB_DIR})
