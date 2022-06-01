#include "JBThreadVideoPlayYUV.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "JBConst.h"
#include <QObject>

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

/*
 static const struct TextureFormatEntry {
 enum AVPixelFormat format;
 int texture_fmt;
 } sdl_texture_format_map[] = {
 { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
 { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
 { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
 { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
 { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
 { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
 { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
 { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
 { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
 { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
 { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
 { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
 { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
 { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
 { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
 { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
 { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
 { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
 { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
 { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
 };
 */


static const SDL_Rect srcRect = { 0, 0, 640, 480 };

JBThreadVideoPlayYUV::JBThreadVideoPlayYUV(QObject* parent) : QObject(parent)
{
    m_myTimer = new QTimer(this);
    m_myTimer->setInterval(33);
    QObject::connect(m_myTimer, SIGNAL(timeout()), this, SLOT(updateMovie()));
}


void JBThreadVideoPlayYUV::play() {
    
    int ret = 0;
    //SDL_Surface* surface = nullptr;
    SDL_Window *window = nullptr;
    
    //	QFile file(k_resource_yuv_sigle_file);
    m_file.setFileName(k_out_temp_dir + "movie_640x480_yuv420p.yuv");
    ret = SDL_Init(SDL_INIT_VIDEO);
    CHECK(ret != 0, "SDL_Init");
    
    window = SDL_CreateWindow("picture title", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, srcRect.w, srcRect.h, SDL_WINDOW_SHOWN);
    
    if (!window)
    {
        JBCommonMethod::logErrorSDL("SDL_CreateWindow");
        goto end;
    }
    
    //渲染上下文
    m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        m_renderer = SDL_CreateRenderer(window, -1, 0);
        CHECK(!m_renderer, "SDL_CreateRenderer");
    }
    //纹理
    // yuv420p -> SDL_PIXELFORMAT_IYUV
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, srcRect.w, srcRect.h);
    
    //texture = SDL_CreateTextureFromSurface(renderer, surface);
    CHECK(!m_texture, "SDL_CreateRenderer");
    
    
    // 打开文件
    if (!m_file.open(QFile::ReadOnly)) {
        qDebug() << "open file failed: " << m_file.fileName();
        goto end;
    }

    m_myTimer->start();
    
    while (true)
    {
        SDL_Event event;
        SDL_WaitEvent(&event);
        switch (event.type)
        {
            case SDL_QUIT:
                goto end;
            case SDL_MOUSEBUTTONUP:
                //showClick(event, renderer, texture);
                break;
            default:
                break;
        }
    }
    
end:
    m_myTimer->stop();
    delete m_myTimer;
    
    m_file.close();
    SDL_DestroyTexture(m_texture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


void JBThreadVideoPlayYUV::stop() {
    
}

void JBThreadVideoPlayYUV::updateMovie() {
    
    int imgSize = static_cast<int>(srcRect.w * srcRect.h * 1.5);

    char *data  = new char[imgSize];
    int ret = 0;
    if (m_file.read(data, imgSize) > 0) {
        // 将YUV的像素数据填充到texture
        ret = SDL_UpdateTexture(m_texture, nullptr, data, srcRect.w);
        RET(ret != 0, "SDL_RenderCopy");
        
        // 设置绘制颜色（画笔颜色）
        ret = SDL_SetRenderDrawColor(m_renderer,
                                   0, 0, 0, SDL_ALPHA_OPAQUE);
        RET(ret != 0, "SDL_SetRenderDrawColor");
        
        // 用绘制颜色（画笔颜色）清除渲染目标
        ret = SDL_RenderClear(m_renderer);
        RET(ret != 0, "SDL_RenderClear");
        
        // 拷贝纹理数据到渲染目标（默认是window）
        ret = SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
        RET(ret != 0, "SDL_RenderCopy");
        
        // 更新所有的渲染操作到屏幕上
        SDL_RenderPresent(m_renderer);
    } else {
        // 文件数据已经读取完毕
        m_myTimer->stop();
        delete[] data;
    }
    
}
