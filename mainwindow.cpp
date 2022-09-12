#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      settings(QCoreApplication::organizationName(), QCoreApplication::applicationName())
{
    prevRow = 1;
    navRow = 1;
    setWindowIcon(QIcon(":/icon/icon.svg"));
    setWindowFlags(Qt::FramelessWindowHint);
    resize(1000,700);
    move((QApplication::desktop()->width() - width())/2,(QApplication::desktop()->height() - height())/2);
    setStyleSheet("color:white; background-color:#232326;");
    connect(new QShortcut(QKeySequence(Qt::Key_Space),this), SIGNAL(activated()), this, SLOT(playPause()));
    connect(new QShortcut(QKeySequence(Qt::Key_Escape),this), SIGNAL(activated()), this, SLOT(exitFullScreen()));
    connect(new QShortcut(QKeySequence(Qt::Key_F11),this), SIGNAL(activated()), this, SLOT(enterExitFullScreen()));
    connect(new QShortcut(QKeySequence(Qt::Key_Left),this), SIGNAL(activated()), this, SLOT(seekBack()));
    connect(new QShortcut(QKeySequence(Qt::Key_Right),this), SIGNAL(activated()), this, SLOT(seekForward()));

    QWidget *widget = new QWidget;
    setCentralWidget(widget);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setSpacing(0);
    vbox->setContentsMargins(0,0,0,0);

    titleBar = new TitleBar;
    connect(titleBar->action_search, SIGNAL(triggered(bool)), this, SLOT(preSearch()));
    connect(titleBar->lineEdit_search, SIGNAL(returnPressed()), this, SLOT(preSearch()));
    connect(titleBar->lineEdit_page, SIGNAL(returnPressed()), this, SLOT(search()));
    connect(titleBar->pushButton_lastPage, SIGNAL(released()), this, SLOT(lastPage()));
    connect(titleBar->pushButton_nextPage, SIGNAL(released()), this, SLOT(nextPage()));
    connect(titleBar->pushButton_minimize, SIGNAL(released()), this, SLOT(showMinimized()));
    connect(titleBar->pushButton_maximize, SIGNAL(released()), this, SLOT(showNormalMaximize()));
    connect(titleBar->pushButton_close, SIGNAL(released()), qApp, SLOT(quit()));
    connect(titleBar->action_set,SIGNAL(triggered()), this, SLOT(dialogSet()));
    connect(titleBar, SIGNAL(moveMainWindow(QPoint)), this, SLOT(moveMe(QPoint)));
    vbox->addWidget(titleBar);

    label_titleBar_bottom = new QLabel;
    label_titleBar_bottom->setFixedHeight(2);
    label_titleBar_bottom->setStyleSheet("background-color:#9F2425");
    vbox->addWidget(label_titleBar_bottom);

    createWidgetToplist();
    QHBoxLayout *hbox = new QHBoxLayout;
    navWidget = new NavWidget;
    navWidget->listWidget->setCurrentRow(1);
    connect(navWidget->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(navPage(int)));
    connect(navWidget->pushButton_albumPic, SIGNAL(clicked(bool)), this, SLOT(swapPlaylist()));
    hbox->addWidget(navWidget);

    stackedWidget = new QStackedWidget;
    stackedWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    stackedWidget->addWidget(rankScrollArea);

    playlistWidget = new QWidget;
    QVBoxLayout *vboxPL = new QVBoxLayout;
    vboxPL->setMargin(0);
    label_playlistTitle = new QLabel;
    label_playlistTitle->setFont(QFont("Timers", 20, 50));
    label_playlistTitle->setMargin(5);
    vboxPL->addWidget(label_playlistTitle);
    tableWidget_playlist = new QTableWidget;
    tableWidget_playlist->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget_playlist->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget_playlist->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget_playlist->setColumnCount(8);
    tableWidget_playlist->setColumnHidden(4,true);
    tableWidget_playlist->setColumnHidden(5,true);
    tableWidget_playlist->setColumnHidden(6,true);
    QStringList header;
    header << "歌名" << "歌手" << "专辑" << "时长" << "id" << "专辑封面" << "mvid" << "MV";
    tableWidget_playlist->setHorizontalHeaderLabels(header);
    tableWidget_playlist->setStyleSheet("QTableView::item:selected { color:white; background:rgb(22,22,22); }"
                                        "QTableCornerButton::section { background-color:#232326; }"
                                        "QHeaderView::section { color:white; background-color:#232326; }");
    connect(tableWidget_playlist, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(playSong(int,int)));
    tableWidget_playlist->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableWidget_playlist, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tableWidget_playlist_ContextMenu(QPoint)));
    vboxPL->addWidget(tableWidget_playlist);
    playlistWidget->setLayout(vboxPL);
    stackedWidget->addWidget(playlistWidget);

    songWidget = new QWidget;
    QVBoxLayout *vboxs = new QVBoxLayout;
    textBrowser = new QTextBrowser;
    vboxs->addWidget(textBrowser);
    songWidget->setLayout(vboxs);
    stackedWidget->addWidget(songWidget);

    videoWidget = new QVideoWidget;
    videoWidget->setStyleSheet("background-color:black;");
    stackedWidget->addWidget(videoWidget);

    hbox->addWidget(stackedWidget);
    vbox->addLayout(hbox);

    controlBar = new ControlBar;
    controlBar->slider_volume->setValue(settings.value("Volume",100).toInt());
    connect(controlBar->pushButton_last, SIGNAL(pressed()), this, SLOT(playLast()));
    connect(controlBar->pushButton_play, SIGNAL(pressed()), this, SLOT(playPause()));
    connect(controlBar->pushButton_next, SIGNAL(pressed()), this, SLOT(playNext()));
    connect(controlBar->pushButton_mute, SIGNAL(pressed()), this, SLOT(mute()));
    connect(controlBar->pushButton_lyric, SIGNAL(clicked(bool)), this, SLOT(showHideLyric(bool)));
    connect(controlBar->pushButton_download, SIGNAL(pressed()), this, SLOT(dialogDownload()));
    connect(controlBar->pushButton_fullscreen, SIGNAL(pressed()), this, SLOT(enterFullScreen()));
    connect(controlBar->slider_progress, SIGNAL(sliderMoved(int)), this, SLOT(sliderProgressMoved(int)));
    connect(controlBar->slider_volume, SIGNAL(sliderMoved(int)), this, SLOT(sliderVolumeMoved(int)));
    vbox->addWidget(controlBar);
    widget->setLayout(vbox);

    player = new QMediaPlayer;
    player->setVideoOutput(videoWidget);
    connect(player, SIGNAL(durationChanged(qint64)), this, SLOT(durationChange(qint64)));
    connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(positionChange(qint64)));
    connect(player, SIGNAL(volumeChanged(int)), this, SLOT(volumeChange(int)));
    //connect(player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(errorHandle(QMediaPlayer::Error)));
    connect(player, SIGNAL(stateChanged(QMediaPlayer::State)), SLOT(stateChange(QMediaPlayer::State)));
