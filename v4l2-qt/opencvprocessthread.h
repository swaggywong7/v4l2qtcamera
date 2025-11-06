#ifndef OPENCVPROCESSTHREAD_H
#define OPENCVPROCESSTHREAD_H

#include <QThread>              // Qt线程基类
#include <QPixmap>              // Qt图像类型
#include <QMutex>               // 互斥锁
#include <opencv2/opencv.hpp>   // OpenCV库

// OpenCV图像处理线程类
class OpenCVProcessThread : public QThread
{
    Q_OBJECT

public:
    // 处理模式枚举
    enum ProcessMode {
        None,              // 不处理
        GaussianBlur,      // 高斯模糊
        EdgeDetect,        // 边缘检测
        Grayscale,         // 灰度化
        Sharpen,           // 锐化
        Emboss,            // 浮雕效果
        Cartoon            // 卡通化
    };

    // 构造函数
    // parent: 父对象
    explicit OpenCVProcessThread(QObject *parent = nullptr);

    // 析构函数
    ~OpenCVProcessThread();

    // 设置处理模式
    // mode: 处理模式
    void setProcessMode(ProcessMode mode);

    // 获取当前处理模式
    ProcessMode getProcessMode() const { return m_currentMode; }

    // 添加待处理的图像
    // pixmap: 原始图像
    void addFrame(const QPixmap &pixmap);

    // 停止线程
    void stop();

signals:
    // 信号: 处理后的图像就绪
    // processed: 处理后的图像
    void frameProcessed(QPixmap processed);

    // 信号: 发生错误
    // error: 错误描述
    void errorOccurred(QString error);

protected:
    // 线程执行函数
    void run() override;

private:
    // QPixmap转cv::Mat
    cv::Mat pixmapToMat(const QPixmap &pixmap);

    // cv::Mat转QPixmap
    QPixmap matToPixmap(const cv::Mat &mat);

    // 各种处理函数
    cv::Mat processNone(const cv::Mat &input);           // 不处理
    cv::Mat processGaussianBlur(const cv::Mat &input);   // 高斯模糊
    cv::Mat processEdgeDetect(const cv::Mat &input);     // 边缘检测
    cv::Mat processGrayscale(const cv::Mat &input);      // 灰度化
    cv::Mat processSharpen(const cv::Mat &input);        // 锐化
    cv::Mat processEmboss(const cv::Mat &input);         // 浮雕
    cv::Mat processCartoon(const cv::Mat &input);        // 卡通化

private:
    bool m_running;                  // 运行标志
    ProcessMode m_currentMode;       // 当前处理模式

    QMutex m_mutex;                  // 互斥锁(保护共享数据)
    QPixmap m_currentFrame;          // 当前待处理帧
    bool m_hasNewFrame;              // 是否有新帧
};

#endif // OPENCVPROCESSTHREAD_H
