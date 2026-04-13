#include "mainwidget.h"

#include <QApplication>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile qss(":/GlobalStyle.qss");
    qss.open(QFile::ReadOnly);
    QString style = qss.readAll();
    qApp->setStyleSheet(style);

    qss.close();

    mainWidget w;
    w.show();
    return a.exec();
}
