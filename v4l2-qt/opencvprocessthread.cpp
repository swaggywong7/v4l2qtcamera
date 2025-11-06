#include "opencvprocessthread.h"
#include <QDebug>               // 调试输出
#include <QMutexLocker>         // 自动加锁/解锁

// 构造函数
OpenCVProcessThread::OpenCVProcessThread(QObject *parent)
    : QThread(parent)           // 调用父类构造函数
    , m_running(false)          // 初始化运行标志
    , m_currentMode(None)       // 默认不处理
    , m_hasNewFrame(false)      // 没有新帧
{
    qDebug() << "OpenCV处理线程创建";
}

// 析构函数
OpenCVProcessThread::~OpenCVProcessThread()
{
    stop();                     // 确保线程已停止
    qDebug() << "OpenCV处理线程销毁";
}

// 设置处理模式
void OpenCVProcessThread::setProcessMode(ProcessMode mode)
{
    QMutexLocker locker(&m_mutex);  // 自动加锁
    m_currentMode = mode;            // 设置模式
    qDebug() << "切换处理模式:" << mode;
}

// 添加待处理的帧
void OpenCVProcessThread::addFrame(const QPixmap &pixmap)
{
    QMutexLocker locker(&m_mutex);  // 自动加锁
    m_currentFrame = pixmap;         // 保存图像
    m_hasNewFrame = true;            // 标记有新帧
}

// 停止线程
void OpenCVProcessThread::stop()
{
    qDebug() << "停止OpenCV处理线程...";
    m_running = false;               // 设置运行标志为false

    // 等待线程结束
    if(!wait(3000)) {                // 等待最多3秒
        qWarning() << "OpenCV线程未在3秒内结束,强制终止";
        terminate();                 // 强制终止
        wait();
    }

    qDebug() << "OpenCV处理线程已停止";
}

// 线程执行函数
void OpenCVProcessThread::run()
{
    qDebug() << "OpenCV处理线程开始运行";

    m_running = true;

    // 主循环
    while(m_running) {
        QPixmap frameToProcess;      // 待处理的帧
        ProcessMode currentMode;     // 当前模式
        bool hasFrame = false;       // 是否有帧

        // ========== 获取待处理帧(临界区) ==========
        {
            QMutexLocker locker(&m_mutex);  // 加锁
            if(m_hasNewFrame) {
                frameToProcess = m_currentFrame;  // 复制图像
                currentMode = m_currentMode;      // 复制模式
                hasFrame = true;
                m_hasNewFrame = false;            // 重置标志
            }
        }  // 离开作用域,自动解锁

        // ========== 如果有新帧,进行处理 ==========
        if(hasFrame) {
            // 转换 QPixmap → cv::Mat
            cv::Mat inputMat = pixmapToMat(frameToProcess);

            if(inputMat.empty()) {
                qWarning() << "转换为cv::Mat失败";
                continue;
            }

            cv::Mat outputMat;  // 输出图像

            // 根据模式选择处理方法
            try {
                switch(currentMode) {
                    case None:
                        outputMat = processNone(inputMat);
                        break;
                    case GaussianBlur:
                        outputMat = processGaussianBlur(inputMat);
                        break;
                    case EdgeDetect:
                        outputMat = processEdgeDetect(inputMat);
                        break;
                    case Grayscale:
                        outputMat = processGrayscale(inputMat);
                        break;
                    case Sharpen:
                        outputMat = processSharpen(inputMat);
                        break;
                    case Emboss:
                        outputMat = processEmboss(inputMat);
                        break;
                    case Cartoon:
                        outputMat = processCartoon(inputMat);
                        break;
                    default:
                        outputMat = inputMat.clone();
                }

                // 转换 cv::Mat → QPixmap
                QPixmap resultPixmap = matToPixmap(outputMat);

                // 发送处理后的图像
                emit frameProcessed(resultPixmap);

            } catch(cv::Exception &e) {
                qCritical() << "OpenCV处理异常:" << e.what();
                emit errorOccurred(QString("OpenCV错误: %1").arg(e.what()));
            }
        } else {
            // 没有新帧,休眠一下降低CPU占用
            msleep(10);  // 休眠10毫秒
        }
    }

    qDebug() << "OpenCV处理线程结束运行";
}

// ========== 格式转换函数 ==========