//    QString vol = settings.value("Volume").toString();
//    if (vol == "") vol = "100";
//    player->setVolume(vol.toInt());

    lyricWidget = new LyricWidget;
    connect(lyricWidget->pushButton_set,SIGNAL(pressed()),this,SLOT(dialogSet()));
    connect(lyricWidget->pushButton_close,SIGNAL(pressed()),this,SLOT(hideLyric()));
    QString slx =  settings.value("LyricX").toString();
    QString sly =  settings.value("LyricY").toString();
    if (slx=="" || sly=="" || slx.toInt()>QApplication::desktop()->width() || sly.toInt()>QApplication::desktop()->height()) {
        lyricWidget->move((QApplication::desktop()->width()-lyricWidget->width())/2, QApplication::desktop()->height()-lyricWidget->height());
    } else {
        lyricWidget->move(slx.toInt(),sly.toInt());
    }
    //qDebug() << "歌词坐标" << slx << sly;
    QString lyricShow = settings.value("LyricShow").toString();
    //qDebug() << "lyricShow" << lyricShow;
    if (lyricShow == "") {
        settings.setValue("LyricShow", "true");
    } else {
        if (lyricShow == "true") {
            showHideLyric(true);
        } else {
            showHideLyric(false);
        }
    }

    QString SColorLeft =  settings.value("LyricFontColorLeft", "#FF0000").toString();
    lyricWidget->color_left = QColor(SColorLeft);
    QString SColorRight =  settings.value("LyricFontColorRight", "#00FF00").toString();
    lyricWidget->color_right = QColor(SColorRight);
    QString sfont =  settings.value("Font").toString();
    if (sfont == "") {
        QFont font = qApp->font();
        font.setPointSize(40);
        lyricWidget->font = font;        
        QString sfont = font.family() + "," + QString::number(font.pointSize()) + "," + QString::number(font.weight()) + "," + font.italic();
        settings.setValue("Font", sfont);
    } else {
        QStringList SLFont = sfont.split(",");
        QFont font(SLFont.at(0),SLFont.at(1).toInt(),SLFont.at(2).toInt(),SLFont.at(3).toInt());
        lyricWidget->font = font;
        textBrowser->setFont(font);
    }
    downloadPath =  settings.value("DownloadPath").toString();
    if (downloadPath == "") {
        settings.setValue("DownloadPath", QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first());
    }

    //信息泡泡
    label_message = new QLabel;
    QFont font = this->font();
    font.setPointSize(20);
    label_message->setFont(font);
    label_message->setWindowFlags(Qt::FramelessWindowHint);
    label_message->setStyleSheet("color:white; padding:10px; border:1px solid white; border-radius:20px; background-color:rgba(0,0,0,100); ");
}

MainWindow::~MainWindow()
{
}

void MainWindow::moveMe(QPoint point)
{
    move(point);
}

void MainWindow::createWidgetToplist()
{
    rankScrollArea = new QScrollArea;
    QWidget *toplistWidget = new QWidget;
    rankScrollArea->setWidget(toplistWidget);
    rankScrollArea->setWidgetResizable(true);   //关键语句
    QGridLayout *gridLayout = new QGridLayout(toplistWidget);
    gridLayout->setSpacing(40);
    QString surl = "http://music.163.com/api/toplist";
    QJsonDocument JD = QJsonDocument::fromJson(getReply(surl));
    qDebug() << surl;
    QJsonArray JA_list = JD.object().value("list").toArray();
    //qDebug() << list;
    for (int i=0; i< JA_list.size(); i++) {
        QString coverImgUrl = JA_list[i].toObject().value("coverImgUrl").toString();
        double id = JA_list[i].toObject().value("id").toDouble();
        QString name = JA_list[i].toObject().value("name").toString();
        QPushButton *pushButton = new QPushButton(this);
        pushButton->setFixedSize(120,120);
        pushButton->setIconSize(QSize(120,120));
        pushButton->setFlat(true);
        pushButton->setCursor(Qt::PointingHandCursor);
        gridLayout->addWidget(pushButton, i/5, i%5);
        getToplistImg(coverImgUrl, pushButton);
        connect(pushButton, &QPushButton::clicked, [=]{
            createPlaylist(id, name);
        });
    }
}

