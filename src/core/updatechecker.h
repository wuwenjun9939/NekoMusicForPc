#pragma once

/**
 * @file updatechecker.h
 * @brief 版本更新检查器
 *
 * 从 API 获取 version.json，对比当前版本，提供下载链接。
 */

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

struct UpdateInfo {
    bool hasUpdate = false;
    QString version;
    QString downloadUrl;
};

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(const QString &currentVersion, QObject *parent = nullptr);

    void checkForUpdates();
    void downloadUpdate(const QString &url);

    QString currentVersion() const { return m_currentVersion; }

signals:
    void updateAvailable(const UpdateInfo &info);
    void noUpdate();
    void checkFailed(const QString &error);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &filePath);
    void downloadFailed(const QString &error);

private:
    QString getOSType() const;
    QString getPlatformKey() const;

    QString m_currentVersion;
    QNetworkAccessManager m_nam;
};
