#pragma once

/**
 * @file searchpage.h
 * @brief 搜索页面 — 显示音乐、歌单和歌手搜索结果
 */

#include <QWidget>
#include <QList>

#include "core/musicinfo.h"

class QScrollArea;
class QVBoxLayout;
class QLabel;
class QPushButton;
class ApiClient;

class SearchPage : public QWidget
{
    Q_OBJECT

public:
    explicit SearchPage(ApiClient *apiClient, QWidget *parent = nullptr);

signals:
    void playMusic(const MusicInfo &info);
    void playAllRequested(const QList<MusicInfo> &results);
    void openPlaylist(int playlistId);
    void backRequested();

public slots:
    void search(const QString &query);
    void retranslate();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    void fetchMusicResults();
    void fetchPlaylistResults();
    void fetchArtistResults();
    void buildMusicList();
    void buildPlaylistList();
    void buildArtistList();
    void showMusicPage();
    void showPlaylistPage();
    void showArtistPage();
    void showLoading();
    void hideLoading();

    ApiClient *m_apiClient = nullptr;
    QPushButton *m_musicTab = nullptr;
    QPushButton *m_playlistTab = nullptr;
    QPushButton *m_artistTab = nullptr;
    QPushButton *m_playAllBtn = nullptr;
    QLabel *m_resultCountLbl = nullptr;
    QLabel *m_statusLabel = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QVBoxLayout *m_listLayout = nullptr;
    QWidget *m_musicHeader = nullptr;

    QString m_query;
    QList<MusicInfo> m_musicResults;
    QList<QVariantMap> m_playlistResults;
    QList<QVariantMap> m_artistResults;
    int m_activeTab = 0; // 0=music, 1=playlist, 2=artist
    int m_page = 1;
    static const int kPageSize = 20;
};