// QPixmap转cv::Mat
cv::Mat OpenCVProcessThread::pixmapToMat(const QPixmap &pixmap)
{
    // QPixmap → QImage
    QImage image = pixmap.toImage();

    // 转换为RGB888格式
    image = image.convertToFormat(QImage::Format_RGB888);

    // QImage → cv::Mat
    cv::Mat mat(image.height(), image.width(), CV_8UC3,
                const_cast<uchar*>(image.bits()),
                image.bytesPerLine());

    // 复制数据(因为QImage可能会被释放)
    cv::Mat result = mat.clone();

    // OpenCV使用BGR,QImage使用RGB,需要转换
    cv::cvtColor(result, result, cv::COLOR_RGB2BGR);

    return result;
}

// cv::Mat转QPixmap
QPixmap OpenCVProcessThread::matToPixmap(const cv::Mat &mat)
{
    cv::Mat temp;

    // 根据通道数处理
    if(mat.channels() == 1) {
        // 灰度图 → RGB
        cv::cvtColor(mat, temp, cv::COLOR_GRAY2RGB);
    } else if(mat.channels() == 3) {
        // BGR → RGB
        cv::cvtColor(mat, temp, cv::COLOR_BGR2RGB);
    } else if(mat.channels() == 4) {
        // BGRA → RGB
        cv::cvtColor(mat, temp, cv::COLOR_BGRA2RGB);
    } else {
        temp = mat.clone();
    }

    // cv::Mat → QImage
    QImage image(temp.data, temp.cols, temp.rows,
                temp.step, QImage::Format_RGB888);

    // QImage → QPixmap (需要复制数据)
    return QPixmap::fromImage(image.copy());
}

// ========== 各种图像处理函数 ==========

// 不处理
cv::Mat OpenCVProcessThread::processNone(const cv::Mat &input)
{
    return input.clone();  // 直接返回副本
}

// 高斯模糊
cv::Mat OpenCVProcessThread::processGaussianBlur(const cv::Mat &input)
{
    cv::Mat output;
    // 高斯模糊: 参数(输入, 输出, 核大小, sigma)
    cv::GaussianBlur(input, output, cv::Size(15, 15), 0);
    return output;
}

// 边缘检测
cv::Mat OpenCVProcessThread::processEdgeDetect(const cv::Mat &input)
{
    cv::Mat gray, edges, output;

    // 转换为灰度图
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

    // 高斯模糊降噪
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);

    // Canny边缘检测
    cv::Canny(gray, edges, 50, 150);

    // 转回BGR格式显示
    cv::cvtColor(edges, output, cv::COLOR_GRAY2BGR);

    return output;
}

// 灰度化
cv::Mat OpenCVProcessThread::processGrayscale(const cv::Mat &input)
{
    cv::Mat gray, output;

    // 转换为灰度
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

    // 转回BGR格式(虽然是灰度,但保持3通道)
    cv::cvtColor(gray, output, cv::COLOR_GRAY2BGR);

    return output;
}

// 锐化
cv::Mat OpenCVProcessThread::processSharpen(const cv::Mat &input)
{
    cv::Mat output;

    // 锐化卷积核
    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
        0, -1,  0,
       -1,  5, -1,
        0, -1,  0);

    // 应用卷积
    cv::filter2D(input, output, input.depth(), kernel);

    return output;
}

// 浮雕效果
cv::Mat OpenCVProcessThread::processEmboss(const cv::Mat &input)
{
    cv::Mat gray, output;

    // 转换为灰度
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

    // 浮雕卷积核
    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
       -2, -1,  0,
       -1,  1,  1,
        0,  1,  2);

    // 应用卷积
    cv::filter2D(gray, output, gray.depth(), kernel);

    // 增加128使其可见
    output = output + 128;

    // 转回BGR
    cv::Mat result;
    cv::cvtColor(output, result, cv::COLOR_GRAY2BGR);

    return result;
}

// 卡通化效果
cv::Mat OpenCVProcessThread::processCartoon(const cv::Mat &input)
{
    cv::Mat gray, edges, color, output;

    // 1. 边缘检测
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(gray, gray, 7);
    cv::adaptiveThreshold(gray, edges, 255,
                         cv::ADAPTIVE_THRESH_MEAN_C,
                         cv::THRESH_BINARY, 9, 2);

    // 2. 双边滤波(保留边缘的平滑)
    cv::bilateralFilter(input, color, 9, 300, 300);

    // 3. 合并边缘和颜色
    cv::cvtColor(edges, edges, cv::COLOR_GRAY2BGR);
    cv::bitwise_and(color, edges, output);

    return output;
}