QByteArray MainWindow::getReply(QString surl)
{
    QNetworkAccessManager *NAM = new QNetworkAccessManager;
    QNetworkRequest request;
    request.setUrl(QUrl(surl));
    request.setRawHeader("Referer", "http://music.163.com/");
    QNetworkReply *reply = NAM->get(request);
    QEventLoop loop;
    connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    loop.exec();
    reply->deleteLater();
    return reply->readAll();
}

QByteArray MainWindow::postReply(QString surl,QString spost)
{
    QNetworkAccessManager *NAM = new QNetworkAccessManager;
    QNetworkRequest request;
    request.setUrl(QUrl(surl));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QByteArray BA_post;
    BA_post.append(spost);
    QNetworkReply *reply = NAM->post(request,BA_post);
    QEventLoop loop;
    connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    loop.exec();
    reply->deleteLater();
    return reply->readAll();
}

void MainWindow::showNormalMaximize()
{
    //qDebug() << "isMaximized=" << isMaximized();    
    if (isMaximized()) {
        showNormal();
        titleBar->pushButton_maximize->setStyleSheet("QPushButton { border-image: url(:/icon/maximize.svg); }"
                                                     "QPushButton:hover { border-image: url(:icon/maximize_hover.svg); }"
                                                     "QPushButton:pressed { border-image: url(:icon/maximize.svg); }");
    } else {
        showMaximized();
        titleBar->pushButton_maximize->setStyleSheet("QPushButton { border-image: url(:/icon/normal.svg); }"
                                                     "QPushButton:hover { border-image: url(:/icon/normal_hover.svg); }"
                                                     "QPushButton:pressed { border-image: url(:/icon/normal.svg); }");
    }
}

void MainWindow::createPlaylist(double id, QString name)
{
    QString surl = QString("http://music.163.com/api/playlist/detail?id=%1").arg(id, 0, 'f', 0);
    qDebug() << surl;
    QJsonDocument JD = QJsonDocument::fromJson(getReply(surl));
    //qDebug() << JD;
    int code = JD.object().value("code").toInt();
    qDebug() << code;
    if (code == 200) {
        tableWidget_playlist->setRowCount(0);
        tableWidget_playlist->scrollToTop();
        QJsonArray tracks = JD.object().value("result").toObject().value("tracks").toArray();
        //qDebug() << tracks;
        for (int i=0; i<tracks.size(); i++) {
            tableWidget_playlist->insertRow(i);
            tableWidget_playlist->setItem(i,0,new QTableWidgetItem(tracks[i].toObject().value("name").toString()));
            QJsonArray artists = tracks[i].toObject().value("artists").toArray();
            QString sartists = "";
            for(int a=0; a<artists.size(); a++){
                sartists += artists[a].toObject().value("name").toString();
                if(a<artists.size()-1) sartists += ",";
            }
            tableWidget_playlist->setItem(i,1,new QTableWidgetItem(sartists));
            tableWidget_playlist->setItem(i,2,new QTableWidgetItem(tracks[i].toObject().value("album").toObject().value("name").toString()));
            int ds = tracks[i].toObject().value("duration").toInt()/1000;
            QTableWidgetItem *TWI = new QTableWidgetItem(QString("%1:%2").arg(ds/60,2,10,QLatin1Char(' ')).arg(ds%60,2,10,QLatin1Char('0')));
            TWI->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            tableWidget_playlist->setItem(i, 3, TWI);
            tableWidget_playlist->setItem(i, 4, new QTableWidgetItem(QString::number(tracks[i].toObject().value("id").toInt())));
            tableWidget_playlist->setItem(i,5,new QTableWidgetItem(tracks[i].toObject().value("album").toObject().value("picUrl").toString()));
            int mvid = tracks[i].toObject().value("mvid").toInt();
            tableWidget_playlist->setItem(i, 6, new QTableWidgetItem(QString::number(mvid)));
            if(mvid != 0){
                QPushButton *pushButton_MV = new QPushButton;
                pushButton_MV->setFixedSize(24,24);
                pushButton_MV->setIcon(QIcon(":/icon/video.svg"));
                pushButton_MV->setIconSize(QSize(24, 24));
                pushButton_MV->setFocusPolicy(Qt::NoFocus);
                pushButton_MV->setFlat(true);
                pushButton_MV->setCursor(Qt::PointingHandCursor);
                connect(pushButton_MV, SIGNAL(clicked()), this, SLOT(pushButtonMVClicked()));
                tableWidget_playlist->setCellWidget(i, 7, pushButton_MV);
            }
        }
        tableWidget_playlist->resizeColumnsToContents();
        navWidget->listWidget->setCurrentRow(2);
        label_playlistTitle->setText(name);
    } else {
        QString message = JD.object().value("message").toString();
        label_message->setText(message);
        label_message->move(x() + width()/2 - label_message->width()/2, y() + height()/2 - label_message->height()/2);
        label_message->show();
        QTimer::singleShot(5000, this, [=]{
            label_message->hide();
        });
    }
}

void MainWindow::playSong(int row, int column)
{
    Q_UNUSED(column);
    navWidget->pushButton_albumPic->setIcon(QIcon(":/icon/disc.svg"));
    QString id = tableWidget_playlist->item(row,4)->text();
    QString surl = "http://music.163.com/song/media/outer/url?id=" + id + ".mp3";
    qDebug() << surl;
    player->setMedia(QUrl(surl));
    player->play();
    navWidget->label_songname->setText(tableWidget_playlist->item(row, 0)->text() + "\n" + tableWidget_playlist->item(row, 1)->text());
    lyricWidget->text = tableWidget_playlist->item(row,0)->text() + " - " + tableWidget_playlist->item(row,1)->text();
    lyricWidget->update();
    getLyric(id);
    QPixmap pixmap;
    pixmap.loadFromData(getReply(tableWidget_playlist->item(row,5)->text()));
    navWidget->pushButton_albumPic->setIcon(QIcon(pixmap));
    pixmap.save(QDir::currentPath() + "/cover.jpg");
    //qDebug() << QDir::currentPath() + "/cover.jpg";
}

