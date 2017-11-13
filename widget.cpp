#include "widget.h"
#include "ui_widget.h"
#include <QStandardItemModel>
#include <QMediaPlaylist>
#include <QStandardPaths>
#include <QMediaPlayer>
#include <QStringList>
#include <QList>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMessageBox>
#include <string>
#include <sstream>
#include <QTableView>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QTabWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDebug>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("TeAudioPlayer");
    mPlayer = new QMediaPlayer(this);
    mPlayer->setVolume(50);

    ///System tray icon/////
    mTrayIcon = new QSystemTrayIcon(QIcon(":/gfx/icons/trayIcon.png"), this);
    mTrayIcon->setVisible(true);
    QMenu *menu = new QMenu(this);
    menu->addAction(ui->action_prevous_track);
    menu->addAction(ui->action_play);
    menu->addAction(ui->action_pause);
    menu->addAction(ui->action_stop);
    menu->addAction(ui->action_next_track);
    menu->addAction(ui->action_quit);
    mTrayIcon->setContextMenu(menu);

    QStringList playModes;
    playModes << "Repeat playlist" << "Repeat selected" <<
                 "Play selected item once" << "Sequential" <<
                 "Random";
    ui->comboBoxPlaybackMode->addItems(playModes);
    ui->comboBoxPlaybackMode->setCurrentIndex(0);
    ui->seekSlider->setMinimum(0);
    ui->seekSlider->setMaximum(100);

    ui->volumeSlider->setMinimum(0);
    ui->volumeSlider->setMaximum(100);
    ui->volumeSlider->setValue(50);

    connect(mPlayer, &QMediaPlayer::durationChanged, ui->progressBar, &QProgressBar::setMaximum);
    connect(mPlayer, &QMediaPlayer::positionChanged, ui->progressBar, &QProgressBar::setValue);
    connect(ui->seekSlider, &QSlider::sliderMoved, this, &Widget::updatePlayerPos);
    //connect(ui->seekSlider, &QSlider::valueChanged, this, &Widget::updatePlayerPos);
    connect(mPlayer, &QMediaPlayer::positionChanged, this, &Widget::updateSeekSliderValue);
    connect(mPlayer, &QMediaPlayer::positionChanged, this, &Widget::updateTimeLabels);
    connect(ui->volumeSlider, &QSlider::sliderMoved, mPlayer, &QMediaPlayer::setVolume);
    connect(ui->volumeSlider, &QSlider::valueChanged, mPlayer, &QMediaPlayer::setVolume);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &Widget::updateVolumeLabel);

    //Button connections

    connect(ui->btnNewPlayList, &QToolButton::clicked, this, &Widget::addNewPlaylist);
    connect(ui->btnPlay, &QToolButton::clicked, mPlayer, &QMediaPlayer::play);
    connect(ui->btnPause, &QToolButton::clicked, this, &Widget::togglePause);
    connect(ui->btnStop, &QToolButton::clicked, mPlayer, &QMediaPlayer::stop);

    connect(ui->btnPrev, &QToolButton::clicked, this, &Widget::next_and_prev_track_slot);

    connect(ui->btnNext, &QToolButton::clicked, this, &Widget::next_and_prev_track_slot);

    connect(ui->btnSeekBackwards, &QToolButton::clicked, [this](){
        mPlayer->setPosition(mPlayer->position() - 5000);

    });
    connect(ui->btnSeekForwards, &QToolButton::clicked, [this](){
        mPlayer->setPosition(mPlayer->position() + 5000);
    });

    connect(ui->btnLoadPlaylist, &QPushButton::clicked, this,
            &Widget::loadChoosenPlaylist);
    connect(ui->btnSavePlaylist, &QPushButton::clicked, this,
            &Widget::savePlaylistAs);
    connect(ui->btnClearPlaylist, &QPushButton::clicked, this,
            &Widget::clearPlaylist);
    connect(ui->btnRemoveSelected, &QPushButton::clicked, this,
            &Widget::removeSelectedTunes);

    connect(ui->tabWidget, &QTabWidget::currentChanged, [this](int index){
        if (index == -1) return;
        ui->comboBoxPlaybackMode->setCurrentIndex(std::get<PLAYBACK_MODE>(mLista.at(index)));
    });

    loadAllPlaylistsFromLastSession();
}

Widget::~Widget()
{
    saveAllPlaylistsFromCurrentSession();
    delete mPlayer;
    delete ui;
}

