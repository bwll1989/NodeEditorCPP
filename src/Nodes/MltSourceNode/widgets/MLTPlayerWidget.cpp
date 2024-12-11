//
// Created by WuBin on 2024/12/5.
//

#include "MLTPlayerWidget.h"
MLTPlayerWidget::MLTPlayerWidget(QWidget *parent): QMainWindow(parent), producer(nullptr), consumer(nullptr), timer(nullptr) {
    // 初始化 MLT
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_QuitOnClose, false);
    QList<QScreen *> screenList = QGuiApplication::screens();

    QRect rect = screenList[1]->geometry();  // 若有多个屏
    this->setGeometry(rect);
//
    Mlt::Factory::init();
    // 创建一个 QVBoxLayout 布局
//        QVBoxLayout *layout = new QVBoxLayout(this);
    const char *filename1 = "D:/02_Media/01_Videos/test.mp4";
    const char *filename2 = "D:/02_Media/01_Videos/gyqd_XuZ_s07c03_带声音_230715.mp4";
    profile = new Mlt::Profile ();
    profile->set_frame_rate(30, 1);
    profile->set_width(1920);
    profile->set_height(1080);
    profile->set_progressive(1);
    profile->set_sample_aspect(1, 1);
    profile->set_display_aspect(16, 9);
    profile->set_colorspace(709);

    Mlt::Producer producer1(*profile, filename1);
    Mlt::Producer producer2(*profile, filename2);
    playlist=new Mlt::Playlist();
    playlist->append(producer1);
    playlist->append(producer2);
    // 创建 MLT Consumer（播放窗口）
    consumer =new Mlt::Consumer(*profile, "sdl2", nullptr);
    // Start playing
    consumer->set("real_time", 1);
    consumer->set("terminate_on_pause", 1);
    consumer->set("window_id", (int) this->winId());  // 将 Qt 窗口句柄传递给 MLT
//    consumer->set("resolution","1920x1080");
    // 将 MLT 播放器嵌入到 Qt 窗口
//        layout->addWidget(new QWidget(this));
    if (consumer->is_valid()) {
        // 创建定时器用于更新播放
//            timer = new QTimer(this);
//            connect(timer, &QTimer::timeout, this, &MLTPlayerWidget::updatePlayer);
//            timer->start(40);  // 每 40 毫秒更新一次

        consumer->connect (*playlist);
        // Make an event handler for when a frame's image should be displayed
//        consumer->listen ("consumer-frame-show", this, (mlt_listener) on_frame_show);

    }

}

MLTPlayerWidget::~MLTPlayerWidget() {
    // 清理 MLT 资源
    delete consumer;
    consumer= nullptr;
    delete producer;
    producer= nullptr;
    delete profile;
    profile= nullptr;
}
void MLTPlayerWidget::updatePlayer() {
    qDebug()<<consumer->position();
    if(consumer->is_stopped()){
        consumer->start ();
        return;
    }

    Mlt::Filter f( *profile, "watermark", "pango:" );
    f.set( "producer.text", "吴斌" );
    f.set( "producer.fgcolour", "0x000000ff" );
    f.set( "producer.bgcolour", "0xff000080" );
    playlist->attach( f );


}