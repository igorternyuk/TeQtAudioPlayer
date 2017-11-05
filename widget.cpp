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
#include <QDebug>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("TeAudioPlayer");
    mPlaylistModel = new QStandardItemModel(this);
    ui->tableViewPlaylist->setModel(mPlaylistModel);

    configurePlaylistView();
    ui->tableViewPlaylist->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    ui->tableViewPlaylist->addAction(ui->action_remove_selected_tunes_from_playlist);
    ui->tableViewPlaylist->addAction(ui->actionClear_playlist);
    ui->tableViewPlaylist->addAction(ui->action_remove_selected_files_from_HDD);
    mPlayer = new QMediaPlayer(this);
    mPlaylist = new QMediaPlaylist(mPlayer);
    mPlayer->setPlaylist(mPlaylist);
    mPlayer->setVolume(50);

    QStringList playModes;
    playModes << "Repeat playlist" << "Repeat selected" <<
                 "Play selected item once" << "Sequential" <<
                 "Random";
    ui->comboBoxPlaybackMode->addItems(playModes);
    ui->comboBoxPlaybackMode->setCurrentIndex(0);
    mPlaylist->setPlaybackMode(QMediaPlaylist::Loop);

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
    connect(ui->btnPlay, &QToolButton::clicked, mPlayer, &QMediaPlayer::play);
    connect(ui->btnPause, &QToolButton::clicked, mPlayer, &QMediaPlayer::pause);
    connect(ui->btnStop, &QToolButton::clicked, mPlayer, &QMediaPlayer::stop);
    connect(ui->btnPrev, &QToolButton::clicked, [this](){
        mPlaylist->previous();
        auto index = mPlaylist->currentIndex();
        ui->tableViewPlaylist->clearSelection();
        ui->tableViewPlaylist->selectRow(index);
    });
    connect(ui->btnNext, &QToolButton::clicked, [this](){
        mPlaylist->next();
        auto index = mPlaylist->currentIndex();
        ui->tableViewPlaylist->clearSelection();
        ui->tableViewPlaylist->selectRow(index);
    });

    connect(ui->btnSeekBackwards, &QToolButton::clicked, [this](){
        mPlayer->setPosition(mPlayer->position() - 5000);

    });
    connect(ui->btnSeekForwards, &QToolButton::clicked, [this](){
        mPlayer->setPosition(mPlayer->position() + 5000);
    });


    connect(ui->tableViewPlaylist, &QTableView::doubleClicked, [this](const QModelIndex &index){
        mPlaylist->setCurrentIndex(index.row());
        ui->tableViewPlaylist->clearSelection();
        ui->tableViewPlaylist->selectRow(index.row());
        if(mPlayer->state() != QMediaPlayer::PlayingState)
            mPlayer->play();
    });

    connect(mPlaylist, &QMediaPlaylist::currentIndexChanged, [this](int row){
        auto title = mPlaylistModel->data(mPlaylistModel->index(row, 0)).toString();
        ui->lblCurrentTrack->setText(title);
    });

    connect(ui->btnLoadPlaylist, &QPushButton::clicked, this, &Widget::loadChoosenPlaylist);
    connect(ui->btnSavePlaylist, &QPushButton::clicked, this, &Widget::savePlaylistAs);
    connect(ui->btnClearPlaylist, &QPushButton::clicked, this, &Widget::clearPlaylist);
    connect(ui->btnRemoveSelected, &QPushButton::clicked, this, &Widget::removeSelectedTunes);

    QFile file(mLastPlaylistFilePath);
    if(file.exists())
    {
        loadPlaylistFromFile(mLastPlaylistFilePath);
    }
}

Widget::~Widget()
{
    savePlaylistToFile(mLastPlaylistFilePath);
    delete mPlaylist;
    delete mPlayer;
    delete mPlaylistModel;
    delete ui;
}