void Widget::keyReleaseEvent(QKeyEvent *event)
{
    auto index = ui->tabWidget->currentIndex();
    if(index == -1) return;
    auto model = std::get<MODEL>(mLista.at(index));
    auto pls = std::get<PLAYLIST>(mLista.at(index));
    auto tv = std::get<VIEW>(mLista.at(index));
    auto key = event->key();
    if(key == Qt::Key_Return)
    {
        auto selectedRows = tv->selectionModel()->selectedRows();
        if(selectedRows.size() == 1)
        {
            pls->setCurrentIndex(selectedRows.at(0).row());
            if(mPlayer->state() != QMediaPlayer::PlayingState)
            {
                mPlayer->play();
            }
        }
    }
    else if(key == Qt::Key_Delete)
    {
        removeSelectedTunes();
    }
    else if(key == Qt::Key_A && event->modifiers() & Qt::Key_Control)
    {
        tv->selectAll();
    }
    else if(key == Qt::Key_Escape)
    {
        tv->clearSelection();
    }
    else if(key == Qt::Key_Space)
    {
        this->togglePause();
    }
    else if(key == Qt::Key_Up)
    {
        if(model->rowCount() > 0)
        {
            --mCurrentlySelectedIndex;
            if(mCurrentlySelectedIndex < 0)
                mCurrentlySelectedIndex = model->rowCount() - 1;
            tv->clearSelection();
            tv->selectRow(mCurrentlySelectedIndex);
        }
    }
    else if(key == Qt::Key_Down)
    {
        if(model->rowCount() > 0)
        {
            ++mCurrentlySelectedIndex;
            if(mCurrentlySelectedIndex > model->rowCount() - 1)
                mCurrentlySelectedIndex = 0;
            tv->clearSelection();
            tv->selectRow(mCurrentlySelectedIndex);
        }
    }
    else if(key == Qt::Key_Left)
    {
        mPlayer->setPosition(mPlayer->position() - 5000);
    }
    else if(key == Qt::Key_Right)
    {
        mPlayer->setPosition(mPlayer->position() + 5000);
    }
    else
    {
        QWidget::keyReleaseEvent(event);
    }
}

void Widget::wheelEvent(QWheelEvent *event)
{
    if(event->angleDelta().x() > 0)
        ui->volumeSlider->setValue(ui->volumeSlider->value() + 1);
    else if(event->angleDelta().x() < 0)
        ui->volumeSlider->setValue(ui->volumeSlider->value() - 1);
    else
        QWidget::wheelEvent(event);
}

void Widget::togglePause()
{
    if(mPlayer->state() == QMediaPlayer::PlayingState)
    {
        mPlayer->pause();
    }
    else if(mPlayer->state() == QMediaPlayer::PausedState)
    {
        mPlayer->play();
    }
}

void Widget::on_btnAdd_clicked()
{
    auto dir = QStandardPaths::standardLocations(QStandardPaths::MusicLocation)
            .value(0, QDir::homePath());

    QStringList tunes = QFileDialog::getOpenFileNames(this, "Open a tunes", dir,
                                                  "Audio files (*.mp3 *.ogg *\
                                                  *.wav);;All files (*.*)");
    auto tab_index = ui->tabWidget->currentIndex();
    if(tab_index == -1) return;
    auto pls_model = std::get<MODEL>(mLista.at(tab_index));
    auto pls = std::get<PLAYLIST>(mLista.at(tab_index));
    for(const auto &tune: tunes){
        QList<QStandardItem*> items;
        items.append(new QStandardItem(QDir(tune).dirName()));
        items.append(new QStandardItem(tune));
        pls_model->appendRow(items);
        pls->addMedia(QUrl::fromLocalFile(tune));
    }
    configurePlaylistView(tab_index);
}

void Widget::loadChoosenPlaylist()
{
    QString startLocation = QStandardPaths::standardLocations(
                QStandardPaths::MusicLocation).value(0, QDir::homePath());
    QString filter = QString::fromStdString("Playlists (*.tpls);;All files (*.*)");
    QString fileName = QFileDialog::getOpenFileName(
                this,
                QString::fromStdString("Open playlist"),
                startLocation,
                filter);
    loadPlaylistFromFile(fileName);
}

void Widget::savePlaylistAs()
{
    QString startLocation = QStandardPaths::standardLocations(
                QStandardPaths::MusicLocation).value(0, QDir::homePath());
    QString fileName = QFileDialog::getSaveFileName(
                this,
                QString::fromStdString("Save playlist to file..."),
                startLocation,
                QString::fromStdString("Playlists (*.tpls);;All files (*.*)"));
    if(!fileName.contains(".tpls"))
        fileName += QString(".tpls");
    savePlaylistToFile(fileName);
}

