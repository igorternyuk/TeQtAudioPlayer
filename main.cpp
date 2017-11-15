#include "widget.h"
#include <QApplication>
#include <QDesktopWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("fusion");
    app.setApplicationName("TeQtAudioPlayer");
    app.setApplicationVersion("1.0");
    app.setObjectName("TernyukCorporation");
    app.setOrganizationDomain("www.igorternyuk.com");
    QRect desktop = QApplication::desktop()->geometry();
    Widget w;
    //auto dx = (desktop.width() - w.width()) / 2;
    //auto dy = (desktop.height() - w.height()) / 2;
    //w.move(dx, dy);
    w.show();

    return app.exec();
}
