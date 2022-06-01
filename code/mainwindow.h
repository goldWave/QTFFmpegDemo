#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class JBThreadAudioRecordPCM;
class JBThreadAudioPlayPCM;
class JBThreadPCM2WavFile;
class JBThreadAudioRecordWav;
class JBThreadResample;
class JBThreadPlayWav;
class JBThreadAACEncodeFile;
class JBThreadAACDecodeFile;
class JBThreadVideoRecordYUV;
class JBThreadVideoPlayYUV;

class JBThreadH264DecodeFile;
class JBThreadH264EncodeFile;

class JBThreadDemuxer;

class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
  
private slots:
    void startAudioCapture();
    void playPcmCapture();
    void switchAudioCapture();
    void recordWavAction();
    void resampleAction();
    void playWavAction();
    void aacCodecFileAction();
    void aacDecodeFileAction();
    void videoRecordYuvAction();
    void videoPlayYuvAction();
    void videoPlayYuvWithQtAction();
    void h264DecodeFileAction();
    void h264EncodeFileAction();
    void demuxerFileAction();

    void copyBtnAction();
    void copyBtn2Action();
    void timeUpdated(int timestamp);
    void updateComandLine(const QString& line);
    void updateComandLines(const QStringList& lines);
    
private:
    template<typename T> void dealThreadBtn(T **_thread, QPushButton *btn, const QString &subStr = "");
    
private:
    Ui::MainWindow *ui;
    JBThreadAudioRecordPCM *m_threadAudioRecordPcm{nullptr};
    JBThreadAudioPlayPCM* m_threadAudioPlayPcm{ nullptr };
    JBThreadPCM2WavFile* m_switchThread{ nullptr };
    JBThreadAudioRecordWav* m_recordWavThread{ nullptr };
    JBThreadResample* m_resampleThread{ nullptr };
    JBThreadPlayWav * m_threadPlayWav{ nullptr };
    JBThreadAACEncodeFile* m_threadAACEncode{ nullptr };
    JBThreadAACDecodeFile* m_threadAACDecode{ nullptr };
    JBThreadVideoRecordYUV* m_threadVideoRecordYuv{ nullptr };
    JBThreadVideoPlayYUV* m_threadVideoPlayYuv{ nullptr };

    JBThreadH264EncodeFile* m_threadH264Encode{ nullptr };
    JBThreadH264DecodeFile* m_threadH264Decode{ nullptr };

    JBThreadDemuxer* m_threadDemuxer{ nullptr };
};
#endif // MAINWINDOW_H