void Widget::clearPlaylist()
{
    auto tab_index = ui->tabWidget->currentIndex();
    if(tab_index == -1) return;
    auto pls_model = std::get<MODEL>(mLista.at(tab_index));
    auto pls = std::get<PLAYLIST>(mLista.at(tab_index));
    pls->clear();
    pls_model->clear();
}

void Widget::removeSelectedTunes()
{
    auto tab_index = ui->tabWidget->currentIndex();
    if(tab_index == -1) return;
    auto pls_model = std::get<MODEL>(mLista.at(tab_index));
    auto pls_view = std::get<VIEW>(mLista.at(tab_index));
    auto tunesToDelete = pls_view->selectionModel()->selectedRows();
    while(!tunesToDelete.isEmpty())
    {
        pls_model->removeRows(tunesToDelete.last().row(), 1);
        tunesToDelete.removeLast();
    }
}

bool Widget::loadPlaylistFromFile(const QString &filePath)
{
    auto current_tab_index = ui->tabWidget->currentIndex();
    if(current_tab_index == -1) return false;
    return loadPlaylistFromFile(current_tab_index, filePath);
}

bool Widget::loadPlaylistFromFile(int tab_index, const QString &filePath)
{
    auto pls_model = std::get<MODEL>(mLista.at(tab_index));
    auto pls = std::get<PLAYLIST>(mLista.at(tab_index));
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", file.errorString());
        return false;
    }
    QTextStream stream(&file);

    while (!stream.atEnd()) {
        QString tuneData = stream.readLine();
        QList campos = tuneData.split("*");
        QList<QStandardItem*> list;
        for(const auto &campo: campos)
        {
            list.append(new QStandardItem(campo));
        }
        pls_model->appendRow(list);
        pls->addMedia(QUrl::fromLocalFile(campos.at(1)));

    }
    file.close();
    configurePlaylistView(tab_index);
    return true;
}

bool Widget::savePlaylistToFile(int index, const QString &filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", file.errorString());
        return false;
    }
    QTextStream stream(&file);
    auto pls_model = std::get<MODEL>(mLista.at(index));
    for(int row{1}; row < pls_model->rowCount(); ++row)
    {
        stream << pls_model->item(row, 0)->text() << "*" <<
                pls_model->item(row, 1)->text() << endl;
    }
    file.close();
    return true;
}

bool Widget::savePlaylistToFile(const QString &filePath)
{
    auto index = ui->tabWidget->currentIndex();
    if(index == -1) return false;
    return savePlaylistToFile(index, filePath);
}

void Widget::loadAllPlaylistsFromLastSession()
{
    QFile fileWithPlaylistTitles("playlists.txt");
    if(fileWithPlaylistTitles.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&fileWithPlaylistTitles);
        QStringList playlist_files = stream.readAll().split("*");
        for(const auto &path: playlist_files)
        {
            QFileInfo fileInfo(path);
            QString name = fileInfo.fileName();
            if(fileInfo.exists())
            {
                auto index = this->create_new_playlist(name);
                this->loadPlaylistFromFile(index, path);
            }
        }
    }
}

void Widget::saveAllPlaylistsFromCurrentSession()
{
    QFile fileWithPlaylistTitles("playlists.txt");
    if(fileWithPlaylistTitles.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&fileWithPlaylistTitles);
        for(int i {0}; i < ui->tabWidget->count(); ++i)
        {
            auto path = ui->tabWidget->tabText(i);
            stream << path;
            if(i != ui->tabWidget->count() - 1) stream << "*";
            savePlaylistToFile(i, path);
        }
        fileWithPlaylistTitles.close();
    }
}

void Widget::configurePlaylistView(int tab_index)
{
    if(tab_index == -1) return;
    auto pls_model = std::get<MODEL>(mLista.at(tab_index));
    QTableView *pls_view = std::get<VIEW>(mLista.at(tab_index));
    QStringList headers;
    headers << tr("Tune") << tr("File path");
    pls_model->setHorizontalHeaderLabels(headers);
    pls_view->hideColumn(1); // Hide file path
    pls_view->verticalHeader()->setVisible(false); //Hide column numbers
    pls_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    pls_view->setSelectionMode(QAbstractItemView::MultiSelection);
    pls_view->resizeColumnsToContents();
    pls_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pls_view->setColumnWidth(1, pls_view->width());
    pls_view->verticalHeader()->setVisible(true);
}

void Widget::configurePlaylistView()
{
    auto tab_index = ui->tabWidget->currentIndex();
    configurePlaylistView(tab_index);
}

