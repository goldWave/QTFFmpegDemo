#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "JBThreadAudioRecordPCM.h"
#include "JBThreadAudioPlayPCM.h"
#include "JBThreadPCM2WavFile.h"
#include "JBThreadAudioRecordWav.h"
#include <qdebug.h>
#include <QClipboard>
#include <QTimer>
#include "JBCommonMethod.h"
#include "JBThreadResample.h"
#include "JBThreadPlayWav.h"
#include "JBThreadAACEncodeFile.h"
#include "JBThreadAACDecodeFile.h"
#include "JBThreadVideoRecordYUV.h"
#include "JBThreadVideoPlayYUV.h"
#include "JBVideoQtPlayYuv.h"
#include "JBThreadH264DecodeFile.h"
#include "JBThreadH264EncodeFile.h"
#include "JBThreadDemuxer.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setMinimumWidth(750);

    connect(ui->copyBtn, &QPushButton::clicked, this, &MainWindow::copyBtnAction);
    connect(ui->copyBtn_2, &QPushButton::clicked, this, &MainWindow::copyBtn2Action);
    
    connect(ui->recordAudioBtn, &QPushButton::clicked, this, &MainWindow::startAudioCapture);
    connect(ui->playAudioBtn, &QPushButton::clicked, this, &MainWindow::playPcmCapture);
    connect(ui->switchButton, &QPushButton::clicked, this, &MainWindow::switchAudioCapture);
    connect(ui->recordWavBtn, &QPushButton::clicked, this, &MainWindow::recordWavAction);
    connect(ui->resampleBtn, &QPushButton::clicked, this, &MainWindow::resampleAction);
    connect(ui->playWavBtn, &QPushButton::clicked, this, &MainWindow::playWavAction);
    connect(ui->aacEncodeBtn, &QPushButton::clicked, this, &MainWindow::aacCodecFileAction);
    connect(ui->aacDecodeBtn, &QPushButton::clicked, this, &MainWindow::aacDecodeFileAction);
	connect(ui->yuvRecordBtn, &QPushButton::clicked, this, &MainWindow::videoRecordYuvAction);
    connect(ui->playYuvBtn, &QPushButton::clicked, this, &MainWindow::videoPlayYuvAction);
    connect(ui->playYuvByQtBtn, &QPushButton::clicked, this, &MainWindow::videoPlayYuvWithQtAction);

    connect(ui->encodeH264Btn, &QPushButton::clicked, this, &MainWindow::h264EncodeFileAction);
    connect(ui->decodeH264Btn, &QPushButton::clicked, this, &MainWindow::h264DecodeFileAction);

    connect(ui->demuxerBtn, &QPushButton::clicked, this, &MainWindow::demuxerFileAction);
  }

void MainWindow::startAudioCapture() {
    dealThreadBtn(&m_threadAudioRecordPcm,  ui->recordAudioBtn, QString::fromLocal8Bit(" 录制 pcm"));
}

void MainWindow::playPcmCapture() {
    dealThreadBtn(&m_threadAudioPlayPcm,  ui->playAudioBtn, QString::fromLocal8Bit(" 播放 pcm"));
}

void MainWindow::switchAudioCapture() {
    dealThreadBtn(&m_switchThread,  ui->switchButton, " pcm to wav");
}

void MainWindow::recordWavAction() {
    dealThreadBtn(&m_recordWavThread,  ui->recordWavBtn, " 录制 wav");
}

void MainWindow::resampleAction() {
    dealThreadBtn(&m_resampleThread,  ui->resampleBtn, QString::fromLocal8Bit(" 重采样(Resample)"));
}

void MainWindow::playWavAction() {
    dealThreadBtn(&m_threadPlayWav,  ui->playWavBtn, QString::fromLocal8Bit(" 播放 wav"));
}

void MainWindow::aacCodecFileAction() {
	dealThreadBtn(&m_threadAACEncode, ui->aacEncodeBtn, QString::fromLocal8Bit(" aac 文件编码(encode)"));
}