void MainWindow::durationChange(qint64 d)
{
    controlBar->slider_progress->setMaximum(static_cast<int>(d));
    QTime t(0,0,0);
    t = t.addMSecs(static_cast<int>(d));
    controlBar->label_song_duration->setText(t.toString("mm:ss"));
}

void MainWindow::positionChange(qint64 p)
{
    //qDebug() << "position =" << p;
    if(!controlBar->slider_progress->isSliderDown())controlBar->slider_progress->setValue(static_cast<int>(p));
    QTime t(0,0,0);
    t = t.addMSecs(static_cast<int>(p));
    controlBar->label_song_timeNow->setText(t.toString("mm:ss"));

    // 歌词选行
    int hl=0;
    // 非最后一句
    for (int i=0; i<lyrics.size()-1; i++) {
        if (t>lyrics.at(i).time && t<lyrics.at(i+1).time) {
            lyricWidget->text = lyrics.at(i).sentence;
            lyricWidget->lp = static_cast<float>(lyrics.at(i).time.msecsTo(t)) / lyrics.at(i).time.msecsTo(lyrics.at(i+1).time);
            //qDebug() << lyrics.at(i).time.msecsTo(t) << lyrics.at(i).time.msecsTo(lyrics.at(i+1).time) << lyricWidget->lp;
//            QFontMetrics FM(lyricWidget->font);
//            if(FM.boundingRect(lyricWidget->text).width() > lyricWidget->width()){
//                lyricWidget->resize(FM.boundingRect(lyricWidget->text).size() + QSize(20,26));
//                lyricWidget->move((QApplication::desktop()->width()-lyricWidget->width())/2, lyricWidget->y());
//            }
            lyricWidget->update();
            hl=i;
            break;
        }
    }
    //最后一句
    if (lyrics.size()>0) {
        int j = lyrics.size() - 1;
        if (t>lyrics.at(j).time) {
            lyricWidget->text = lyrics.at(j).sentence;
            lyricWidget->update();
            hl=j;
        }
    }

    // 歌词文本着色
    for(int a=0; a<lyrics.size(); a++){
        QTextCursor cursor(textBrowser->document()->findBlockByLineNumber(a));
        QTextBlockFormat TBF = cursor.blockFormat();
        TBF.setForeground(QBrush(Qt::white));
        //TBF.setBackground(QBrush(Qt::transparent));
        TBF.clearBackground();
        cursor.setBlockFormat(TBF);
    }
    if(lyrics.size()>0){
        QTextCursor cursor1(textBrowser->document()->findBlockByLineNumber(hl));
        QTextBlockFormat TBF1 = cursor1.blockFormat();
        TBF1.setForeground(QBrush(Qt::green));
        TBF1.setBackground(QBrush(QColor(255,255,255,80)));
        cursor1.setBlockFormat(TBF1);
        //textBrowser->setTextCursor(cursor1);
        QScrollBar *scrollBar = textBrowser->verticalScrollBar();
        //qDebug() << "scrollBar" << scrollBar->maximum() << scrollBar->maximum()*hl/(lyrics.size()) ;
        scrollBar->setSliderPosition(scrollBar->maximum()*hl/(lyrics.size()));
    }
}

void MainWindow::volumeChange(int v)
{
    if (!controlBar->slider_volume->isSliderDown())
        controlBar->slider_volume->setValue(v);
    controlBar->slider_volume->setToolTip(QString::number(v));
}

void MainWindow::stateChange(QMediaPlayer::State state)
{
    //qDebug() << state;
    if (state == QMediaPlayer::PlayingState) {
        controlBar->pushButton_play->setStyleSheet("QPushButton { border-image: url(:/icon/pause.svg); }"
                                                   "QPushButton:hover { border-image: url(:/icon/pause_hover.svg); }"
                                                   "QPushButton:pressed { border-image: url(:/icon/pause.svg); }");
    } else if (state == QMediaPlayer::PausedState) {
        controlBar->pushButton_play->setStyleSheet("QPushButton { border-image: url(:/icon/play.svg); }"
                                                   "QPushButton:hover { border-image: url(:/icon/play_hover.svg); }"
                                                   "QPushButton:pressed { border-image: url(:/icon/play.svg); }");
    } else if (state == QMediaPlayer::StoppedState) {
        controlBar->pushButton_play->setStyleSheet("QPushButton { border-image: url(:/icon/play.svg); }"
                                                   "QPushButton:hover { border-image: url(:/icon/play_hover.svg); }"
                                                   "QPushButton:pressed { border-image: url(:/icon/play.svg); }");
    }
}

void MainWindow::playPause()
{
    //qDebug() << "state=" << player->state();
    if(player->state() == QMediaPlayer::PlayingState){
        player->pause();
    }else if(player->state() == QMediaPlayer::PausedState){
        player->play();
    }else if(player->state() == QMediaPlayer::StoppedState){
        player->play();
    }
}

void MainWindow::navPage(int row)
{
    qDebug() << "nav" << row;
    prevRow = navRow;
    navRow = row;
    switch (row) {
    case 1:
        stackedWidget->setCurrentWidget(rankScrollArea);
        break;
    case 2:
        stackedWidget->setCurrentWidget(playlistWidget);
        break;
    case 3:
        textBrowser->setStyleSheet("QTextBrowser { color:white; border-image:url(cover.jpg); }");
        stackedWidget->setCurrentWidget(songWidget);
        break;
    case 10:
        stackedWidget->setCurrentWidget(videoWidget);
        break;
    }
}