void Widget::addNewPlaylist()
{
    bool ok = false;
    auto title = QInputDialog::getText(this, QString("Enter playlist name"),
                                       QString("Playlist name"),
                                       QLineEdit::Normal, QString(), &ok);
    if(!ok) return;
    create_new_playlist(title);
}

int Widget::create_new_playlist(const QString &title)
{
    QWidget *widget = new QWidget(ui->tabWidget);
    QTableView *tv = new QTableView(widget);
    tv->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    tv->addAction(ui->action_add_music);
    tv->addAction(ui->action_add_music_from_other_playlist);
    tv->addAction(ui->action_rename_playlist);
    tv->addAction(ui->action_remove_selected_tunes_from_playlist);
    tv->addAction(ui->actionClear_playlist);
    tv->addAction(ui->action_remove_selected_files_from_HDD);
    QStandardItemModel *pm = new QStandardItemModel(this);
    tv->setModel(pm);
    QMediaPlaylist *pls = new QMediaPlaylist(mPlayer);
    pls->setPlaybackMode(QMediaPlaylist::Loop);
    mLista.append(std::make_tuple(pm, pls, tv, 0));
    QVBoxLayout *l = new QVBoxLayout(widget);
    l->addWidget(tv);
    widget->setLayout(l);
    ui->tabWidget->addTab(widget, title);

    connect(tv, &QTableView::doubleClicked,
            [this, pls, tv](const QModelIndex &index){
                auto tab_index = ui->tabWidget->currentIndex();
                if(tab_index == -1) return;
                mPlayer->setPlaylist(std::get<PLAYLIST>(mLista.at(tab_index)));
                mCurrentlySelectedIndex = index.row();
                pls->setCurrentIndex(index.row());
                tv->clearSelection();
                tv->selectRow(index.row());
                if(mPlayer->state() != QMediaPlayer::PlayingState)
                mPlayer->play();
    });

    connect(pls, &QMediaPlaylist::currentIndexChanged,
            [this, pm, tv](int row){
                auto tab_index = ui->tabWidget->currentIndex();
                if(tab_index == -1) return;
                auto title = pm->data(pm->index(row, 0)).toString();
                tv->clearSelection();
                tv->selectRow(row);
                ui->lblCurrentTrack->setText(title);
    });

    auto index = ui->tabWidget->indexOf(widget);
    ui->tabWidget->setCurrentIndex(index);

    return index;
}

void Widget::on_action_remove_selected_tunes_from_playlist_triggered()
{
    removeSelectedTunes();
}

void Widget::on_actionClear_playlist_triggered()
{
    clearPlaylist();
}

void Widget::updateVolumeLabel(int volume)
{
    ui->lblVolume->setText(QString("Volume:%1").arg(volume) + QString("%"));
}

void Widget::updateTimeLabels(qint64 pos)
{
    auto curr_pos = pos / 1000;
    std::stringstream ss_curr_pos;
    ss_curr_pos << "Current playtime:" << curr_pos / 3600 << "h:" <<
                   (curr_pos % 3600) / 60 << "m:" <<
                   (curr_pos % 3600) % 60 << "s";
    auto total_duration = mPlayer->duration() / 1000;
    std::stringstream ss_total_dur;
    ss_total_dur << total_duration / 3600 << "h:" << (total_duration % 3600) / 60 <<
                    "m:" << (total_duration % 3600) % 60 << "s";
    auto remaining_time = total_duration - curr_pos;
    std::stringstream ss_remains;
    ss_remains << remaining_time / 3600 << "h:" << (remaining_time % 3600) / 60 <<
                  "m:" << (remaining_time % 3600) % 60 << "s";
    ui->lblCurrentTime->setText(QString::fromStdString(ss_curr_pos.str()));
    ui->lblRemainingTime->setText(QString("Remains:%1/(Total:%2)")
                                    .arg(QString::fromStdString(ss_remains.str()))
                                  .arg(QString::fromStdString(ss_total_dur.str())));
}

void Widget::updateSeekSliderValue(qint64 pos)
{
    ui->seekSlider->setValue(
                static_cast<int>(static_cast<float>(pos) / mPlayer->duration() * 100));
}

void Widget::updatePlayerPos(int pos)
{
    auto p = mPlayer->duration() * pos / 100;
    mPlayer->setPosition(p);
}

