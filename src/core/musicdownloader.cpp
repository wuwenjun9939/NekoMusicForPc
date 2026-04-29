#include "musicdownloader.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDir>

MusicDownloader::MusicDownloader(QObject *parent) : QObject(parent) {}

MusicDownloader::~MusicDownloader()
{
    if (m_reply) {
        m_reply->disconnect();
        m_reply->abort();
    }
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }
}

void MusicDownloader::download(const QUrl &url)
{
    // Clean up previous download
    if (m_reply) {
        m_reply->disconnect();
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_file) {
        if (m_file->isOpen()) {
            m_file->close();
        }
        m_file->deleteLater();
        m_file = nullptr;
    }

    // Generate cache path from URL hash (no extension - FFmpeg detects format from content)
    QString hash = QCryptographicHash::hash(url.toEncoded(), QCryptographicHash::Md5).toHex();
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/nekomusic-cache";
    QDir().mkpath(cacheDir);
    m_tempPath = cacheDir + "/" + hash;

    // Check if already cached
    if (QFile::exists(m_tempPath)) {
        emit bufferReady(m_tempPath);
        emit downloadFinished(m_tempPath);
        return;
    }

    // Remove old partial file if exists
    QString partPath = m_tempPath + ".part";
    QFile::remove(partPath);

    m_file = new QFile(partPath, this);
    if (!m_file->open(QIODevice::WriteOnly)) {
        emit downloadError(QStringLiteral("无法创建临时文件"));
        return;
    }

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_nam.get(req);
    m_reply->setParent(this);

    connect(m_reply, &QNetworkReply::downloadProgress, this, &MusicDownloader::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &MusicDownloader::onReplyFinished);
    connect(m_reply, &QNetworkReply::readyRead, this, &MusicDownloader::onReadyRead);
}

void MusicDownloader::onReadyRead()
{
    if (m_file && m_reply) {
        m_file->write(m_reply->readAll());
    }
}

void MusicDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percent = static_cast<int>(bytesReceived * 100 / bytesTotal);
        emit downloadProgress(percent);
    } else {
        emit downloadProgress(-1); // unknown
    }
}

void MusicDownloader::onReplyFinished()
{
    if (!m_reply) return;

    QNetworkReply *reply = m_reply;
    m_reply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        emit downloadError(reply->errorString());
        reply->deleteLater();
        return;
    }

    // Write remaining data
    if (m_file && m_file->isOpen()) {
        m_file->write(reply->readAll());
        m_file->close();
    }

    // Rename partial file to final path if needed
    QString partPath = m_tempPath + ".part";
    if (QFile::exists(partPath)) {
        QFile::rename(partPath, m_tempPath);
    }

    emit downloadFinished(m_tempPath);

    reply->deleteLater();
    if (m_file) {
        m_file->deleteLater();
        m_file = nullptr;
    }
}