void MainWindow::sliderProgressMoved(int p)
{
    player->setPosition(p);
}


void MainWindow::sliderVolumeMoved(int v)
{
    player->setVolume(v);
}

void MainWindow::mute()
{
    if (player->isMuted()) {
        player->setMuted(false);
        controlBar->pushButton_mute->setIcon(QIcon(":/icon/volume.svg"));
        controlBar->slider_volume->setValue(volume);
    } else {
        volume = player->volume();
        player->setMuted(true);
        controlBar->pushButton_mute->setIcon(QIcon(":/icon/mute.svg"));
        controlBar->slider_volume->setValue(0);
    }
}

void MainWindow::preSearch()
{
    titleBar->lineEdit_page->setText("1");
    search();
}

void MainWindow::search()
{
    if(titleBar->lineEdit_search->text() != ""){
        navWidget->listWidget->setCurrentRow(2);
        label_playlistTitle->setText("搜索：" + titleBar->lineEdit_search->text());
        int limit = 20;
        QString surl = "http://music.163.com/api/search/pc";
        QString spost = "type=1&s=" + titleBar->lineEdit_search->text() + "&limit=" + QString::number(limit) + "&offset=" + QString::number((titleBar->lineEdit_page->text().toInt()-1)*limit);
        qDebug() << surl + "?" + spost;
        //qDebug() << postReply(surl,spost);
        stackedWidget->setCurrentIndex(1);
        tableWidget_playlist->setRowCount(0);
        QJsonDocument JD = QJsonDocument::fromJson(postReply(surl, spost));
        int code = JD.object().value("code").toInt();
        qDebug() << code;
        if (code == 200) {
            QJsonArray songs = JD.object().value("result").toObject().value("songs").toArray();
            //qDebug() << songs;
            for(int i=0; i<songs.size(); i++){
                tableWidget_playlist->insertRow(i);
                tableWidget_playlist->setItem(i,0,new QTableWidgetItem(songs[i].toObject().value("name").toString()));
                tableWidget_playlist->setItem(i,1,new QTableWidgetItem(songs[i].toObject().value("artists").toArray()[0].toObject().value("name").toString()));
                tableWidget_playlist->setItem(i,2,new QTableWidgetItem(songs[i].toObject().value("album").toObject().value("name").toString()));
                int ds = songs[i].toObject().value("duration").toInt()/1000;
                QTableWidgetItem *TWI = new QTableWidgetItem(QString("%1:%2").arg(ds/60,2,10,QLatin1Char(' ')).arg(ds%60,2,10,QLatin1Char('0')));
                TWI->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                tableWidget_playlist->setItem(i,3,TWI);
                tableWidget_playlist->setItem(i,4,new QTableWidgetItem(QString::number(songs[i].toObject().value("id").toInt())));
                tableWidget_playlist->setItem(i,5,new QTableWidgetItem(songs[i].toObject().value("album").toObject().value("picUrl").toString()));
                int mvid = songs[i].toObject().value("mvid").toInt();
                tableWidget_playlist->setItem(i,6,new QTableWidgetItem(QString::number(mvid)));
                if(mvid != 0){
                    QPushButton *pushButton_MV = new QPushButton;
                    pushButton_MV->setFixedSize(24,24);
                    pushButton_MV->setIcon(QIcon(":/icon/video.svg"));
                    pushButton_MV->setIconSize(QSize(24,24));
                    pushButton_MV->setFocusPolicy(Qt::NoFocus);
                    pushButton_MV->setFlat(true);
                    pushButton_MV->setCursor(Qt::PointingHandCursor);
                    connect(pushButton_MV, SIGNAL(clicked()), this, SLOT(pushButtonMVClicked()));
                    tableWidget_playlist->setCellWidget(i,7,pushButton_MV);
                }
            }
            tableWidget_playlist->resizeColumnsToContents();
        } else {
            QString message = JD.object().value("message").toString();
            label_message->setText(message);
            label_message->move(x() + width()/2 - label_message->width()/2, y() + height()/2 - label_message->height()/2);
            label_message->show();
            QTimer::singleShot(5000, this, [=]{
                label_message->hide();
            });
        }
    }
}

void MainWindow::lastPage()
{
    int page = titleBar->lineEdit_page->text().toInt();
    if(page > 1){
        titleBar->lineEdit_page->setText(QString::number(page-1));
        search();
    }
}

void MainWindow::nextPage()
{
    int page = titleBar->lineEdit_page->text().toInt();
    if (page < 99) {
        titleBar->lineEdit_page->setText(QString::number(page+1));
        search();
    }
}

void MainWindow::getLyric(QString id)
{
    QString surl = "http://music.163.com/api/song/lyric?os=pc&lv=-1&kv=-1&tv=-1&id=" + id;
    qDebug() << surl;
    QJsonDocument json = QJsonDocument::fromJson(getReply(surl));
    QString slyric = json.object().value("lrc").toObject().value("lyric").toString();
    textBrowser->setText("");

    lyrics.clear();
    QStringList line = slyric.split("\n");
    for(int i=0; i<line.size(); i++){
        if(line.at(i).contains("]")){
            QStringList SL = line.at(i).split("]");
            //qDebug() << SL.at(0).mid(1);
            for(int j=0; j<SL.length()-1; j++){
                Lyric lyric;
                QString stime = SL.at(j).mid(1);
                //qDebug() << stime.length() << stime.indexOf(".");
                if((stime.length() - stime.indexOf(".")) == 3) stime += "0";
                lyric.time = QTime::fromString(stime, "mm:ss.zzz");
                lyric.sentence = SL.at(SL.length()-1);
                lyrics.append(lyric);
            }
        }
    }
    std::sort(lyrics.begin(), lyrics.end(), [](Lyric a, Lyric b){return a.time < b.time;});
    for(int i=0; i<lyrics.size(); i++){
        textBrowser->insertPlainText(lyrics.at(i).sentence + "\n");
    }
    textBrowser->selectAll();
    textBrowser->setAlignment(Qt::AlignCenter);
    QTextCursor cursor = textBrowser->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    textBrowser->setTextCursor(cursor);
}