void MainWindow::aacDecodeFileAction() {
	dealThreadBtn(&m_threadAACDecode, ui->aacDecodeBtn, QString::fromLocal8Bit(" aac 文件解码(decode)"));
}

void MainWindow::videoRecordYuvAction()
{
    dealThreadBtn(&m_threadVideoRecordYuv, ui->yuvRecordBtn, QString::fromLocal8Bit(" 录制 yuv"));
}

void MainWindow::videoPlayYuvAction()
{
    static bool isPlaying = false;
    if (isPlaying)
    { 
        return;
    }
    isPlaying = !isPlaying;
    JBThreadVideoPlayYUV * yuv = new JBThreadVideoPlayYUV(this);
    yuv->play();
    delete yuv;
}

void MainWindow::videoPlayYuvWithQtAction()
{
    JBVideoQtPlayYuv *qtYuv = new JBVideoQtPlayYuv(this);
    qtYuv->exec();
    delete qtYuv;
}

void MainWindow::h264DecodeFileAction()
{
    dealThreadBtn(&m_threadH264Decode, ui->decodeH264Btn, QString::fromLocal8Bit(" h264 解码(decode)"));
}

void MainWindow::h264EncodeFileAction()
{
    dealThreadBtn(&m_threadH264Encode, ui->encodeH264Btn, QString::fromLocal8Bit(" h264 编码(encode)"));
}

void MainWindow::demuxerFileAction()
{
    dealThreadBtn(&m_threadDemuxer, ui->demuxerBtn, QString::fromLocal8Bit(" 解封装(demuxer)"));
}
 
void MainWindow::copyBtnAction()
{
	QClipboard* clip = QApplication::clipboard();
	clip->setText(ui->cLineLabel->text());
}

void MainWindow::copyBtn2Action()
{
    QClipboard* clip = QApplication::clipboard();
    clip->setText(ui->cLineLabel_2->text());
}
 
template<typename T> void MainWindow::dealThreadBtn(T **_thread, QPushButton *btn, const QString &subStr)
{
	QString start = QString::fromLocal8Bit("开始").append(subStr);
	QString stop = QString::fromLocal8Bit("停止").append(subStr);

    qDebug() << btn->objectName() << "click";
    if (!(*_thread)) {
        updateComandLines({}); //清空命令行 label
        *_thread = new T(this);
        (*_thread)->start();
        connect(*_thread, &T::finished, [=]() {
            //线程结束后，置空，和修改文字
            *_thread = nullptr;
            btn->setText(start);
        });
        connect(*_thread, &T::timeChanged, this, &MainWindow::timeUpdated, Qt::QueuedConnection);
        connect(*_thread, &T::comandLineGenerate, this, &MainWindow::updateComandLine, Qt::QueuedConnection);
        connect(*_thread, &T::comandLinesGenerate, this, &MainWindow::updateComandLines, Qt::QueuedConnection);
        btn->setText(stop);
    }
    else {
        (*_thread)->requestInterruption();
        *_thread = nullptr;
        btn->setText(start);
    }
}

void MainWindow::timeUpdated(int timestamp) {
    
    QTime time(0, 0, 0, 0);
    QString text = time.addMSecs(timestamp).toString("mm:ss.z");
    ui->timeLabel->setText(text.left(7));
}

void MainWindow::updateComandLine(const QString& line)
{
    qDebug() << line;
    ui->cLineLabel->setText(line);
    ui->cLineLabel_2->setText("");
}

void MainWindow::updateComandLines(const QStringList& lines) {
    qDebug() << lines;
    ui->cLineLabel->setText("");
    ui->cLineLabel_2->setText("");
    if (lines.size() >= 1) {
        ui->cLineLabel->setText(lines[0]);
    }
    if (lines.size() >= 2) {
        ui->cLineLabel_2->setText(lines[1]);
    }
    
}

MainWindow::~MainWindow()
{
    delete ui;
}


