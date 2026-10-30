#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "yuvglwidget.h"
#include "VideoWorker.h"
#include "VideoFrame.h"

extern "C" {
#include <libswscale/swscale.h>     // YUV→RGB 转换（线程池里调用）
}

#include <QtConcurrent>             // QtConcurrent::run
#include <QFutureWatcher>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    worker_ = new VideoWorker(this);
    ui->glWidget->attach(worker_->hub());//把帧信箱地址从VideoWorker传递给YuvGLWidget

    connect(worker_, &VideoWorker::stats, this, [this](double fps, double kbps) {
        ui->statLabel->setText(QString("FPS: %1    码率: %2 kbps    显示端累计丢帧: %3")
                               .arg(fps, 0, 'f', 1)
                               .arg(kbps, 0, 'f', 0)
                               .arg(ui->glWidget->dropped_));
    });
    connect(worker_, &VideoWorker::errorOccurred, this, [this](const QString &msg) {
        ui->statLabel->setText(msg);
    });

    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(ui->btnStop,  &QPushButton::clicked, this, &MainWindow::onStop);
    connect(ui->btnShot,  &QPushButton::clicked, this, &MainWindow::onSnapshot);

    // 切换协议自动同步地址（与推流端行为一致）
    connect(ui->protocolCombo, &QComboBox::currentTextChanged, this,
            [this](const QString &text) {
        QString url = ui->urlEdit->text().trimmed();
        if (text == "RTMP" && url.startsWith("rtsp://", Qt::CaseInsensitive)) {
            url.replace(0, 7, "rtmp://");
            url.replace("127.0.0.1:8554", "127.0.0.1:1935");
            ui->urlEdit->setText(url);
        } else if (text == "RTSP" && url.startsWith("rtmp://", Qt::CaseInsensitive)) {
            url.replace(0, 7, "rtsp://");
            url.replace("127.0.0.1:1935", "127.0.0.1:8554");
            ui->urlEdit->setText(url);
        }
    });
}

MainWindow::~MainWindow()
{
    if (worker_) worker_->stopStream();
    delete ui;
}

void MainWindow::onStart()
{
    const QString url = ui->urlEdit->text().trimmed();
    if (url.isEmpty()) return;

    const QString proto = ui->protocolCombo->currentText().trimmed().toLower();
    if (!url.startsWith(proto + "://", Qt::CaseInsensitive)) {
       ui->statLabel->setText(QString("地址与协议不符：当前协议 %1，地址应以 \"%2://\" 开头")
                              .arg(proto.toUpper(), proto));
       return;
    }

    ui->statLabel->setText("连接中…");
    worker_->startStream(url);
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);
    ui->btnShot->setEnabled(true);
}

void MainWindow::onStop()
{
    worker_->stopStream();
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);
    ui->btnShot->setEnabled(false);
    ui->statLabel->setText("已断开");
}

// ============================================================
// 截图：GUI 线程只取帧指针（零拷贝），转换+编码+写盘全在线程池
// ============================================================
void MainWindow::onSnapshot()
{
    if (saving_) return;

    // ① 从信箱取最新解码帧（shared_ptr 拷贝仅指针，像素不复制）
    auto f = worker_->hub()->grab(snapshotSeq_);
    if (!f || f->w == 0) {
        ui->statLabel->setText("当前没有可截图的画面");
        return;
    }

    // ② 立即生成保存路径
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString file = QDir(dir).filePath(
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz") + ".png");

    saving_ = true;
    ui->btnShot->setEnabled(false);
    ui->statLabel->setText("截图保存中…");

    // ③ 全部耗时工作丢进线程池：swscale 转换 + PNG 编码 + 写盘
    QFuture<bool> fut = QtConcurrent::run([f, file]() -> bool {
        // 源：YUV420P 三平面（带各自 stride，可能是解码器对齐后的行距）
        SwsContext *sws = sws_getContext(
                    f->w, f->h, AV_PIX_FMT_YUV420P,
                    f->w, f->h, AV_PIX_FMT_RGB24,
                    SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws) return false;

        QImage img(f->w, f->h, QImage::Format_RGB888);
        const uint8_t *src[4] = {
            reinterpret_cast<const uint8_t*>(f->y.constData()),
            reinterpret_cast<const uint8_t*>(f->u.constData()),
            reinterpret_cast<const uint8_t*>(f->v.constData()),
            nullptr };
        const int srcStride[4] = { f->yStride, f->uStride, f->vStride, 0 };
        uint8_t *dst[4]  = { img.bits(), nullptr, nullptr, nullptr };
        int     dstStride[4] = { int(img.bytesPerLine()), 0, 0, 0 };

        sws_scale(sws, src, srcStride, 0, f->h, dst, dstStride);
        sws_freeContext(sws);

        return img.save(file, "PNG");
    });

    // ④ 完成后回 GUI 线程更新状态、恢复按钮
    auto *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, file]() {
        saving_ = false;
        ui->btnShot->setEnabled(true);
        ui->statLabel->setText(watcher->result() ? ("已保存: " + file)
                                                 : ("保存失败: " + file));
        watcher->deleteLater();
    });
    watcher->setFuture(fut);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (worker_) worker_->stopStream();
    event->accept();
}