void MainWindow::swapPlaylist()
{
    navWidget->listWidget->setCurrentRow(3);
    //stackedWidget->setCurrentWidget(songWidget);
}

void MainWindow::hideLyric()
{
    lyricWidget->hide();
    controlBar->pushButton_lyric->setChecked(false);
    settings.setValue("LyricShow", "false");
}

void MainWindow::showHideLyric(bool on)
{
    if(on){
        lyricWidget->show();
        controlBar->pushButton_lyric->setChecked(true);
        settings.setValue("LyricShow", "true");
    }else{
        lyricWidget->hide();
        controlBar->pushButton_lyric->setChecked(false);
        settings.setValue("LyricShow", "false");
    }
}

void MainWindow::dialogSet()
{
    QDialog *dialog_set = new QDialog(this);
    dialog_set->setWindowTitle("设置");
    dialog_set->setFixedSize(300,200);
    dialog_set->setStyleSheet("QLineEdit { border:1px solid gray; } ");
    QGridLayout *gridLayout = new QGridLayout;

    QLabel *label = new QLabel("歌词字体");
    label->setAlignment(Qt::AlignCenter);
    gridLayout->addWidget(label,0,0,1,1);
    pushButton_font = new QPushButton;
    QString sfont = lyricWidget->font.family() + "," + QString::number(lyricWidget->font.pointSize()) + "," + QString::number(lyricWidget->font.weight()) + "," + lyricWidget->font.italic();
    pushButton_font->setText(sfont);
    pushButton_font->setFocusPolicy(Qt::NoFocus);
    connect(pushButton_font,SIGNAL(pressed()),this,SLOT(chooseFont()));
    gridLayout->addWidget(pushButton_font,0,1,1,2);

    //歌词颜色
    label = new QLabel("歌词颜色");
    label->setAlignment(Qt::AlignCenter);
    gridLayout->addWidget(label,1,0,1,1);

    pushButton_fontcolorleft = new QPushButton;
    pushButton_fontcolorleft->setObjectName("LyricFontColorLeft");
    pushButton_fontcolorleft->setText("■已播放");
    pushButton_fontcolorleft->setFocusPolicy(Qt::NoFocus);
    pushButton_fontcolorleft->setStyleSheet("color:" + lyricWidget->color_left.name());
    connect(pushButton_fontcolorleft,SIGNAL(pressed()),this,SLOT(chooseFontColor()));
    gridLayout->addWidget(pushButton_fontcolorleft,1,1,1,1);

    pushButton_fontcolorright = new QPushButton;
    pushButton_fontcolorright->setObjectName("LyricFontColorRight");
    pushButton_fontcolorright->setText("■未播放");
    pushButton_fontcolorright->setFocusPolicy(Qt::NoFocus);
    pushButton_fontcolorright->setStyleSheet("color:" + lyricWidget->color_right.name());
    connect(pushButton_fontcolorright,SIGNAL(pressed()),this,SLOT(chooseFontColor()));
    gridLayout->addWidget(pushButton_fontcolorright,1,2,1,1);

    // 保存路径
    QPushButton *pushButton_downloadPath = new QPushButton("保存路径");
    pushButton_downloadPath->setFocusPolicy(Qt::NoFocus);
    pushButton_downloadPath->setFlat(true);
    connect(pushButton_downloadPath,SIGNAL(pressed()),this,SLOT(openDownloadPath()));
    gridLayout->addWidget(pushButton_downloadPath,2,0,1,1);
    lineEdit_downloadPath = new QLineEdit;
    downloadPath =  settings.value("DownloadPath").toString();
    lineEdit_downloadPath->setText(downloadPath);
    QAction *action_browse = new QAction(this);
    action_browse->setObjectName("SettingDialogChooseDownloadPath");
    action_browse->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    connect(action_browse,SIGNAL(triggered(bool)),this,SLOT(chooseDownloadPath()));
    lineEdit_downloadPath->addAction(action_browse,QLineEdit::TrailingPosition);
    gridLayout->addWidget(lineEdit_downloadPath,2,1,1,2);

    dialog_set->setLayout(gridLayout);
    dialog_set->show();
}

void MainWindow::chooseFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, lyricWidget->font, this, "选择字体");
    if (ok) {
       lyricWidget->font = font;
       textBrowser->setFont(font);
       QFontMetrics FM(font);
       lyricWidget->resize(FM.boundingRect(lyricWidget->text).size() + QSize(20,30));
       lyricWidget->move((QApplication::desktop()->width()-lyricWidget->width())/2, lyricWidget->y());
       QString sfont = font.family() + "," + QString::number(font.pointSize()) + "," + QString::number(font.weight()) + "," + font.italic();
       pushButton_font->setText(sfont);
       settings.setValue("Font", sfont);
    }
}

void MainWindow::chooseFontColor()
{
    QObject *object = sender();
    QPalette plt;
    if (object->objectName() == "LyricFontColorLeft")
        plt = pushButton_fontcolorleft->palette();
    if (object->objectName() == "LyricFontColorRight")
        plt = pushButton_fontcolorright->palette();
    QBrush brush = plt.color(QPalette::WindowText);
    QColor color = QColorDialog::getColor(brush.color(), this);
    if (color.isValid()) {
        plt.setColor(QPalette::ButtonText, color);
        if (object->objectName() == "LyricFontColorLeft") {
            lyricWidget->color_left = color;
            pushButton_fontcolorleft->setPalette(plt);
            settings.setValue("LyricFontColorLeft", color.name());
        }
        if (object->objectName() == "LyricFontColorRight") {
            lyricWidget->color_right = color;
            pushButton_fontcolorright->setPalette(plt);
            settings.setValue("LyricFontColorRight", color.name());
        }
    }
}

