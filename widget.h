#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QString>
#include <QList>
#include <tuple>
#include <QMap>

namespace Ui
{
    class Widget;
}

class QStandardItemModel;
class QMediaPlayer;
class QMediaPlaylist;
class QTableView;
class QSystemTrayIcon;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

private:    
    const QString mLastPlaylistFilePath{ "currentPlaylist.dat" };
    Ui::Widget *ui;
    QMediaPlayer *mPlayer;
    QList<std::tuple<QStandardItemModel*, QMediaPlaylist*, QTableView*, int>> mLista;
    enum { MODEL, PLAYLIST, VIEW, PLAYBACK_MODE };
    int mCurrentlySelectedIndex{0};
    QSystemTrayIcon *mTrayIcon;
    bool mIsPlaylistSwitched { true };
    const QString mWindowSettings { "WindowSettings" };
    const QString mPlayerSettings { "PlayerSettings" };
    enum class SettingsKey
    {
        WINDOW_SIZE,
        WINDOW_POS,
        VOLUME_LEVEL,
        PLAYER_POS,
        CURRENT_TAB,
        CURRENT_TRACK_IN_PLAYLIST
    };

    QMap<SettingsKey, QString> mSKeys
    {
        { SettingsKey::WINDOW_SIZE, "WindowSize" },
        { SettingsKey::WINDOW_POS, "WindowPos" },
        { SettingsKey::VOLUME_LEVEL, "VolumeLevel" },
        { SettingsKey::PLAYER_POS, "SeekSliderValue" },
        { SettingsKey::CURRENT_TAB, "CurrentTabIndex" },
        { SettingsKey::CURRENT_TRACK_IN_PLAYLIST, "CurrentTrack"}
    };


    QString getTextAt(QStandardItemModel *model, int row, int col);
    void setValueAt(QStandardItemModel *model, int row, int col, const QVariant &value);
    bool loadPlaylistFromFile(const QString &filePath);
    bool loadPlaylistFromFile(int tab_index, const QString &filePath);
    bool savePlaylistToFile(int index, const QString &filePath);
    bool savePlaylistToFile(const QString &filePath);
    void loadAllPlaylistsFromLastSession();
    void saveAllPlaylistsFromCurrentSession();
    void configurePlaylistView(int tab_index);
    void configurePlaylistView();
    void save_current_session();
    void load_previous_session();

private slots:

    void addNewPlaylist();
    int create_new_playlist(const QString &title);
    void togglePause();
    void on_btnAdd_clicked();
    void loadChoosenPlaylist();
    void savePlaylistAs();
    void clearPlaylist();
    void removeSelectedTunes();
    void updateVolumeLabel(int volume);
    void updateTimeLabels(qint64 pos);
    void updateSeekSliderValue(qint64 pos);
    void updatePlayerPos(int pos);
    void next_and_prev_track_slot();
    void addListOfMusic(QStringList &tunes);

    //Actions

    void on_action_remove_selected_tunes_from_playlist_triggered();
    void on_actionClear_playlist_triggered();
    void on_action_remove_selected_files_from_HDD_triggered();
    void on_tabWidget_tabCloseRequested(int index);
    void on_comboBoxPlaybackMode_currentIndexChanged(int index);
    void on_action_add_music_triggered();
    void on_action_rename_playlist_triggered();
    void on_btnQuit_clicked();
    void on_action_add_music_from_other_playlist_triggered();
    void on_action_open_music_triggered();
    void on_action_prevous_track_triggered();
    void on_action_play_triggered();
    void on_action_pause_triggered();
    void on_action_stop_triggered();
    void on_action_next_track_triggered();
    void on_action_quit_triggered();
};

#endif // WIDGET_H
