#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("fusion");
    app.setApplicationName("TeQtAudioPlayer");
    app.setApplicationVersion("1.0");
    app.setObjectName("TernyukCorporation");
    app.setOrganizationDomain("www.igorternyuk.com");
    Widget w;
    w.show();

    return app.exec();
}