void MainWindow::chooseDownloadPath()
{
    downloadPath = QFileDialog::getExistingDirectory(this, "保存路径", downloadPath, QFileDialog::ShowDirsOnly |QFileDialog::DontResolveSymlinks);
    if (downloadPath != "") {
        QObject *object = sender();
        qDebug() << object->objectName() << downloadPath;
        if (object->objectName() == "SettingDialogChooseDownloadPath") {
            lineEdit_downloadPath->setText(downloadPath);
        }
        if (object->objectName() == "DownloadDialogPath") {
            pushButton_path->setText(downloadPath);
            pushButton_path->setToolTip(downloadPath);
        }
        settings.setValue("DownloadPath", downloadPath);
    }
}

void MainWindow::openDownloadPath()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(lineEdit_downloadPath->text()));
}

void MainWindow::playLast()
{
    int row = tableWidget_playlist->currentRow();
    qDebug() << row;
    if (row != -1) {
        if (row > 0) {
            row--;
            playSong(row,0);
            tableWidget_playlist->setCurrentCell(row,0);
        }
    }
}

void MainWindow::playNext()
{
    int row = tableWidget_playlist->currentRow();
    int rc = tableWidget_playlist->rowCount();
    qDebug() << row << rc;
    if (row != -1) {
        if (row < rc-1) {
            row++;
            playSong(row,0);
            tableWidget_playlist->setCurrentCell(row,0);
        }
    }
}

void MainWindow::enterExitFullScreen()
{
    if (!isFullScreen()) {
        enterFullScreen();
    } else {
        exitFullScreen();
    }
}

void MainWindow::enterFullScreen()
{
    if (navRow==3 || navRow==10) {
        if (!isFullScreen()) {
            //winState = windowState();
            //qDebug() << winState;
            showFullScreen();
            titleBar->hide();
            label_titleBar_bottom->hide();
            navWidget->hide();
            controlBar->hide();
            lyricWidget->hide();
        }
    }
}

void MainWindow::exitFullScreen()
{
    if (isFullScreen()) {
        showNormal();
        //restoreState(winState);
//        if (winState == Qt::WindowNoState)
//            showNormal();
//        else if (winState == Qt::WindowMaximized)
//            showMaximized();
        titleBar->show();
        label_titleBar_bottom->show();
        navWidget->show();
        controlBar->show();
        if (controlBar->pushButton_lyric->isChecked()) lyricWidget->show();
    }
}

void MainWindow::dialogDownload()
{
    QDialog *dialog = new QDialog(this);
    dialog->setFixedWidth(300);
    dialog->setWindowTitle("下载");
    dialog->setStyleSheet("QLineEdit { border:1px solid gray; }"
                          "QToolTip { border:1px solid black; background-color: black; }");
    QGridLayout *gridLayout = new QGridLayout;
    QLabel *label = new QLabel("歌名");
    gridLayout->addWidget(label,0,0,1,1);
    QLineEdit *lineEdit_songname = new QLineEdit;
    gridLayout->addWidget(lineEdit_songname,0,1,1,1);
    label = new QLabel("下载地址");
    gridLayout->addWidget(label,1,0,1,1);
    QLineEdit *lineEdit_url = new QLineEdit;
    lineEdit_url->setText(player->media().canonicalUrl().toString());
    gridLayout->addWidget(lineEdit_url,1,1,1,1);
    QString suffix = QFileInfo(lineEdit_url->text()).suffix();
    suffix = suffix.left(suffix.indexOf("?"));
    lineEdit_songname->setText(navWidget->label_songname->text().replace("\n"," - ") + "." + suffix);
    QPushButton *pushButton_open = new QPushButton("保存路径");
    pushButton_open->setFlat(true);
    gridLayout->addWidget(pushButton_open,2,0,1,1);
    pushButton_path = new QPushButton;
    pushButton_path->setObjectName("DownloadDialogPath");
    pushButton_path->setFocusPolicy(Qt::NoFocus);
    downloadPath =  settings.value("DownloadPath").toString();
    pushButton_path->setText(downloadPath);
    pushButton_path->setToolTip(downloadPath);
    connect(pushButton_path,SIGNAL(pressed()),this,SLOT(chooseDownloadPath()));
    gridLayout->addWidget(pushButton_path,2,1,1,1);
    connect(pushButton_open, &QPushButton::pressed, [=](){
         QDesktopServices::openUrl(QUrl(downloadPath));
    });
    dialog->setLayout(gridLayout);
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addStretch();
    QPushButton *pushButton_confirm = new QPushButton("确定");
    connect(pushButton_confirm, SIGNAL(clicked()), dialog, SLOT(accept()));
    hbox->addWidget(pushButton_confirm);
    QPushButton *pushButton_cancel = new QPushButton("取消");
    connect(pushButton_cancel, SIGNAL(clicked()), dialog, SLOT(reject()));
    hbox->addWidget(pushButton_cancel);
    hbox->addStretch();
    gridLayout->addLayout(hbox,3,0,1,2);
    int result = dialog->exec();
    if (result == QDialog::Accepted) {
        download(lineEdit_url->text(), pushButton_path->text() + "/" + lineEdit_songname->text());
    } else if (result == QDialog::Rejected) {
        dialog->close();
    }
}

