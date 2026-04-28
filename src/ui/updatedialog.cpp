/**
 * @file updatedialog.cpp
 * @brief 版本更新通知对话框实现
 */

#include "updatedialog.h"
#include "core/i18n.h"
#include "theme/theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QPainter>
#include <QPainterPath>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>

UpdateDialog::UpdateDialog(const QString &currentVersion, const QString &newVersion,
                           const QString &downloadUrl, QWidget *parent)
    : QDialog(parent), m_currentVersion(currentVersion), m_newVersion(newVersion),
      m_downloadUrl(downloadUrl)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setFixedSize(400, 280);

    setupUi();
}

void UpdateDialog::setupUi()
{
    auto *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);

    // 背景容器
    auto *container = new QWidget(this);
    container->setObjectName("updateDialogBg");
    container->setStyleSheet(
        "QWidget#updateDialogBg { "
        "  background: rgba(30, 30, 50, 0.98); "
        "  border: 1px solid rgba(255, 255, 255, 0.1); "
        "  border-radius: 16px; "
        "}"
    );

    auto *containerLay = new QVBoxLayout(container);
    containerLay->setContentsMargins(32, 32, 32, 24);
    containerLay->setSpacing(20);

    // 标题
    m_titleLbl = new QLabel(I18n::instance().tr("newVersionAvailable"), container);
    m_titleLbl->setObjectName("updateTitle");
    m_titleLbl->setStyleSheet(
        "QLabel#updateTitle { "
        "  font-size: 20px; font-weight: 700; color: white; "
        "}"
    );
    m_titleLbl->setAlignment(Qt::AlignCenter);
    containerLay->addWidget(m_titleLbl);

    // 版本信息
    auto *infoWidget = new QWidget(container);
    auto *infoLay = new QVBoxLayout(infoWidget);
    infoLay->setContentsMargins(16, 0, 16, 0);
    infoLay->setSpacing(12);

    m_currentVerLbl = new QLabel(
        QString("%1: %2").arg(I18n::instance().tr("currentVersion"), m_currentVersion),
        infoWidget);
    m_currentVerLbl->setObjectName("currentVer");
    m_currentVerLbl->setStyleSheet(
        "QLabel#currentVer { font-size: 14px; color: rgba(255, 255, 255, 0.6); }"
    );
    infoLay->addWidget(m_currentVerLbl);

    m_newVerLbl = new QLabel(
        QString("%1: %2").arg(I18n::instance().tr("newVersion"), m_newVersion),
        infoWidget);
    m_newVerLbl->setObjectName("newVer");
    m_newVerLbl->setStyleSheet(
        "QLabel#newVer { font-size: 16px; font-weight: 600; color: " + QString(Theme::kLavender) + "; }"
    );
    infoLay->addWidget(m_newVerLbl);

    containerLay->addWidget(infoWidget);

    // 进度条（初始隐藏）
    m_progressBar = new QProgressBar(container);
    m_progressBar->setObjectName("updateProgress");
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar#updateProgress { "
        "  background: rgba(255, 255, 255, 0.1); "
        "  border-radius: 4px; "
        "}"
        "QProgressBar::chunk#updateProgress { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #667eea, stop:1 #764ba2); "
        "  border-radius: 4px; "
        "}"
    );
    m_progressBar->hide();
    containerLay->addWidget(m_progressBar);

    // 状态标签（初始隐藏）
    m_statusLbl = new QLabel(container);
    m_statusLbl->setObjectName("updateStatus");
    m_statusLbl->setAlignment(Qt::AlignCenter);
    m_statusLbl->setStyleSheet(
        "QLabel#updateStatus { font-size: 13px; color: rgba(255, 255, 255, 0.6); }"
    );
    m_statusLbl->hide();
    containerLay->addWidget(m_statusLbl);

    containerLay->addStretch();

    // 按钮
    auto *btnLay = new QHBoxLayout();
    btnLay->setSpacing(12);

    m_remindLaterBtn = new QPushButton(I18n::instance().tr("remindLater"), container);
    m_remindLaterBtn->setFixedHeight(40);
    m_remindLaterBtn->setCursor(Qt::PointingHandCursor);
    m_remindLaterBtn->setStyleSheet(
        "QPushButton { "
        "  background: rgba(255, 255, 255, 0.1); "
        "  color: rgba(255, 255, 255, 0.9); "
        "  border: 1px solid rgba(255, 255, 255, 0.2); "
        "  border-radius: 8px; "
        "  font-size: 14px; "
        "}"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.15); }"
    );
    connect(m_remindLaterBtn, &QPushButton::clicked, this, [this]() {
        emit remindLater();
        accept();
    });
    btnLay->addWidget(m_remindLaterBtn, 1);

    m_updateBtn = new QPushButton(I18n::instance().tr("updateNow"), container);
    m_updateBtn->setFixedHeight(40);
    m_updateBtn->setCursor(Qt::PointingHandCursor);
    m_updateBtn->setStyleSheet(
        "QPushButton { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #667eea, stop:1 #764ba2); "
        "  color: white; "
        "  border: none; "
        "  border-radius: 8px; "
        "  font-size: 14px; font-weight: 600; "
        "}"
        "QPushButton:hover { opacity: 0.9; }"
    );
    connect(m_updateBtn, &QPushButton::clicked, this, [this]() {
        emit downloadRequested(m_downloadUrl);
        // 切换到下载状态
        setupDownloadingUi();
    });
    btnLay->addWidget(m_updateBtn, 1);

    containerLay->addLayout(btnLay);

    mainLay->addWidget(container);
}

