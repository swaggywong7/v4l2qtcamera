#ifndef V4L2CAPTURETHREAD_H
#define V4L2CAPTURETHREAD_H

#include <QThread>              // Qt线程基类
#include <QPixmap>              // 图像类型
#include <sys/ioctl.h>          // ioctl函数
#include <linux/videodev2.h>    // V4L2定义
#include <stdio.h>

// V4L2采集线程类,继承自QThread
class V4L2CaptureThread : public QThread
{
    Q_OBJECT                    // 必须加这个宏才能使用信号槽

public:
    // 构造函数
    // fd: 视频设备文件描述符
    // buffers: 缓冲区地址数组
    // parent: 父对象
    V4L2CaptureThread(int fd, char *buffers[4], QObject *parent = nullptr);

    // 析构函数
    ~V4L2CaptureThread();

    // 停止线程
    void stop();

signals:
    // 信号: 新帧就绪
    // pixmap: 采集到的图像(MJPEG已解码为QPixmap)
    void frameReady(QPixmap pixmap);

    // 信号: 发生错误
    // error: 错误描述
    void errorOccurred(QString error);

protected:
    // 线程执行函数(重写QThread的run方法)
    void run() override;

private:
    int m_fd;                   // 视频设备文件描述符
    char *m_userbuff[4];        // 缓冲区地址数组(指向用户空间的mmap内存)
    bool m_running;             // 运行标志
};

#endif // V4L2CAPTURETHREAD_H