void MainWindow::download(QString surl, QString filepath)
{
    controlBar->pushButton_download->setEnabled(false);
    qDebug() <<  "download -> " << surl << "->" << filepath;
    QUrl url = QString(surl);
    QNetworkAccessManager *NAM = new QNetworkAccessManager;
    QNetworkRequest request(url);
    QNetworkReply *reply = NAM->get(request);
    controlBar->pushButton_download->setIcon(QIcon());
    QEventLoop loop;
    if(!surl.contains("http://music.163.com/song/media/outer/url?id="))
        connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(updateProgress(qint64,qint64)));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    //跳转URL处理  https://blog.csdn.net/mingzznet/article/details/9724371
    if(surl.contains("http://music.163.com/song/media/outer/url?id=")){
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "HttpStatusCode" << statusCode;
        surl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
        qDebug() << "redirect" << surl;
        url.setUrl(surl);
        request.setUrl(url);
        reply = NAM->get(request);
        connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(updateProgress(qint64,qint64)));
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
    }
    QFile file(filepath);
    file.open(QIODevice::WriteOnly);
    file.write(reply->readAll());
    //qDebug() << reply->readAll();
    file.close();
    controlBar->pushButton_download->setIcon(QIcon(":/icon/download.svg"));
    controlBar->pushButton_download->setText("");
    controlBar->pushButton_download->setEnabled(true);
}

void MainWindow::updateProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    controlBar->pushButton_download->setText(QString("%1%").arg(bytesReceived*100/bytesTotal));
    double p = static_cast<double>(bytesReceived) / bytesTotal;
    controlBar->pushButton_download->setStyleSheet(QString("QPushButton { background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0,"
                                                   "stop:0 rgba(48, 194, 124, 255), stop:%1 rgba(48, 194, 124, 255),"
                                                   "stop:%2 rgba(255, 255, 255, 255), stop:1 rgba(255, 255, 255, 255)); }")
                                      .arg(p - 0.001)
                                      .arg(p));
    qDebug() << p << controlBar->pushButton_download->styleSheet();
}

void MainWindow::pushButtonMVClicked()
{
    QPushButton *senderObj = qobject_cast<QPushButton*>(sender());
    if (senderObj == nullptr) {
        return;
    }
    navWidget->pushButton_albumPic->setIcon(QIcon(":/icon/disc.svg"));
    QModelIndex index = tableWidget_playlist->indexAt(QPoint(senderObj->frameGeometry().x(),senderObj->frameGeometry().y()));
    int row = index.row();
    tableWidget_playlist->setCurrentCell(row,0);
    QString songname = tableWidget_playlist->item(row,0)->text() + "\n" + tableWidget_playlist->item(row,1)->text();
    navWidget->label_songname->setText(songname);
    QString mvid = tableWidget_playlist->item(row,6)->text();
    QString surl = "http://music.163.com/api/mv/detail/?type=mp4&id=" + mvid;
    qDebug() << "getMV -> " <<  surl;
    QJsonDocument JD = QJsonDocument::fromJson(getReply(surl));
    //qDebug() << JD;
    QJsonObject brs = JD.object().value("data").toObject().value("brs").toObject();
    QString mvurl = brs.value("1080").toString();
    qDebug() << 1080 << mvurl;
    if (mvurl == "") {
        mvurl = brs.value("720").toString();
        qDebug() << 720 << mvurl;
    }
    if (mvurl == "") {
        mvurl = brs.value("480").toString();
        qDebug() << 480 << mvurl;
    }
    if (mvurl == "") {
        mvurl = brs.value("240").toString();
        qDebug() << 240 << mvurl;
    }
    lyricWidget->hide();
    controlBar->pushButton_lyric->setChecked(false);
    stackedWidget->setCurrentWidget(videoWidget);
    player->setMedia(QUrl(mvurl));
    player->play();
    navWidget->listWidget->setCurrentRow(10);
    QPixmap pixmap;
    pixmap.loadFromData(getReply(tableWidget_playlist->item(row,5)->text()));
    navWidget->pushButton_albumPic->setIcon(QIcon(pixmap));
    pixmap.save(QDir::currentPath() + "/cover.jpg");
}

void MainWindow::seekBack()
{
    player->setPosition(player->position() - 5000);
}

void MainWindow::seekForward()
{
    player->setPosition(player->position() + 5000);
}

void MainWindow::tableWidget_playlist_ContextMenu(const QPoint &position)
{
    QAction *action_search;
    QModelIndex index = tableWidget_playlist->indexAt(position);
    QString text = index.data(Qt::DisplayRole).toString();
    QList<QAction *> actions;
    action_search = new QAction(this);
    action_search->setText("搜索 \""+ text + "\" &S");
    actions.append(action_search);
    if(!index.isValid()){
        action_search->setVisible(false);
    }
    QAction *result_action = QMenu::exec(actions, tableWidget_playlist->mapToGlobal(position));
    if(result_action == action_search){
        titleBar->lineEdit_search->setText(text);
        preSearch();
    }
}

void MainWindow::getToplistImg(QString coverImgUrl, QPushButton *pushButton)
{    
    QNetworkAccessManager *NAM = new QNetworkAccessManager;
    QNetworkRequest request;
    request.setUrl(QUrl(coverImgUrl));
    QNetworkReply *reply = NAM->get(request);
    connect(reply, &QNetworkReply::finished, [=]{
        if (pushButton) {
            QPixmap pixmap;
            pixmap.loadFromData(reply->readAll());
            pixmap = pixmap.scaled(pushButton->iconSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            pushButton->setIcon(QIcon(pixmap));
        }
    });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    settings.setValue("Volume", player->volume());
    QMainWindow::closeEvent(event);
}