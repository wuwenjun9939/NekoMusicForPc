/**
 * @file updatechecker.cpp
 * @brief 版本更新检查器实现
 */

#include "updatechecker.h"
#include "theme/theme.h"
#include "version.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>

UpdateChecker::UpdateChecker(const QString &currentVersion, QObject *parent)
    : QObject(parent), m_currentVersion(currentVersion)
{
}

void UpdateChecker::checkForUpdates()
{
    QUrl url(QString::fromUtf8("%1/version.json").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    // 禁用缓存
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    req.setRawHeader("Cache-Control", "no-cache");

    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit checkFailed(reply->errorString());
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        auto root = doc.object();

        // 解析 PC 版本信息
        if (!root.contains("pc")) {
            emit checkFailed("Invalid response format");
            return;
        }

        auto pcObj = root.value("pc").toObject();
        QString remoteVersion = pcObj.value("pc_ver").toString();

        if (remoteVersion.isEmpty()) {
            emit checkFailed("No version info found");
            return;
        }

        // 比较版本号
        if (remoteVersion != m_currentVersion) {
            // 获取当前平台的下载链接
            QString platformKey = getPlatformKey();
            QString urlTemplate = pcObj.value(platformKey).toString();
            if (urlTemplate.isEmpty()) {
                // 回退到 windows
                urlTemplate = pcObj.value("windows").toString();
            }

            // 替换 {pc_ver} 占位符
            QString downloadUrl = urlTemplate.replace("{pc_ver}", remoteVersion);

            UpdateInfo info;
            info.hasUpdate = true;
            info.version = remoteVersion;
            info.downloadUrl = downloadUrl;
            emit updateAvailable(info);
        } else {
            emit noUpdate();
        }
    });
}

void UpdateChecker::downloadUpdate(const QString &url)
{
    QUrl fileUrl(url);
    QNetworkRequest req(fileUrl);
    auto *reply = m_nam.get(req);

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 bytesReceived, qint64 bytesTotal) {
        emit downloadProgress(bytesReceived, bytesTotal);
    });

    connect(reply, &QNetworkReply::finished, this, [this, url]() {
        auto *reply = qobject_cast<QNetworkReply*>(sender());
        if (!reply) return;
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit downloadFailed(reply->errorString());
            return;
        }

        // 保存到下载目录
        QString downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        QDir dir(downloadPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        // 从 URL 提取文件名
        QUrl fileUrl(url);
        QString fileName = fileUrl.fileName();
        if (fileName.isEmpty()) {
            fileName = "NekoMusic_update";
        }
        QString filePath = dir.filePath(fileName);

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit downloadFailed("Cannot open file for writing: " + filePath);
            return;
        }

        file.write(reply->readAll());
        file.close();

        emit downloadFinished(filePath);
    });
}

QString UpdateChecker::getOSType() const
{
#if defined(Q_OS_WIN)
    return "windows";
#elif defined(Q_OS_MACOS)
    return "mac";
#elif defined(Q_OS_LINUX)
    return "linux";
#else
    return "windows";
#endif
}

QString UpdateChecker::getPlatformKey() const
{
#if defined(Q_OS_WIN)
    return "windows";
#elif defined(Q_OS_MACOS)
    return "mac";
#elif defined(Q_OS_LINUX)
    return "linux";
#else
    return "windows";
#endif
}
