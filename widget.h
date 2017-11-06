#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QString>

namespace Ui
{
    class Widget;
}

class QStandardItemModel;
class QMediaPlayer;
class QMediaPlaylist;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void togglePause();
    void on_btnAdd_clicked();
    void on_comboBoxPlaybackMode_currentIndexChanged(int index);
    void loadChoosenPlaylist();
    void savePlaylistAs();
    void clearPlaylist();
    void removeSelectedTunes();
    void updateVolumeLabel(int volume);
    void updateTimeLabels(qint64 pos);
    void updateSeekSliderValue(qint64 pos);
    void updatePlayerPos(int pos);

    //Actions

    void on_action_remove_selected_tunes_from_playlist_triggered();
    void on_actionClear_playlist_triggered();
    void on_action_remove_selected_files_from_HDD_triggered();

private:
    const QString mLastPlaylistFilePath{"currentPlaylist.dat"};
    Ui::Widget *ui;
    QMediaPlayer *mPlayer;
    QStandardItemModel *mPlaylistModel;
    QMediaPlaylist *mPlaylist;
    int mCurrentIndex{0};
    QString getTextAt(QStandardItemModel *model, int row, int col);
    void setValueAt(QStandardItemModel *model, int row, int col, const QVariant &value);
    bool loadPlaylistFromFile(const QString &filePath);
    bool savePlaylistToFile(const QString &filePath);
    void configurePlaylistView();

};

#endif // WIDGET_H