void Widget::next_and_prev_track_slot()
{
    QToolButton* sender = static_cast<QToolButton*>(QObject::sender());
    auto tab_index = ui->tabWidget->currentIndex();
    if(tab_index == -1) return;
    auto pls = std::get<PLAYLIST>(mLista.at(tab_index));
    auto pls_view = std::get<VIEW>(mLista.at(tab_index));
    if(sender == ui->btnPrev)
        pls->previous();
    else if(sender == ui->btnNext)
        pls->next();
    auto index = pls->currentIndex();
    pls_view->clearSelection();
    pls_view->selectRow(index);
}

void Widget::on_action_remove_selected_files_from_HDD_triggered()
{
    auto index = ui->tabWidget->currentIndex();
    if(index == -1) return;
    QMessageBox::StandardButton reply =
            QMessageBox::question(this, "Confirm deleting",
                                  "Do you really want to delete\
                                  selected tracks from your HDD?",
                                  QMessageBox::Yes | QMessageBox::No);
    if(reply == QMessageBox::Yes)
    {
        auto pls_model = std::get<MODEL>(mLista.at(index));
        auto pls = std::get<PLAYLIST>(mLista.at(index));
        auto currTableView = std::get<VIEW>(mLista.at(index));
        auto tunesToDelete = currTableView->selectionModel()->selectedRows();
        while(!tunesToDelete.isEmpty())
        {
            auto row = tunesToDelete.last().row();
            pls->removeMedia(row);
            QString path = pls_model->item(row, 1)->text();
            pls_model->removeRows(row, 1);
            tunesToDelete.removeLast();
            QFile file(path);
            if(file.exists())
            {
                if(!file.remove())
                {
                    QMessageBox::critical(this, "Error", "Failed to remove");
                }
            }
        }
    }
}

void Widget::on_tabWidget_tabCloseRequested(int index)
{
    if(index == -1) return;
    QMessageBox::StandardButton reply =
            QMessageBox::question(this, "Save playlist?",
                                  "Would you like to save the playlist?",
                                  QMessageBox::Yes | QMessageBox::No);
    if(reply == QMessageBox::Yes)
        savePlaylistAs();
    QWidget *widget = ui->tabWidget->widget(index);
    ui->tabWidget->removeTab(index);
    auto pls_model = std::get<MODEL>(mLista.at(index));
    auto pls = std::get<PLAYLIST>(mLista.at(index));
    delete pls_model;
    delete pls;
    delete widget;
    mLista.removeAt(index);
}

void Widget::on_comboBoxPlaybackMode_currentIndexChanged(int index)
{
    auto tab_index = ui->tabWidget->currentIndex();
    if(tab_index == -1) return;
    auto pls = std::get<PLAYLIST>(mLista.at(tab_index));
    std::get<PLAYBACK_MODE>(mLista[tab_index]) = index;
    switch (index) {
    case 0:
        pls->setPlaybackMode(QMediaPlaylist::Loop);
        break;
    case 1:
        pls->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
        break;
    case 2:
        pls->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
        break;
    case 3:
        pls->setPlaybackMode(QMediaPlaylist::Sequential);
        break;
    case 4:
        pls->setPlaybackMode(QMediaPlaylist::Random);
        break;
    default:
        break;
    }
}

void Widget::on_action_add_music_triggered()
{
    on_btnAdd_clicked();
}

void Widget::on_action_rename_playlist_triggered()
{
    auto tab_index = ui->tabWidget->currentIndex();
    if(tab_index == -1) return;
    bool ok = false;
    auto prevTitle = ui->tabWidget->tabText(tab_index);
    auto title = QInputDialog::getText(this, QString("Enter new playlist name"),
                                       QString("Playlist name"),
                                       QLineEdit::Normal, prevTitle, &ok);
    if(!ok) return;
    ui->tabWidget->setTabText(tab_index, title);
}

void Widget::on_btnQuit_clicked()
{
    auto answer = QMessageBox::question(this, "Confirm exit, please",
                                        "Do you really want to quit?",
                                        QMessageBox::Yes | QMessageBox::No);
    if(answer == QMessageBox::Yes) this->close();
}

void Widget::on_action_add_music_from_other_playlist_triggered()
{
    ui->btnLoadPlaylist->click();
}

void Widget::on_action_open_music_triggered()
{
    ui->btnAdd->click();
}

void Widget::on_action_prevous_track_triggered()
{
    ui->btnPrev->click();
}

void Widget::on_action_play_triggered()
{
    ui->btnPlay->click();
}

void Widget::on_action_pause_triggered()
{
    ui->btnPause->click();
}

void Widget::on_action_stop_triggered()
{
    ui->btnStop->click();
}

void Widget::on_action_next_track_triggered()
{
    ui->btnNext->click();
}

void Widget::on_action_quit_triggered()
{
    ui->btnQuit->click();
}
