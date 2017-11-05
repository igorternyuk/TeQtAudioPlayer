#include "widget.h"
#include <QApplication>
#include <QDesktopWidget>

///Надо сделать сохранение плейлиста и настроек
/// при помощи QSettings
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("fusion");
    QRect desktop = QApplication::desktop()->geometry();
    Widget w;
    auto dx = (desktop.width() - w.width()) / 2;
    auto dy = (desktop.height() - w.height()) / 2;
    w.move(dx, dy);
    w.show();

    return a.exec();
}
