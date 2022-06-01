#include "JBVideoQtPlayYuv.h"
#include <QPainter>
#include <QDebug>

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


JBVideoQtPlayYuv::JBVideoQtPlayYuv(QWidget* parent)
: QDialog{ parent }
{
    setMinimumSize({ 640, 480 });
    
    m_srcYuvData.file = k_out_temp_dir + "movie_640x480_yuv420p.yuv";
    m_srcYuvData.fps = 30;
    m_srcYuvData.width = 640;
    m_srcYuvData.height = 480;
    m_srcYuvData.pixelFormat = AV_PIX_FMT_YUV420P;
    
    m_myTimer = new QTimer(this);
    m_myTimer->setInterval(33);
    QObject::connect(m_myTimer, SIGNAL(timeout()), this, SLOT(updateMovie()));
    
    m_file.setFileName(m_srcYuvData.file);
    if (!m_file.open(QFile::ReadOnly)) {
        qDebug() << "open file failed: " << m_file.fileName();
        return;
    }
    
    m_dstRect = { 0, 0, m_srcYuvData.width, m_srcYuvData.height };
    m_pixelSize = av_image_get_buffer_size(m_srcYuvData.pixelFormat, m_srcYuvData.width, m_srcYuvData.height, 1);
    qDebug() << "m_pixelSize: " << m_pixelSize;
    m_myTimer->start();
    
}

void JBVideoQtPlayYuv::updateMovie() {
    
    char* data = new char[m_pixelSize];
    
    if (m_file.read(data, m_pixelSize) == m_pixelSize)
    {
        freeCurrentImage();
        if (m_srcYuvData.pixelFormat != AV_PIX_FMT_RGB24) {
            
            //conver other format to rgb24
            auto inData = m_srcYuvData;
            auto outData = m_srcYuvData;
            outData.pixelFormat = AV_PIX_FMT_RGB24;
            
            
            inData.pixels = new char[m_pixelSize];
            memcpy(inData.pixels, data, m_pixelSize);
            JBCommonMethod::convertYuvFormat(inData, outData);
          
            delete []  inData.pixels;
            inData.pixels = nullptr;
            
            m_img = new QImage((uchar*)outData.pixels, m_srcYuvData.width, m_srcYuvData.height, QImage::Format_RGB888);
        } else {
            
            m_img = new QImage((uchar*)data, m_srcYuvData.width, m_srcYuvData.height, QImage::Format_RGB888);
        }
        update(); //触发QT的重绘机制， 进入paintEvent
        
        
        
    }
    else {
        m_myTimer->stop();
    }
    delete [] data;
    
    
}
void JBVideoQtPlayYuv::freeCurrentImage() {
    if (!m_img) return;
    if (!m_img->bits()) {
        return;
    }
    free(m_img->bits());
    delete m_img;
    m_img = nullptr;
}

JBVideoQtPlayYuv::~JBVideoQtPlayYuv()
{
    if (m_myTimer)
    {
        m_myTimer->stop();
        delete m_myTimer;
    }
    freeCurrentImage();
}

void JBVideoQtPlayYuv::paintEvent(QPaintEvent* )
{
    if (!m_img)
    {
        return;
    }
    
    QPainter painter(this);
    painter.drawImage(m_dstRect, *m_img);
}