void Widget::keyReleaseEvent(QKeyEvent *event)
{
    auto key = event->key();
    if(key == Qt::Key_Return)
    {
        auto selectedRows = ui->tableViewPlaylist->selectionModel()->selectedRows();
        if(selectedRows.size() == 1)
        {
            mPlaylist->setCurrentIndex(selectedRows.at(0).row());
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
        ui->tableViewPlaylist->selectAll();
    }
    else if(key == Qt::Key_Escape)
    {
        ui->tableViewPlaylist->clearSelection();
    }
    else if(key == Qt::Key_Space)
    {
        this->togglePause();
    }
    else if(key == Qt::Key_Up)
    {
        if(mPlaylistModel->rowCount() > 0)
        {
            --mCurrentIndex;
            if(mCurrentIndex < 0)
                mCurrentIndex = mPlaylistModel->rowCount() - 1;
            ui->tableViewPlaylist->clearSelection();
            ui->tableViewPlaylist->selectRow(mCurrentIndex);
        }
    }
    else if(key == Qt::Key_Down)
    {
        if(mPlaylistModel->rowCount() > 0)
        {
            ++mCurrentIndex;
            if(mCurrentIndex > mPlaylistModel->rowCount() - 1)
                mCurrentIndex = 0;
            ui->tableViewPlaylist->clearSelection();
            ui->tableViewPlaylist->selectRow(mCurrentIndex);
        }
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
    for(const auto &tune: tunes){
        QList<QStandardItem*> items;
        items.append(new QStandardItem(QDir(tune).dirName()));
        items.append(new QStandardItem(tune));
        mPlaylistModel->appendRow(items);
        mPlaylist->addMedia(QUrl::fromLocalFile(tune));
    }
    configurePlaylistView();
}

void Widget::on_comboBoxPlaybackMode_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
        mPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
        break;
    case 1:
        mPlaylist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
        break;
    case 2:
        mPlaylist->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
        break;
    case 3:
        mPlaylist->setPlaybackMode(QMediaPlaylist::Sequential);
        break;
    case 4:
        mPlaylist->setPlaybackMode(QMediaPlaylist::Random);
        break;
    default:
        break;
    }
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
                QString::fromStdString("Save playlist ot file..."),
                startLocation,
                QString::fromStdString("Playlists (*.tpls);;All files (*.*)"));
    if(!fileName.contains(".tpls"))
        fileName += QString(".tpls");
    savePlaylistToFile(fileName);
}

void Widget::clearPlaylist()
{
    mPlaylist->clear();
    mPlaylistModel->clear();
}

void Widget::removeSelectedTunes()
{
    QModelIndexList tunesToDelete = ui->tableViewPlaylist->selectionModel()->selectedRows();
    while(!tunesToDelete.isEmpty())
    {
        mPlaylistModel->removeRows(tunesToDelete.last().row(), 1);
        tunesToDelete.removeLast();
    }
}

bool Widget::loadPlaylistFromFile(const QString &filePath)
{
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
        mPlaylistModel->appendRow(list);
        mPlaylist->addMedia(QUrl::fromLocalFile(campos.at(1)));

    }
    file.close();
    configurePlaylistView();
    return true;
}

bool Widget::savePlaylistToFile(const QString &filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", file.errorString());
        return false;
    }
    QTextStream stream(&file);
    for(int row{1}; row < mPlaylistModel->rowCount(); ++row)
    {
        stream << mPlaylistModel->item(row, 0)->text() << "*" <<
                mPlaylistModel->item(row, 1)->text() << endl;
    }
    file.close();
    return true;
}

void Widget::configurePlaylistView()
{
    QStringList headers;
    headers << tr("Tune") << tr("File path");
    mPlaylistModel->setHorizontalHeaderLabels(headers);
    ui->tableViewPlaylist->hideColumn(1); // Hide file path
    ui->tableViewPlaylist->verticalHeader()->setVisible(false); //Hide column numbers
    ui->tableViewPlaylist->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewPlaylist->setSelectionMode(QAbstractItemView::MultiSelection); //Only one row can be selected
    ui->tableViewPlaylist->resizeColumnsToContents();
    ui->tableViewPlaylist->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewPlaylist->horizontalHeader()->setStretchLastSection(true);
    ui->tableViewPlaylist->setColumnWidth(1, ui->tableViewPlaylist->width());
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

void Widget::on_action_remove_selected_files_from_HDD_triggered()
{
    QModelIndexList tunesToDelete = ui->tableViewPlaylist->
            selectionModel()->selectedRows();
    int counter{0};
    while(!tunesToDelete.isEmpty())
    {
        qDebug() << "counter = " << ++counter;
        auto row = tunesToDelete.last().row();
        mPlaylist->removeMedia(row);
        QString path = mPlaylistModel->item(row, 1)->text();
        mPlaylistModel->removeRows(row, 1);
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
