#include "v4l2capturethread.h"
#include <QDebug>               // 调试输出
#include <errno.h>              // 错误码
#include <string.h>             // strerror函数

// 构造函数
V4L2CaptureThread::V4L2CaptureThread(int fd, char *buffers[4], QObject *parent)
    : QThread(parent)           // 调用父类构造函数
    , m_fd(fd)                  // 保存文件描述符
    , m_running(false)          // 初始化运行标志为false
{
    // 复制缓冲区地址(4个缓冲区)
    for(int i = 0; i < 4; i++) {
        m_userbuff[i] = buffers[i];
    }

    qDebug() << "V4L2采集线程创建";
}

// 析构函数
V4L2CaptureThread::~V4L2CaptureThread()
{
    stop();                     // 确保线程已停止
    qDebug() << "V4L2采集线程销毁";
}

// 停止线程
void V4L2CaptureThread::stop()
{
    qDebug() << "停止采集线程...";
    m_running = false;          // 设置标志为false,循环会退出

    // 等待线程结束(最多3秒)
    if(!wait(3000)) {           // wait()阻塞,直到run()执行完
        qWarning() << "线程未在3秒内结束,强制终止";
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(m_fd, VIDIOC_STREAMOFF, &type);
        wait();                 // 等待终止完成
    }

    qDebug() << "采集线程已停止";
}

// 线程执行函数(在独立线程中运行)
void V4L2CaptureThread::run()
{
    qDebug() << "采集线程开始运行";

    m_running = true;           // 设置运行标志

    // 定义V4L2缓冲区结构体
    struct v4l2_buffer buffer;

    // ========== 主循环:持续采集视频帧 ==========
    while(m_running) {
        // 清零缓冲区结构体
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  // 类型:视频捕获

        // ========== 从内核队列中取出已填充的缓冲区 ==========
        // VIDIOC_DQBUF: Dequeue Buffer(出队)
        // 这个调用会阻塞,直到有数据就绪
        // 但因为在子线程中,不会阻塞主线程UI
        int ret = ioctl(m_fd, VIDIOC_DQBUF, &buffer);

        if(ret < 0) {           // 如果ioctl失败
            // 检查是否是被中断(比如线程被停止)
            if(errno == EINTR) {
                qDebug() << "ioctl被中断";
                continue;       // 继续循环
            }

            // 其他错误
            QString errMsg = QString("DQBUF失败: %1").arg(strerror(errno));
            qCritical() << errMsg;
            emit errorOccurred(errMsg);  // 发送错误信号
            break;              // 退出循环
        }

        // ========== 处理图像数据 ==========
        // 获取缓冲区索引
        int index = buffer.index;

        // 获取数据指针和大小
        unsigned char *data = (unsigned char *)m_userbuff[index];
        unsigned int dataSize = buffer.bytesused;

        // 将MJPEG数据解码为QPixmap
        QPixmap pixmap;
        // loadFromData: 从内存中的JPEG/PNG等格式数据加载图像
        bool loaded = pixmap.loadFromData(data, dataSize);

        if(!loaded) {           // 如果解码失败
            qWarning() << "解码MJPEG失败";
        } else {
            // 发送信号到主线程(Qt会自动处理线程间通信)
            emit frameReady(pixmap);
        }

        // ========== 将缓冲区放回内核队列 ==========
        // VIDIOC_QBUF: Queue Buffer(入队)
        // 让驱动可以重新使用这个缓冲区
        ret = ioctl(m_fd, VIDIOC_QBUF, &buffer);

        if(ret < 0) {           // 如果放回失败
            QString errMsg = QString("QBUF失败: %1").arg(strerror(errno));
            qCritical() << errMsg;
            emit errorOccurred(errMsg);
        }

        // 可选:稍微休眠一下,降低CPU占用
        // 因为摄像头是30fps,所以即使不休眠也不会太快
        // msleep(10);  // 休眠10毫秒
    }

    qDebug() << "采集线程结束运行";
}