void UpdateDialog::setupDownloadingUi()
{
    m_updateBtn->setEnabled(false);
    m_updateBtn->setText(I18n::instance().tr("downloading"));
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_statusLbl->show();
    m_statusLbl->setText("0%");
}

void UpdateDialog::setupFinishedUi(const QString &filePath)
{
    m_progressBar->setValue(100);
    m_statusLbl->setText(I18n::instance().tr("downloadComplete"));
    m_updateBtn->setEnabled(true);
    m_updateBtn->setText(I18n::instance().tr("installNow"));
    m_updateBtn->disconnect();
    connect(m_updateBtn, &QPushButton::clicked, this, [this, filePath]() {
#if defined(Q_OS_WIN)
        QProcess::startDetached(filePath);
#elif defined(Q_OS_MACOS)
        QProcess::startDetached("open", {filePath});
#elif defined(Q_OS_LINUX)
        // Linux: 尝试 chmod +x 然后运行
        QProcess::execute("chmod", {"+x", filePath});
        QProcess::startDetached(filePath);
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
#endif
        accept();
    });
    m_remindLaterBtn->setText(I18n::instance().tr("installLater"));
}

void UpdateDialog::setupFailedUi(const QString &error)
{
    m_progressBar->hide();
    m_statusLbl->show();
    m_statusLbl->setText(QString("%1: %2").arg(I18n::instance().tr("downloadFailed"), error));
    m_statusLbl->setStyleSheet(
        "QLabel#updateStatus { font-size: 13px; color: #ef4444; }"
    );
    m_updateBtn->setText(I18n::instance().tr("retry"));
    m_updateBtn->setEnabled(true);
    m_updateBtn->disconnect();
    connect(m_updateBtn, &QPushButton::clicked, this, [this]() {
        emit downloadRequested(m_downloadUrl);
        setupDownloadingUi();
        m_statusLbl->setStyleSheet(
            "QLabel#updateStatus { font-size: 13px; color: rgba(255, 255, 255, 0.6); }"
        );
    });
}

void UpdateDialog::showDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percent = static_cast<int>(bytesReceived * 100 / bytesTotal);
        m_progressBar->setValue(percent);

        // 格式化进度文本
        QString receivedStr;
        if (bytesReceived < 1024 * 1024) {
            receivedStr = QString("%1 KB").arg(bytesReceived / 1024);
        } else {
            receivedStr = QString("%1 MB").arg(bytesReceived / (1024.0 * 1024.0), 0, 'f', 1);
        }
        QString totalStr;
        if (bytesTotal < 1024 * 1024) {
            totalStr = QString("%1 KB").arg(bytesTotal / 1024);
        } else {
            totalStr = QString("%1 MB").arg(bytesTotal / (1024.0 * 1024.0), 0, 'f', 1);
        }
        m_statusLbl->setText(QString("%1% - %2 / %3").arg(percent).arg(receivedStr).arg(totalStr));
    }
}

void UpdateDialog::showDownloadFinished(const QString &filePath)
{
    setupFinishedUi(filePath);
}

void UpdateDialog::showDownloadFailed(const QString &error)
{
    setupFailedUi(error);
}

void UpdateDialog::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    // 透明背景，由子控件处理
}
