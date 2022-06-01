//
//  JBThreadBase.m
//  JBFFTest
//
//  Created by 任金波 on 2022/4/16.
//

#include "JBThreadBase.h"

#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include "JBCommonMethod.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
}


JBThreadBase::JBThreadBase(QObject *parent)
: QThread{parent}
{
    connect(this, &JBThreadBase::finished, this, &JBThreadBase::deleteLater);
}

JBThreadBase::~JBThreadBase() {
    
    disconnect();
    
    requestInterruption();
    quit();
    wait();
    
    qDebug() << this << "destory";
}
